"""RP2040-Touch-LCD-1.28
"""

from machine import Pin


class Buttons:
    """Buttons class."""

    def __init__(self):
        self.name = "RP2040-Touch-LCD-1.28"
        self.left = 0
        self.right = 0
        self.hyper = 0
        self.thrust = 0
        self.fire = 0
