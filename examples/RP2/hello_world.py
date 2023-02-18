"""
hello_world.py

    Writes "Hello World" in random colors at random locations split
    across a pair of GC9A01 displays connected to a Raspberry Pi Pico.

    Pico Pin   tft0
    =========  =======
    27 (GP21)  BL
    26 (GP20)  RST
    21 (GP16)  DC
    22 (GP17)  CS
    23 (GND)   GND
    24 (GP18)  CLK
    25 (GP19)  DIN

    Pico Pin   tft1
    =========  =======
    14 (GP10)  BL
    15 (GP11)  RST
    16 (GP12)  DC
    17 (GP13)  CS
    18 (GND)   GND
    19 (GP14)  CLK
    20 (GP15)  DIN

"""
from machine import Pin, SPI
import random
import gc9a01
import vga1_bold_16x32 as font


def main():
    spi0 = SPI(0, baudrate=60000000, sck=Pin(18), mosi=Pin(19))
    tft0 = gc9a01.GC9A01(
        spi0,
        240,
        240,
        reset=Pin(20, Pin.OUT),
        cs=Pin(17, Pin.OUT),
        dc=Pin(16, Pin.OUT),
        backlight=Pin(21, Pin.OUT),
        rotation=0
    )

    spi1 = SPI(1, baudrate=60000000, sck=Pin(14), mosi=Pin(15))
    tft1 = gc9a01.GC9A01(
        spi1,
        240,
        240,
        reset=Pin(11, Pin.OUT),
        cs=Pin(13, Pin.OUT),
        dc=Pin(12, Pin.OUT),
        backlight=Pin(10, Pin.OUT),
        rotation=0
    )

    tft0.init()
    tft1.init()

    while True:
        for rotation in range(4):
            tft0.rotation(rotation)
            tft0.fill(0)

            tft1.rotation(rotation)
            tft1.fill(0)

            col_max = tft0.width() - font.WIDTH*5
            row_max = tft0.height() - font.HEIGHT

            for _ in range(128):
                col = random.randint(0, col_max)
                row = random.randint(0, row_max)

                fg = gc9a01.color565(
                    random.getrandbits(8),
                    random.getrandbits(8),
                    random.getrandbits(8)
                )

                bg = gc9a01.color565(
                    random.getrandbits(8),
                    random.getrandbits(8),
                    random.getrandbits(8)
                )

                tft0.text(font, "Hello", col, row, fg, bg)
                tft1.text(font, "World", col, row, fg, bg)


main()
