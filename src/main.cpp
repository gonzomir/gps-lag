
#include <cstdio>
#include <nmeaparser.h>

#include "draw.h"

NMEAParser parser;

void setup() {
  Serial.begin(115200);

  setupDisplay();

  drawStatus("Waiting for GPS...");

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
  char status[200];

  String data = "";

  while (Serial2.available()) {
    data = Serial2.readStringUntil('\n');
    // Remove trailing newline.
    data.remove(data.length() -1);

    const char * str = data.c_str();
    if (parser.dispatch(str)) {
      Serial.println(parser.getLastProcessedType());

      // Guess the last parsed sentence type.
      switch(parser.getLastProcessedType()) {
        // Is it a GPRMC sentence?
        case NMEAParser::TYPE_GPRMC:
          // Show speed.
          drawSpeed(parser.last_gprmc.speed_over_ground);
          Serial.println(parser.last_gprmc.speed_over_ground);
          break;
        case NMEAParser::TYPE_GPGGA:
          sprintf(status, "Sats: %d; Acc: %d m", parser.last_gpgga.satellites_used, parser.last_gpgga.hdop);
          drawStatus(status);
          break;
        case NMEAParser::UNKNOWN:
        case NMEAParser::TYPE_PLSR2451:
        case NMEAParser::TYPE_PLSR2452:
        case NMEAParser::TYPE_PLSR2457:
        case NMEAParser::TYPE_GPGSV:
        case NMEAParser::TYPE_GPGSA:
        case NMEAParser::TYPE_HCHDG:
        case NMEAParser::TYPE_GPGLL:
        case NMEAParser::TYPE_GPVTG:
        case NMEAParser::TYPE_GPTXT:
          break;
      }

    }
  }

  delay(500);
}
