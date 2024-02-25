"""
scroll.py
=========

    Smoothly scroll all characters of a font up the display.
    Fonts heights must be even multiples of the screen height
    (i.e. 8 or 16 pixels high).
"""

import time
import s3lcd
import tft_config
import vga1_bold_16x32 as big
import vga1_8x8 as small


print(0)
tft = tft_config.config(tft_config.WIDE)


def cycle(p):
    try:
        len(p)
    except TypeError:
        cache = []
        for i in p:
            yield i
            cache.append(i)
        p = cache
    while p:
        yield from p


def main():

    try:
        tft.init()

        color = cycle(
            (
                s3lcd.RED,
                s3lcd.GREEN,
                s3lcd.BLUE,
                s3lcd.CYAN,
                s3lcd.MAGENTA,
                s3lcd.YELLOW,
                s3lcd.WHITE,
            )
        )

        foreground = next(color)
        background = s3lcd.BLACK

        tft.fill(background)

        height = tft.height()
        width = tft.width()

        font = small if tft.width() < 96 else big
        line = height - font.HEIGHT

        while True:
            for character in range(font.FIRST, font.LAST + 1):
                # write character hex value as a string
                tft.text(font, f"x{character:02x}", 16, line, foreground, background)

                # write character using a integer (could be > 0x7f)
                tft.text(
                    font,
                    character,
                    width - font.WIDTH * 2,
                    line,
                    foreground,
                    background,
                )

                # change color for next line
                foreground = next(color)

                # next character with rollover at 256
                character = (character +1) % height

                # scroll the screen up by one character height
                for _ in range(font.HEIGHT // 2):
                    tft.scroll(0, -2)
                    tft.show()

            time.sleep(1)

    finally:
        tft.deinit()


main()
