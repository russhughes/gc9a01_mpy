"""
arcs.py
=======

.. figure:: /_static/arcs.png
  :align: center

  Arcs and Filled Arcs

Draw Arcs and Filled Arcs on the display.
"""

import gc9a01
import tft_config


def color_wheel(step=1):
    """Generator that yields a 565 color from the color wheel"""
    position = 0
    while True:
        # Adjust the position within the range [0, 255)
        adjusted_position = (255 - position) % 255

        # Calculate the RGB values based on the adjusted position
        if adjusted_position < 85:
            r, g, b = 255 - adjusted_position * 3, 0, adjusted_position * 3
        elif adjusted_position < 170:
            adjusted_position -= 85
            r, g, b = 0, adjusted_position * 3, 255 - adjusted_position * 3
        else:
            adjusted_position -= 170
            r, g, b = adjusted_position * 3, 255 - adjusted_position * 3, 0

        # Yield the next color in the wheel
        yield gc9a01.color565(r, g, b)

        # Move to the next position in the wheel
        position = (position + step) % 255


def main():
    """
    This function initializes the display and draws a series of colored arcs in a continuous loop.
    """

    # Initialize the TFT display and fill it with black color
    tft = tft_config.config(tft_config.WIDE)
    tft.init()
    tft.fill(0)

    # Get the width and height of the display
    width, height = tft.width(), tft.height()

    # Calculate the center of the display
    center_x, center_y = width // 2, height // 2

    # The radii of the arcs are equal to the half of the display's dimensions
    radius_x, radius_y = center_x, center_y

    # The step size for the angle of the arcs
    step = 5

    # Create a color wheel generator
    wheel = color_wheel()

    # Enter an infinite loop
    while True:
        # The start angle for the arcs
        start_angle = 0

        # Draw a series of arcs with increasing start and end angles
        for end_angle in range(0, 361, step):
            # Draw a filled arc with a color from the color wheel
            tft.fill_arc(
                center_x,
                center_y,
                start_angle,
                end_angle,
                5,
                radius_x,
                radius_y,
                next(wheel),
            )

            # Draw a series of white arcs with with increasing radii
            for fraction in range(20, 100, 33):
                # Calculate the radius based on the fraction of the display's height
                radius = int(fraction * center_y / 100 + 0.5)

                # Draw a white arc
                tft.arc(
                    center_x,
                    center_y,
                    start_angle,
                    end_angle,
                    step,
                    radius,
                    radius,
                    1,
                    gc9a01.WHITE,
                )

            # The start angle for the next arc is the end angle of the current arc
            start_angle = end_angle


# Call the main function
main()
