"""tft0 of dual 240x240 GC9A01

240x240 GC9A01 display connected to a Raspberry Pi Pico.

.. list-table:: **Connections**
  :header-rows: 1

  * - Pico Pin
    - Display
  * - 27 (GP21)
    - BL
  * - 26 (GP20)
    - RST
  * - 21 (GP16)
    - DC
  * - 22 (GP17)
    - CS
  * - 23 (GND)
    - GND
  * - 24 (GP18)
    - CLK
  * - 25 (GP19)
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
    spi0 = SPI(0, baudrate=60000000, sck=Pin(18), mosi=Pin(19))
    tft0 = gc9a01.GC9A01(
        spi0,
        240,
        240,
        reset=Pin(20, Pin.OUT),
        cs=Pin(17, Pin.OUT),
        dc=Pin(16, Pin.OUT),
        backlight=Pin(21, Pin.OUT),
        rotation=rotation,
        options=options,
        buffer_size=buffer_size,
    )
