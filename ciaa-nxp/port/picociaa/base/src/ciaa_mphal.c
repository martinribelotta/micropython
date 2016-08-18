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

#include <string.h>
#include <time.h>

#include "ciaa_mphal.h"
#include "board.h"
#include "chip.h"
#include "rtc_ut.h"

static int interrupt_char;

static volatile mp_uint_t tick_ct = 0;

void SysTick_Handler(void) {
	tick_ct++;
}


void mp_hal_init(void) {
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);
	Board_Init();
	Board_Buttons_Init();
}



// Time funcions
mp_uint_t mp_hal_get_milliseconds(void) {
    return tick_ct;
}

// Time funcions
mp_uint_t mp_hal_ticks_ms(void) {
    return mp_hal_get_milliseconds();
}

void mp_hal_milli_delay(mp_uint_t ms) {
	uint32_t end = tick_ct + ms;
	while(tick_ct < end)
		__WFI();
}

void mp_hal_delay_ms(mp_uint_t ms) { mp_hal_milli_delay(ms); }

// STD Functions
void mp_hal_set_interrupt_char(int c) {
    interrupt_char = c;
}

int mp_hal_stdin_rx_chr(void) {
   for (;;) {
        int r = Board_UARTGetChar();
        if (r!= EOF) {
            return r;
        } else {
            __WFI();
        }
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for (; len > 0; --len) {
    	Board_UARTPutChar(*str++);
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len) {
    for (; len > 0; --len) {
        if (*str == '\n') {
        	Board_UARTPutChar('\r');
        }
        Board_UARTPutChar(*str++);
    }
}

//RTC
void mp_hal_initRTC(void)
{
	Chip_RTC_Init(LPC_RTC);
}

void mp_hal_setTimeRTC(uint32_t hr,uint32_t min, uint32_t sec, uint32_t day, uint32_t mon, uint32_t yr,uint32_t dayOfWeek)
{
	uint32_t rtcTick;
	struct tm m_time;
	m_time.tm_hour = hr;
	m_time.tm_min = min;
	m_time.tm_sec = sec;
	m_time.tm_mday = day;
	m_time.tm_mon = mon;
	m_time.tm_year = yr;
	m_time.tm_wday = dayOfWeek;
	ConvertTimeRtc(&m_time, &rtcTick);
	Chip_RTC_SetCount(LPC_RTC, rtcTick);
}

void mp_hal_getTimeRTC(uint32_t* hr,uint32_t* min, uint32_t* sec, uint32_t* day, uint32_t* mon, uint32_t* yr,uint32_t* dayOfWeek)
{
	struct tm m_time;
	uint32_t rtcTick = Chip_RTC_GetCount(LPC_RTC);
	ConvertRtcTime(rtcTick, &m_time);
	*hr        = m_time.tm_hour;
	*min       = m_time.tm_min;
	*sec       = m_time.tm_sec;
	*day       = m_time.tm_mday;
	*mon       = m_time.tm_mon;
	*yr        = m_time.tm_year;
	*dayOfWeek = m_time.tm_wday;
}
