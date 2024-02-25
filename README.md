# GC9A01 Display Driver for MicroPython

<p align="center">
  <img src="https://raw.githubusercontent.com/russhughes/gc9a01_mpy/master/docs/GC9A01.jpg" alt="GC9A01 display photo"/>
</p>

This driver is based on [devbis' st7789_mpy driver.](https://github.com/devbis/st7789_mpy)
I modified the original driver for one of my projects to add:

- Display Rotation.
- Scrolling
- Writing text using bitmaps converted from True Type fonts
- Drawing text using 8 and 16 bit wide bitmap fonts
- Drawing text using Hershey vector fonts
- Drawing JPG's, including a SLOW mode to draw jpg's larger than available ram
  using the TJpgDec - Tiny JPEG Decompressor R0.01d. from
  http://elm-chan.org/fsw/tjpgd/00index.html

Included are 12 bitmap fonts derived from classic pc text mode fonts, 26 Hershey vector fonts and several example programs for different devices. The driver supports 240x240 displays.

## Documentation

Documentation can be found in the docs directory and at https://russhughes.github.io/gc9a01_mpy/

## Pre-compiled firmware files

The firmware directory contains pre-compiled firmware for various devices with the gc9a01 C driver and frozen python font files. See the README.md file in the fonts folder for more information on the font files.

### ESP32 BOARDS firmware

  - ARDUINO_NANO_ESP32
  - ESP32_GENERIC with 4, 8, 16, or 32MB flash
  - ESP32_GENERIC_C3 with 4, 8, 16, or 32MB flash
  - ESP32_GENERIC_S2 with 4, 8, 16, or 32MB flash
  - ESP32_GENERIC_S3 with 4, 8, 16, or 32MB flash
  - LILYGO_TTGO_LORA32
  - LOLIN_C3_MINI
  - LOLIN_S2_MINI
  - LOLIN_S2_PICO
  - M5STACK_ATOM
  - OLIMEX_ESP32_POE
  - SIL_WESP32
  - UM_FEATHERS2
  - UM_FEATHERS2NEO
  - UM_FEATHERS3
  - UM_NANOS3
  - UM_PROS3
  - UM_TINYPICO
  - UM_TINYS2
  - UM_TINYS3
  - UM_TINYWATCHS3

### RP2040 BOARDS firmware

  - ADAFRUIT_ITSYBITSY_RP2040
  - ADAFRUIT_QTPY_RP2040
  - ARDUINO_NANO_RP2040_CONNECT
  - ADAFRUIT_FEATHER_RP2040
  - GARATRONIC_PYBSTICK26_RP2040
  - NULLBITS_BIT_C_PRO
  - PIMORONI_PICOLIPO_16MB
  - PIMORONI_PICOLIPO_4MB
  - PIMORONI_TINY2040
  - RPI_PICO
  - RPI_PICO_W
  - SPARKFUN_PROMICRO
  - SPARKFUN_THINGPLUS
  - W5100S_EVB_PICO
  - W5500_EVB_PICO
  - WAVESHARE_RP2040_LCD_1.28
  - WEACTSTUDIO

This is a work in progress.

-- Russ
