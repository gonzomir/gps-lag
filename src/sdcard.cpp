#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"
#include "sdcard.h"

static bool mounted = false;
static File current_file;

// Our own track-logging code (main/loop task) and the USB mass-storage
// callbacks (TinyUSB's task) both reach these functions. Without a lock,
// a raw sector read/write can race a mount/unmount and corrupt the card's
// directory structures. Recursive because sdcard_mkdir() calls itself and
// sdcard_remount() calls sdcard_close()/sdcard_init().
static SemaphoreHandle_t sd_mutex = xSemaphoreCreateRecursiveMutex();

class SdLock {
public:
	SdLock() {
		xSemaphoreTakeRecursive(sd_mutex, portMAX_DELAY);
	}
	~SdLock() {
		xSemaphoreGiveRecursive(sd_mutex);
	}
};

/**
 * Mount the built-in microSD card, if not already mounted.
 */
bool sdcard_init() {
	SdLock lock;

	if (mounted) {
		return true;
	}

	if (!SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0)) {
		ets_printf("SD card: failed to set pins.\n");
		return false;
	}

	// 1-bit mode: only CLK, CMD and D0 are wired.
	if (!SD_MMC.begin("/sdcard", true)) {
		ets_printf("SD card: mount failed.\n");
		return false;
	}

	if (SD_MMC.cardType() == CARD_NONE) {
		ets_printf("SD card: no card found.\n");
		SD_MMC.end();
		return false;
	}

	mounted = true;
	return true;
}

/**
 * Create a directory on the card, including missing parent directories
 * (like "mkdir -p"), or return true if it already exists.
 */
bool sdcard_mkdir(const String &path) {
	SdLock lock;

	if (!mounted) {
		return false;
	}

	if (SD_MMC.exists(path)) {
		return true;
	}

	// SD_MMC.mkdir() only creates one level, so ensure the parent exists first.
	int slash = path.lastIndexOf('/');
	if (slash > 0 && !sdcard_mkdir(path.substring(0, slash))) {
		return false;
	}

	if (!SD_MMC.mkdir(path)) {
		ets_printf("SD card: failed to create directory %s.\n", path.c_str());
		return false;
	}

	return true;
}

/**
 * Open a file for appending, creating it if it doesn't exist yet.
 */
bool sdcard_open_append(const String &path) {
	SdLock lock;

	if (!mounted) {
		return false;
	}

	sdcard_close();

	current_file = SD_MMC.open(path, FILE_APPEND);
	if (!current_file) {
		ets_printf("SD card: failed to open %s.\n", path.c_str());
		return false;
	}

	return true;
}

/**
 * Append a line of text to the currently open file and flush it to the card.
 */
bool sdcard_write_line(const String &line) {
	SdLock lock;

	if (!current_file) {
		return false;
	}

	size_t written = current_file.println(line);
	current_file.flush();

	return written > 0;
}

/**
 * Close the currently open file, if any.
 */
void sdcard_close() {
	SdLock lock;

	if (current_file) {
		current_file.close();
	}
}

/**
 * Number of raw sectors on the card.
 */
uint32_t sdcard_sector_count() {
	SdLock lock;

	if (!mounted) {
		return 0;
	}

	// SD_MMC.numSectors() is the FAT data region only (via f_getfree()) and
	// undercounts the card; readRAW/writeRAW address the whole physical
	// card from sector 0, so report capacity from the card's own CSD
	// register instead, or MSC and any format tool will size the
	// filesystem for a card smaller than the one actually being written to.
	return SD_MMC.cardSize() / SD_MMC.sectorSize();
}

/**
 * Size of one raw sector, in bytes (512 in practice).
 */
uint16_t sdcard_sector_size() {
	SdLock lock;

	if (!mounted) {
		return 0;
	}

	return SD_MMC.sectorSize();
}

/**
 * Read one raw sector from the card, bypassing the filesystem.
 */
bool sdcard_read_sector(uint32_t sector, uint8_t *buffer) {
	SdLock lock;

	if (!mounted) {
		return false;
	}

	return SD_MMC.readRAW(buffer, sector);
}

/**
 * Write one raw sector to the card, bypassing the filesystem.
 */
bool sdcard_write_sector(uint32_t sector, const uint8_t *buffer) {
	SdLock lock;

	if (!mounted) {
		return false;
	}

	return SD_MMC.writeRAW(const_cast<uint8_t *>(buffer), sector);
}

/**
 * Fully unmount and remount the card, discarding any cached filesystem state.
 */
bool sdcard_remount() {
	SdLock lock;

	sdcard_close();

	if (mounted) {
		SD_MMC.end();
		mounted = false;
	}

	return sdcard_init();
}
