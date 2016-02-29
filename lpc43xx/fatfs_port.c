/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "py/mphal.h"
#include "py/runtime.h"
#include "lib/fatfs/ff.h"        /* FatFs lower layer API */
#include "lib/fatfs/diskio.h"    /* FatFs lower layer API */

#include <chip.h>

extern void PYB_rtc_init(void);

const PARTITION VolToPart[MICROPY_FATFS_VOLUMES] = {
    {0, 1},     // Logical drive 0 ==> Physical drive 0, 1st partition
    {1, 0},     // Logical drive 1 ==> Physical drive 1 (auto detection)
    {2, 0},     // Logical drive 2 ==> Physical drive 2 (auto detection)
    /*
    {3, 0},     // Logical drive 3 ==> Physical drive 3 (auto detection)
    {0, 2},     // Logical drive 2 ==> Physical drive 0, 2nd partition
    {0, 3},     // Logical drive 3 ==> Physical drive 0, 3rd partition
    */
};

DWORD get_fattime(void) {
	PYB_rtc_init();
	RTC_TIME_T date;
    Chip_RTC_GetFullTime(LPC_RTC, &date);
    return ((2000 + date.time[RTC_TIMETYPE_YEAR] - 1980) << 25) |
    		((date.time[RTC_TIMETYPE_MONTH]) << 21) |
			((date.time[RTC_TIMETYPE_DAYOFMONTH]) << 16) |
			((date.time[RTC_TIMETYPE_HOUR]) << 11) |
			((date.time[RTC_TIMETYPE_MINUTE]) << 5) |
			(date.time[RTC_TIMETYPE_SECOND]/ 2);
}
