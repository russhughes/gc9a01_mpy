"""
jpg_test.py
===========

    Draw a full screen jpg using the slower but less memory intensive method
    of blitting each Minimum Coded Unit (MCU) block. Usually 8x8 pixels but can
    be other multiples of 8.

    .. figure:: /_static/bluemarble.png
      :align: center


    bigbuckbunny.jpg (c) copyright 2008, Blender Foundation / www.bigbuckbunny.org
"""

import gc
import time
import gc9a01
import tft_config


def main():
    """
    Decode and draw jpg on display
    """
    gc.enable()
    gc.collect()

    tft = tft_config.config(tft_config.TALL)

    # enable display and clear screen
    tft.init()

    # cache width and height
    width = tft.width()
    height = tft.height()

    # cycle thru jpg's
    while True:
        for image in ["bigbuckbunny.jpg", "bluemarble.jpg"]:
            tft.jpg(image, 0, 0, gc9a01.SLOW)
            time.sleep(5)


main()
