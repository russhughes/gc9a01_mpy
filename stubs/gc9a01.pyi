BLACK = 0x0000
BLUE = 0x001F
RED = 0xF800
GREEN = 0x07E0
CYAN = 0x07FF
MAGENTA = 0xF81F
YELLOW = 0xFFE0
WHITE = 0xFFFF

WRAP_V = 0x01
WRAP_H = 0x02
WRAP = 0x03

JPG_MODE_FAST = 0
JPG_MODE_SLOW = 1


def color565(r: int, g: int, b: int):
    """Convert red, green, and blue values into a 16-bit RGB565 value.
      :param r: red value
      :param g: green value
      :param b: blue value
      :returns: 16-bit RGB565 value
    """


class GC9A01:

    def __init__(self, spi, width: int, height: int, reset=None, dc=None, cs=None, backlight=None, rotation: int = 0,
                 options: int = WRAP, buffer_size: int = 0):
        """ Create a GC9A01 display instance using the given SPI bus, display width, and display height.

          :param spi: SPI bus to use
          :param width: width of the display in pixels
          :param height: height of the display in pixels
          :param reset: reset pin to use
          :type reset: :class:`machine.Pin`
          :param dc: data/command pin to use
          :type dc: :class:`machine.Pin`
          :param cs: chip select pin to use
          :type cs: :class:`machine.Pin`
          :param backlight: backlight pin to use
          :type backlight: :class:`machine.Pin`
          :param rotation: rotation of the display (0-7) Rotation | Orientation.
                `0`: 0 degrees (default);
                `1`: 90 degrees;
                `2`: 180 degrees;
                `3`: 270 degrees;
                `4`: 0 degrees mirrored;
                `5`: 90 degrees mirrored;
                `6`: 180 degrees mirrored;
                `7`: 270 degrees mirrored;
          :param options: display options. ``WRAP``: pixels wrap at the edges;
                ``WRAP_H``: pixels wrap at the horizontal edges;
                ``WRAP_V``: pixels wrap at the vertical edges
          :param buffer_size: size of the display buffer (default 0 buffer is dynamically allocated and freed as needed.)
                If buffer_size is specified it must be large enough to contain the largest
                bitmap, font character and/or JPG used (Rows x Columns x 2 bytes).
                Specifying a buffer_size reserves memory for use by the driver otherwise
                memory required is allocated and freed dynamically as it is needed.  Dynamic
                allocation can cause heap fragmentation so garbage collection (GC) should
                be enabled.
        """

    def init(self):
        """ Initialize the display
        """

    def reset(self):
        """ Reset the display using the hardware reset pin.
        """

    def soft_reset(self):
        """ Reset the display using the software reset command
        """

    def sleep_mode(self, value):
        """Set the display sleep mode on or off (True/False).
            :param value: True/False
        """

    def set_window(self, x0: int, y0: int, x1: int, y1: int):
        """Set the frame memory write window.
        :param x0: x position of the top left corner
        :param y0: y position of the top left corner
        :param x1: x position of the bottom right corner
        :param y1: y position of the bottom right corner
        """

    def circle(self, x: int, y: int, r: int, color):
        """Draw a circle outline.
            Circle/Fill_Circle by https://github.com/c-logic

            https://github.com/russhughes/st7789_mpy/pull/46

            https://github.com/c-logic/st7789_mpy.git patch-1
          :param x: x position of the center
          :param y: y position of the center
          :param r: radius
          :param color: color of the circle
        """

    def fill_circle(self, x: int, y: int, r: int, color):
        """Draw a filled circle.
        Circle/Fill_Circle by https://github.com/c-logic

        https://github.com/russhughes/st7789_mpy/pull/46 https://github.com/c-logic/st7789_mpy.git patch-1
          :param x: x position of the center
          :param y: y position of the center
          :param r: radius
          :param color: color of the circle
        """

    def fill_rect(self, x: int, y: int, w: int, h: int, color):
        """Draw a filled rectangle.

          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param w: width
          :param h: height
          :param color: color of the rectangle
          """

    def fill(self, color):
        """Fill the entire display with a color.
         :param color: color to fill the display with
        """

    def pixel(self, x: int, y: int, color):
        """Draw a pixel.
          :param x: column position of the pixel
          :param y: row position of the pixel
          :param color: color of the pixel
        """

    def line(self, x0: int, y0: int, x1: int, y1: int, color):
        """Draw a line.
          :param x0: x position of the start point
          :param y0: y position of the start point
          :param x1: x position of the end point
          :param y1: y position of the end point
          :param color: color of the line
        """

    def blit_buffer(self, buffer, x: int, y: int, w: int, h: int):
        """Copy a color565 bytes() or bytearray() to the display.
          :param buffer: bytes() or bytearray() containing the pixel data
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param w: width of the buffer
          :param h: height of the buffer
        """

    def draw(self, font, text: str | int, x: int, y: int, color=WHITE, scale: float = 1):
        """Draw a string or character.

        See the README.md in the `vector/fonts` directory for
        example fonts and the utils directory for a font conversion program.

          :param font: font to use
          :param text: string or character to draw
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param color: color of the text, defaults to `WHITE`
          :param scale: scale of the text, defaults to 1
        """

    def draw_len(self, font, text: str | int, scale: float = 1) -> int:
        """Return the length of `string`  when drawn with the given font and scale.

          :param font: font to use
          :param text: string or character to draw
          :param scale: scale of the text, defaults to 1
          :returns: length of the string in pixels
          """

    def write_len(self, font, text: str | int) -> int:
        """Returns the width of the string or character in pixels if printed in the font.
          :param font: font to use
          :param text: string or character to draw
          :returns: width of the string in pixels
        """

    def write(self, font, text: str | int, x: int, y: int, fg_color=WHITE, bg_color=BLACK) -> int:
        """Write a string or character to the display. See the `README.md` in the `truetype/fonts`
          directory for example fonts. Returns the width of the string as printed in pixels.
          The `font2bitmap` utility creates compatible 1 bit per pixel bitmap modules
          from Proportional or Monospaced True Type fonts. The character size,
          foreground, background colors and the characters to include in the bitmap
          module may be specified as parameters. Use the -h option for details. If you
          specify a buffer_size during the display initialization it must be large
          enough to hold the widest character (HEIGHT * MAX_WIDTH * 2).
          :param font: font to use
          :param text: string or character to draw
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param fg_color: foreground color of the text, defaults to ``WHITE``
          :param bg_color: background color of the text, defaults to ``BLACK``
          :returns: Returns the width of the string as printed in pixels.
        """

    def bitmap(self, bitmap, x: int, y: int, idx: int = 0):
        """Draw a bitmap. Supports multiple bitmaps in one module that can be selected by index.
        The `image_converter.py` utility creates compatible 1 to 8 bit per pixel bitmap
        modules from image files using the Pillow Python Imaging Library.
          :param bitmap: bitmap to draw
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param idx: index of the bitmap to draw (default: 0)

        The `monofont2bitmap.py` utility creates compatible 1 to 8 bit per pixel
        bitmap modules from **Monospaced** True Type fonts. See the `inconsolata_16.py`,
        `inconsolata_32.py` and `inconsolata_64.py` files in the `examples/lib` folder
        for sample modules and the `mono_font.py` program for an example using the
        generated modules.

        The character sizes, bit per pixel, foreground, background
        colors and the characters to include in the bitmap module may be specified as
        parameters. Use the -h option for details. Bits per pixel settings larger than
        one may be used to create antialiased characters at the expense of memory use.
        If you specify a buffer_size during the display initialization it must be
        large enough to hold the one character (HEIGHT * WIDTH * 2).
        """

    def pbitmap(self, bitmap, x: int, y: int, idx: int = 0):
        """Progressively draw a bitmap one line at a time. Supports multiple bitmaps in one
        module that can be selected by index.
          :param bitmap: bitmap to draw
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param idx: index of the bitmap to draw (default: 0)
        """

    def text(self, font, text: str | int, x: int, y: int, fg_color=WHITE, bg_color=BLACK):
        """Draw a string or character using a converted PC BIOS ROM font.
        See the `README.md` in the `fonts/bitmap` directory for example fonts and
        the `font_from_romfont.py` utility in utils to convert PC BIOS ROM fonts from the
        font-bin directory of [spacerace's romfont repo.](https://github.com/spacerace/romfont)
          :param font: PC BIOS ROM font to use
          :param text: string or character to draw
          :param x: x position of the top left corner
          :param y: y position of the top left corner
          :param fg_color: foreground color of the text, defaults to ``WHITE``
          :param bg_color: background color of the text, defaults to ``BLACK``
        """

    def rotate(self, rotation: int):
        """Rotate the display to the given orientation.
          :param rotation: rotation of the display
              ``0``: portrait;
              ``1``: landscape;
              ``2``: inverted portrait;
              ``3``: inverted landscape;
              ``4``: portrait mirrored;
              ``5``: landscape mirrored;
              ``6``: inverted portrait mirrored;
              ``7``: inverted landscape mirrored;
        """

    def width(self) -> int:
        """Return the logical width of the display in the current orientation.
            :returns: width of the display
          """

    def height(self) -> int:
        """Return the logical height of the display in the current orientation.
          :returns: height of the display
        """

    def vscrdef(self, tfa: int, vsa: int, bfa: int = 0):
        """Set the hardware vertical scrolling definition.
          :param tfa: top fixed area
          :param vsa: vertical scrolling area
          :param bfa: bottom fixed area, defaults to 0
        """

    def vscsad(self, vssa: int):
        """Set the hardware vertical scrolling start address.
          :param vssa: vertical scrolling start address
        """

    def on(self):
        """Turn on the backlight pin if one was defined during init.
        """

    def off(self):
        """Turn off the backlight pin if one was defined during init.
        """

    def hline(self, x: int, y: int, w: int, color):
        """Draw a single horizontal line. This is a fast version with reduced number of SPI calls.
          :param x: column to start at
          :param y: row to start at
          :param w: width of the line
          :param color: color of the line
        """

    def vline(self, x: int, y: int, h: int, color):
        """Draw a vertical line. This is a fast version with reduced number of SPI calls.
          :param x: column to start at
          :param y: row to start at
          :param h: height of the line
          :param color: color of the line
        """

    def rect(self, x: int, y: int, w: int, h: int, color):
        """Draw the outline of a rectangle.
          :param x: column to start at
          :param y: row to start at
          :param w: width of the rectangle
          :param h: height of the rectangle
          :param color: color of the rectangle
        """

    def offset(self, x: int, y: int):
        """Set the xstart and ystart offset of the display.
          :param x: x offset
          :param y: y offset
        """

    def map_bitarray_to_rgb565(self, bitarray, buffer, width: int, color=WHITE, bg_color=BLACK):
        """Convert the given bitarray into a RGB565 bitmap.
          :param bitarray: bitarray to convert
          :param buffer: buffer to write the bitmap to
          :param width: width of the bitmap
          :param color: color of the bitarray (default: white)
          :param bg_color: background color of the bitarray (default: black)
        """

    def jpg(self, filename: str, x: int, y: int, mode: int):
        """Draw a JPG image.
        The memory required to decode and display a JPG can be considerable as a
        full screen 320x240 JPG would require at least 3100 bytes for the working
        area + 320x240x2 bytes of ram to buffer the image. Jpg images that would
        require a buffer larger than available memory can be drawn by passing `SLOW`
        for method. The `SLOW` method will draw the image a piece at a time using
        the Minimum Coded Unit (MCU, typically 8x8) of the image.
          :param filename: filename of the JPG image
          :param x: column to start at
          :param y: row to start at
          :param mode: mode to use for drawing the image (default: ``JPG_MODE_FAST``)
            `JPG_MODE_FAST` draw the entire image at once;
            `JPG_MODE_SLOW` draw the image progressively by each Minimum Coded Unit
          """

    def polygon_center(self, polygon: list[tuple[int, int]]) -> tuple[int, int]:
        """Return the center of a polygon as an (x, y) tuple.
          :param polygon: polygon to get the center of
          :return: the center of a polygon as an (x, y) tuple
        """

    def polygon(self, polygon: list[tuple[int, int]],
                x: int, y: int,
                color,
                angle: float = 0.0,
                cx: int = 0,
                cy: int = 0):
        """Draw a polygon.
        The polygon should consist of a list of (x, y) tuples forming a closed polygon.
        See `roids.py` for an example.
          :param polygon: polygon to draw
          :param x: column to start at
          :param y: row to start at
          :param color: color of the polygon
          :param angle: angle in radians to rotate the polygon (default: 0.0)
          :param cx: x coordinate of the center of rotation (default: 0)
          :param cy: y coordinate of the center of rotation (default: 0)
        """

    def fill_polygon(self, polygon: list[tuple[int, int]],
                     x: int, y: int,
                     color,
                     angle: float = 0.0,
                     cx: int = 0,
                     cy: int = 0):
        """Draw a filled polygon.
        The polygon should consist of a list of (x, y) tuples forming a closed polygon.
        See `watch.py` for an example.

          :param polygon: polygon to draw
          :param x: column to start at
          :param y: row to start at
          :param color: color of the polygon
          :param angle: angle in radians to rotate the polygon (default: 0.0)
          :param cx: x coordinate of the center of rotation (default: 0)
          :param cy: y coordinate of the center of rotation (default: 0)
        """

    def arc(self, x: int, y: int, start_angle: int, end_angle: int, segments: int, rx: int, ry: int, w: int, color):
        """Draw an arc.
          :param x: column to start at
          :param y: row to start at
          :param start_angle: start angle of the arc in degrees
          :param end_angle: end angle of the arc in degrees
          :param segments: number of segments to draw
          :param rx: x radius of the arc
          :param ry: y radius of the arc
          :param w: width of the arc (currently limited to 1)
          :param color: color of the arc
          """

    def fill_arc(self, x: int, y: int, start_angle: int, end_angle: int, segments: int, rx: int, ry: int, color):
        """Draw a filled arc.
          :param x: column to start at
          :param y: row to start at
          :param start_angle: start angle of the arc in degrees
          :param end_angle: end angle of the arc in degrees
          :param segments: number of segments to draw
          :param rx: x radius of the arc
          :param ry: y radius of the arc
          :param color: color of the arc
        """

    def bounding(self, status: bool = False, as_rect: bool = False) -> tuple[int, int, int, int]:
        """Enables or disables tracking the display area that has been written to. Initially, tracking is disabled.
          :param status: Pass a ``True`` value to enable tracking and False to disable it. Passing a ``True`` or ``False`` parameter will reset the current bounding rectangle to (display_width, display_height, 0, 0).
          :param as_rect: If ``True``, the returned tuple will contain (min_x, min_y, width, height) values.
          :returns: ``(x, y, w, h)``:  tuple of the bounding box or ``(min_x, min_y, width, height)`` if ``as_rect`` parameter is ``True``
        """
