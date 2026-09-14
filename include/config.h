#include <Arduino.h>

#define GNSS_RX 1
#define GNSS_TX 2
#define GNSS_PPS 3
#define GNSS_EN 17
#define GNSS_EN_GPIO GPIO_NUM_17

#define MAIN_BTN 0
#define MAIN_BTN_GPIO GPIO_NUM_0

#define MODE_BTN 18
#define NODE_BTN_GPIO GPIO_NUM_18

#define BATTERY_STATUS 4
#define BATTERY_STATUS_ADC_CHANNEL ADC1_CHANNEL_3
#define BATTERY_STATUS_ATTENUATION ADC_ATTEN_DB_12

#define DISPLAY_MOSI 12
#define DISPLAY_SCL 11
#define DISPLAY_CS 40
#define DISPLAY_DC 5
#define DISPLAY_RST 41
#define DISPLAY_W 400
#define DISPLAY_H 300

// Built-in microSD card slot, 1-bit SDMMC mode.
#define SD_CLK 38
#define SD_CMD 21
#define SD_D0 39

#ifndef CONFIG_H
#define CONFIG_H

enum tkb_mode { speed, start };

extern bool should_sleep;
extern bool gps_is_ready;
extern bool do_read_gnss;
extern bool do_start_timer;
extern tkb_mode display_mode;
extern bool start_timer_started;

#endif
