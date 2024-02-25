"""
noto_fonts.py
=============

.. figure:: /_static/noto_fonts.png
  :align: center


Writes the names of three Noto fonts centered on the display using the font. The fonts were
converted from True Type fonts using the write_font_converter.py utility.
"""

from machine import SPI, Pin
import gc9a01
import tft_config

import NotoSans_32 as noto_sans
import NotoSerif_32 as noto_serif
import NotoSansMono_32 as noto_mono


def main():
    def center(font, s, row, color=gc9a01.WHITE):
        screen = tft.width()  # get screen width
        width = tft.write_len(font, s)  # get the width of the string
        col = tft.width() // 2 - width // 2 if width and width < screen else 0
        tft.write(font, s, col, row, color)  # and write the string

    tft = tft_config.config(tft_config.TALL)

    tft.init()

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLACK)

    # center the name of the first font, using the font
    row = 16
    center(noto_sans, "NotoSans", row, gc9a01.RED)
    row += noto_sans.HEIGHT

    # center the name of the second font, using the font
    center(noto_serif, "NotoSerif", row, gc9a01.GREEN)
    row += noto_serif.HEIGHT

    # center the name of the third font, using the font
    center(noto_mono, "NotoSansMono", row, gc9a01.BLUE)
    row += noto_mono.HEIGHT


main()
