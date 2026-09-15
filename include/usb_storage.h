#pragma once

/**
 * Set up the USB device: a serial console plus a mass-storage endpoint that
 * exposes the SD card's raw blocks to a connected PC as a removable drive.
 * Call once from setup(), after sdcard_init().
 */
void usb_storage_init();
