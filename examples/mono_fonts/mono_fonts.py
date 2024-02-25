"""
mono_fonts.py
=============

.. figure:: /_static/mono_fonts.png
  :align: center

  Test for write_font_converter.py and bitmap method

"""

import time
import gc9a01
import tft_config

import inconsolata_16 as font_16
import inconsolata_32 as font_32
import inconsolata_64 as font_64


def main():
    fast = False

    def display_font(font):
        tft.fill(gc9a01.BLUE)
        column = 0
        row = 0
        for char in font.MAP:
            tft.bitmap(font, column, row, font.MAP.index(char))
            column += font.WIDTH
            if column >= tft.width() - font.WIDTH:
                row += font.HEIGHT
                column = 0

                if row > tft.height() - font.HEIGHT:
                    row = 0

            if not fast:
                time.sleep(0.05)

    tft = tft_config.config(tft_config.TALL)

    tft.init()

    while True:
        for font in [font_16, font_32, font_64]:
            display_font(font)

        fast = not fast


main()
