'''
alien.py

    Randomly draw a jpg using the fast method on a GC9A01 display
    connected to an ESP32.

    The alien.jpg is from the Erik Flowers Weather Icons available from
    https://github.com/erikflowers/weather-icons and is licensed under
    SIL OFL 1.1

    It was was converted from the wi-alien.svg icon using
    ImageMagick's convert utility:

    convert wi-alien.svg -type TrueColor alien.jpg
'''

import gc
import random
from machine import Pin, SPI
import gc9a01

gc.enable()
gc.collect()


def main():
    '''
    Decode and draw jpg on display
    '''
    tft = gc9a01.GC9A01(
        SPI(2, baudrate=60000000, sck=Pin(18), mosi=Pin(23)),
        240,
        240,
        reset=Pin(33, Pin.OUT),
        cs=Pin(14, Pin.OUT),
        dc=Pin(27, Pin.OUT),
        backlight=Pin(32, Pin.OUT),
        rotation=0)
        
    # enable display and clear screen
    tft.init()

    # display jpg in random locations
    while True:
        tft.rotation(random.randint(0, 4))
        tft.jpg(
            "alien.jpg",
            random.randint(0, tft.width() - 30),
            random.randint(0, tft.height() - 30),
            gc9a01.FAST)


main()
