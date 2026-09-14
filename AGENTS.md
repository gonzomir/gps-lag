# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project overview

**The Knots Box** (project site: https://theknotsbox.com) is a DIY, battery-powered
GPS speedometer for small sailboats. The device shows speed over ground (in knots) plus a
regatta start-sequence countdown timer, on a sunlight-readable reflective LCD.

This repository contains all the firmware for the device. A companion 3D-printed
enclosure lives in a separate repo: https://github.com/gonzomir/the-knots-box-case.

## Hardware

- **MCU / board**: [Waveshare ESP32-S3 4.2" RLCD Development Board](https://www.waveshare.com/esp32-s3-rlcd-4.2.htm?sku=33507) — integrates the ESP32-S3, a reflective (low-power, sunlight-readable) LCD, and PSRAM in one module.
- **GNSS**: QUESCAN G10A-F30 module (UBX-M10050-KB chip), connected over UART (`GNSS_RX`/`GNSS_TX` in `include/config.h`), with a PPS pin driving an interrupt-based read cadence.
- **Power**: single Li-ion cell, charged via USB-C; battery voltage read through an ADC pin with calibration in `src/battery.cpp`.
- **Inputs**: two buttons (`MAIN_BTN` for Enter, `MODE_BTN` for switching screens), both interrupt-driven.
- Pin assignments and display dimensions are centralized in `include/config.h` — check there before touching wiring-related code.

## Software architecture

- **Framework**: PlatformIO, Arduino framework on top of ESP-IDF (`pioarduino` ESP32 platform). Single build environment: `[env:rlcd]` in `platformio.ini`.
- **Display stack**: [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) drives the reflective LCD at the low level (`src/display_bsp.cpp`, adapted from the display manufacturer's example code), with [LVGL v8](https://lvgl.io/) rendering the UI on top (`src/draw.cpp`, `src/lv_port.cpp`). LVGL is configured via `include/lv_conf.h`. Fonts are pre-generated LVGL C arrays (`src/lvgl_rethinksans_bold_*.c`) — do not hand-edit these.
- **GPS parsing**: NMEA sentences are parsed with a fork of [x99/NMEAParser](https://github.com/x99/NMEAParser) (`gonzomir/NMEAParser#parse-multiple-gnss`, pulled in via `lib_deps`). `src/main.cpp` dispatches on sentence type (GPRMC for time, GPVTG for speed, GPGGA for fix quality/satellite count).
- **Main loop pattern**: ISRs (`IRAM_ATTR` functions in `src/main.cpp`) only set volatile-ish flag/state variables (e.g. `do_read_gnss`, `should_sleep`, `main_button`); the actual work happens in `loop()`/`do_speed()`/`do_start()`, which poll those flags. Keep this pattern when adding new interrupt-driven behavior — don't do real work inside an ISR.
- **Power management**: the device uses `esp_deep_sleep_start()` with `gpio_hold_en`/`gpio_deep_sleep_hold_en` to keep the GNSS module powered off during sleep, waking on the main button (`go_to_sleep()` in `src/main.cpp`). WiFi/BT are disabled at boot since they're unused and would waste power.
- **Two screens**: `tkb_mode::speed` (default speed display) and `tkb_mode::start` (regatta start countdown timer), toggled by the mode button via `change_screen()` in `src/draw.cpp`.

## Repository layout

- `src/`, `include/` — firmware source and headers (PlatformIO convention).
- `lib/` — for local/private libraries (currently empty; external deps are pulled via `lib_deps` in `platformio.ini`).
- `test/` — reserved for PlatformIO unit tests (currently empty).
- `assets/` — project images/assets.

## Build / run

This is embedded firmware, not a script you can run locally — commands require a connected board and PlatformIO.

```sh
pio run                         # build
pio run -t upload               # build and flash
pio device monitor -b 115200    # serial monitor (matches monitor_speed in platformio.ini)
```

There is no test suite currently in place (`test/` is a placeholder), and no way to exercise this code without the physical hardware — don't claim a change "works" without either hardware-in-the-loop testing or a clear caveat that it's unverified.

## Conventions

- Indentation is tabs, not spaces.
- Public functions in headers get short Doxygen-style `/** ... */` comments describing params/return; keep that style for new public functions in `include/*.h`.
- `ets_printf` is used for debug/serial logging throughout (not `Serial.print`) — stay consistent within `src/main.cpp` and friends.
- Pin numbers and other board constants belong in `include/config.h`, not scattered magic numbers in `.cpp` files.
