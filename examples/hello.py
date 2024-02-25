"""
hello.py
========

.. figure:: /_static/hello.png
  :align: center

Writes "Hello!" in random colors at random locations on the display.
"""

import random

import vga1_bold_16x32 as font
import gc9a01
import tft_config

tft = tft_config.config(tft_config.TALL)


def main():

    tft.init()

    while True:
        for rotation in range(4):
            tft.rotation(rotation)
            tft.fill(0)
            col_max = tft.width() - font.WIDTH * 6
            row_max = tft.height() - font.HEIGHT

            for _ in range(128):
                tft.text(
                    font,
                    "Hello!",
                    random.randint(0, col_max),
                    random.randint(0, row_max),
                    gc9a01.color565(
                        random.getrandbits(8),
                        random.getrandbits(8),
                        random.getrandbits(8),
                    ),
                    gc9a01.color565(
                        random.getrandbits(8),
                        random.getrandbits(8),
                        random.getrandbits(8),
                    ),
                )


main()
