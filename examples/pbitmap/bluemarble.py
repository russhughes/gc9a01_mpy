"""
bluemarble.py
=============

    .. figure:: /_static/bluemarble.png
      :align: center

      Draw a full screen bitmap on the display.

    Convert bluemarble.jpg to bitmap module

    .. code-block:: console

      utils/image_converter.py bluemarble.jpg 8 >bluemarble.py

    Since the bitmap is large use mpy-cross to precompile the bluemarble.py module to save memory.

    .. code-block:: console

      mpy-cross bluemarble.py

    Upload the compiled bitmap module `bluemarble.mpy` and example program `pbitmap.py`

    Bluemarble image courtesy of the NASA image and video gallery available at
    https://images.nasa.gov/
"""

import gc
import time
import gc9a01
import tft_config

import bluemarble_bitmap


def main():
    """
    Draw the bitmap on the display
    """
    gc.enable()
    gc.collect()

    # initialize display
    tft = tft_config.config(tft_config.TALL)

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLACK)

    # display bitmap
    tft.pbitmap(bluemarble_bitmap, 0, 0)


main()
