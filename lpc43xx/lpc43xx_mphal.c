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
#include "mphalport.h"
#include "board.h"
#include "cdc_vcom.h"

static int interrupt_char;

static volatile mp_uint_t tick_ct = 0;

void SysTick_Handler(void) {
	tick_ct++;
	Board_Systick();
}

void mp_hal_init(void) {
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);
	Board_Init();
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
	while (tick_ct < end)
		__WFI();
}

void mp_hal_delay_ms(mp_uint_t ms) {
	mp_hal_milli_delay(ms);
}

// STD Functions
void mp_hal_set_interrupt_char(int c) {
	interrupt_char = c;
}

int mp_hal_stdin_rx_chr(void) {
    for (;;) {
#if defined(USE_USB0) || defined(USE_USB1)
		uint8_t c;
	    if (vcom_connected())
	    	if (vcom_bread(&c, 1) != 0)
	    		return c;
#endif
        if (MP_STATE_PORT(pyb_stdio_uart) != NULL && uart_rx_any(MP_STATE_PORT(pyb_stdio_uart))) {
            return uart_rx_char(MP_STATE_PORT(pyb_stdio_uart));
        }
        __WFI();
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (MP_STATE_PORT(pyb_stdio_uart) != NULL) {
        uart_tx_strn(MP_STATE_PORT(pyb_stdio_uart), str, len);
    }
#if defined(USE_USB0) || defined(USE_USB1)
	while (vcom_connected() && len) {
		uint32_t n = vcom_write((uint8_t*) str, len);
		str += n;
		len -= n;
	}
#endif
}

void vcom_tx_strn_cooked(const char *str, uint len) {
    for (const char *top = str + len; str < top; str++) {
        if (*str == '\n') {
        	uint8_t c = '\r';
            while (vcom_connected() && vcom_write(&c, 1) == 0)
            	__WFI();
        }
        while (vcom_connected() && vcom_write((uint8_t*) str, 1) == 0)
        	__WFI();
    }
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // send stdout to UART and USB CDC VCP
    if (MP_STATE_PORT(pyb_stdio_uart) != NULL) {
        uart_tx_strn_cooked(MP_STATE_PORT(pyb_stdio_uart), str, len);
    }
#if defined(USE_USB0) || defined(USE_USB1)
    if (vcom_connected())
    	vcom_tx_strn_cooked(str, len);
#endif
}

void mp_hal_stdout_tx_str(const char *str) {
	mp_hal_stdout_tx_strn(str, strlen(str));
}
