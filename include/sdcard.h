#pragma once

#include <Arduino.h>

/**
 * Mount the built-in microSD card, if not already mounted.
 *
 * @return bool True if the card is mounted and ready to use.
 */
bool sdcard_init();

/**
 * Create a directory on the card, including it if it already exists.
 *
 * @param path Directory path, e.g. "/tracks/20260914".
 * @return bool True on success, or if the directory already exists.
 */
bool sdcard_mkdir(const String &path);

/**
 * Open a file for appending, creating it if it doesn't exist yet.
 * Only one file can be held open through this abstraction at a time;
 * opening a new one implicitly closes the previous one.
 *
 * @param path File path.
 * @return bool True on success.
 */
bool sdcard_open_append(const String &path);

/**
 * Append a line of text, followed by a newline, to the currently open file
 * and flush it to the card.
 *
 * @param line Text to write.
 * @return bool True on success.
 */
bool sdcard_write_line(const String &line);

/**
 * Close the currently open file, if any.
 */
void sdcard_close();
