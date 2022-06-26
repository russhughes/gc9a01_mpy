# GC9A01 Display Driver for MicroPython

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

Included are 12 bitmap fonts derived from classic pc text mode fonts, 26
Hershey vector fonts and several example programs for different devices. The
driver supports 240x240 displays.

## Pre-compiled firmware files

The firmware directory contains pre-compiled firmware for various devices with
the gc9a01 C driver and frozen python font files. See the README.md file in the
fonts folder for more information on the font files.

MicroPython v1.19.1-60-ge22b7fb4a compiled with ESP IDF v4.4 using CMake

Directory                       | File         | Device
------------------------------- | ------------ | ----------------------------------
ESP32-GENERIC-GC9A01-4M         | firmware.bin | Generic ESP32 devices
ESP32-GENERIC-GC9A01-16M        | firmware.bin | Generic ESP32 devices
ESP32-GENERIC-SPIRAM-GC9A01-4M  | firmware.bin | Generic ESP32 devices with SPI Ram
ESP32-GENERIC-SPIRAM-GC9A01-16M | firmware.bin | Generic ESP32 devices with SPI Ram
RP2                             | firmware.uf2 | Raspberry Pi Pico RP2040

## Additional Modules

Module             | Source
------------------ | -----------------------------------------------------------


This is a work in progress.

-- Russ

## Overview

This is a driver for MicroPython to handle cheap displays based on the GC9A01
chip.

<p align="center">
  <img src="https://raw.githubusercontent.com/russhughes/gc9a01_mpy/master/docs/GC9A01.jpg" alt="GC9A01 display photo"/>
</p>

The driver is written in C. Firmware is provided for ESP32, ESP32 with SPIRAM,
and Raspberry Pi Pico devices.


## CMake building instructions for MicroPython 1.14 and later

Prepare build tools as described in the manual. You should follow the
instruction for building MicroPython and ensure that you can build the
firmware without this display module.

Clone this module alongside the MPY sources:

    $ git clone https://github.com/russhughes/gc9a01_mpy.git


 ESP32 Build Example:

    $ cd micropython/ports/esp32

And then compile the module with specified USER_C_MODULES dir

    $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake all

Erase the target device if this is the first time uploading this firmware

    $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake erase

Upload the new firmware

    $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake deploy

### Additional build parameters

Additional build parameters may be specified during the build, erase and upload operations by including them after the `make` command. For example you can specify the DEBUG flag, Upload BAUD rate, alternate partition.csv file for different flash sizes and the USB serial port.

      make -j 4 \
        DEBUG=1 \
        BOARD=GENERIC \
        BAUD=3000000 \
        PART_SRC=partitions-16MiB.csv \
        PORT=/dev/tty.SLAB_USBtoUART \
        USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake all

## Working examples

This module was tested on ESP32 and the Raspberry Pi Pico. You need to provide a `SPI` object and the pin to use for
the DC pin of the screen.


    # ESP32

    import machine
    import gc9a01
    spi = machine.SPI(2, baudrate=40000000, polarity=1, sck=machine.Pin(18), mosi=machine.Pin(23))
    display = gc9a01.GC9A01(spi, 240, 240, reset=machine.Pin(4, machine.Pin.OUT), dc=machine.Pin(2, machine.Pin.OUT))
    display.init()


I was not able to run the display with a baudrate higher than 40MHZ.

## Methods

- `gc9a01.GC9A01(spi, width, height, reset, dc, cs, backlight, rotation, buffer_size)`

    required args:

        `spi` spi device
        `width` display width
        `height` display height

    optional args:

        `reset` reset pin
        `dc` dc pin
        `cs` cs pin
        `backlight` backlight pin
        `rotation` Orientation of display.
        `buffer_size` 0= buffer dynamically allocated and freed as needed.

        Rotation | Orientation
        -------- | --------------------
           0     | 0 degrees
           1     | 90 degrees
           2     | 180 degrees
           3     | 270 degrees
           4     | 0 degrees mirrored
           5     | 90 degrees mirrored
           6     | 180 degrees mirrored
           7     | 270 degrees mirrored


    If buffer_size is specified it must be large enough to contain the largest
    bitmap, font character and/or JPG used (Rows * Columns *2 bytes).
    Specifying a buffer_size reserves memory for use by the driver otherwise
    memory required is allocated and free dynamicly as it is needed.  Dynamic
    allocation can cause heap fragmentation so garbage collection (GC) should
    be enabled.


This driver supports only 16bit colors in RGB565 notation.

- `GC9A01.on()`

  Turn on the backlight pin if one was defined during init.

- `GC9A01.off()`

  Turn off the backlight pin if one was defined during init.

- `GC9A01.pixel(x, y, color)`

  Set the specified pixel to the given color.

- `GC9A01.line(x0, y0, x1, y1, color)`

  Draws a single line with the provided `color` from (`x0`, `y0`) to
  (`x1`, `y1`).

- `GC9A01.hline(x, y, length, color)`

  Draws a single horizontal line with the provided `color` and `length`
  in pixels. Along with `vline`, this is a fast version with reduced
  number of SPI calls.

- `GC9A01.vline(x, y, length, color)`

  Draws a single horizontal line with the provided `color` and `length`
  in pixels.

- `GC9A01.rect(x, y, width, height, color)`

  Draws a rectangle from (`x`, `y`) with corresponding dimensions

