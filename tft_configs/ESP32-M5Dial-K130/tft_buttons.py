"""M5Dial K130 Touch screen buttons

Touch zones:

========  =====  ======  =======
 .        80px    80px    80px
========  =====  ======  =======
**80px**  Left   Thrust  Right
**80px**  Left   Fire    Right
**80px**  Left   Hyper   Right
========  =====  ======  =======

"""
import time
from machine import Pin, I2C
from ft3267 import TouchDriverFT3267


class Buttons:
    """Buttons class."""

    def __init__(self):
        self._last_read = time.ticks_ms()
        self._touch = TouchDriverFT3267(I2C(0, sda=Pin(11), scl=Pin(12), freq=400000), int_pin=Pin(14))
        self._touch.init()
        self._left = self._right = self._hyper = self._trust = self._fire = 1
        self.name = "M5Dial K130 Virtual Touch Buttons"
        self.left = type('obj', (object,), {'value': lambda s: self.read()._left})()
        self.right = type('obj', (object,), {'value': lambda s: self.read()._right})()
        self.hyper = type('obj', (object,), {'value': lambda s: self.read()._hyper})()
        self.thrust = type('obj', (object,), {'value': lambda s: self.read()._trust})()
        self.fire = type('obj', (object,), {'value': lambda s: self.read()._fire})()
        Buttons._initialized = self

    def read(self):
        now = time.ticks_ms()
        if time.ticks_diff(now, self._last_read) >= 50:
            self._last_read = now
            pos = self._touch.read_pos()
            self._left = self._right = self._hyper = self._trust = self._fire = 1
            if pos is not None:
                for idx, (x, y) in enumerate(pos):
                    if x < 80:
                        self._left = 0
                    elif x >= 160:
                        self._right = 0
                    elif y < 80:
                        self._trust = 0
                    elif y < 160:
                        self._fire = 0
                    else:
                        self._hyper = 0
        return self
