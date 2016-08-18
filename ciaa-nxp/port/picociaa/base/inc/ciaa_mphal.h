/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
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
#ifndef __MICROPY_INCLUDED_EDUCIAA_MPHAL_H__
#define __MICROPY_INCLUDED_EDUCIAA_MPHAL_H__

#define HAL_GetTick mp_hal_get_milliseconds

#include "py/mpstate.h"
#include <board.h>

void mp_hal_init(void);
mp_uint_t mp_hal_get_milliseconds(void);
void mp_hal_milli_delay(mp_uint_t ms);
void mp_hal_set_interrupt_char(int c);

void mp_hal_initRTC(void);
void mp_hal_setTimeRTC(uint32_t hr,uint32_t min, uint32_t sec, uint32_t day, uint32_t mon, uint32_t yr,uint32_t dayOfWeek);
void mp_hal_getTimeRTC(uint32_t* hr,uint32_t* min, uint32_t* sec, uint32_t* day, uint32_t* mon, uint32_t* yr,uint32_t* dayOfWeek);

#endif
