#pragma once

/**
 * Set up the USB device: a serial console plus a mass-storage endpoint that
 * exposes the SD card's raw blocks to a connected PC as a removable drive.
 * Call once from setup(), after sdcard_init().
 */
void usb_storage_init();

/**
 * Whether a USB host is currently connected. Set from the USB event task,
 * so poll it from loop() rather than reacting to it from another task
 * directly - in particular, never touch LVGL from that task.
 */
bool usb_storage_is_connected();
