#include <cmath>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "config.h"

#include "lvgl.h"
#include "lv_conf.h"

#define CANVAS

/**
 * Setup the display.
 */
void setup_display();

/**
 * Power off the display.
 */
void power_off_display();

/**
 * Clear the display.
 */
void clear_display();

/**
 * Draw speed in knots.
 *
 * @param speed
 */
void draw_speed(float speed);

/**
 * Draw status text.
 *
 * @param text
 */
void draw_status(String text);

/**
 * Clear status text area.
 */
void clear_status();

/**
 * Draw battery capacity indicator.
 *
 * @param percentage Battery capacity.
 */
void draw_battery_status(int percentage);

/**
 * Draw time.
 *
 * @param hours
 * @param minutes
 * @param seconds
 */
void draw_time(int hours, int minutes, int seconds);

/**
 * Draw units.
 *
 * @param text
 */
void draw_units(String text);

/**
 * Draw rectangle on the screen.
 *
 * @param x Start point X coordinate.
 * @param y Start point Y coordinate.
 * @param w Width of the rectangle.
 * @param h Height of the rectangle.
 * @param partial Partial or full screen draw.
 */
void draw_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial);

/**
 * Draw start timer.
 *
 * @param seconds
 */
void draw_start_timer(int seconds);

/**
 * Call LVGL timer handler and flush display.
 */
void timer_handler();

/**
 * Change LVGL screen.
 */
void change_screen();

/**
 * Show or hide the USB-connected overlay.
 *
 * @param connected
 */
void draw_usb_status(bool connected);
