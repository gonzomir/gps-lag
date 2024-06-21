#include <sys/time.h>
#include <cstdio>
#include <string>
#include <nmeaparser.h>

#include "config.h"
#include "battery.h"
#include "draw.h"

NMEAParser parser;
bool should_sleep = false;
bool gps_is_ready = false;
bool do_read_gnss = false;
int  last_fix = 0;
String last_status = "";
float last_speed = 0.0;
bool battery_read = true;
int last_battery_percents = 0;

hw_timer_t *battery_timer = NULL;

/**
 * On/off button unterrupt handler.
 */
void IRAM_ATTR button_pressed() {
  should_sleep = true;
}

/**
 * Put the chip in deep sleep and power off the display.
 */
void go_to_sleep() {
  detachInterrupt(digitalPinToInterrupt(POWER_BTN));
  detachInterrupt(digitalPinToInterrupt(GNSS_PPS));

  power_off_display();

  // GNSS off.
  digitalWrite(18, LOW);

  // Hold output pin states.
  gpio_hold_en(GNSS_EN_GPIO);
  gpio_deep_sleep_hold_en();

  Serial.println("Going to sleep.");
  esp_sleep_enable_ext0_wakeup(POWER_BTN_GPIO, 1);
  esp_deep_sleep_start();
}

void IRAM_ATTR read_gnss() {
  ets_printf("PPS triggered\n");
  do_read_gnss = true;
  last_fix = millis();
}

void IRAM_ATTR read_battery() {
  battery_read = true;
}

/**
 * Setup routine.
 */
void setup() {
  should_sleep = false;
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // Disable the hold on output pin states.
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(GNSS_EN_GPIO);

  // On/off button.
  pinMode(POWER_BTN, INPUT_PULLDOWN);

  // Battery voltage.
  pinMode(BATTERY_STATUS, INPUT);
  calibrate_adc();

  // GNSS PPS
  pinMode(GNSS_PPS, INPUT_PULLDOWN);

  // GNSS On.
  pinMode(GNSS_EN, OUTPUT);
  digitalWrite(GNSS_EN, HIGH);

  Serial.begin(115200);

  setup_display();
  clear_display();
  draw_top_bar();
  draw_bottom_bar();
  draw_units("SOG, Kn");
  draw_speed(0.0);

  draw_status("Waiting for GPS...");

  Serial2.begin(38400, SERIAL_8N1, GNSS_RX, GNSS_TX);

  attachInterrupt(digitalPinToInterrupt(POWER_BTN), button_pressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(GNSS_PPS), read_gnss, RISING);

  battery_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(battery_timer, &read_battery, true);
  timerAlarmWrite(battery_timer, 1000000 * 60, true);
  timerAlarmEnable(battery_timer);

  Serial.println("Setup complete.");
}

/**
 * Main loop.
 */
void loop() {
  if (should_sleep) {
    go_to_sleep();
    return;
  }

  if (do_read_gnss) {
    do_read_gnss = false;

    String data = "";

    while (Serial2.available()) {
      data = Serial2.readStringUntil('\n');
      // Remove trailing newline.
      data.remove(data.length() -1);
      //Serial.println(data);

      const char * str = data.c_str();
      if (parser.dispatch(str)) {
        // Get the last parsed sentence type.
        switch(parser.getLastProcessedType()) {
          case NMEAParser::TYPE_GPRMC:
            if (gps_is_ready) {
              std::string gps_time = parser.lastGPRMC.utc_time;
              int hours = atoi(gps_time.substr(0, 2).c_str());
              int minutes = atoi(gps_time.substr(2, 2).c_str());
              int seconds = atoi(gps_time.substr(4, 2).c_str());

              // Clear display every 5 minutes.
              if (minutes % 5 == 0 && seconds == 0) {
                clear_display();
                draw_top_bar();
                draw_bottom_bar();
                draw_units("SOG, Kn");
                battery_read = true;
                last_battery_percents = 0;
              }

              draw_time(hours, minutes, seconds);
            }
            break;
          case NMEAParser::TYPE_GPVTG:
            if (gps_is_ready) {
              float speed = 0;
              if (parser.lastGPVTG.ground_speed_unit_1 == 'N') {
                speed = parser.lastGPVTG.ground_speed_1;
              } else if (parser.lastGPVTG.ground_speed_unit_2 == 'N') {
                speed = parser.lastGPVTG.ground_speed_2;
              }

              if (speed != last_speed) {
                // Show speed.
                draw_speed(speed);
                last_speed = speed;
              }
            }
            break;
          case NMEAParser::TYPE_GPGGA:
            gps_is_ready = (parser.lastGPGGA.satellites_used > 3);
            if (gps_is_ready) {
              char s[22] = "";
              // Update status.
              sprintf(s, "Sats: %d; Acc: %.2f m", parser.lastGPGGA.satellites_used, parser.lastGPGGA.hdop);
              String status = String(s);
              if (status != last_status) {
                draw_status(status);
                last_status = status;
              }
            }
            break;
          case NMEAParser::UNKNOWN:
          case NMEAParser::TYPE_PLSR2451:
          case NMEAParser::TYPE_PLSR2452:
          case NMEAParser::TYPE_PLSR2457:
          case NMEAParser::TYPE_GPGSV:
          case NMEAParser::TYPE_GPGSA:
          case NMEAParser::TYPE_HCHDG:
          case NMEAParser::TYPE_GPGLL:
          case NMEAParser::TYPE_GPTXT:
            break;
        }

      } else {
        Serial.println("Failed parsing NMEA sentence.");
      }
    }
  } else {
    int now = millis();
    if (now > last_fix + 30 * 1000) {
      String status = "Waiting for GPS...";
      if (status != last_status) {
        draw_status(status);
        last_status = status;
      }
      if (last_speed != 0.0) {
        draw_speed(0.0);
        last_speed = 0.0;
      }
    }
  }

  if (battery_read) {
    battery_read = false;
    float voltage = get_battery_voltage();
    int battery_percents = get_battery_percents(voltage);
    //Serial.println(voltage);
    //Serial.println(battery_percents);

    if (battery_percents < 2.0) {
      go_to_sleep();
      return;
    }

    if (battery_percents != last_battery_percents) {
      draw_battery_status(battery_percents);
      last_battery_percents = battery_percents;
    }
  }
}
