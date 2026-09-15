#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <esp_rom_sys.h>
#include <cstring>

#include "sdcard.h"
#include "track.h"
#include "usb_storage.h"

// SD cards are practically always 512-byte sectors; used to size the
// staging buffer for reads/writes that don't cover a whole sector.
#define SECTOR_SIZE 512

static USBMSC msc;
static volatile bool connected = false;

/**
 * Copy disk data into `buffer` for the host. The real address is
 * lba * SECTOR_SIZE + offset, and bufsize can span several sectors (TinyUSB's
 * transfer buffer is larger than one sector), so walk one physical sector
 * at a time.
 */
static int32_t msc_read(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
	uint8_t sector[SECTOR_SIZE];
	uint8_t *dest = (uint8_t *)buffer;
	uint32_t address = lba * SECTOR_SIZE + offset;
	uint32_t remaining = bufsize;

	while (remaining > 0) {
		uint32_t sector_num = address / SECTOR_SIZE;
		uint32_t sector_offset = address % SECTOR_SIZE;
		uint32_t chunk = remaining < (SECTOR_SIZE - sector_offset) ? remaining : (SECTOR_SIZE - sector_offset);

		if (!sdcard_read_sector(sector_num, sector)) {
			return -1;
		}
		memcpy(dest, sector + sector_offset, chunk);

		dest += chunk;
		address += chunk;
		remaining -= chunk;
	}

	return bufsize;
}

/**
 * Write host data to disk. Same multi-sector addressing as msc_read(); a
 * chunk that doesn't cover a whole sector is read-modify-written.
 */
static int32_t msc_write(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
	uint8_t sector[SECTOR_SIZE];
	uint8_t *src = buffer;
	uint32_t address = lba * SECTOR_SIZE + offset;
	uint32_t remaining = bufsize;

	while (remaining > 0) {
		uint32_t sector_num = address / SECTOR_SIZE;
		uint32_t sector_offset = address % SECTOR_SIZE;
		uint32_t chunk = remaining < (SECTOR_SIZE - sector_offset) ? remaining : (SECTOR_SIZE - sector_offset);

		if (sector_offset != 0 || chunk != SECTOR_SIZE) {
			if (!sdcard_read_sector(sector_num, sector)) {
				return -1;
			}
			memcpy(sector + sector_offset, src, chunk);
			if (!sdcard_write_sector(sector_num, sector)) {
				return -1;
			}
		} else if (!sdcard_write_sector(sector_num, src)) {
			return -1;
		}

		src += chunk;
		address += chunk;
		remaining -= chunk;
	}

	return bufsize;
}

/**
 * Mirror ets_printf() output to the USB serial console too, since it
 * otherwise only reaches UART0, which isn't wired to anything on this board.
 */
static void debug_over_usb(char c) {
	Serial.write((uint8_t)c);
}

static void usb_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base != ARDUINO_USB_EVENTS) {
		return;
	}

	switch (event_id) {
		case ARDUINO_USB_STARTED_EVENT:
			// A PC took over: stop logging and hand the card to MSC with a
			// clean, freshly-mounted filesystem view. Only touch flags here,
			// not LVGL - this runs on the USB event task, not loop()'s.
			ets_printf("USB connected: exposing SD card as mass storage.\n");
			track_stop();
			sdcard_remount();
			connected = true;
			break;

		case ARDUINO_USB_STOPPED_EVENT:
			// Remount so our filesystem view picks up whatever the PC changed,
			// instead of acting on stale cached state.
			ets_printf("USB disconnected: resuming track logging.\n");
			sdcard_remount();
			connected = false;
			break;

		default:
			break;
	}
}

/**
 * Set up the USB device: a serial console plus a mass-storage endpoint that
 * exposes the SD card's raw blocks to a connected PC as a removable drive.
 */
void usb_storage_init() {
	USB.onEvent(usb_event);

	msc.vendorID("TKBOX");
	msc.productID("TRACKS");
	msc.productRevision("1.0");
	msc.onRead(msc_read);
	msc.onWrite(msc_write);
	msc.mediaPresent(true);
	msc.isWritable(true);
	msc.begin(sdcard_sector_count(), sdcard_sector_size());

	USB.begin();

	esp_rom_install_channel_putc(2, debug_over_usb);
}

/**
 * Whether a USB host is currently connected.
 */
bool usb_storage_is_connected() {
	return connected;
}
