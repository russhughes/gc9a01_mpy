"""
lines.py
========
.. figure:: /_static/lines.png
  :align: center

Benchmarks the speed of drawing horizontal and vertical lines on the display.
"""

import random
import time
import gc9a01
import tft_config

tft = tft_config.config(tft_config.TALL)
WIDTH = tft.width()
HEIGHT = tft.height()


def time_function(func, *args):
    """time a function"""
    start = time.ticks_ms()
    func(*args)
    return time.ticks_ms() - start


def horizontal(line_count):
    """draw line_count horizontal lines on random y positions"""
    for _ in range(line_count):
        y = random.randint(0, HEIGHT)
        tft.line(
            0,
            y,
            WIDTH,
            y,
            gc9a01.color565(
                random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)
            ),
        )


def vertical(line_count):
    """draw line_count vertical lines on random x positions"""
    for _ in range(line_count):
        x = random.randint(0, WIDTH)
        tft.line(
            x,
            0,
            x,
            HEIGHT,
            gc9a01.color565(
                random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)
            ),
        )


def diagonal(line_count):
    """draw line_count diagnal lines on random y positions"""
    x = 0
    for _ in range(line_count):
        x += 1
        x %= WIDTH
        tft.line(
            x,
            0,
            WIDTH - x,
            HEIGHT,
            gc9a01.color565(
                random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)
            ),
        )


def main():
    tft.init()
    tft.fill(0)
    print("")
    print("horizonal: ", time_function(horizontal, 1000))
    print("vertical: ", time_function(vertical, 1000))
    print("diagonal: ", time_function(diagonal, 1000))


main()
