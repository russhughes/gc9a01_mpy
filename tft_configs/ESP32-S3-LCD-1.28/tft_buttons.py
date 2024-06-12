# input pins for ESP32-S3-LCD-1.28

from machine import Pin

class Buttons():
    def __init__(self):
        self.name = "ESP32-S3-LCD-1.28"
        self.left = Pin(0, Pin.IN)
        self.right = Pin(35, Pin.IN)
