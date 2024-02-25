"""
fonts.py
========

.. figure:: /_static/fonts.png
  :align: center

  Pages through all characters of four fonts on the Display.
"""

import utime
from machine import Pin, SPI
import gc9a01
import tft_config

import vga1_8x8 as font1
import vga1_8x16 as font2
import vga1_bold_16x16 as font3
import vga1_bold_16x32 as font4


def main():
    tft = tft_config.config(tft_config.TALL)

    tft.init()
    tft.fill(gc9a01.BLACK)
    utime.sleep(1)

    while True:
        for font in (font1, font2, font3, font4):
            tft.fill(gc9a01.BLUE)
            line = 0
            col = 0
            for char in range(font.FIRST, font.LAST):

                tft.text(font, chr(char), col, line, gc9a01.WHITE, gc9a01.BLUE)

                col += font.WIDTH
                if col > tft.width() - font.WIDTH:
                    col = 0
                    line += font.HEIGHT

                    if line > tft.height() - font.HEIGHT:
                        utime.sleep(3)
                        tft.fill(gc9a01.BLUE)
                        line = 0
                        col = 0

            utime.sleep(3)


main()
