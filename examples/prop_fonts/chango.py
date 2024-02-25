"""
chango.py
=========

.. figure:: /_static/chango.png
  :align: center

  Test for font2bitmap converter.

"""

import gc9a01
import tft_config

import chango_16 as font_16
import chango_32 as font_32
import chango_64 as font_64


def main():
    """
    Initializes and clears the screen. Writes different strings
    using different fonts to the display.
    """
    # initialize display
    tft = tft_config.config(tft_config.TALL)

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLUE)

    row = 0

    tft.write(font_16, "abcdefghijklmnopqrstuvwxyz", 0, row, gc9a01.WHITE, gc9a01.BLUE)
    row += font_16.HEIGHT

    tft.write(font_32, "abcdefghijklm", 0, row, gc9a01.WHITE, gc9a01.BLUE)
    row += font_32.HEIGHT

    tft.write(font_32, "nopqrstuvwxy", 0, row, gc9a01.WHITE, gc9a01.BLUE)
    row += font_32.HEIGHT

    tft.write(font_64, "abcdef", 0, row, gc9a01.WHITE, gc9a01.BLUE)
    row += font_64.HEIGHT

    tft.write(font_64, "ghijkl", 0, row, gc9a01.WHITE, gc9a01.BLUE)
    row += font_64.HEIGHT


main()
