"""tft1 of dual 240x240 GC9A01

240x240 GC9A01 display connected to a Raspberry Pi Pico.

.. list-table:: **Connections**
  :header-rows: 1

  * - Pico Pin
    - Display
  * - 14 (GP10)
    - BL
  * - 15 (GP11)
    - RST
  * - 16 (GP12)
    - DC
  * - 17 (GP13)
    - CS
  * - 18 (GND)
    - GND
  * - 19 (GP14)
    - CLK
  * - 20 (GP15)
    - DIN
"""

from machine import Pin, SPI
import gc9a01

TFA = 0
BFA = 0
WIDE = 1
TALL = 0


def config(rotation=0, buffer_size=0, options=0):
    """Configure the display and return an instance of gc9a01.GC9A01."""
    spi1 = SPI(1, baudrate=60000000, sck=Pin(14), mosi=Pin(15))
    tft1 = gc9a01.GC9A01(
        spi1,
        240,
        240,
        reset=Pin(11, Pin.OUT),
        cs=Pin(13, Pin.OUT),
        dc=Pin(12, Pin.OUT),
        backlight=Pin(10, Pin.OUT),
        rotation=rotation,
        options=options,
        buffer_size=buffer_size,
    )
