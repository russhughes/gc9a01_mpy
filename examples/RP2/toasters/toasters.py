'''
toasters.py

    An example using bitmap to draw sprites on a GC9A01 display connected
    to a Raspberry Pi Pico.

    Pico Pin   Display
    =========  =======
    14 (GP10)  BL
    15 (GP11)  RST
    16 (GP12)  DC
    17 (GP13)  CS
    18 (GND)   GND
    19 (GP14)  CLK
    20 (GP15)  DIN

    spritesheet from CircuitPython_Flying_Toasters
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters
'''

import time
import random
from machine import Pin, SPI
import gc9a01
import t1, t2, t3, t4, t5

TOASTERS = [t1, t2, t3, t4]
TOAST = [t5]


class toast():
    '''
    toast class to keep track of a sprites locaton and step
    '''
    def __init__(self, sprites, x, y):
        self.sprites = sprites
        self.steps = len(sprites)
        self.x = x
        self.y = y
        self.step = random.randint(0, self.steps-1)
        self.speed = random.randint(2, 5)

    def move(self):
        if self.x <= 0:
            self.speed = random.randint(2, 5)
            self.x = 240-64

        self.step += 1
        self.step %= self.steps
        self.x -= self.speed


def main():
    '''
    Draw and move sprite
    '''
    spi = SPI(1, baudrate=60000000, sck=Pin(14), mosi=Pin(15))
    tft = gc9a01.GC9A01(
        spi,
        240,
        240,
        reset=Pin(11, Pin.OUT),
        cs=Pin(13, Pin.OUT),
        dc=Pin(12, Pin.OUT),
        backlight=Pin(10, Pin.OUT),
        rotation=0)

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLACK)

    # create toast spites in random positions
    sprites = [
        toast(TOASTERS, tft.width()-64, 0),
        toast(TOAST, tft.width()-64*2, 80),
        toast(TOASTERS, tft.width()-64*4, 160)
    ]

    # move and draw sprites
    while True:
        for man in sprites:
            bitmap = man.sprites[man.step]
            tft.fill_rect(
                man.x+bitmap.WIDTH-man.speed,
                man.y,
                man.speed,
                bitmap.HEIGHT,
                gc9a01.BLACK)

            man.move()

            if man.x > 0:
                tft.bitmap(bitmap, man.x, man.y)
            else:
                tft.fill_rect(
                    0,
                    man.y,
                    bitmap.WIDTH,
                    bitmap.HEIGHT,
                    gc9a01.BLACK)

        time.sleep(0.05)


main()
