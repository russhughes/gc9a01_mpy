#!/usr/bin/env python3
"""
Convert an image file to a python module for use with the bitmap method. Use redirection to save the
output to a file. The image is converted to a bitmap using the number of bits per pixel you specify.
The bitmap is saved as a python module that can be imported and used with the bitmap method.

.. seealso::
    - :ref:`alien.py<alien>`.

Example
^^^^^^^

.. code-block:: console

    # create python module from cat.png with 4 bits per pixel and save it to cat_bitmap.py.
    # Use '-s' to swap the color bytes for the GC9A01 display.

    ./image_converter cat.png 4 -s > cat_bitmap.py

    # cross compile the python module
    mpy-cross cat_bitmap.py

    # upload the compiled module to the board
    mpremote cp cat_bitmap.mpy :

The python file can be imported and displayed with the bitmap method. For example:

.. code-block:: python

    import tft_config
    import cat_bitmap
    tft = tft_config.config(1)
    tft.bitmap(cat_bitmap, 0, 0)

Usage
^^^^^

.. code-block:: console

    usage: image_converter.py [-h] [-s] image_file bits_per_pixel

    Convert image file to python module for use with bitmap method.

    positional arguments:
    image_file      Name of file containing image to convert
    bits_per_pixel  The number of bits to use per pixel (1..8)

    optional arguments:
    -h, --help      show this help message and exit
    -s, --swap      Swap color565 bytes
"""

import sys
import argparse
from PIL import Image

def main():

    parser = argparse.ArgumentParser(
        description='Convert image file to python module for use with bitmap method.')

    parser.add_argument(
        'image_file',
        help='Name of file containing image to convert')

    parser.add_argument(
        'bits_per_pixel',
        type=int,
        choices=range(1, 9),
        default=1,
        metavar='bits_per_pixel',
        help='The number of bits to use per pixel (1..8)')

    parser.add_argument(
        "-s",
        "--swap",
        action="store_true",
        help="Swap color565 bytes",
    )

    args = parser.parse_args()
    bits = args.bits_per_pixel
    colors_requested = 1 << bits
    img = Image.open(args.image_file)
    img = img.convert("P", palette=Image.Palette.ADAPTIVE, colors=colors_requested)
    palette = img.getpalette()  # Make copy of palette colors
    palette_colors = len(palette) // 3
    actual_colors = min(palette_colors, colors_requested)

    bits_required = palette_colors.bit_length()
    if (bits_required < bits):
        print(f'\nNOTE: Quantization reduced colors to {palette_colors} from the {colors_requested} '
        f'requested, reconverting using {bits_required} bit per pixel could save memory.\n''', file=sys.stderr)

    # For all the colors in the palette
    colors = []

    for color in range(actual_colors):

        # get rgb values and convert to 565
        color565 = (
            ((palette[color*3] & 0xF8) << 8)
            | ((palette[color*3+1] & 0xFC) << 3)
            | ((palette[color*3+2] & 0xF8) >> 3))

        # swap bytes in 565
        if (args.swap):
            color = ((color565 & 0xff) << 8) + ((color565 & 0xff00) >> 8)

        # append byte swapped 565 color to colors
        colors.append(f'{color:04x}')

    image_bitstring = ''
    max_colors = len(colors)

    # Run through the image and create a string with the ascii binary
    # representation of the color of each pixel.
    for y in range(img.height):
        for x in range(img.width):
            pixel = img.getpixel((x, y))
            color = pixel
            bstring = ''.join(
                '1' if (color & (1 << bit - 1)) else '0'
                for bit in range(bits, 0, -1)
            )

            image_bitstring += bstring

    bitmap_bits = len(image_bitstring)

    # Create python source with image parameters
    print(f'HEIGHT = {img.height}')
    print(f'WIDTH = {img.width}')
    print(f'COLORS = {actual_colors}')
    print(f'BITS = {bitmap_bits}')
    print(f'BPP = {bits}')
    print('PALETTE = [', sep='', end='')

    for color, rgb in enumerate(colors):
        if color:
            print(',', sep='', end='')
        print(f'0x{rgb}', sep='', end='')
    print("]")

    # Run though image bit string 8 bits at a time
    # and create python array source for memoryview

    print("_bitmap =\\", sep='')
    print("b'", sep='', end='')

    for i in range(0, bitmap_bits, 8):

        if i and i % (16*8) == 0:
            print("'\\\nb'", end='', sep='')

        value = image_bitstring[i:i+8]
        color = int(value, 2)
        print(f'\\x{color:02x}', sep='', end='')

    print("'\nBITMAP = memoryview(_bitmap)")


if __name__ == "__main__":
    main()
