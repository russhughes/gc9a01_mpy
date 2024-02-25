"""
bitarray.py
===========

.. figure:: /_static/bitarray.png
  :align: center

  An example using map_bitarray_to_rgb565 to draw sprites
"""

# pylint: disable=import-error
import random
import time
import gc9a01
import tft_config


tft = tft_config.config(1)

#
SPRITE_WIDTH = 16
SPRITE_HEIGHT = 16
SPRITE_STEPS = 3
# fmt: off
SPRITE_BITMAPS = [
    bytearray([
        0b00000000, 0b00000000,
        0b00000001, 0b11110000,
        0b00000111, 0b11110000,
        0b00001111, 0b11100000,
        0b00001111, 0b11000000,
        0b00011111, 0b10000000,
        0b00011111, 0b00000000,
        0b00011110, 0b00000000,
        0b00011111, 0b00000000,
        0b00011111, 0b10000000,
        0b00001111, 0b11000000,
        0b00001111, 0b11100000,
        0b00000111, 0b11110000,
        0b00000001, 0b11110000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000]),

    bytearray([
        0b00000000, 0b00000000,
        0b00000011, 0b11100000,
        0b00001111, 0b11111000,
        0b00011111, 0b11111100,
        0b00011111, 0b11111100,
        0b00111111, 0b11110000,
        0b00111111, 0b10000000,
        0b00111100, 0b00000000,
        0b00111111, 0b10000000,
        0b00111111, 0b11110000,
        0b00011111, 0b11111100,
        0b00011111, 0b11111100,
        0b00001111, 0b11111000,
        0b00000011, 0b11100000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000]),

    bytearray([
        0b00000000, 0b00000000,
        0b00000111, 0b11000000,
        0b00011111, 0b11110000,
        0b00111111, 0b11111000,
        0b00111111, 0b11111000,
        0b01111111, 0b11111100,
        0b01111111, 0b11111100,
        0b01111111, 0b11111100,
        0b01111111, 0b11111100,
        0b01111111, 0b11111100,
        0b00111111, 0b11111000,
        0b00111111, 0b11111000,
        0b00011111, 0b11110000,
        0b00000111, 0b11000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000]),

    bytearray([
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000,
        0b00000000, 0b00000000])]

# fmt: on


def collide(first, second):
    """return true if two sprites collide or overlap"""

    return (
        first.sprite_x + first.width > second.sprite_x
        and first.sprite_x < second.sprite_x + second.width
        and first.sprite_y + first.height > second.sprite_y
        and first.sprite_y < second.sprite_y + second.height
    )


class Pacman:
    """
    Pacman class to keep track of a sprites location and step
    """

    def __init__(self, sprites, width, height, steps):
        """create a new pacman sprite that does not overlap another sprite"""
        self.sprite_x = 0
        self.sprite_y = 0
        self.steps = steps
        self.step = random.randint(0, self.steps)
        self.width = width
        self.height = height

        # Big problem if there are too many sprites for the size of the display
        while True:
            self.sprite_x = random.randint(0, tft.width() - width)
            self.sprite_y = random.randint(0, tft.height() - height)
            # if this sprite does not overlap another sprite then break
            if not any(collide(self, sprite) for sprite in sprites):
                break

    def move(self):
        """move the sprite one step"""
        max_x = tft.width() - SPRITE_WIDTH
        self.step += 1
        self.step %= SPRITE_STEPS
        self.sprite_x += 1
        if self.sprite_x == max_x - 1:
            self.step = SPRITE_STEPS

        self.sprite_x %= max_x

    def draw(self, blitable):
        """draw the sprite on the screen"""
        tft.blit_buffer(blitable, self.sprite_x, self.sprite_y, self.width, self.height)


def main():
    """
    Draw on screen using map_bitarray_to_rgb565
    """

    # enable display and clear screen
    tft.init()
    tft.fill(gc9a01.BLACK)

    # convert bitmaps into rgb565 blitable buffers
    blitable = []
    for sprite_bitmap in SPRITE_BITMAPS:
        sprite = bytearray(512)
        tft.map_bitarray_to_rgb565(
            sprite_bitmap, sprite, SPRITE_WIDTH, gc9a01.YELLOW, gc9a01.BLACK
        )
        blitable.append(sprite)

    sprite_count = tft.width() // SPRITE_WIDTH * tft.height() // SPRITE_HEIGHT // 4

    # create pacman spites in random positions
    sprites = []
    for _ in range(sprite_count):
        sprites.append(Pacman(sprites, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_STEPS))

    # move and draw sprites
    while True:
        for sprite in sprites:
            sprite.move()
            sprite.draw(blitable[sprite.step])
        time.sleep(0.05)


main()
