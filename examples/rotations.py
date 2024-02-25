"""
rotations.py
============

.. figure:: /_static/rotations.png
  :align: center

  Shows the effect of each of the 8 rotation values on the display.
"""

import utime
import gc9a01
import tft_config
import vga1_bold_16x32 as font


def main():
    tft = tft_config.config(tft_config.TALL)

    tft.init()
    tft.fill(gc9a01.BLACK)
    utime.sleep(1)

    while True:
        for rot in range(8):
            tft.fill(gc9a01.BLACK)
            tft.rotation(rot)
            s = f"Rotation {rot}"
            tft.text(font, s, 40, 104, gc9a01.WHITE)
            utime.sleep(2)


main()
