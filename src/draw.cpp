#include <cmath>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "display_bsp.h"
#include "lv_port.h"
#include "lvgl.h"
#include "lv_conf.h"

#include "draw.h"

static uint32_t screenWidth;
static uint32_t screenHeight;
static uint32_t bufSize;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

DisplayPort RlcdPort(DISPLAY_MOSI, DISPLAY_SCL, DISPLAY_DC, DISPLAY_CS, DISPLAY_RST, DISPLAY_W, DISPLAY_H);

lv_obj_t *speed_screen = NULL;
lv_obj_t *timer_screen = NULL;

lv_obj_t *speed_label = NULL;
lv_obj_t *timer_label = NULL;

lv_obj_t *status_label = NULL;
lv_obj_t *time_label = NULL;
lv_obj_t *battery_label = NULL;
lv_obj_t *units_label = NULL;

lv_obj_t *usb_overlay = NULL;

/**
 * Flush display.
 *
 * @param disp
 * @param area
 * @param color_p
 */
static void lvgl_flushcallback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
  uint16_t *buffer = (uint16_t *)color_map;
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      uint8_t color = (*buffer < 0x7fff) ? ColorBlack : ColorWhite;
      RlcdPort.RLCD_SetPixel(x, y, color);
      buffer++;
    }
  }
  RlcdPort.RLCD_Display();
  lv_disp_flush_ready(drv);
}

/**
 * Setup the display.
 */
