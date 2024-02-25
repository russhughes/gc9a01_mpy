"""
alien.py
========

    .. figure:: /_static/alien.png
      :align: center

      Randomly draw a jpg using the fast method on the display.

    The alien.jpg is from the Erik Flowers Weather Icons available from
    https://github.com/erikflowers/weather-icons and is licensed under
    SIL OFL 1.1

    It was was converted from the wi-alien.svg icon using
    ImageMagick's convert utility:

    convert wi-alien.svg -type TrueColor alien.jpg
"""

import gc
import random
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

    # display jpg in random locations
    while True:
        tft.rotation(random.randint(0, 4))
        tft.jpg(
            "alien.jpg",
            random.randint(0, width - 30),
            random.randint(0, height - 30),
            gc9a01.FAST,
        )


main()
