"""
circle.py
========
.. figure:: /_static/circle.png
  :align: center

Draws circle to determine visable pixels on a round display.
"""

import random
import time
import gc9a01
import tft_config

tft = tft_config.config(tft_config.TALL)
WIDTH = tft.width()
HEIGHT = tft.height()

def main():
    tft.init()
    tft.fill(0)
    tft.circle(119, 119, 119, gc9a01.color565(255, 255, 255))

main()
