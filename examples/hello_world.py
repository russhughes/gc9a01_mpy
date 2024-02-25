"""
hello_world.py
==============

.. figure:: /_static/hello_world.png
  :align: center

  Dual display Hello World example.

Writes "Hello World" in random colors at random locations split across a pair of GC9A01 displays
connected to a Raspberry Pi Pico.

"""

from machine import Pin, SPI
import random
import gc9a01
import tft_config0
import tft_config1

import vga1_bold_16x32 as font


def main():
    """
    Initializes tft0 and tft1 displays and displays "Hello" and
    "World" at random locations on both displays.
    """
    tft0 = tft_config0.config(tft_config0.TALL)
    tft1 = tft_config1.config(tft_config1.TALL)

    tft0.init()
    tft1.init()

    while True:
        for rotation in range(4):
            tft0.rotation(rotation)
            tft0.fill(0)

            tft1.rotation(rotation)
            tft1.fill(0)

            col_max = tft0.width() - font.WIDTH * 5
            row_max = tft0.height() - font.HEIGHT

            for _ in range(128):
                col = random.randint(0, col_max)
                row = random.randint(0, row_max)

                fg = gc9a01.color565(
                    random.getrandbits(8), random.getrandbits(8), random.getrandbits(8)
                )

                bg = gc9a01.color565(
                    random.getrandbits(8), random.getrandbits(8), random.getrandbits(8)
                )

                tft0.text(font, "Hello", col, row, fg, bg)
                tft1.text(font, "World", col, row, fg, bg)


main()