void setup_display() {
	screenWidth = DISPLAY_W;
	screenHeight = DISPLAY_H;

  RlcdPort.RLCD_Init();
  lvgl_portinit(screenWidth, screenHeight, lvgl_flushcallback);

	// Start drawing.
	lv_color_t background_color = lv_color_hex(0xFFFFFF);
	lv_color_t text_color = lv_color_hex(0x000000);

	static lv_style_t style_screen;
	lv_style_init(&style_screen);

	lv_style_set_bg_color(&style_screen, background_color);
	lv_style_set_text_color(&style_screen, text_color);
	lv_style_set_text_font(&style_screen, &lvgl_rethinksans_bold_16);
	lv_style_set_pad_left(&style_screen, 0);
	lv_style_set_pad_right(&style_screen, 0);
	lv_style_set_pad_top(&style_screen, 0);
	lv_style_set_pad_bottom(&style_screen, 0);

	static lv_style_t style_big_label;
	lv_style_init(&style_big_label);

	lv_style_set_text_font(&style_big_label, &lvgl_rethinksans_bold_200);
	lv_style_set_text_color(&style_big_label, text_color);
	lv_style_set_text_line_space(&style_big_label, 0);
	lv_style_set_pad_left(&style_big_label, 0);
	lv_style_set_pad_right(&style_big_label, 0);
	lv_style_set_pad_top(&style_big_label, 25);
	lv_style_set_pad_bottom(&style_big_label, 0);

	speed_screen = lv_obj_create(NULL);
	lv_obj_add_style(speed_screen, &style_screen, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(speed_screen, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_flex_flow(speed_screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(speed_screen, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	speed_label = lv_label_create(speed_screen);
	lv_obj_align(speed_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_style(speed_label, &style_big_label, LV_PART_MAIN);

	lv_scr_load(speed_screen);

	timer_screen = lv_obj_create(NULL);
	lv_obj_add_style(timer_screen, &style_screen, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(timer_screen, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_flex_flow(timer_screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(timer_screen, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	timer_label = lv_label_create(timer_screen);
	lv_obj_align(timer_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_style(timer_label, &style_big_label, LV_PART_MAIN);
	lv_obj_add_flag(timer_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(timer_label, LV_OBJ_FLAG_EVENT_BUBBLE);

	draw_start_timer(300);

	lv_obj_t * top_bar = lv_obj_create(lv_layer_top());
	lv_obj_set_size(top_bar, screenWidth - 1, 40);
	lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 5);
	lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_color(top_bar, background_color, LV_PART_MAIN);
	lv_obj_set_style_text_color(top_bar, text_color, LV_PART_MAIN);
	lv_obj_set_style_border_width(top_bar, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(top_bar, text_color, LV_PART_MAIN);
	lv_obj_set_style_border_side(top_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
	lv_obj_set_style_pad_left(top_bar, 20, LV_PART_MAIN);
	lv_obj_set_style_pad_right(top_bar, 20, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

	time_label = lv_label_create(top_bar);
	lv_obj_align(time_label, LV_ALIGN_OUT_LEFT_MID, 0, 0);
	battery_label = lv_label_create(top_bar);
	lv_obj_align(battery_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t * bottom_bar = lv_obj_create(lv_layer_top());
	lv_obj_set_size(bottom_bar, screenWidth - 1, 40);
	lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 5);
	lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_color(bottom_bar, background_color, LV_PART_MAIN);
	lv_obj_set_style_text_color(bottom_bar, text_color, LV_PART_MAIN);
	lv_obj_set_style_border_width(bottom_bar, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(bottom_bar, text_color, LV_PART_MAIN);
	lv_obj_set_style_border_side(bottom_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
	lv_obj_set_style_pad_left(bottom_bar, 20, LV_PART_MAIN);
	lv_obj_set_style_pad_right(bottom_bar, 20, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(bottom_bar, LV_SCROLLBAR_MODE_OFF);

	status_label = lv_label_create(bottom_bar);
	lv_obj_align(status_label, LV_ALIGN_OUT_LEFT_MID, 0, 0);
	units_label = lv_label_create(bottom_bar);
	lv_obj_align(units_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Overlay covering the speed/timer area, shown while a PC has the SD
	// card mounted over USB. Sized to the middle strip between the bars.
	usb_overlay = lv_obj_create(lv_layer_top());
	lv_obj_add_style(usb_overlay, &style_screen, LV_PART_MAIN);
	lv_obj_set_size(usb_overlay, screenWidth, screenHeight - 90);
	lv_obj_align(usb_overlay, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_scrollbar_mode(usb_overlay, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_flag(usb_overlay, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t *usb_label = lv_label_create(usb_overlay);
	lv_obj_center(usb_label);
	lv_obj_add_style(usb_label, &style_big_label, LV_PART_MAIN);
	lv_label_set_text(usb_label, "USB");
}

/**
 * Power off the display.
 */
void power_off_display() {
}

/**
 * Draw speed in knots.
 *
 * @param speed
 */
void draw_speed(float speed) {
	lv_label_set_text_fmt(speed_label, "%0.1f", speed);
}

/**
 * Draw status text.
 *
 * @param text
 */
void draw_status(String text) {
	lv_label_set_text(status_label, text.c_str());
}

/**
 * Clear status text area.
 */
void clear_status() {
	lv_label_set_text(status_label, "");
}

/**
 * Draw battery capacity indicator.
 *
 * @param percentage Battery capacity.
 */
void draw_battery_status(int percentage) {
	lv_label_set_text_fmt(battery_label, "%d%%", percentage);
}

/**
 * Draw time.
 *
 * @param hours
 * @param minutes
 * @param seconds
 */
void draw_time(int hours, int minutes, int seconds) {
	lv_label_set_text_fmt(time_label, "%02d:%02d", hours, minutes);
}

/**
 * Draw units.
 *
 * @param text
 */
void draw_units(String text) {
	lv_label_set_text(units_label, text.c_str());
}

/**
 * Draw start timer.
 *
 * @param seconds
 */
void draw_start_timer(int seconds) {
	int minutes;
	minutes = seconds / 60;
	seconds = seconds % 60;

	lv_label_set_text_fmt(timer_label, "%d:%02d", minutes, seconds);
}

/**
 * Show or hide the USB-connected overlay.
 *
 * @param connected
 */
void draw_usb_status(bool connected) {
	if (connected) {
		lv_obj_clear_flag(usb_overlay, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(usb_overlay, LV_OBJ_FLAG_HIDDEN);
	}
}

/**
 * Change LVGL screen.
 */
void change_screen() {
	if (display_mode == tkb_mode::speed) {
		ets_printf("Change screen to start timer.\n");
		lv_scr_load_anim(timer_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
		display_mode = tkb_mode::start;
		draw_units("Time to start");
		draw_start_timer(300);
	} else {
		ets_printf("Change screen to speed.\n");
		lv_scr_load_anim(speed_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
		display_mode = tkb_mode::speed;
		draw_units("SOG, Kn");
	}
	do_start_timer = false;
	start_timer_started = false;
}
