"""
noto_fonts Writes the names of three Noto fonts centered on the display
    using the font. The fonts were converted from True Type fonts using
    the font2bitmap utility.
"""

from machine import Pin, SPI
import gc9a01

import NotoSans_32 as noto_sans
import NotoSerif_32 as noto_serif
import NotoSansMono_32 as noto_mono


def main():

    def center(font, s, row, color=gc9a01.WHITE):
        screen = tft.width()                     # get screen width
        width = tft.write_len(font, s)           # get the width of the string
        if width and width < screen:             # if the string < display
            col = tft.width() // 2 - width // 2  # find the column to center
        else:                                    # otherwise
            col = 0                              # left justify

        tft.write(font, s, col, row, color)      # and write the string

    spi = SPI(2, baudrate=60000000, sck=Pin(18), mosi=Pin(23))
    tft = gc9a01.GC9A01(
        spi,
        240,
        240,
        reset=Pin(26, Pin.OUT),
        cs=Pin(13, Pin.OUT),
        dc=Pin(21, Pin.OUT),
        backlight=Pin(14, Pin.OUT),
        rotation=0,
        buffer_size=16*32*2)

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
