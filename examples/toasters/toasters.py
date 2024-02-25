"""
toasters.py
===========

.. figure:: /_static/toasters.png
  :align: center

  An example using bitmap to draw sprites on the display.

Spritesheet from CircuitPython_Flying_Toasters
    https://learn.adafruit.com/circuitpython-sprite-animation-pendant-mario-clouds-flying-toasters
"""

import time
import random
import gc9a01
import tft_config

import t1, t2, t3, t4, t5

TOASTERS = [t1, t2, t3, t4]
TOAST = [t5]


class Toast:
    """
    toast class to keep track of a sprites locaton and step
    """

    def __init__(self, sprites, x, y, width, height):
        self.sprites = sprites
        self.steps = len(sprites)
        self.x = x
        self.y = y
        self.step = random.randint(0, self.steps - 1)
        self.speed = random.randint(2, 5)
        self.width = width
        self.height = height

    def move(self):
        """
        Moves an object horizontally on the screen. If the object reaches the left
        edge of the screen, it resets its position and speed.
        """
        if self.x <= 0:
            self.speed = random.randint(2, 5)
            self.x = self.width - 64

        self.step += 1
        self.step %= self.steps
        self.x -= self.speed


def main():
    """
    Draw and move sprite
    """
    tft = tft_config.config(tft_config.TALL)

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLACK)

    # cache width and height
    width = tft.width()
    height = tft.height()

    # create toast spites in random positions
    sprites = [
        Toast(TOASTERS, width - 64, 0, width, height),
        Toast(TOAST, width - 64 * 2, 80, width, height),
        Toast(TOASTERS, width - 64 * 4, 160, width, height),
    ]

    # move and draw sprites
    while True:
        for man in sprites:
            bitmap = man.sprites[man.step]
            tft.fill_rect(
                man.x + bitmap.WIDTH - man.speed,
                man.y,
                man.speed,
                bitmap.HEIGHT,
                gc9a01.BLACK,
            )

            man.move()

            if man.x > 0:
                tft.bitmap(bitmap, man.x, man.y)
            else:
                tft.fill_rect(0, man.y, bitmap.WIDTH, bitmap.HEIGHT, gc9a01.BLACK)

        time.sleep(0.05)


main()
