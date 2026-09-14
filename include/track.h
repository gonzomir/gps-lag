#pragma once

#include <Arduino.h>

/**
 * Whether a track file is currently open.
 */
bool track_is_active();

/**
 * Start a new track log on the SD card, named after the given UTC date/time.
 * Meant to be called once, on the first valid GNSS fix after power-on.
 *
 * @param date NMEA date field, DDMMYY.
 * @param time NMEA UTC time field, HHMMSS(.ss).
 * @return bool True if the track file was created successfully.
 */
bool track_start(const char *date, const char *time);

/**
 * Log one GNSS fix as a row in the currently open track file.
 * No-op if no track is open (e.g. no SD card present).
 *
 * @param date NMEA date field, DDMMYY.
 * @param time NMEA UTC time field, HHMMSS(.ss).
 * @param latitude NMEA latitude, DDMM.MMMM.
 * @param north_south 'N' or 'S'.
 * @param longitude NMEA longitude, DDDMM.MMMM.
 * @param east_west 'E' or 'W'.
 * @param speed_knots Speed over ground, in knots.
 */
void track_log_fix(const char *date, const char *time, const char *latitude, char north_south, const char *longitude, char east_west, float speed_knots);

/**
 * Stop the current track log, flushing and closing the file.
 * Call before powering off / going to sleep.
 */
void track_stop();
