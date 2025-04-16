import machine
import time


#  Rewritten by elmot from original Espressif's code
#  Original copyright notice follows:
#  @file ft3267.c
#  @brief ft3267 Capacitive Touch Panel Controller Driver
#  @version 0.1
#  @date 2021-01-13

#
#  @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#                http://www.apache.org/licenses/LICENSE-2.0

#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.

class TouchDriverFT3267(object):
    # @brief FT5x06 register map and function codes

    _FT5x06_ADDR = 0x38

    _FT5x06_DEVICE_MODE = 0x00
    _FT5x06_GESTURE_ID = 0x01  # probably not supported for FT3267
    _FT5x06_TOUCH_POINTS = 0x02

    _FT5x06_TOUCH1_EV_FLAG = 0x03
    _FT5x06_TOUCH1_XH = 0x03
    _FT5x06_TOUCH1_XL = 0x04
    _FT5x06_TOUCH1_YH = 0x05
    _FT5x06_TOUCH1_YL = 0x06

    _FT5x06_TOUCH2_EV_FLAG = 0x09
    _FT5x06_TOUCH2_XH = 0x09
    _FT5x06_TOUCH2_XL = 0x0A
    _FT5x06_TOUCH2_YH = 0x0B
    _FT5x06_TOUCH2_YL = 0x0C

    _FT5x06_TOUCH3_EV_FLAG = 0x0F
    _FT5x06_TOUCH3_XH = 0x0F
    _FT5x06_TOUCH3_XL = 0x10
    _FT5x06_TOUCH3_YH = 0x11
    _FT5x06_TOUCH3_YL = 0x12

    _FT5x06_TOUCH4_EV_FLAG = 0x15
    _FT5x06_TOUCH4_XH = 0x15
    _FT5x06_TOUCH4_XL = 0x16
    _FT5x06_TOUCH4_YH = 0x17
    _FT5x06_TOUCH4_YL = 0x18

    _FT5x06_TOUCH5_EV_FLAG = 0x1B
    _FT5x06_TOUCH5_XH = 0x1B
    _FT5x06_TOUCH5_XL = 0x1C
    _FT5x06_TOUCH5_YH = 0x1D
    _FT5x06_TOUCH5_YL = 0x1E

    _FT5x06_ID_G_THGROUP = 0x80
    _FT5x06_ID_G_THPEAK = 0x81
    _FT5x06_ID_G_THCAL = 0x82
    _FT5x06_ID_G_THWATER = 0x83
    _FT5x06_ID_G_THTEMP = 0x84
    _FT5x06_ID_G_THDIFF = 0x85
    _FT5x06_ID_G_CTRL = 0x86
    _FT5x06_ID_G_TIME_ENTER_MONITOR = 0x87
    _FT5x06_ID_G_PERIODACTIVE = 0x88
    _FT5x06_ID_G_PERIODMONITOR = 0x89
    _FT5x06_ID_G_AUTO_CLB_MODE = 0xA0
    _FT5x06_ID_G_LIB_VERSION_H = 0xA1
    _FT5x06_ID_G_LIB_VERSION_L = 0xA2
    _FT5x06_ID_G_CIPHER = 0xA3
    _FT5x06_ID_G_MODE = 0xA4
    _FT5x06_ID_G_PMODE = 0xA5
    _FT5x06_ID_G_FIRMID = 0xA6
    _FT5x06_ID_G_STATE = 0xA7
    _FT5x06_ID_G_FT5201ID = 0xA8
    _FT5x06_ID_G_ERR = 0xA9

    _i2c: machine.I2C
    _buf1 = bytearray(1)
    _buf4 = bytearray(4)

    def __init__(self, i2c: machine.I2C, int_pin: machine.Pin, rst_pin: machine.Pin | None = None):
        self._i2c = i2c
        self._int_pin = int_pin
        self._rst_pin = rst_pin

    def _read_byte(self, reg_addr: int) -> int:
        self._i2c.readfrom_mem_into(self._FT5x06_ADDR, reg_addr, self._buf1)
        return self._buf1[0]

    def _write_byte(self, reg_addr: int, data: int):
        self._buf1[0] = data
        self._i2c.writeto_mem(self._FT5x06_ADDR, reg_addr, self._buf1)

    def init(self):
        self._int_pin.init(mode=machine.Pin.IN)
        if self._rst_pin is not None:
            self._rst_pin.init(mode=machine.Pin.OUT, value=0)
            time.sleep_ms(10)
            self._rst_pin.value(1)
            time.sleep_ms(400)
        # Valid touching detect threshold
        self._write_byte(self._FT5x06_ID_G_THGROUP, 70)

        # valid touching peak detect threshold
        self._write_byte(self._FT5x06_ID_G_THPEAK, 60)

        # Touch focus threshold
        self._write_byte(self._FT5x06_ID_G_THCAL, 16)

        # threshold when there is surface water
        self._write_byte(self._FT5x06_ID_G_THWATER, 60)

        # threshold of temperature compensation
        self._write_byte(self._FT5x06_ID_G_THTEMP, 10)

        # Touch difference threshold
        self._write_byte(self._FT5x06_ID_G_THDIFF, 20)

        # Delay to enter 'Monitor' status (s)
        self._write_byte(self._FT5x06_ID_G_TIME_ENTER_MONITOR, 2)

        # Period of 'Active' status (ms)
        self._write_byte(self._FT5x06_ID_G_PERIODACTIVE, 12)

        # Timer to enter 'idle' when in 'Monitor' (ms)
        self._write_byte(self._FT5x06_ID_G_PERIODMONITOR, 40)

        # setting interrupt
        self._write_byte(self._FT5x06_ID_G_MODE, 0)

    def get_touch_points_num(self) -> int:
        if self._int_pin.value() == 1:
            return 0
        return self._read_byte(self._FT5x06_TOUCH_POINTS)

    def read_pos(self) -> list[tuple[int, int]] | None:
        if self._int_pin.value() == 1:
            return None
        num_touch = self.get_touch_points_num() & 0x0f
        result = []
        for i in range(num_touch):
            self._i2c.readfrom_mem_into(self._FT5x06_ADDR, self._FT5x06_TOUCH1_XH + i * 6, self._buf4)
            _x = ((self._buf4[0] & 0x0f) << 8) + self._buf4[1]
            _y = ((self._buf4[2] & 0x0f) << 8) + self._buf4[3]
            result.append((_x, _y))
        return result
