from _typeshed import Incomplete
from machine import SoftI2C as SoftI2C

class FocalTouch:
    chip: Incomplete
    bus: Incomplete
    address: Incomplete
    vend_id: Incomplete
    def __init__(self, i2c, address=..., debug: bool = False) -> None: ...
    @property
    def touched(self): ...
    @property
    def touches(self): ...