- `GC9A01.fill_rect(x, y, width, height, color)`

  Fill a rectangle starting from (`x`, `y`) coordinates

- `GC9A01.blit_buffer(buffer, x, y, width, height)`

  Copy bytes() or bytearray() content to the screen internal memory.
  Note: every color requires 2 bytes in the array

- `GC9A01.text(font, s, x, y[, fg, bg])`

  Write text to the display using the specified bitmap font with the
  coordinates as the upper-left corner of the text. The foreground and
  background colors of the text can be set by the optional arguments fg and bg,
  otherwise the foreground color defaults to `WHITE` and the background color
  defaults to `BLACK`.  See the `README.md` in the `fonts/bitmap` directory for
  example fonts.

- `GC9A01.write(bitap_font, s, x, y[, fg, bg])`

  Write text to the display using the specified proportional or Monospace bitmap
  font module with the coordinates as the upper-left corner of the text. The
  foreground and background colors of the text can be set by the optional
  arguments fg and bg, otherwise the foreground color defaults to `WHITE` and
  the background color defaults to `BLACK`.  See the `README.md` in the
  `truetype/fonts` directory for example fonts. Returns the width of the string
  as printed in pixels.

  The `font2bitmap` utility creates compatible 1 bit per pixel bitmap modules
  from Proportional or Monospaced True Type fonts. The character size,
  foreground, background colors and the characters to include in the bitmap
  module may be specified as parameters. Use the -h option for details. If you
  specify a buffer_size during the display initialization it must be large
  enough to hold the widest character (HEIGHT * MAX_WIDTH * 2).

- `GC9A01.write_len(bitap_font, s)`

  Returns the width of the string in pixels if printed in the specified font.

- `GC9A01.draw(vector_font, s, x, y[, fg, bg])`

  Draw text to the display using the specified hershey vector font with the
  coordinates as the lower-left corner of the text. The foreground and
  background colors of the text can be set by the optional arguments fg and bg,
  otherwise the foreground color defaults to `WHITE` and the background color
  defaults to `BLACK`.  See the README.md in the `vector/fonts` directory for
  example fonts and the utils directory for a font conversion program.

- `GC9A01.jpg(jpg_filename, x, y [, method])`

  Draw JPG file on the display at the given x and y coordinates as the upper
  left corner of the image. There memory required to decode and display a JPG
  can be considerable as a full screen 320x240 JPG would require at least 3100
  bytes for the working area + 320x240x2 bytes of ram to buffer the image. Jpg
  images that would require a buffer larger than available memory can be drawn
  by passing `SLOW` for method. The `SLOW` method will draw the image a piece
  at a time using the Minimum Coded Unit (MCU, typically 8x8) of the image.

- `GC9A01.bitmap(bitmap, x , y [, index])`

  Draw bitmap using the specified x, y coordinates as the upper-left corner of
  the of the bitmap. The optional index parameter provides a method to select
  from multiple bitmaps contained a bitmap module. The index is used to
  calculate the offset to the beginning of the desired bitmap using the modules
  HEIGHT, WIDTH and BPP values.

  The `imgtobitmap.py` utility creates compatible 1 to 8 bit per pixel bitmap
  modules from image files using the Pillow Python Imaging Library.

  The `monofont2bitmap.py` utility creates compatible 1 to 8 bit per pixel
  bitmap modules from Monospaced True Type fonts. See the `inconsolata_16.py`,
  `inconsolata_32.py` and `inconsolata_64.py` files in the `examples/lib` folder
  for sample modules and the `mono_font.py` program for an example using the
  generated modules.

  The character sizes, bit per pixel, foreground, background
  colors and the characters to include in the bitmap module may be specified as
  parameters. Use the -h option for details. Bits per pixel settings larger than
  one may be used to create antialiased characters at the expense of memory use.
  If you specify a buffer_size during the display initialization it must be
  large enough to hold the one character (HEIGHT * WIDTH * 2).

- `GC9A01.width()`

  Returns the current logical width of the display. (ie a 135x240 display
  rotated 90 degrees is 240 pixels wide)

- `GC9A01.height()`

  Returns the current logical height of the display. (ie a 135x240 display
  rotated 90 degrees is 135 pixels high)

- `GC9A01.rotation(r)`

  Set the rotates the logical display in a clockwise direction. 0-Portrait
  (0 degrees), 1-Landscape (90 degrees), 2-Inverse Portrait (180 degrees),
  3-Inverse Landscape (270 degrees)

The module exposes predefined colors:
  `BLACK`, `BLUE`, `RED`, `GREEN`, `CYAN`, `MAGENTA`, `YELLOW`, and `WHITE`


## Helper functions

- `color565(r, g, b)`

  Pack a color into 2-bytes rgb565 format

- `map_bitarray_to_rgb565(bitarray, buffer, width, color=WHITE, bg_color=BLACK)`

  Convert a bitarray to the rgb565 color buffer which is suitable for blitting.
  Bit 1 in bitarray is a pixel with `color` and 0 - with `bg_color`.

  This is a helper with a good performance to print text with a high
  resolution font. You can use an awesome tool
  https://github.com/peterhinch/micropython-font-to-py
  to generate a bitmap fonts from .ttf and use them as a frozen bytecode from
  the ROM memory.
