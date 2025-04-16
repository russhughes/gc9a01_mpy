"""M5-Dial-Touch-LCD-1.28
"""

from machine import Pin, SPI
import time
import gc9a01

TFA = 0
BFA = 0
WIDE = 0
TALL = 1

_rst_pin = Pin(8, mode=Pin.OUT, value=0)
time.sleep_ms(5)
_rst_pin.value(1)
time.sleep_ms(300)


def config(rotation=0, buffer_size=0, options=0):
    """Configure the display and return an instance of gc9a01.GC9A01."""

    spi = SPI(1, baudrate=60000000, sck=Pin(6), mosi=Pin(5))
    return gc9a01.GC9A01(
        spi,
        240,
        240,
        cs=Pin(7, Pin.OUT),
        dc=Pin(4, Pin.OUT),
        backlight=Pin(9, Pin.OUT),
        rotation=rotation,
        options=options,
        buffer_size=buffer_size
    )
