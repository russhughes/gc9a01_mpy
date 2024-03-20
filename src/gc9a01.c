/*
 * Modifications and additions Copyright (c) 2020-2023 Russ Hughes
 *
 * This file licensed under the MIT License and incorporates work covered by
 * the following copyright and permission notice:
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ivan Belokobylskiy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#define __GC9A01_VERSION__  "0.1.6"

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mphal.h"

// Fix for MicroPython > 1.21 https://github.com/ricksorensen
#if MICROPY_VERSION_MAJOR >= 1 && MICROPY_VERSION_MINOR > 21
#include "extmod/modmachine.h"
#else
#include "extmod/machine_spi.h"
#endif

#include "gc9a01.h"

#include "mpfile.h"
#include "tjpgd565.h"

#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#define _swap_bytes(val) ((((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00))

#define ABS(N) (((N) < 0)?(-(N)):(N))
#define mp_hal_delay_ms(delay)  (mp_hal_delay_us(delay * 1000))

#define CS_LOW()     { if (self->cs) {mp_hal_pin_write(self->cs, 0);} }
#define CS_HIGH()    { if (self->cs) {mp_hal_pin_write(self->cs, 1);} }
#define DC_LOW()     (mp_hal_pin_write(self->dc, 0))
#define DC_HIGH()    (mp_hal_pin_write(self->dc, 1))
#define RESET_LOW()  { if (self->reset) mp_hal_pin_write(self->reset, 0); }
#define RESET_HIGH() { if (self->reset) mp_hal_pin_write(self->reset, 1); }
#define DISP_HIGH()  { if (self->backlight) mp_hal_pin_write(self->backlight, 1); }
#define DISP_LOW()   { if (self->backlight) mp_hal_pin_write(self->backlight, 0); }

// OPTIONAL_ARG helper macro
#define OPTIONAL_ARG(arg_num, arg_type, arg_obj_get, arg_name, arg_default) \
    arg_type arg_name = arg_default;                                        \
    if (n_args > arg_num) {                                                 \
        arg_name = arg_obj_get(args[arg_num]);                              \
    }                                                                       \

// MP_PI is not always defined in MicroPython
#ifndef MP_PI
#define MP_PI MICROPY_FLOAT_CONST(3.14159265358979323846)
#endif
#define RADIANS (MP_PI / MICROPY_FLOAT_CONST(180.0))
#define DEGREES (MICROPY_FLOAT_CONST(180.0) / MP_PI)

typedef struct _gc9a01_GC9A01_obj_t {
    mp_obj_base_t base;                             // base class
    mp_obj_base_t *spi_obj;                         // SPI object
    mp_file_t *fp;                                  // file object
    uint16_t *i2c_buffer;                           // resident buffer if buffer_size given
    void *work;                                     // work buffer for jpg decoding
    uint32_t buffer_size;                           // resident buffer size, 0=dynamic
    uint16_t display_width;                         // physical display width
    uint16_t width;                                 // logical width after rotation
    uint16_t display_height;                        // physical display width
    uint16_t height;                                // logical height after rotation
    uint8_t xstart;                                 // x offset to align to physical display
    uint8_t ystart;                                 // y offset to align to physical display
    uint8_t rotation;                               // current rotation setting
    uint8_t options;                                // options bit array
    mp_hal_pin_obj_t reset;                         // reset pin
    mp_hal_pin_obj_t dc;                            // data/command pin
    mp_hal_pin_obj_t cs;                            // chip select pin
    mp_hal_pin_obj_t backlight;                     // backlight pin
    uint8_t bounding;                               // bounding box mode
    uint16_t min_x;                                 // bounding box minimum x
    uint16_t min_y;                                 // bounding box minimum y
    uint16_t max_x;                                 // bounding box maximum x
    uint16_t max_y;                                 // bounding box maximum y
} gc9a01_GC9A01_obj_t;

// Point and Polygon structures
typedef struct _Point {
    mp_float_t x;
    mp_float_t y;
} Point;

typedef struct _Polygon {
    mp_int_t length;
    Point *points;
} Polygon;

///
/// ### GC9A01(`spi`, `width`, `height`  {, `reset`, `dc`, `cs`, `backlight`, `rotation`, `options`, `buffer_size`})
///
///   Create a GC9A01 display instance using the given SPI bus, display width, and display height.
///
///     * **Required Parameters:**
///         * ``spi``: SPI bus to use
///         * ``width``: width of the display in pixels
///         * ``height``: height of the display in pixels
///         * ``dc``: data/command pin to use
///
///     * **Optional Parameters:**
///
///         * ``reset``: reset pin to use
///         * ``cs``: chip select pin to use
///         * ``backlight``: backlight pin to use
///         * ``rotation``: rotation of the display (0-7)Rotation | Orientation
///
///             * ``0``: 0 degrees
///             * ``1``: 90 degrees
///             * ``2``: 180 degrees
///             * ``3``: 270 degrees
///             * ``4``: 0 degrees mirrored
///             * ``5``: 90 degrees mirrored
///             * ``6``: 180 degrees mirrored
///             * ``7``: 270 degrees mirrored
///
///         * ``options``: display options
///
///             * ``WRAP``: pixels wrap at the edges
///             * ``WRAP_H``: pixels wrap at the horizontal edges
///             * ``WRAP_V``: pixels wrap at the vertical edges
///
///         * ``buffer_size``: size of the display buffer (default 0 buffer is dynamically allocated and freed as needed.)
///
///     * **Returns:**
///         * ``GC9A01`` instance
///
/// If buffer_size is specified it must be large enough to contain the largest
/// bitmap, font character and/or JPG used (Rows x Columns x 2 bytes).
/// Specifying a buffer_size reserves memory for use by the driver otherwise
/// memory required is allocated and freed dynamically as it is needed.  Dynamic
/// allocation can cause heap fragmentation so garbage collection (GC) should
/// be enabled.

mp_obj_t gc9a01_GC9A01_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);


/// #### .init()
/// Initialize the display.

static mp_obj_t gc9a01_GC9A01_init(mp_obj_t self_in);


static void gc9a01_GC9A01_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "<GC9A01 width=%u, height=%u, spi=%p>", self->width, self->height, self->spi_obj);
}

static void write_spi(mp_obj_base_t *spi_obj, const uint8_t *buf, int len) {
    #if MICROPY_OBJ_TYPE_REPR == MICROPY_OBJ_TYPE_REPR_SLOT_INDEX
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(spi_obj->type, protocol);
    #else
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)spi_obj->type->protocol;
    #endif
    spi_p->transfer(spi_obj, len, buf, NULL);
}

static void write_cmd(gc9a01_GC9A01_obj_t *self, uint8_t cmd, const uint8_t *data, int len) {
    CS_LOW()
    if (cmd) {
        DC_LOW();
        write_spi(self->spi_obj, &cmd, 1);
    }
    if (len > 0) {
        DC_HIGH();
        write_spi(self->spi_obj, data, len);
    }
    CS_HIGH()
}

static void set_window(gc9a01_GC9A01_obj_t *self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    if (x0 > x1 || x1 >= self->width) {
        return;
    }
    if (y0 > y1 || y1 >= self->height) {
        return;
    }

    if (self->bounding) {
        if (x0 < self->min_x) {
            self->min_x = x0;
        }
        if (x1 > self->max_x) {
            self->max_x = x1;
        }
        if (y0 < self->min_y) {
            self->min_y = y0;
        }
        if (y1 > self->max_y) {
            self->max_y = y1;
        }
    }

    uint8_t bufx[4] = {(x0 + self->xstart) >> 8, (x0 + self->xstart) & 0xFF, (x1 + self->xstart) >> 8, (x1 + self->xstart) & 0xFF};
    uint8_t bufy[4] = {(y0 + self->ystart) >> 8, (y0 + self->ystart) & 0xFF, (y1 + self->ystart) >> 8, (y1 + self->ystart) & 0xFF};

    write_cmd(self, GC9A01_CASET, bufx, 4);
    write_cmd(self, GC9A01_RASET, bufy, 4);
    write_cmd(self, GC9A01_RAMWR, NULL, 0);
}

static void fill_color_buffer(mp_obj_base_t *spi_obj, uint16_t color, int length) {
    const int buffer_pixel_size = 128;
    int chunks = length / buffer_pixel_size;
    int rest = length % buffer_pixel_size;
    uint16_t color_swapped = _swap_bytes(color);
    uint16_t buffer[buffer_pixel_size];             // 128 pixels

    // fill buffer with color data
    for (int i = 0; i < length && i < buffer_pixel_size; i++) {
        buffer[i] = color_swapped;
    }
    if (chunks) {
        for (int j = 0; j < chunks; j++) {
            write_spi(spi_obj, (uint8_t *)buffer, buffer_pixel_size * 2);
        }
    }
    if (rest) {
        write_spi(spi_obj, (uint8_t *)buffer, rest * 2);
    }
}

void draw_pixel(gc9a01_GC9A01_obj_t *self, int16_t x, int16_t y, uint16_t color) {
    if ((self->options & OPTIONS_WRAP)) {
        if ((self->options & OPTIONS_WRAP_H) && ((x >= self->width) || (x < 0))) {
            x = x % self->width;
        }
        if ((self->options & OPTIONS_WRAP_V) && ((y >= self->height) || (y < 0))) {
            y = y % self->height;
        }
    }

    if ((x < self->width) && (y < self->height) && (x >= 0) && (y >= 0)) {
        uint8_t hi = color >> 8, lo = color & 0xff;

        set_window(self, x, y, x, y);
        DC_HIGH();
        CS_LOW();
        write_spi(self->spi_obj, &hi, 1);
        write_spi(self->spi_obj, &lo, 1);
        CS_HIGH();
    }
}

void fast_hline(gc9a01_GC9A01_obj_t *self, int16_t x, int16_t y, int16_t w, uint16_t color) {
    if ((self->options & OPTIONS_WRAP) == 0) {
        if (y >= 0 && self->width > x && self->height > y) {
            if (0 > x) {
                w += x;
                x = 0;
            }

            if (self->width < x + w) {
                w = self->width - x;
            }

            if (w > 0) {
                int16_t x2 = x + w - 1;

                set_window(self, x, y, x2, y);
                DC_HIGH();
                CS_LOW();
                fill_color_buffer(self->spi_obj, color, w);
                CS_HIGH();
            }
        }
    } else {
        for (int d = 0; d < w; d++) {
            draw_pixel(self, x + d, y, color);
        }
    }
}

static void fast_vline(gc9a01_GC9A01_obj_t *self, int16_t x, int16_t y, int16_t h, uint16_t color) {
    if ((self->options & OPTIONS_WRAP) == 0) {
        if (x >= 0 && self->width > x && self->height > y) {
            if (0 > y) {
                h += y;
                y = 0;
            }

            if (self->height < y + h) {
                h = self->height - y;
            }

            if (h > 0) {
                int16_t y2 = y + h - 1;

                set_window(self, x, y, x, y2);
                DC_HIGH();
                CS_LOW();
                fill_color_buffer(self->spi_obj, color, h);
                CS_HIGH();
            }
        }
    } else {
        for (int d = 0; d < h; d++) {
            draw_pixel(self, x, y + d, color);
        }
    }
}

static void line(gc9a01_GC9A01_obj_t *self, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t color) {

    bool steep = ABS(y1 - y0) > ABS(x1 - x0);

    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx = x1 - x0, dy = ABS(y1 - y0);
    int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

    if (y0 < y1) {
        ystep = 1;
    }

    // Split into steep and not steep for FastH/V separation
    if (steep) {
        for (; x0 <= x1; x0++) {
            dlen++;
            err -= dy;
            if (err < 0) {
                err += dx;
                if (dlen == 1) {
                    draw_pixel(self, y0, xs, color);
                } else {
                    fast_vline(self, y0, xs, dlen, color);
                }
                dlen = 0;
                y0 += ystep;
                xs = x0 + 1;
            }
        }
        if (dlen) {
            fast_vline(self, y0, xs, dlen, color);
        }
    } else {
        for (; x0 <= x1; x0++) {
            dlen++;
            err -= dy;
            if (err < 0) {
                err += dx;
                if (dlen == 1) {
                    draw_pixel(self, xs, y0, color);
                } else {
                    fast_hline(self, xs, y0, dlen, color);
                }
                dlen = 0;
                y0 += ystep;
                xs = x0 + 1;
            }
        }
        if (dlen) {
            fast_hline(self, xs, y0, dlen, color);
        }
    }
}

/// #### .reset()
/// Reset the display using the hardware reset pin.

static mp_obj_t gc9a01_GC9A01_hard_reset(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    CS_LOW();
    RESET_HIGH();
    mp_hal_delay_ms(50);
    RESET_LOW();
    mp_hal_delay_ms(50);
    RESET_HIGH();
    mp_hal_delay_ms(150);
    CS_HIGH();

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_hard_reset_obj, gc9a01_GC9A01_hard_reset);


/// #### .soft_reset()
/// Reset the display using the software reset command.

static mp_obj_t gc9a01_GC9A01_soft_reset(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    write_cmd(self, GC9A01_SWRESET, NULL, 0);
    mp_hal_delay_ms(150);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_soft_reset_obj, gc9a01_GC9A01_soft_reset);


/// #### .sleep_mode(value)
/// Set the display sleep mode on or off (True/False).
///
///     * **Required Parameters:**
///         * ``value``: True/False

static mp_obj_t gc9a01_GC9A01_sleep_mode(mp_obj_t self_in, mp_obj_t value) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (mp_obj_is_true(value)) {
        write_cmd(self, GC9A01_SLPIN, NULL, 0);
    } else {
        write_cmd(self, GC9A01_SLPOUT, NULL, 0);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(gc9a01_GC9A01_sleep_mode_obj, gc9a01_GC9A01_sleep_mode);


/// #### .set_window(`x0`, `y0`, `x1`, `y1`)
/// Set the frame memory write window.
///
///     * **Required Parameters:**
///         * ``x0``: x position of the top left corner
///         * ``y0``: y position of the top left corner
///         * ``x1``: x position of the bottom right corner
///         * ``y1``: y position of the bottom right corner

static mp_obj_t gc9a01_GC9A01_set_window(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t x1 = mp_obj_get_int(args[2]);
    mp_int_t y0 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);

    set_window(self, x0, y0, x1, y1);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_set_window_obj, 5, 5, gc9a01_GC9A01_set_window);


//
// #### .inversion_mode(`value`)
// Turn the color inversion mode on or off (True/False).
//

static mp_obj_t gc9a01_GC9A01_inversion_mode(mp_obj_t self_in, mp_obj_t value) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (mp_obj_is_true(value)) {
        write_cmd(self, GC9A01_INVON, NULL, 0);
    } else {
        write_cmd(self, GC9A01_INVOFF, NULL, 0);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(gc9a01_GC9A01_inversion_mode_obj, gc9a01_GC9A01_inversion_mode);


/// #### .circle(`x`, `y`, `r`, `color`)
/// Draw a circle outline.
///
///     * **Required Parameters:**
///         * ``x``: x position of the center
///         * ``y``: y position of the center
///         * ``r``: radius
///         * ``color``: color of the circle
///
/// Circle/Fill_Circle by https://github.com/c-logic
/// https://github.com/russhughes/st7789_mpy/pull/46 https://github.com/c-logic/st7789_mpy.git patch-1

static mp_obj_t gc9a01_GC9A01_circle(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xm = mp_obj_get_int(args[1]);
    mp_int_t ym = mp_obj_get_int(args[2]);
    mp_int_t r = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);
    mp_int_t f = 1 - r;
    mp_int_t ddF_x = 1;
    mp_int_t ddF_y = -2 * r;
    mp_int_t x = 0;
    mp_int_t y = r;

    draw_pixel(self, xm, ym + r, color);
    draw_pixel(self, xm, ym - r, color);
    draw_pixel(self, xm + r, ym, color);
    draw_pixel(self, xm - r, ym, color);
    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        draw_pixel(self, xm + x, ym + y, color);
        draw_pixel(self, xm - x, ym + y, color);
        draw_pixel(self, xm + x, ym - y, color);
        draw_pixel(self, xm - x, ym - y, color);
        draw_pixel(self, xm + y, ym + x, color);
        draw_pixel(self, xm - y, ym + x, color);
        draw_pixel(self, xm + y, ym - x, color);
        draw_pixel(self, xm - y, ym - x, color);
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_circle_obj, 5, 5, gc9a01_GC9A01_circle);


/// #### .fill_circle(`x`, `y`, `r`, `color`)
/// Draw a filled circle.
///
///     * **Required Parameters:**
///         * ``x``: x position of the center
///         * ``y``: y position of the center
///         * ``r``: radius
///         * ``color``: color of the circle
///
/// Circle/Fill_Circle by https://github.com/c-logic
/// https://github.com/russhughes/st7789_mpy/pull/46 https://github.com/c-logic/st7789_mpy.git patch-1

static mp_obj_t gc9a01_GC9A01_fill_circle(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xm = mp_obj_get_int(args[1]);
    mp_int_t ym = mp_obj_get_int(args[2]);
    mp_int_t r = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);
    mp_int_t f = 1 - r;
    mp_int_t ddF_x = 1;
    mp_int_t ddF_y = -2 * r;
    mp_int_t x = 0;
    mp_int_t y = r;

    fast_vline(self, xm, ym - y, 2 * y + 1, color);

    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        fast_vline(self, xm + x, ym - y, 2 * y + 1, color);
        fast_vline(self, xm + y, ym - x, 2 * x + 1, color);
        fast_vline(self, xm - x, ym - y, 2 * y + 1, color);
        fast_vline(self, xm - y, ym - x, 2 * x + 1, color);
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_fill_circle_obj, 5, 5, gc9a01_GC9A01_fill_circle);


/// #### .fill_rect(`x`, `y`, `w`, `h`, `color`)
/// Draw a filled rectangle.
///
///     * **Required Parameters:**
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///         * ``w``: width
///         * ``h``: height
///         * ``color``: color of the rectangle

static mp_obj_t gc9a01_GC9A01_fill_rect(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);
    uint16_t right = x + w - 1;
    uint16_t bottom = y + h - 1;

    if (x < self->width && y < self->height) {
        if (right > self->width) {
            right = self->width;
        }

        if (bottom > self->height) {
            bottom = self->height;
        }

        set_window(self, x, y, right, bottom);
        DC_HIGH();
        CS_LOW();
        fill_color_buffer(self->spi_obj, color, w * h);
        CS_HIGH();
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_fill_rect_obj, 6, 6, gc9a01_GC9A01_fill_rect);


/// #### .fill(`color`)
/// Fill the entire display with a color.
//
///     * **Required Parameters:**
///         * ``color``: color to fill the display with

static mp_obj_t gc9a01_GC9A01_fill(mp_obj_t self_in, mp_obj_t _color) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t color = mp_obj_get_int(_color);

    set_window(self, 0, 0, self->width - 1, self->height - 1);
    DC_HIGH();
    CS_LOW();
    fill_color_buffer(self->spi_obj, color, self->width * self->height);
    CS_HIGH();

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(gc9a01_GC9A01_fill_obj, gc9a01_GC9A01_fill);


/// #### .pixel(`x`, `y`, `color`)
/// Draw a pixel.
///
///     * **Required Parameters:**
///         * ``x``: column position of the pixel
///         * ``y``: row position of the pixel
///         * ``color``: color of the pixel

static mp_obj_t gc9a01_GC9A01_pixel(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t color = mp_obj_get_int(args[3]);

    draw_pixel(self, x, y, color);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_pixel_obj, 4, 4, gc9a01_GC9A01_pixel);


/// #### .line(`x0`, `y0`, `x1`, `y1`, `color`)
/// Draw a line.
///
///     * **Required Parameters:**
///         * ``x0``: x position of the start point
///         * ``y0``: y position of the start point
///         * ``x1``: x position of the end point
///         * ``y1``: y position of the end point
///         * ``color``: color of the line

static mp_obj_t gc9a01_GC9A01_line(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t y0 = mp_obj_get_int(args[2]);
    mp_int_t x1 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);

    line(self, x0, y0, x1, y1, color);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_line_obj, 6, 6, gc9a01_GC9A01_line);


/// #### .blit_buffer(`buffer`, `x`, `y`, `w`, `h`)
/// Copy a color565 bytes() or bytearray() to the display.
///
///     * **Required Parameters:**
///         * ``buffer``: bytes() or bytearray() containing the pixel data
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///         * ``w``: width of the buffer
///         * ``h``: height of the buffer

static mp_obj_t gc9a01_GC9A01_blit_buffer(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t buf_info;

    mp_get_buffer_raise(args[1], &buf_info, MP_BUFFER_READ);

    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);

    set_window(self, x, y, x + w - 1, y + h - 1);
    DC_HIGH();
    CS_LOW();

    const int buf_size = 256;
    int limit = MIN(buf_info.len, w * h * 2);
    int chunks = limit / buf_size;
    int rest = limit % buf_size;
    int i = 0;

    for (; i < chunks; i++) {
        write_spi(self->spi_obj, (const uint8_t *)buf_info.buf + i * buf_size, buf_size);
    }
    if (rest) {
        write_spi(self->spi_obj, (const uint8_t *)buf_info.buf + i * buf_size, rest);
    }
    CS_HIGH();

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_blit_buffer_obj, 6, 6, gc9a01_GC9A01_blit_buffer);


/// #### .draw(`font`, `string|int`, `x`, `y`, {`color` , `scale`})
/// Draw a string or character.
///
/// See the README.md in the `vector/fonts` directory for
/// example fonts and the utils directory for a font conversion program.
///
///     * **Required Parameters:**
///         * ``font``: font to use
///         * ``string|int``: string or character to draw
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///
///     * **Optional Parameters:**
///         * ``color``: color of the text, defaults to `WHITE`
///         * ``scale``: scale of the text, defaults to 1

static mp_obj_t gc9a01_GC9A01_draw(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    char single_char_s[] = {0, 0};
    const char *s;
    mp_obj_module_t *hershey = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);

        single_char_s[0] = c & 0xff;
        s = single_char_s;
    } else {
        s = mp_obj_str_get_str(args[2]);
    }

    mp_int_t x = mp_obj_get_int(args[3]);
    mp_int_t y = mp_obj_get_int(args[4]);
    mp_int_t color = (n_args > 5) ? mp_obj_get_int(args[5]) : WHITE;
    mp_float_t scale = 1.0;

    if (n_args > 6) {
        if (mp_obj_is_float(args[6])) {
            scale = mp_obj_float_get(args[6]);
        }
        if (mp_obj_is_int(args[6])) {
            scale = (mp_float_t)mp_obj_get_int(args[6]);
        }
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(hershey->globals);
    mp_obj_t *index_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_INDEX));
    mp_buffer_info_t index_bufinfo;

    mp_get_buffer_raise(index_data_buff, &index_bufinfo, MP_BUFFER_READ);

    uint8_t *index = index_bufinfo.buf;
    mp_obj_t *font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t font_bufinfo;

    mp_get_buffer_raise(font_data_buff, &font_bufinfo, MP_BUFFER_READ);

    int8_t *font = font_bufinfo.buf;
    int16_t from_x = x;
    int16_t from_y = y;
    int16_t to_x = x;
    int16_t to_y = y;
    int16_t pos_x = x;
    int16_t pos_y = y;
    bool penup = true;
    char c;
    int16_t ii;

    while ((c = *s++)) {
        if (c >= 32 && c <= 127) {
            ii = (c - 32) * 2;

            int16_t offset = index[ii] | (index[ii + 1] << 8);
            int16_t length = font[offset++];
            int16_t left = (int)(scale * (font[offset++] - 0x52) + MICROPY_FLOAT_CONST(0.5));
            int16_t right = (int)(scale * (font[offset++] - 0x52) + MICROPY_FLOAT_CONST(0.5));
            int16_t width = right - left;

            if (length) {
                int16_t i;

                for (i = 0; i < length; i++) {
                    if (font[offset] == ' ') {
                        offset += 2;
                        penup = true;
                        continue;
                    }

                    int16_t vector_x = (int)(scale * (font[offset++] - 0x52) + MICROPY_FLOAT_CONST(0.5));
                    int16_t vector_y = (int)(scale * (font[offset++] - 0x52) + MICROPY_FLOAT_CONST(0.5));

                    if (!i || penup) {
                        from_x = pos_x + vector_x - left;
                        from_y = pos_y + vector_y;
                    } else {
                        to_x = pos_x + vector_x - left;
                        to_y = pos_y + vector_y;

                        line(self, from_x, from_y, to_x, to_y, color);
                        from_x = to_x;
                        from_y = to_y;
                    }
                    penup = false;
                }
            }
            pos_x += width;
        }
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_draw_obj, 5, 7, gc9a01_GC9A01_draw);


/// #### .draw_len(`font`, `string|int` {, `scale`})
/// Return the length of `string`  when drawn with the given font and scale.
///
///     * **Required Parameters:**
///         * ``font``: font to use
///         * ``string|int``: string or character to draw
///
///     * **Optional Parameters:**
///         * ``scale``: scale of the text, defaults to 1
///
///     * **Returns:**
///         * ``int``: length of the string in pixels

static mp_obj_t gc9a01_GC9A01_draw_len(size_t n_args, const mp_obj_t *args) {
    char single_char_s[] = {0, 0};
    const char *s;
    mp_obj_module_t *hershey = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);

        single_char_s[0] = c & 0xff;
        s = single_char_s;
    } else {
        s = mp_obj_str_get_str(args[2]);
    }

    mp_float_t scale = 1.0;

    if (n_args > 3) {
        if (mp_obj_is_float(args[3])) {
            scale = mp_obj_float_get(args[3]);
        }
        if (mp_obj_is_int(args[3])) {
            scale = (mp_float_t)mp_obj_get_int(args[3]);
        }
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(hershey->globals);
    mp_obj_t *index_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_INDEX));
    mp_buffer_info_t index_bufinfo;

    mp_get_buffer_raise(index_data_buff, &index_bufinfo, MP_BUFFER_READ);

    uint8_t *index = index_bufinfo.buf;
    mp_obj_t *font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t font_bufinfo;

    mp_get_buffer_raise(font_data_buff, &font_bufinfo, MP_BUFFER_READ);

    int8_t *font = font_bufinfo.buf;
    int16_t print_width = 0;
    char c;
    int16_t ii;

    while ((c = *s++)) {
        if (c >= 32 && c <= 127) {
            ii = (c - 32) * 2;

            int16_t offset = (index[ii] | (index[ii + 1] << 8)) + 1;
            int16_t left = font[offset++] - 0x52;
            int16_t right = font[offset++] - 0x52;
            int16_t width = right - left;

            print_width += width;
        }
    }
    return mp_obj_new_int((int)(print_width * scale + MICROPY_FLOAT_CONST(0.5)));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_draw_len_obj, 3, 4, gc9a01_GC9A01_draw_len);


static uint32_t bs_bit = 0;
uint8_t *bitmap_data = NULL;

uint8_t get_color(uint8_t bpp) {
    uint8_t color = 0;
    int i;

    for (i = 0; i < bpp; i++) {
        color <<= 1;
        color |= (bitmap_data[bs_bit / 8] & 1 << (7 - (bs_bit % 8))) > 0;
        bs_bit++;
    }
    return color;
}

mp_obj_t dict_lookup(mp_obj_t self_in, mp_obj_t index) {
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(self_in);
    mp_map_elem_t *elem = mp_map_lookup(&self->map, index, MP_MAP_LOOKUP);

    if (elem == NULL) {
        return NULL;
    } else {
        return elem->value;
    }
}

/// #### .write_len(`font`, `string|int`)
/// Returns the width of the string or character in pixels if printed in the font.
///
///     * **Required Parameters:**
///         * ``font``: font to use
///         * ``string|int``: string or character to draw
///
///     * **Returns:**
///         * ``int``: width of the string in pixels

static mp_obj_t gc9a01_GC9A01_write_len(size_t n_args, const mp_obj_t *args) {
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);
    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    mp_obj_t widths_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTHS));
    mp_buffer_info_t widths_bufinfo;
    mp_get_buffer_raise(widths_data_buff, &widths_bufinfo, MP_BUFFER_READ);
    const uint8_t *widths_data = widths_bufinfo.buf;

    uint16_t print_width = 0;

    mp_obj_t map_obj = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_MAP));
    GET_STR_DATA_LEN(map_obj, map_data, map_len);
    GET_STR_DATA_LEN(args[2], str_data, str_len);
    const byte *s = str_data, *top = str_data + str_len;

    while (s < top) {
        unichar ch;
        ch = utf8_get_char(s);
        s = utf8_next_char(s);

        const byte *map_s = map_data, *map_top = map_data + map_len;
        uint16_t char_index = 0;

        while (map_s < map_top) {
            unichar map_ch;
            map_ch = utf8_get_char(map_s);
            map_s = utf8_next_char(map_s);

            if (ch == map_ch) {
                print_width += widths_data[char_index];
                break;
            }
            char_index++;
        }
    }
    return mp_obj_new_int(print_width);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_write_len_obj, 3, 3, gc9a01_GC9A01_write_len);




/// #### .write(`font`, `string|int`, `x`, `y`, {`fg_color`, `bg_color`})
/// Write a string or character to the display. See the `README.md` in the `truetype/fonts`
/// directory for example fonts. Returns the width of the string as printed in pixels.
///
/// The `font2bitmap` utility creates compatible 1 bit per pixel bitmap modules
/// from Proportional or Monospaced True Type fonts. The character size,
/// foreground, background colors and the characters to include in the bitmap
/// module may be specified as parameters. Use the -h option for details. If you
/// specify a buffer_size during the display initialization it must be large
/// enough to hold the widest character (HEIGHT * MAX_WIDTH * 2).
///
///     * **Required Parameters:**
///         * ``font``: font to use
///         * ``string|int``: string or character to draw
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///
///     * **Optional Parameters:**
///         * ``fg_color``: foreground color of the text, defaults to `WHITE`
///         * ``bg_color``: background color of the text, defaults to `BLACK`
///
///     * **Returns:**
///         * ``int``: The width of the string or character in pixels.

static mp_obj_t gc9a01_GC9A01_write(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);

    mp_int_t x = mp_obj_get_int(args[3]);
    mp_int_t y = mp_obj_get_int(args[4]);
	uint16_t fg_color = (n_args > 5) ? _swap_bytes(mp_obj_get_int(args[5])) : _swap_bytes(WHITE);
	uint16_t bg_color = (n_args > 6) ? _swap_bytes(mp_obj_get_int(args[6])) : _swap_bytes(BLACK);

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    const uint8_t bpp = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BPP)));
    const uint8_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint8_t offset_width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_OFFSET_WIDTH)));
	const uint8_t max_width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_MAX_WIDTH)));

    mp_obj_t widths_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTHS));
    mp_buffer_info_t widths_bufinfo;
    mp_get_buffer_raise(widths_data_buff, &widths_bufinfo, MP_BUFFER_READ);
    const uint8_t *widths_data = widths_bufinfo.buf;

    mp_obj_t offsets_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_OFFSETS));
    mp_buffer_info_t offsets_bufinfo;
    mp_get_buffer_raise(offsets_data_buff, &offsets_bufinfo, MP_BUFFER_READ);
    const uint8_t *offsets_data = offsets_bufinfo.buf;

    mp_obj_t bitmaps_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAPS));
    mp_buffer_info_t bitmaps_bufinfo;
    mp_get_buffer_raise(bitmaps_data_buff, &bitmaps_bufinfo, MP_BUFFER_READ);
    bitmap_data = bitmaps_bufinfo.buf;

	uint32_t buf_size = max_width * height * 2;
	if (self->buffer_size == 0) {
		self->i2c_buffer = m_malloc(buf_size);
	}

    uint16_t print_width = 0;
    mp_obj_t map_obj = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_MAP));
    GET_STR_DATA_LEN(map_obj, map_data, map_len);
    GET_STR_DATA_LEN(args[2], str_data, str_len);
    const byte *s = str_data, *top = str_data + str_len;
    while (s < top) {
        unichar ch;
        ch = utf8_get_char(s);
        s = utf8_next_char(s);

        const byte *map_s = map_data, *map_top = map_data + map_len;
        uint16_t char_index = 0;

        while (map_s < map_top) {
            unichar map_ch;
            map_ch = utf8_get_char(map_s);
            map_s = utf8_next_char(map_s);

            if (ch == map_ch) {
                uint8_t width = widths_data[char_index];

                bs_bit = 0;
                switch (offset_width) {
                    case 1:
                        bs_bit = offsets_data[char_index * offset_width];
                        break;

                    case 2:
                        bs_bit = (offsets_data[char_index * offset_width] << 8) +
                            (offsets_data[char_index * offset_width + 1]);
                        break;

                    case 3:
                        bs_bit = (offsets_data[char_index * offset_width] << 16) +
                            (offsets_data[char_index * offset_width + 1] << 8) +
                            (offsets_data[char_index * offset_width + 2]);
                        break;
                }

                uint32_t ofs = 0;
                for (int yy = 0; yy < height; yy++) {
                    for (int xx = 0; xx < width; xx++) {
                        self->i2c_buffer[ofs++] = get_color(bpp) ? fg_color : bg_color;
                    }
                }

                uint32_t data_size = width * height * 2;
                uint16_t x1 = x + width - 1;
                if (x1 < self->width) {
                    set_window(self, x, y, x1, y + height - 1);
                    DC_HIGH();
                    CS_LOW();
                    write_spi(self->spi_obj, (uint8_t *) self->i2c_buffer, data_size);
                    CS_HIGH();
                    print_width += width;
                }
                else
                    break;

                x += width;
                print_width += width;
                break;
            }
            char_index++;
        }
    }
    return mp_obj_new_int(print_width);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_write_obj, 5, 7, gc9a01_GC9A01_write);


/// #### .bitmap(`bitmap`, `x`, `y` {, `idx`})
/// Draw a bitmap. Supports multiple bitmaps in one module that can be selected by index.
/// The `image_converter.py` utility creates compatible 1 to 8 bit per pixel bitmap
/// modules from image files using the Pillow Python Imaging Library.
///
///
///     * **Required Parameters:**
///         * ``bitmap``: bitmap to draw
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///
///     * **Optional Parameters:**
///         * ``idx``: index of the bitmap to draw (default: 0)
///
/// The `monofont2bitmap.py` utility creates compatible 1 to 8 bit per pixel
/// bitmap modules from **Monospaced** True Type fonts. See the `inconsolata_16.py`,
/// `inconsolata_32.py` and `inconsolata_64.py` files in the `examples/lib` folder
/// for sample modules and the `mono_font.py` program for an example using the
/// generated modules.
///
/// The character sizes, bit per pixel, foreground, background
/// colors and the characters to include in the bitmap module may be specified as
/// parameters. Use the -h option for details. Bits per pixel settings larger than
/// one may be used to create antialiased characters at the expense of memory use.
/// If you specify a buffer_size during the display initialization it must be
/// large enough to hold the one character (HEIGHT * WIDTH * 2).

static mp_obj_t gc9a01_GC9A01_bitmap(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *bitmap = MP_OBJ_TO_PTR(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t idx;

    if (n_args > 4) {
        idx = mp_obj_get_int(args[4]);
    } else {
        idx = 0;
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(bitmap->globals);
    const uint16_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint16_t width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    uint16_t bitmaps = 0;
    const uint8_t bpp = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BPP)));
    mp_obj_t *palette_arg = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_PALETTE));
    mp_obj_t *palette = NULL;
    size_t palette_len = 0;
    mp_map_elem_t *elem = dict_lookup(bitmap->globals, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAPS));

    if (elem) {
        bitmaps = mp_obj_get_int(elem);
    }

    mp_obj_get_array(palette_arg, &palette_len, &palette);

    mp_obj_t *bitmap_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAP));
    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(bitmap_data_buff, &bufinfo, MP_BUFFER_READ);
    bitmap_data = bufinfo.buf;

    uint32_t buf_size = width * height * 2;

    if (self->buffer_size == 0) {
        self->i2c_buffer = m_malloc(buf_size);
    }

    uint32_t ofs = 0;

    bs_bit = 0;
    if (bitmaps) {
        if (idx < bitmaps) {
            bs_bit = height * width * bpp * idx;
        } else {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("index out of range"));
        }
    }

    for (int yy = 0; yy < height; yy++) {
        for (int xx = 0; xx < width; xx++) {
            self->i2c_buffer[ofs++] = mp_obj_get_int(palette[get_color(bpp)]);
        }
    }

    uint16_t x1 = x + width - 1;

    if (x1 < self->width) {
        set_window(self, x, y, x1, y + height - 1);
        DC_HIGH();
        CS_LOW();
        write_spi(self->spi_obj, (uint8_t *)self->i2c_buffer, buf_size);
        CS_HIGH();
    }

    if (self->buffer_size == 0) {
        m_free(self->i2c_buffer);
    }
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_bitmap_obj, 4, 5, gc9a01_GC9A01_bitmap);


/// #### .pbitmap(`bitmap`, `x`, `y` {, `idx`})
/// Progressively draw a bitmap one line at a time. Supports multiple bitmaps in one
/// module that can be selected by index.
///
///     * **Required Parameters:**
///         * ``bitmap``: bitmap to draw
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///
///     * **Optional Parameters:**
///         * ``idx``: index of the bitmap to draw (default: 0)

static mp_obj_t gc9a01_GC9A01_pbitmap(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *bitmap = MP_OBJ_TO_PTR(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t idx;

    if (n_args > 4) {
        idx = mp_obj_get_int(args[4]);
    } else {
        idx = 0;
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(bitmap->globals);
    const uint16_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint16_t width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    uint16_t bitmaps = 0;
    const uint8_t bpp = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BPP)));
    mp_obj_t *palette_arg = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_PALETTE));
    mp_obj_t *palette = NULL;
    size_t palette_len = 0;
    mp_map_elem_t *elem = dict_lookup(bitmap->globals, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAPS));

    if (elem) {
        bitmaps = mp_obj_get_int(elem);
    }

    mp_obj_get_array(palette_arg, &palette_len, &palette);

    mp_obj_t *bitmap_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAP));
    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(bitmap_data_buff, &bufinfo, MP_BUFFER_READ);
    bitmap_data = bufinfo.buf;

    uint32_t buf_size = width * 2;

    if (self->buffer_size == 0) {
        self->i2c_buffer = m_malloc(buf_size);
    }

    bs_bit = 0;
    if (bitmaps) {
        if (idx < bitmaps) {
            bs_bit = height * width * bpp * idx;
        } else {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("index out of range"));
        }
    }

    uint16_t x1 = x + width - 1;

    for (int yy = 0; yy < height; yy++) {
        uint32_t ofs = 0;

        for (int xx = 0; xx < width; xx++) {
            self->i2c_buffer[ofs++] = mp_obj_get_int(palette[get_color(bpp)]);
        }

        set_window(self, x, y + yy, x1, y + yy);
        DC_HIGH();
        CS_LOW();
        write_spi(self->spi_obj, (uint8_t *)self->i2c_buffer, buf_size);
        CS_HIGH();
    }

    if (self->buffer_size == 0) {
        m_free(self->i2c_buffer);
    }
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_pbitmap_obj, 4, 5, gc9a01_GC9A01_pbitmap);


/// #### .text(`font`, `string|int`, `x`, `y` {, `fg_color`, `bg_color`})
/// Draw a string or character using a converted PC BIOS ROM font.
/// See the `README.md` in the `fonts/bitmap` directory for example fonts and
/// the `font_from_romfont.py` utility in utils to convert PC BIOS ROM fonts from the
/// font-bin directory of [spacerace's romfont repo.](https://github.com/spacerace/romfont)
///
///     * **Required Parameters:**
///         * ``font``: PC BIOS ROM font to use
///         * ``string|int``: string or character to draw
///         * ``x``: x position of the top left corner
///         * ``y``: y position of the top left corner
///
///     * **Optional Parameters:**
///         * ``fg_color``: foreground color of the text, defaults to `WHITE`
///         * ``bg_color``: background color of the text, defaults to `BLACK`

static mp_obj_t gc9a01_GC9A01_text(size_t n_args, const mp_obj_t *args) {
    char single_char_s[2] = { 0, 0};
    const char *str;

    // extract arguments
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);

        single_char_s[0] = c & 0xff;
        str = single_char_s;
    } else {
        str = mp_obj_str_get_str(args[2]);
    }

    mp_int_t x0 = mp_obj_get_int(args[3]);
    mp_int_t y0 = mp_obj_get_int(args[4]);
    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    const uint8_t width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    const uint8_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint8_t first = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FIRST)));
    const uint8_t last = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_LAST)));
    mp_obj_t font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(font_data_buff, &bufinfo, MP_BUFFER_READ);

    const uint8_t *font_data = bufinfo.buf;
    mp_int_t fg_color;
    mp_int_t bg_color;

    if (n_args > 5) {
        fg_color = _swap_bytes(mp_obj_get_int(args[5]));
    } else {
        fg_color = _swap_bytes(WHITE);
    }

    if (n_args > 6) {
        bg_color = _swap_bytes(mp_obj_get_int(args[6]));
    } else {
        bg_color = _swap_bytes(BLACK);
    }

    uint8_t wide = width / 8;
    uint32_t buf_size = width * height * 2;

    if (self->buffer_size == 0) {
        self->i2c_buffer = m_malloc(buf_size);
    }

    if (self->i2c_buffer) {
        uint8_t chr;

        while ((chr = *str++)) {
            if (chr >= first && chr <= last) {
                uint16_t buf_idx = 0;
                uint16_t chr_idx = (chr - first) * (height * wide);

                for (uint8_t line = 0; line < height; line++) {
                    for (uint8_t line_byte = 0; line_byte < wide; line_byte++) {
                        uint8_t chr_data = font_data[chr_idx];

                        for (uint8_t bit = 8; bit; bit--) {
                            if (chr_data >> (bit - 1) & 1) {
                                self->i2c_buffer[buf_idx] = fg_color;
                            } else {
                                self->i2c_buffer[buf_idx] = bg_color;
                            }
                            buf_idx++;
                        }
                        chr_idx++;
                    }
                }

                uint16_t x1 = x0 + width - 1;

                if (x1 < self->width) {
                    set_window(self, x0, y0, x1, y0 + height - 1);
                    DC_HIGH();
                    CS_LOW();
                    write_spi(self->spi_obj, (uint8_t *)self->i2c_buffer, buf_size);
                    CS_HIGH();
                }
                x0 += width;
            }
        }
        if (self->buffer_size == 0) {
            m_free(self->i2c_buffer);
        }
    }
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_text_obj, 5, 7, gc9a01_GC9A01_text);


static void set_rotation(gc9a01_GC9A01_obj_t *self) {
    uint8_t madctl_value = GC9A01_MADCTL_BGR;

    if (self->rotation == 0) {                      // Portrait
        madctl_value |= GC9A01_MADCTL_MX;
        self->width = self->display_width;
        self->height = self->display_height;
    } else if (self->rotation == 1) {               // Landscape
        madctl_value |= GC9A01_MADCTL_MV;
        self->width = self->display_height;
        self->height = self->display_width;
    } else if (self->rotation == 2) {               // Inverted Portrait
        madctl_value |= GC9A01_MADCTL_MY;
        self->width = self->display_width;
        self->height = self->display_height;
    } else if (self->rotation == 3) {               // Inverted Landscape
        madctl_value |= GC9A01_MADCTL_MX | GC9A01_MADCTL_MY | GC9A01_MADCTL_MV;
        self->width = self->display_height;
        self->height = self->display_width;
    } else if (self->rotation == 4) {               // Portrait Mirrored
        self->width = self->display_width;
        self->height = self->display_height;
    } else if (self->rotation == 5) {               // Landscape Mirrored
        madctl_value |= GC9A01_MADCTL_MX | GC9A01_MADCTL_MV;
        self->width = self->display_height;
        self->height = self->display_width;
    } else if (self->rotation == 6) {               // Inverted Portrait Mirrored
        madctl_value |= GC9A01_MADCTL_MX | GC9A01_MADCTL_MY;
        self->width = self->display_width;
        self->height = self->display_height;
    } else if (self->rotation == 7) {               // Inverted Landscape Mirrored
        madctl_value |= GC9A01_MADCTL_MV | GC9A01_MADCTL_MY;
        self->width = self->display_height;
        self->height = self->display_width;
    }

    self->min_x = self->width;
    self->min_y = self->height;
    self->max_x = 0;
    self->max_y = 0;

    const uint8_t madctl[] = { madctl_value };

    write_cmd(self, GC9A01_MADCTL, madctl, 1);
}

/// #### .rotate(`rotation`)
/// Rotate the display to the given orientation.
///
///     * **Required Parameters:**
///
///         * ``rotation``: rotation of the display
///
///             * ``0``: portrait
///             * ``1``: landscape
///             * ``2``: inverted portrait
///             * ``3``: inverted landscape
///             * ``4``: portrait mirrored
///             * ``5``: landscape mirrored
///             * ``6``: inverted portrait mirrored
///             * ``7``: inverted landscape mirrored

static mp_obj_t gc9a01_GC9A01_rotation(mp_obj_t self_in, mp_obj_t value) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rotation = mp_obj_get_int(value) % 8;

    self->rotation = rotation;
    set_rotation(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(gc9a01_GC9A01_rotation_obj, gc9a01_GC9A01_rotation);


/// #### .width()
/// Return the logical width of the display in the current orientation.
///
///     * **Returns:**
///         * ``(int)``: width of the display

static mp_obj_t gc9a01_GC9A01_width(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return mp_obj_new_int(self->width);
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_width_obj, gc9a01_GC9A01_width);


/// #### .height()
/// Return the logical height of the display in the current orientation.
///
///     * **Returns:**
///         * ``(int)``: height of the display

static mp_obj_t gc9a01_GC9A01_height(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return mp_obj_new_int(self->height);
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_height_obj, gc9a01_GC9A01_height);


/// #### .vscrdef(`tfa`, `vsa`, `bfa`)
/// Set the hardware vertical scrolling definition.
///
///     * **Required Parameters:**
///         * ``tfa``: top fixed area
///         * ``vsa``: vertical scrolling area
///         * ``bfa``: bottom fixed area

static mp_obj_t gc9a01_GC9A01_vscrdef(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t tfa = mp_obj_get_int(args[1]);
    mp_int_t vsa = mp_obj_get_int(args[2]);
    mp_int_t bfa = mp_obj_get_int(args[3]);
    uint8_t buf[6] = {(tfa) >> 8, (tfa) & 0xFF, (vsa) >> 8, (vsa) & 0xFF, (bfa) >> 8, (bfa) & 0xFF};

    write_cmd(self, GC9A01_VSCRDEF, buf, 6);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_vscrdef_obj, 4, 4, gc9a01_GC9A01_vscrdef);


/// #### .vscsad(`vssa`)
/// Set the hardware vertical scrolling start address.
///
///     * **Required Parameters:**
///         * ``vssa``: vertical scrolling start address

static mp_obj_t gc9a01_GC9A01_vscsad(mp_obj_t self_in, mp_obj_t vssa_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t vssa = mp_obj_get_int(vssa_in);
    uint8_t buf[2] = {(vssa) >> 8, (vssa) & 0xFF};

    write_cmd(self, GC9A01_VSCSAD, buf, 2);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(gc9a01_GC9A01_vscsad_obj, gc9a01_GC9A01_vscsad);


/// #### .on()
/// Turn on the backlight pin if one was defined during init.

static mp_obj_t gc9a01_GC9A01_on(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    DISP_HIGH();
    mp_hal_delay_ms(10);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_on_obj, gc9a01_GC9A01_on);


/// #### .off()
/// Turn off the backlight pin if one was defined during init.

static mp_obj_t gc9a01_GC9A01_off(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    DISP_LOW();
    mp_hal_delay_ms(10);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_off_obj, gc9a01_GC9A01_off);


/// #### .hline(`x`, `y`, `w`, `color`)
/// Draw a single horizontal line. This is a fast version with reduced number of SPI calls.
///
///     * **Required Parameters:**
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``w``: width of the line
///         * ``color``: color of the line

static mp_obj_t gc9a01_GC9A01_hline(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);

    fast_hline(self, x, y, w, color);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_hline_obj, 5, 5, gc9a01_GC9A01_hline);


/// #### .vline(`x`, `y`, `h`, `color`)
/// Draw a vertical line. This is a fast version with reduced number of SPI calls.
///
///     * **Required Parameters:**
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``h``: height of the line
///         * ``color``: color of the line

static mp_obj_t gc9a01_GC9A01_vline(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);

    fast_vline(self, x, y, w, color);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_vline_obj, 5, 5, gc9a01_GC9A01_vline);


/// #### .rect(`x`, `y`, `w`, `h`, `color`)
/// Draw the outline of a rectangle.
///
///     * **Required Parameters:**
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``w``: width of the rectangle
///         * ``h``: height of the rectangle
///         * ``color``: color of the rectangle

static mp_obj_t gc9a01_GC9A01_rect(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);

    fast_hline(self, x, y, w, color);
    fast_vline(self, x, y, h, color);
    fast_hline(self, x, y + h - 1, w, color);
    fast_vline(self, x + w - 1, y, h, color);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_rect_obj, 6, 6, gc9a01_GC9A01_rect);


/// #### .offset(`x`, `y`)
/// Set the xstart and ystart offset of the display.
///
///     * **Required Parameters:**
///         * ``x``: x offset
///         * ``y``: y offset

static mp_obj_t gc9a01_GC9A01_offset(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xstart = mp_obj_get_int(args[1]);
    mp_int_t ystart = mp_obj_get_int(args[2]);

    self->xstart = xstart;
    self->ystart = ystart;

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_offset_obj, 3, 3, gc9a01_GC9A01_offset);


static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

/// #### .color565(`r`, `g`, `b`)
/// Convert red, green, and blue values into a 16-bit RGB565 value.
///
///     * **Required Parameters:**
///         * ``r``: red value
///         * ``g``: green value
///         * ``b``: blue value
///
///     * **return value**:
///         * ``(int)``:  16-bit RGB565 value

static mp_obj_t gc9a01_color565(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    return MP_OBJ_NEW_SMALL_INT(color565((uint8_t)mp_obj_get_int(r), (uint8_t)mp_obj_get_int(g), (uint8_t)mp_obj_get_int(b)
        ));
}

static MP_DEFINE_CONST_FUN_OBJ_3(gc9a01_color565_obj, gc9a01_color565);


static void map_bitarray_to_rgb565(uint8_t const *bitarray, uint8_t *buffer, int length, int width, uint16_t color, uint16_t bg_color) {
    int row_pos = 0;

    for (int i = 0; i < length; i++) {
        uint8_t byte = bitarray[i];

        for (int bi = 7; bi >= 0; bi--) {
            uint8_t b = byte & (1 << bi);
            uint16_t cur_color = b ? color : bg_color;

            *buffer = (cur_color & 0xff00) >> 8;
            buffer++;
            *buffer = cur_color & 0xff;
            buffer++;

            row_pos++;
            if (row_pos >= width) {
                row_pos = 0;
                break;
            }
        }
    }
}

/// #### .map_bitarray_to_rgb565(`bitarray`, `buffer`, `width` {, `color`, `bg_color`})
/// Convert the given bitarray into a RGB565 bitmap.
///
///     * **Required Parameters:**
///         * ``bitarray``: bitarray to convert
///         * ``buffer``: buffer to write the bitmap to
///         * ``width``: width of the bitmap
///
///     * **Optional Parameters:**
///         * ``color``: color of the bitarray (default: white)
///         * ``bg_color``: background color of the bitarray (default: black)

static mp_obj_t gc9a01_map_bitarray_to_rgb565(size_t n_args, const mp_obj_t *args) {

    mp_buffer_info_t bitarray_info;
    mp_buffer_info_t buffer_info;

    mp_get_buffer_raise(args[1], &bitarray_info, MP_BUFFER_READ);
    mp_get_buffer_raise(args[2], &buffer_info, MP_BUFFER_WRITE);

    mp_int_t width = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);
    mp_int_t bg_color = mp_obj_get_int(args[5]);

    map_bitarray_to_rgb565(bitarray_info.buf, buffer_info.buf, bitarray_info.len, width, color, bg_color);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_map_bitarray_to_rgb565_obj, 3, 6, gc9a01_map_bitarray_to_rgb565);


//
// jpg routines
//

#define JPG_MODE_FAST (0)
#define JPG_MODE_SLOW (1)

// User defined device identifier
typedef struct {
    mp_file_t *fp;                                  // File pointer for input function
    uint8_t *fbuf;                                  // Pointer to the frame buffer for output function
    unsigned int wfbuf;                             // Width of the frame buffer [pix]
    gc9a01_GC9A01_obj_t *self;                      // display object
} IODEV;

//
// User defined input function
//

static unsigned int in_func(                        // Returns number of bytes read (zero on error)
    JDEC *jd,                                       // Decompression object
    uint8_t *buff,                                  // Pointer to the read buffer (null to remove data)
    unsigned int nbyte) {                           // Number of bytes to read/remove
    IODEV *dev = (IODEV *)jd->device;               // Device identifier for the session (5th argument of jd_prepare function)
    unsigned int nread;

    if (buff) {                                     // Read data from input stream
        nread = (unsigned int)mp_readinto(dev->fp, buff, nbyte);
        return nread;
    }

    // Remove data from input stream if buff was NULL
    mp_seek(dev->fp, nbyte, SEEK_CUR);
    return 0;
}

//
// Fast output function to draw the entire image at once
//

static int out_fast(                                // 1:Ok, 0:Aborted
    JDEC *jd,                                       // Decompression object
    void *bitmap,                                   // Bitmap data to be output
    JRECT *rect) {                                  // Rectangular region of output image
    IODEV *dev = (IODEV *)jd->device;
    uint8_t *src, *dst;
    uint16_t y, bws, bwd;

    // Copy the decompressed RGB rectangular to the frame buffer (assuming RGB565)
    src = (uint8_t *)bitmap;
    dst = dev->fbuf + 2 * (rect->top * dev->wfbuf + rect->left); // Left-top of destination rectangular
    bws = 2 * (rect->right - rect->left + 1);       // Width of source rectangular [byte]
    bwd = 2 * dev->wfbuf;                           // Width of frame buffer [byte]
    for (y = rect->top; y <= rect->bottom; y++) {
        memcpy(dst, src, bws);                      // Copy a line
        src += bws;
        dst += bwd;                                 // Next line
    }

    return 1;                                       // Continue to decompress
}

//
// Slow output function to draw the image progressively by each Minimum Coded Unit
//

static int out_slow(                                // 1:Ok, 0:Aborted
    JDEC *jd,                                       // Decompression object
    void *bitmap,                                   // Bitmap data to be output
    JRECT *rect) {                                  // Rectangular region of output image
    IODEV *dev = (IODEV *)jd->device;
    gc9a01_GC9A01_obj_t *self = dev->self;
    uint8_t *src, *dst;
    uint16_t y;
    uint16_t wx2 = (rect->right - rect->left + 1) * 2;
    uint16_t h = rect->bottom - rect->top + 1;

    // Copy the decompressed RGB rectangular to the frame buffer (assuming RGB565)
    src = (uint8_t *)bitmap;
    dst = dev->fbuf;                                // Left-top of destination rectangular
    for (y = rect->top; y <= rect->bottom; y++) {
        memcpy(dst, src, wx2);                      // Copy a line
        src += wx2;
        dst += wx2;                                 // Next line
    }

    // blit buffer to display

    set_window(self, rect->left + jd->x_offs, rect->top + jd->y_offs, rect->right + jd->x_offs, rect->bottom + jd->y_offs);

    DC_HIGH();
    CS_LOW();
    write_spi(self->spi_obj, (uint8_t *)dev->fbuf, wx2 * h);
    CS_HIGH();

    return 1;                                       // Continue to decompress
}

/// #### .jpg(`filename`, `x`, `y` {, `mode`})
/// Draw a JPG image.
/// The memory required to decode and display a JPG can be considerable as a
/// full screen 320x240 JPG would require at least 3100 bytes for the working
/// area + 320x240x2 bytes of ram to buffer the image. Jpg images that would
/// require a buffer larger than available memory can be drawn by passing `SLOW`
/// for method. The `SLOW` method will draw the image a piece at a time using
/// the Minimum Coded Unit (MCU, typically 8x8) of the image.
///
///     * **Required Parameters:**
///         * ``filename``: filename of the JPG image
///         * ``x``: column to start at
///         * ``y``: row to start at
///
///     * **Optional Parameters:**
///
///         * ``mode``: mode to use for drawing the image (default: JPG_MODE_FAST)
///
///             * ``JPG_MODE_FAST``: draw the entire image at once
///             * ``JPG_MODE_SLOW``: draw the image progressively by each Minimum Coded Unit

static mp_obj_t gc9a01_GC9A01_jpg(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t mode;

    if (n_args > 4) {
        mode = mp_obj_get_int(args[4]);
    } else {
        mode = JPG_MODE_FAST;
    }

    int (*outfunc)(JDEC *, void *, JRECT *);

    JRESULT res;                                    // Result code of TJpgDec API
    JDEC jdec;                                      // Decompression object

    self->work = (void *)m_malloc(3100);            // Pointer to the work area

    IODEV devid;                                    // User defined device identifier
    size_t bufsize;

    self->fp = mp_open(filename, "rb");
    devid.fp = self->fp;
    if (devid.fp) {
        // Prepare to decompress
        res = jd_prepare(&jdec, in_func, self->work, 3100, &devid);
        if (res == JDR_OK) {
            // Initialize output device
            if (mode == JPG_MODE_FAST) {
                bufsize = 2 * jdec.width * jdec.height;
                outfunc = out_fast;
            } else {
                bufsize = 2 * jdec.msx * 8 * jdec.msy * 8;
                outfunc = out_slow;
                jdec.x_offs = x;
                jdec.y_offs = y;
            }
            if (self->buffer_size && bufsize > self->buffer_size) {
                mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("buffer too small"));
            }

            if (self->buffer_size == 0) {
                self->i2c_buffer = m_malloc(bufsize);
            }

            if (!self->i2c_buffer) {
                mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("out of memory"));
            }

            devid.fbuf = (uint8_t *)self->i2c_buffer;
            devid.wfbuf = jdec.width;
            devid.self = self;
            res = jd_decomp(&jdec, outfunc, 0);     // Start to decompress with 1/1 scaling
            if (res == JDR_OK) {
                if (mode == JPG_MODE_FAST) {
                    set_window(self, x, y, x + jdec.width - 1, y + jdec.height - 1);
                    DC_HIGH();
                    CS_LOW();
                    write_spi(self->spi_obj, (uint8_t *)self->i2c_buffer, bufsize);
                    CS_HIGH();
                }
            } else {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg decompress failed."));
            }
            if (self->buffer_size == 0) {
                m_free(self->i2c_buffer);           // Discard frame buffer
                self->i2c_buffer = MP_OBJ_NULL;
            }
            devid.fbuf = MP_OBJ_NULL;
        } else {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg prepare failed."));
        }
        mp_close(devid.fp);
    }
    m_free(self->work);                             // Discard work area
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_jpg_obj, 4, 5, gc9a01_GC9A01_jpg);


/// #### .polygon_center(`polygon`)
/// Return the center of a polygon as an (x, y) tuple.
///
///     * **Required Parameters:**
///         * ``polygon``: polygon to get the center of
///
///     * **Returns:**
///         * ``(x, y)``: tuple of the center of the polygon

static mp_obj_t gc9a01_GC9A01_polygon_center(size_t n_args, const mp_obj_t *args) {
    size_t poly_len;
    mp_obj_t *polygon;

    mp_obj_get_array(args[1], &poly_len, &polygon);

    mp_float_t sum = 0.0;
    mp_int_t vsx = 0;
    mp_int_t vsy = 0;

    if (poly_len > 0) {
        for (mp_int_t idx = 0; idx < poly_len; idx++) {
            size_t point_from_poly_len;
            mp_obj_t *point_from_poly;

            mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
            if (point_from_poly_len < 2) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
            }

            mp_int_t v1x = mp_obj_get_int(point_from_poly[0]);
            mp_int_t v1y = mp_obj_get_int(point_from_poly[1]);

            mp_obj_get_array(polygon[(idx + 1) % poly_len], &point_from_poly_len, &point_from_poly);
            if (point_from_poly_len < 2) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
            }

            mp_int_t v2x = mp_obj_get_int(point_from_poly[0]);
            mp_int_t v2y = mp_obj_get_int(point_from_poly[1]);
            mp_float_t cross = v1x * v2y - v1y * v2x;

            sum += cross;
            vsx += (mp_int_t)(((v1x + v2x) * cross) + MICROPY_FLOAT_CONST(0.5));
            vsy += (mp_int_t)(((v1y + v2y) * cross) + MICROPY_FLOAT_CONST(0.5));
        }

        mp_float_t z = 1.0 / (3.0 * sum);

        vsx = (mp_int_t)((vsx * z) + MICROPY_FLOAT_CONST(0.5));
        vsy = (mp_int_t)((vsy * z) + MICROPY_FLOAT_CONST(0.5));
    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }

    mp_obj_t center[2] = {mp_obj_new_int(vsx), mp_obj_new_int(vsy)};

    return mp_obj_new_tuple(2, center);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_polygon_center_obj, 2, 2, gc9a01_GC9A01_polygon_center);


//
// RotatePolygon: Rotate a polygon around a center point angle radians
//

static void RotatePolygon(Polygon *polygon, Point *center, mp_float_t angle) {

    // reject null polygons
    if (polygon == NULL || center == NULL || polygon->length == 0) {
        return;
    }

    mp_float_t cosAngle = MICROPY_FLOAT_C_FUN(cos)(angle);
    mp_float_t sinAngle = MICROPY_FLOAT_C_FUN(sin)(angle);

    for (int i = 0; i < polygon->length; i++) {
        mp_float_t dx = (polygon->points[i].x - center->x);
        mp_float_t dy = (polygon->points[i].y - center->y);

        polygon->points[i].x = (int)center->x + (dx * cosAngle - dy * sinAngle) + MICROPY_FLOAT_CONST(0.5);
        polygon->points[i].y = (int)center->y + (dx * sinAngle + dy * cosAngle) + MICROPY_FLOAT_CONST(0.5);
    }
}

//
// Draw the polygon at the given x, y location rotated around cx, cy, angle degrees in the given color
//

static void draw_polygon_outline(gc9a01_GC9A01_obj_t *self, uint16_t x, uint16_t y, Polygon *polygon, uint16_t color) {

    for (int idx = 1; idx < polygon->length; idx++) {
        line(self, x + (int)(polygon->points[idx - 1].x + MICROPY_FLOAT_CONST(0.5)), y + (int)(polygon->points[idx - 1].y + MICROPY_FLOAT_CONST(0.5)), x + (int)(polygon->points[idx].x + MICROPY_FLOAT_CONST(0.5)), y + (int)(polygon->points[idx].y + MICROPY_FLOAT_CONST(0.5)), color);
    }
}

/// #### .polygon(`polygon`, `x`, `y`, `color` {, `angle` {, `cx`, `cy`}})
/// Draw a polygon.
/// The polygon should consist of a list of (x, y) tuples forming a closed polygon.
/// See `roids.py` for an example.
///
///     * **Required Parameters:**
///         * ``polygon``: polygon to draw
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``color``: color of the polygon
///
///     * **Optional Parameters:**
///         * ``angle``: angle in radians to rotate the polygon (default: 0.0)
///         * ``cx``: x coordinate of the center of rotation (default: 0)
///         * ``cy``: y coordinate of the center of rotation (default: 0)

static mp_obj_t gc9a01_GC9A01_polygon(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    size_t poly_len;
    mp_obj_t *polygon;

    mp_obj_get_array(args[1], &poly_len, &polygon);

    self->work = NULL;

    if (poly_len > 0) {
        mp_int_t x = mp_obj_get_int(args[2]);
        mp_int_t y = mp_obj_get_int(args[3]);
        mp_int_t color = mp_obj_get_int(args[4]);
        mp_float_t angle = MICROPY_FLOAT_CONST(0.0);

        if (n_args > 5 && mp_obj_is_float(args[5])) {
            angle = mp_obj_float_get(args[5]);
        }

        mp_int_t cx = 0;
        mp_int_t cy = 0;

        if (n_args > 6) {
            cx = mp_obj_get_int(args[6]);
            cy = mp_obj_get_int(args[7]);
        }

        self->work = m_malloc(poly_len * sizeof(Point));
        if (self->work) {
            Point *point = (Point *)self->work;

            for (int idx = 0; idx < poly_len; idx++) {
                size_t point_from_poly_len;
                mp_obj_t *point_from_poly;

                mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
                if (point_from_poly_len < 2) {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
                }

                mp_int_t px = mp_obj_get_int(point_from_poly[0]);
                mp_int_t py = mp_obj_get_int(point_from_poly[1]);

                point[idx] = (Point) {px, py};
                // point[idx].x = px;
                // point[idx].y = py;
            }

            Point center = { cx, cy};
            Polygon polygon = { poly_len, self->work};

            if (angle > 0) {
                RotatePolygon(&polygon, &center, angle);
            }

            draw_polygon_outline(self, x, y, &polygon, color);

            m_free(self->work);
            self->work = NULL;
        } else {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
        }
    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_polygon_obj, 4, 8, gc9a01_GC9A01_polygon);


//
// public-domain code by Darel Rex Finley, 2007
// https://alienryderflex.com/polygon_fill/
//

#define MAX_POLY_CORNERS 32

static void PolygonFill(gc9a01_GC9A01_obj_t *self, Polygon *polygon, Point location, uint16_t color) {
    int nodes, nodeX[MAX_POLY_CORNERS], pixelY, i, j, swap;
    int minX = INT_MAX;
    int maxX = INT_MIN;
    int minY = INT_MAX;
    int maxY = INT_MIN;

    for (i = 0; i < polygon->length; i++) {
        Point point = polygon->points[i];

        minX = MIN(minX, (int)(point.x + MICROPY_FLOAT_CONST(0.5)));
        maxX = MAX(maxX, (int)(point.x + MICROPY_FLOAT_CONST(0.5)));
        minY = MIN(minY, (int)(point.y + MICROPY_FLOAT_CONST(0.5)));
        maxY = MAX(maxY, (int)(point.y + MICROPY_FLOAT_CONST(0.5)));
    }

    //  Loop through the rows
    for (pixelY = minY; pixelY <= maxY; pixelY++) {
        //  Build a list of nodes.
        nodes = 0;
        j = polygon->length - 1;
        for (i = 0; i < polygon->length; i++) {
            Point point1 = polygon->points[i];
            Point point2 = polygon->points[j];

            if ((point1.y < (mp_float_t)pixelY && point2.y >= (mp_float_t)pixelY) || (point2.y < (mp_float_t)pixelY && point1.y >= (mp_float_t)pixelY)) {
                if (nodes < MAX_POLY_CORNERS) {
                    nodeX[nodes++] = (int)((point1.x + (pixelY - point1.y) / (point2.y - point1.y) * (point2.x - point1.x)) + MICROPY_FLOAT_CONST(0.5));
                } else {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon too complex increase MAX_POLY_CORNERS."));
                }
            }
            j = i;
        }

        //  Sort the nodes, via a simple “Bubble” sort.
        for (i = 0; i < nodes - 1; i++) {
            for (j = i + 1; j < nodes; j++) {
                if (nodeX[i] > nodeX[j]) {
                    swap = nodeX[i];
                    nodeX[i] = nodeX[j];
                    nodeX[j] = swap;
                }
            }
        }

        //  Fill the pixels between node pairs.
        for (i = 0; i < nodes; i += 2) {
            if (nodeX[i] >= maxX) {
                break;
            }

            if (nodeX[i + 1] > minX) {
                if (nodeX[i] < minX) {
                    nodeX[i] = minX;
                }

                if (nodeX[i + 1] > maxX) {
                    nodeX[i + 1] = maxX;
                }

                fast_hline(self, nodeX[i] + (int)location.x, pixelY + (int)location.y, nodeX[i + 1] - nodeX[i], color);
            }
        }
    }
}

/// #### .fill_polygon(`polygon`, `x`, `y`, `color` {, `angle` {, `cx`, `cy`}})
/// Draw a filled polygon.
/// The polygon should consist of a list of (x, y) tuples forming a closed polygon.
/// See `watch.py` for an example.
///
///     * **Required Parameters:**
///         * ``polygon``: polygon to draw
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``color``: color of the polygon
///
///     * **Optional Parameters:**
///         * ``angle``: angle in radians to rotate the polygon (default: 0.0)
///         * ``cx``: x coordinate of the center of rotation (default: 0)
///         * ``cy``: y coordinate of the center of rotation (default: 0)

static mp_obj_t gc9a01_GC9A01_fill_polygon(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    size_t poly_len;
    mp_obj_t *polygon;

    mp_obj_get_array(args[1], &poly_len, &polygon);

    self->work = NULL;

    if (poly_len > 0) {
        mp_int_t x = mp_obj_get_int(args[2]);
        mp_int_t y = mp_obj_get_int(args[3]);
        mp_int_t color = mp_obj_get_int(args[4]);
        mp_float_t angle = MICROPY_FLOAT_CONST(0.0);

        if (n_args > 5) {
            angle = mp_obj_float_get(args[5]);
        }

        mp_int_t cx = 0;
        mp_int_t cy = 0;

        if (n_args > 6) {
            cx = mp_obj_get_int(args[6]);
            cy = mp_obj_get_int(args[7]);
        }

        self->work = m_malloc(poly_len * sizeof(Point));
        if (self->work) {
            Point *point = (Point *)self->work;

            for (int idx = 0; idx < poly_len; idx++) {
                size_t point_from_poly_len;
                mp_obj_t *point_from_poly;

                mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
                if (point_from_poly_len < 2) {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
                }

                point[idx].x = mp_obj_get_int(point_from_poly[0]);
                point[idx].y = mp_obj_get_int(point_from_poly[1]);
            }

            Point location = {x, y};
            Point center = {cx, cy};
            Polygon polygon = {poly_len, self->work};

            if (angle != 0) {
                RotatePolygon(&polygon, &center, angle);
            }

            draw_polygon_outline(self, x, y, &polygon, color);
            PolygonFill(self, &polygon, location, color);

            m_free(self->work);
            self->work = NULL;
        } else {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
        }

    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_fill_polygon_obj, 4, 8, gc9a01_GC9A01_fill_polygon);


void draw_arc(gc9a01_GC9A01_obj_t *self, uint16_t x, uint16_t y, int16_t start_angle_degrees, int16_t end_angle_degrees, uint16_t segments, uint16_t radius_x, uint16_t radius_y, uint16_t arc_width, uint16_t color) {

    if (start_angle_degrees == end_angle_degrees || arc_width == 0 || radius_x == 0 || radius_y == 0 || segments == 0) {
        return;
    }

    if (start_angle_degrees > end_angle_degrees) {
        int16_t temp = start_angle_degrees;

        start_angle_degrees = end_angle_degrees;
        end_angle_degrees = temp;
    }

    // Convert degrees to radians
    mp_float_t start_angle_radians = start_angle_degrees * RADIANS;
    mp_float_t end_angle_radians = end_angle_degrees * RADIANS;

    // Calculate start and end points
    int16_t x_start = x + (int)(radius_x * MICROPY_FLOAT_C_FUN(cos)(start_angle_radians) + MICROPY_FLOAT_CONST(0.5));
    int16_t y_start = y + (int)(radius_y * MICROPY_FLOAT_C_FUN(sin)(start_angle_radians) + MICROPY_FLOAT_CONST(0.5));
    int16_t x_end = x + (int)(radius_x * MICROPY_FLOAT_C_FUN(cos)(end_angle_radians) + MICROPY_FLOAT_CONST(0.5));
    int16_t y_end = y + (int)(radius_y * MICROPY_FLOAT_C_FUN(sin)(end_angle_radians) + MICROPY_FLOAT_CONST(0.5));

    // Calculate angle step
    mp_float_t angle_step = (end_angle_radians - start_angle_radians) / segments;

    // Bresenham's algorithm for drawing arcs
    for (mp_float_t angle = start_angle_radians; angle <= end_angle_radians; angle += angle_step) {
        int16_t x_current = x + (int)(radius_x * MICROPY_FLOAT_C_FUN(cos)(angle) + MICROPY_FLOAT_CONST(0.5));
        int16_t y_current = y + (int)(radius_y * MICROPY_FLOAT_C_FUN(sin)(angle) + MICROPY_FLOAT_CONST(0.5));

        // Draw a line segment from the previous point to the current point
        line(self, x_start, y_start, x_current, y_current, color);

        // Update start point for the next iteration
        x_start = x_current;
        y_start = y_current;
    }

    // Draw a line segment from the previous point to the end point
    line(self, x_start, y_start, x_end, y_end, color);
}

/// #### .arc(`x`, `y`, `start_angle`, `end_angle`, `segments`, `rx`, `ry`, `w`, `color`)
/// Draw an arc.
///
///     * **Required Parameters:**
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``start_angle``: start angle of the arc in degrees
///         * ``end_angle``: end angle of the arc in degrees
///         * ``segments``: number of segments to draw
///         * ``rx``: x radius of the arc
///         * ``ry``: y radius of the arc
///         * ``w``: width of the arc (currently limited to 1)
///         * ``color``: color of the arc

static mp_obj_t gc9a01_GC9A01_arc(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t start_angle = mp_obj_get_int(args[3]);
    mp_int_t end_angle = mp_obj_get_int(args[4]);
    mp_int_t segments = mp_obj_get_int(args[5]);
    mp_int_t rx = mp_obj_get_int(args[6]);
    mp_int_t ry = mp_obj_get_int(args[7]);
    mp_int_t w = mp_obj_get_int(args[8]);
    mp_int_t color = mp_obj_get_int(args[9]);

    draw_arc(self, x, y, start_angle, end_angle, segments, rx, ry, w, color);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_arc_obj, 10, 10, gc9a01_GC9A01_arc);


void draw_filled_arc(gc9a01_GC9A01_obj_t *self, uint16_t x, uint16_t y, int16_t start_angle_degrees, int16_t end_angle_degrees, uint16_t segments, uint16_t radius_x, uint16_t radius_y, uint16_t color) {

    if (start_angle_degrees == end_angle_degrees || radius_x == 0 || radius_y == 0 || segments == 0) {
        return;
    }

    if (start_angle_degrees > end_angle_degrees) {
        int16_t temp = start_angle_degrees;

        start_angle_degrees = end_angle_degrees;
        end_angle_degrees = temp;
    }

    bool full_circle = (end_angle_degrees - start_angle_degrees) == 360;
    mp_float_t start_angle_radians = start_angle_degrees * RADIANS;
    mp_float_t end_angle_radians = end_angle_degrees * RADIANS;
    int points = 0;
    Point *arc_points = m_malloc(sizeof(Point) * (segments + 4));

    if (arc_points == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("out of memory"));
    }

    if (!full_circle) {
        arc_points[points++] = (Point) {x, y};
    }

    mp_float_t angle_step = (end_angle_radians - start_angle_radians) / segments;
    uint16_t x_last = UINT16_MAX;
    uint16_t y_last = UINT16_MAX;

    for (mp_float_t angle = start_angle_radians; angle <= end_angle_radians; angle += angle_step) {
        uint16_t x_current = x + (int)(radius_x * MICROPY_FLOAT_C_FUN(cos)(angle) + MICROPY_FLOAT_CONST(0.5));
        uint16_t y_current = y + (int)(radius_y * MICROPY_FLOAT_C_FUN(sin)(angle) + MICROPY_FLOAT_CONST(0.5));

        if (x_current == x_last && y_current == y_last) {
            continue;
        }

        arc_points[points++] = (Point) { x_current, y_current};
        x_last = x_current;
        y_last = y_current;
    }

    uint16_t x_end = x + (int)(radius_x * MICROPY_FLOAT_C_FUN(cos)(end_angle_radians) + MICROPY_FLOAT_CONST(0.5));
    uint16_t y_end = y + (int)(radius_y * MICROPY_FLOAT_C_FUN(sin)(end_angle_radians) + MICROPY_FLOAT_CONST(0.5));

    arc_points[points++] = (Point) {x_end, y_end};

    if (!full_circle) {
        arc_points[points++] = (Point) {x, y};
    } else {
        arc_points[points++] = arc_points[0];
    }

    if (points > 2) {
        Polygon polygon = {points, arc_points};

        PolygonFill(self, &polygon, (Point) {0, 0}, color);
        draw_polygon_outline(self, 0, 0,  &polygon, color);
    }

    m_free(arc_points);
}

/// #### .fill_arc(`x`, `y`, `start_angle`, `end_angle`, `segments`, `rx`, `ry`, `color`)
/// Draw a filled arc.
///
///     * **Required Parameters:**
///         * ``x``: column to start at
///         * ``y``: row to start at
///         * ``start_angle``: start angle of the arc in degrees
///         * ``end_angle``: end angle of the arc in degrees
///         * ``segments``: number of segments to draw
///         * ``rx``: x radius of the arc
///         * ``ry``: y radius of the arc
///         * ``color``: color of the arc

static mp_obj_t gc9a01_GC9A01_fill_arc(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t start_angle = mp_obj_get_int(args[3]);
    mp_int_t end_angle = mp_obj_get_int(args[4]);
    mp_int_t segments = mp_obj_get_int(args[5]);
    mp_int_t rx = mp_obj_get_int(args[6]);
    mp_int_t ry = mp_obj_get_int(args[7]);
    mp_int_t color = mp_obj_get_int(args[8]);

    draw_filled_arc(self, x, y, start_angle, end_angle, segments, rx, ry, color);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_fill_arc_obj, 9, 9, gc9a01_GC9A01_fill_arc);


/// #### .bounding({`status` {, `as_rect`}})
/// Enables or disables tracking the display area that has been written to. Initially, tracking is disabled.
///
///     * **Optional Parameters:**
///         * ``status``: Pass a True value to enable tracking and False to disable it. Passing a True or False parameter will reset the current bounding rectangle to (display_width, display_height, 0, 0).
///         * ``as_rect``: If True, the returned tuple will contain (min_x, min_y, width, height) values.
///
///     * **Returns:**
///         * ``(x, y, w, h)``:  tuple of the bounding box or (min_x, min_y, width, height) if as_rect parameter is True

static mp_obj_t gc9a01_GC9A01_bounding(size_t n_args, const mp_obj_t *args) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t bounds[4] = {
        mp_obj_new_int(self->min_x), mp_obj_new_int(self->min_y), (n_args > 2 && mp_obj_is_true(args[2])) ? mp_obj_new_int(self->max_x - self->min_x + 1) : mp_obj_new_int(self->max_x), (n_args > 2 && mp_obj_is_true(args[2])) ? mp_obj_new_int(self->max_y - self->min_y + 1) : mp_obj_new_int(self->max_y)
    };

    if (n_args > 1) {
        if (mp_obj_is_true(args[1])) {
            self->bounding = 1;
        } else {
            self->bounding = 0;
        }

        self->min_x = self->width;
        self->min_y = self->height;
        self->max_x = 0;
        self->max_y = 0;
    }
    return mp_obj_new_tuple(4, bounds);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gc9a01_GC9A01_bounding_obj, 1, 3, gc9a01_GC9A01_bounding);


static mp_obj_t gc9a01_GC9A01_init(mp_obj_t self_in) {
    gc9a01_GC9A01_obj_t *self = MP_OBJ_TO_PTR(self_in);

    gc9a01_GC9A01_hard_reset(self_in);
    mp_hal_delay_ms(100);

    gc9a01_GC9A01_soft_reset(self_in);
    mp_hal_delay_ms(100);

    write_cmd(self, 0xEF, (const uint8_t *)NULL, 0);
    write_cmd(self, 0xEB, (const uint8_t *)"\x14", 1);
    write_cmd(self, 0xFE, (const uint8_t *)NULL, 0);
    write_cmd(self, 0xEF, (const uint8_t *)NULL, 0);
    write_cmd(self, 0xEB, (const uint8_t *)"\x14", 1);
    write_cmd(self, 0x84, (const uint8_t *)"\x40", 1);
    write_cmd(self, 0x85, (const uint8_t *)"\xFF", 1);
    write_cmd(self, 0x86, (const uint8_t *)"\xFF", 1);
    write_cmd(self, 0x87, (const uint8_t *)"\xFF", 1);
    write_cmd(self, 0x88, (const uint8_t *)"\x0A", 1);
    write_cmd(self, 0x89, (const uint8_t *)"\x21", 1);
    write_cmd(self, 0x8A, (const uint8_t *)"\x00", 1);
    write_cmd(self, 0x8B, (const uint8_t *)"\x80", 1);
    write_cmd(self, 0x8C, (const uint8_t *)"\x01", 1);
    write_cmd(self, 0x8D, (const uint8_t *)"\x01", 1);
    write_cmd(self, 0x8E, (const uint8_t *)"\xFF", 1);
    write_cmd(self, 0x8F, (const uint8_t *)"\xFF", 1);
    write_cmd(self, 0xB6, (const uint8_t *)"\x00\x00", 2);
    write_cmd(self, 0x3A, (const uint8_t *)"\x55", 1); // COLMOD
    write_cmd(self, 0x90, (const uint8_t *)"\x08\x08\x08\x08", 4);
    write_cmd(self, 0xBD, (const uint8_t *)"\x06", 1);
    write_cmd(self, 0xBC, (const uint8_t *)"\x00", 1);
    write_cmd(self, 0xFF, (const uint8_t *)"\x60\x01\x04", 3);
    write_cmd(self, 0xC3, (const uint8_t *)"\x13", 1);
    write_cmd(self, 0xC4, (const uint8_t *)"\x13", 1);
    write_cmd(self, 0xC9, (const uint8_t *)"\x22", 1);
    write_cmd(self, 0xBE, (const uint8_t *)"\x11", 1);
    write_cmd(self, 0xE1, (const uint8_t *)"\x10\x0E", 2);
    write_cmd(self, 0xDF, (const uint8_t *)"\x21\x0c\x02", 3);
    write_cmd(self, 0xF0, (const uint8_t *)"\x45\x09\x08\x08\x26\x2A", 6);
    write_cmd(self, 0xF1, (const uint8_t *)"\x43\x70\x72\x36\x37\x6F", 6);
    write_cmd(self, 0xF2, (const uint8_t *)"\x45\x09\x08\x08\x26\x2A", 6);
    write_cmd(self, 0xF3, (const uint8_t *)"\x43\x70\x72\x36\x37\x6F", 6);
    write_cmd(self, 0xED, (const uint8_t *)"\x1B\x0B", 2);
    write_cmd(self, 0xAE, (const uint8_t *)"\x77", 1);
    write_cmd(self, 0xCD, (const uint8_t *)"\x63", 1);
    write_cmd(self, 0x70, (const uint8_t *)"\x07\x07\x04\x0E\x0F\x09\x07\x08\x03", 9);
    write_cmd(self, 0xE8, (const uint8_t *)"\x34", 1);
    write_cmd(self, 0x62, (const uint8_t *)"\x18\x0D\x71\xED\x70\x70\x18\x0F\x71\xEF\x70\x70", 12);
    write_cmd(self, 0x63, (const uint8_t *)"\x18\x11\x71\xF1\x70\x70\x18\x13\x71\xF3\x70\x70", 12);
    write_cmd(self, 0x64, (const uint8_t *)"\x28\x29\xF1\x01\xF1\x00\x07", 7);
    write_cmd(self, 0x66, (const uint8_t *)"\x3C\x00\xCD\x67\x45\x45\x10\x00\x00\x00", 10);
    write_cmd(self, 0x67, (const uint8_t *)"\x00\x3C\x00\x00\x00\x01\x54\x10\x32\x98", 10);
    write_cmd(self, 0x74, (const uint8_t *)"\x10\x85\x80\x00\x00\x4E\x00", 7);
    write_cmd(self, 0x98, (const uint8_t *)"\x3e\x07", 2);
    write_cmd(self, 0x35, (const uint8_t *)NULL, 0);
    write_cmd(self, 0x21, (const uint8_t *)NULL, 0);
    write_cmd(self, 0x11, (const uint8_t *)NULL, 0);
    mp_hal_delay_ms(120);

    write_cmd(self, 0x29, (const uint8_t *)NULL, 0);
    mp_hal_delay_ms(20);

    set_rotation(self);

    if (self->backlight) {
        mp_hal_pin_write(self->backlight, 1);
    }

    write_cmd(self, GC9A01_DISPON, (const uint8_t *)NULL, 0);
    mp_hal_delay_ms(120);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gc9a01_GC9A01_init_obj, gc9a01_GC9A01_init);

// *FORMAT-OFF*
static const mp_rom_map_elem_t gc9a01_GC9A01_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&gc9a01_GC9A01_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_len), MP_ROM_PTR(&gc9a01_GC9A01_write_len_obj) },
    { MP_ROM_QSTR(MP_QSTR_hard_reset), MP_ROM_PTR(&gc9a01_GC9A01_hard_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset), MP_ROM_PTR(&gc9a01_GC9A01_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_mode), MP_ROM_PTR(&gc9a01_GC9A01_sleep_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_inversion_mode), MP_ROM_PTR(&gc9a01_GC9A01_inversion_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_window), MP_ROM_PTR(&gc9a01_GC9A01_set_window_obj) },
    { MP_ROM_QSTR(MP_QSTR_map_bitarray_to_rgb565), MP_ROM_PTR(&gc9a01_map_bitarray_to_rgb565_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&gc9a01_GC9A01_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&gc9a01_GC9A01_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&gc9a01_GC9A01_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&gc9a01_GC9A01_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&gc9a01_GC9A01_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_buffer), MP_ROM_PTR(&gc9a01_GC9A01_blit_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&gc9a01_GC9A01_draw_obj)},
    { MP_ROM_QSTR(MP_QSTR_draw_len), MP_ROM_PTR(&gc9a01_GC9A01_draw_len_obj)},
    { MP_ROM_QSTR(MP_QSTR_bitmap), MP_ROM_PTR(&gc9a01_GC9A01_bitmap_obj)},
    { MP_ROM_QSTR(MP_QSTR_pbitmap), MP_ROM_PTR(&gc9a01_GC9A01_pbitmap_obj)},
    { MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&gc9a01_GC9A01_fill_circle_obj)},
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&gc9a01_GC9A01_circle_obj)},
    { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&gc9a01_GC9A01_arc_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_arc), MP_ROM_PTR(&gc9a01_GC9A01_fill_arc_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&gc9a01_GC9A01_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&gc9a01_GC9A01_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&gc9a01_GC9A01_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&gc9a01_GC9A01_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&gc9a01_GC9A01_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&gc9a01_GC9A01_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&gc9a01_GC9A01_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&gc9a01_GC9A01_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&gc9a01_GC9A01_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_vscrdef), MP_ROM_PTR(&gc9a01_GC9A01_vscrdef_obj) },
    { MP_ROM_QSTR(MP_QSTR_vscsad), MP_ROM_PTR(&gc9a01_GC9A01_vscsad_obj) },
    { MP_ROM_QSTR(MP_QSTR_offset), MP_ROM_PTR(&gc9a01_GC9A01_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_jpg), MP_ROM_PTR(&gc9a01_GC9A01_jpg_obj)},
    { MP_ROM_QSTR(MP_QSTR_polygon_center), MP_ROM_PTR(&gc9a01_GC9A01_polygon_center_obj)},
    { MP_ROM_QSTR(MP_QSTR_polygon), MP_ROM_PTR(&gc9a01_GC9A01_polygon_obj)},
    { MP_ROM_QSTR(MP_QSTR_fill_polygon), MP_ROM_PTR(&gc9a01_GC9A01_fill_polygon_obj)},
    { MP_ROM_QSTR(MP_QSTR_bounding), MP_ROM_PTR(&gc9a01_GC9A01_bounding_obj)},
};
static MP_DEFINE_CONST_DICT(gc9a01_GC9A01_locals_dict, gc9a01_GC9A01_locals_dict_table);

#if MICROPY_OBJ_TYPE_REPR == MICROPY_OBJ_TYPE_REPR_SLOT_INDEX
MP_DEFINE_CONST_OBJ_TYPE(gc9a01_GC9A01_type, MP_QSTR_GC9A01, MP_TYPE_FLAG_NONE, print, gc9a01_GC9A01_print, make_new, gc9a01_GC9A01_make_new, locals_dict, &gc9a01_GC9A01_locals_dict);
#else
const mp_obj_type_t gc9a01_GC9A01_type = {
    { &mp_type_type },
    .name = MP_QSTR_GC9A01,
    .print = gc9a01_GC9A01_print,
    .make_new = gc9a01_GC9A01_make_new,
    .locals_dict = (mp_obj_dict_t *)&gc9a01_GC9A01_locals_dict,
};
#endif
// *FORMAT-ON*


mp_obj_t gc9a01_GC9A01_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum {
        ARG_spi, ARG_width, ARG_height, ARG_reset, ARG_dc, ARG_cs, ARG_backlight, ARG_rotation, ARG_options, ARG_buffer_size
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} }, { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} }, { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} }, { MP_QSTR_reset, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }, { MP_QSTR_dc, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }, { MP_QSTR_cs, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }, { MP_QSTR_backlight, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }, { MP_QSTR_rotation, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0 } }, { MP_QSTR_options, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0}}, { MP_QSTR_buffer_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];

    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create new object
    gc9a01_GC9A01_obj_t *self = m_new_obj(gc9a01_GC9A01_obj_t);

    self->base.type = &gc9a01_GC9A01_type;

    // set parameters
    mp_obj_base_t *spi_obj = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[ARG_spi].u_obj);

    self->spi_obj = spi_obj;
    self->display_width = args[ARG_width].u_int;
    self->width = args[ARG_width].u_int;
    self->display_height = args[ARG_height].u_int;
    self->height = args[ARG_height].u_int;
    self->rotation = args[ARG_rotation].u_int % 8;
    self->options = args[ARG_options].u_int & 0xff;
    self->buffer_size = args[ARG_buffer_size].u_int;

    if (self->buffer_size) {
        self->i2c_buffer = m_malloc(self->buffer_size);
    }

    if ((self->display_height != 240 && (self->display_width != 240 || self->display_width != 135)) && (self->display_height != 320 && self->display_width != 240)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Unsupported display. Only 240x320, 240x240 and 135x240 are supported"));
    }

    if (args[ARG_dc].u_obj == MP_OBJ_NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("must specify dc pin"));
    }

    if (args[ARG_reset].u_obj != MP_OBJ_NULL) {
        self->reset = mp_hal_get_pin_obj(args[ARG_reset].u_obj);
    }

    self->dc = mp_hal_get_pin_obj(args[ARG_dc].u_obj);

    if (args[ARG_cs].u_obj != MP_OBJ_NULL) {
        self->cs = mp_hal_get_pin_obj(args[ARG_cs].u_obj);
    }

    if (args[ARG_backlight].u_obj != MP_OBJ_NULL) {
        self->backlight = mp_hal_get_pin_obj(args[ARG_backlight].u_obj);
    }

    self->bounding = 0;
    self->min_x = self->display_width;
    self->min_y = self->display_height;
    self->max_x = 0;
    self->max_y = 0;

    return MP_OBJ_FROM_PTR(self);
}

// *FORMAT-OFF*
static const mp_map_elem_t gc9a01_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gc9a01) },
    { MP_ROM_QSTR(MP_QSTR_color565), (mp_obj_t)&gc9a01_color565_obj },
    { MP_ROM_QSTR(MP_QSTR_map_bitarray_to_rgb565), (mp_obj_t)&gc9a01_map_bitarray_to_rgb565_obj },
    { MP_ROM_QSTR(MP_QSTR_GC9A01), (mp_obj_t)&gc9a01_GC9A01_type },
    { MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(BLACK) },
    { MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(BLUE) },
    { MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(RED) },
    { MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(GREEN) },
    { MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(CYAN) },
    { MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(MAGENTA) },
    { MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(WHITE) },
    { MP_ROM_QSTR(MP_QSTR_FAST), MP_ROM_INT(JPG_MODE_FAST)},
    { MP_ROM_QSTR(MP_QSTR_SLOW), MP_ROM_INT(JPG_MODE_SLOW)},
    { MP_ROM_QSTR(MP_QSTR_WRAP), MP_ROM_INT(OPTIONS_WRAP)},
    { MP_ROM_QSTR(MP_QSTR_WRAP_H), MP_ROM_INT(OPTIONS_WRAP_H)},
    { MP_ROM_QSTR(MP_QSTR_WRAP_V), MP_ROM_INT(OPTIONS_WRAP_V)}
};
static MP_DEFINE_CONST_DICT(mp_module_gc9a01_globals, gc9a01_module_globals_table);

const mp_obj_module_t mp_module_gc9a01 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_gc9a01_globals,
};
// *FORMAT-ON*

#if MICROPY_VERSION >= 0x011300                     // MicroPython 1.19 or later
MP_REGISTER_MODULE(MP_QSTR_gc9a01, mp_module_gc9a01);
#else
MP_REGISTER_MODULE(MP_QSTR_gc9a01, mp_module_gc9a01, MODULE_GC9A01_ENABLED);
#endif
