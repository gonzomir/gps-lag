#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#include "config.h"
#include "sdcard.h"

static bool mounted = false;
static File current_file;

/**
 * Mount the built-in microSD card, if not already mounted.
 */
bool sdcard_init() {
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
 * Create a directory on the card, including it if it already exists.
 */
bool sdcard_mkdir(const String &path) {
	if (!mounted) {
		return false;
	}

	if (SD_MMC.exists(path)) {
		return true;
	}

	return SD_MMC.mkdir(path);
}

/**
 * Open a file for appending, creating it if it doesn't exist yet.
 */
bool sdcard_open_append(const String &path) {
	if (!mounted) {
		return false;
	}

	sdcard_close();

	current_file = SD_MMC.open(path, FILE_APPEND);
	return (bool)current_file;
}

/**
 * Append a line of text to the currently open file and flush it to the card.
 */
bool sdcard_write_line(const String &line) {
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
	if (current_file) {
		current_file.close();
	}
}
