"""
chango.py test for font2bitmap converter.
"""

from machine import Pin, SPI
import gc9a01

import chango_16 as font_16
import chango_32 as font_32
import chango_64 as font_64


def main():

    try:

        spi = SPI(2, baudrate=60000000, sck=Pin(18), mosi=Pin(23))

        # initialize display
        tft = gc9a01.GC9A01(
            spi,
            240,
            240,
            reset=Pin(33, Pin.OUT),
            cs=Pin(14, Pin.OUT),
            dc=Pin(27, Pin.OUT),
            backlight=Pin(32, Pin.OUT),
            rotation=0,
            buffer_size=64*64*2)

        # enable display and clear screen
        tft.init()
        tft.fill(gc9a01.BLACK)

        row = 0
        tft.write(font_16, "abcdefghijklmnopqrstuvwxyz", 0, row)
        row += font_16.HEIGHT

        tft.write(font_32, "abcdefghijklm", 0, row)
        row += font_32.HEIGHT

        tft.write(font_32, "nopqrstuvwxy", 0, row)
        row += font_32.HEIGHT

        tft.write(font_64, "abcdef", 0, row)
        row += font_64.HEIGHT

        tft.write(font_64, "ghijkl", 0, row)

    finally:
        # shutdown spi
        if 'spi' in locals():
            spi.deinit()


main()
