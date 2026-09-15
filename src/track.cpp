#include <Arduino.h>
#include <cstdio>
#include <cstdlib>

#include "sdcard.h"
#include "track.h"

static bool active = false;

/**
 * Convert an NMEA DDMM.MMMM / DDDMM.MMMM coordinate to signed decimal degrees.
 */
static double nmea_to_decimal_degrees(const char *nmea, char hemisphere) {
	double raw = atof(nmea);
	int degrees = (int)(raw / 100);
	double minutes = raw - (degrees * 100);
	double decimal = degrees + minutes / 60.0;

	if (hemisphere == 'S' || hemisphere == 'W') {
		decimal = -decimal;
	}

	return decimal;
}

/**
 * Format an NMEA date (DDMMYY) and UTC time (HHMMSS.ss) as an ISO 8601 timestamp.
 *
 * @param buffer Destination buffer, at least 21 bytes.
 */
static void nmea_to_iso8601(const char *date, const char *time, char *buffer, size_t buffer_size) {
	int day = 1, month = 1, year = 0;
	int hour = 0, minute = 0, second = 0;

	sscanf(date, "%2d%2d%2d", &day, &month, &year);
	sscanf(time, "%2d%2d%2d", &hour, &minute, &second);

	snprintf(buffer, buffer_size, "20%02d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);
}

/**
 * Whether a track file is currently open.
 */
bool track_is_active() {
	return active;
}

/**
 * Start a new track log on the SD card, named after the given UTC date/time.
 */
bool track_start(const char *date, const char *time) {
	active = false;

	if (!sdcard_init()) {
		return false;
	}

	char timestamp[21];
	nmea_to_iso8601(date, time, timestamp, sizeof(timestamp));
	// timestamp now holds "YYYY-MM-DDTHH:MM:SSZ"; carve out the pieces we need.
	char dir[24];
	snprintf(dir, sizeof(dir), "/tracks/%.4s%.2s%.2s", timestamp, timestamp + 5, timestamp + 8);

	if (!sdcard_mkdir(dir)) {
		return false;
	}

	char path[40];
	snprintf(path, sizeof(path), "%s/%.2s%.2s%.2s.csv", dir, timestamp + 11, timestamp + 14, timestamp + 17);

	if (!sdcard_open_append(path)) {
		return false;
	}

	sdcard_write_line("timestamp,latitude,longitude,speed_kn");

	active = true;
	ets_printf("Track log started: %s\n", path);
	return true;
}

/**
 * Log one GNSS fix as a row in the currently open track file.
 */
void track_log_fix(const char *date, const char *time, const char *latitude, char north_south, const char *longitude, char east_west, float speed_knots) {
	if (!active) {
		return;
	}

	char timestamp[21];
	nmea_to_iso8601(date, time, timestamp, sizeof(timestamp));

	double lat = nmea_to_decimal_degrees(latitude, north_south);
	double lon = nmea_to_decimal_degrees(longitude, east_west);

	char row[96];
	snprintf(row, sizeof(row), "%s,%.6f,%.6f,%.2f", timestamp, lat, lon, speed_knots);

	sdcard_write_line(row);
}

/**
 * Stop the current track log, flushing and closing the file.
 */
void track_stop() {
	if (!active) {
		return;
	}

	sdcard_close();
	active = false;
}
