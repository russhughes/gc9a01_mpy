"""
hello.py

    Writes "Hello!" in random colors at random locations the display.

"""
import time
import random
from machine import Pin, SPI

import gc9a01
import vga1_bold_16x32 as font


def main():
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
    time.sleep(1)

    while True:
        for rotation in range(4):
            tft.rotation(rotation)
            tft.fill(0)
            col_max = tft.width() - font.WIDTH*6
            row_max = tft.height() - font.HEIGHT

            for _ in range(1000):
                tft.text(
                    font,
                    "Hello!",
                    random.randint(0, col_max),
                    random.randint(0, row_max),
                    gc9a01.color565(
                        random.getrandbits(8),
                        random.getrandbits(8),
                        random.getrandbits(8)),
                    gc9a01.color565(
                        random.getrandbits(8),
                        random.getrandbits(8),
                        random.getrandbits(8))
                )


main()
