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

#include "py/runtime.h"
#include "board.h"
#include "modpyb.h"
#include "lpc43xx_mphal.h"

#include <chip.h>

#include "py/stream.h"

#define RX_BUFFER_MAX_SIZE	2048

typedef struct _pyb_uart_obj_t {
	mp_obj_base_t base;
	uint8_t uartNumber;
	LPC_USART_T *pUART;
	RINGBUFF_T rbuf;
	uint32_t timeoutFirstChar;
	uint32_t bufferMaxSize;
	uint32_t timeoutBtwChars;
	uint8_t bufferEnabled;
	uint32_t havePacket;
	uint32_t currentIndex;
} pyb_uart_obj_t;

STATIC pyb_uart_obj_t pyb_uart_obj[] = { /* */
{ { &pyb_uart_type }, 0, LPC_USART0 }, /* */
{ { &pyb_uart_type }, 1, LPC_UART1 }, /* */
{ { &pyb_uart_type }, 2, LPC_USART2 }, /* */
{ { &pyb_uart_type }, 3, LPC_USART3 }, /* */
};

STATIC uint8_t uart_buffer[MP_ARRAY_SIZE(pyb_uart_obj)][RX_BUFFER_MAX_SIZE];

void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	pyb_uart_obj_t *self = self_in;
	mp_printf(print, "UART%d", self->uartNumber);
}

STATIC mp_obj_t pyb_uart_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 1, 1, false);
	mp_int_t uart_id = mp_obj_get_int(args[0]);

	if (uart_id < 0 || uart_id >= MP_ARRAY_SIZE(pyb_uart_obj)) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART %d does not exist", uart_id));
	}

	return (mp_obj_t) &pyb_uart_obj[uart_id];
}
//_______________________________________________________________________________________________________________

// Function init
STATIC mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args,
		mp_map_t *kw_args) {
	static const mp_arg_t allowed_args[] = { /* */
	{ MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 9600 } }, /* */
	{ MP_QSTR_bits, MP_ARG_INT, { .u_int = 8 } }, /* */
	{ MP_QSTR_parity, MP_ARG_OBJ, { .u_obj = mp_const_none } }, /* */
	{ MP_QSTR_stop, MP_ARG_INT, { .u_int = 1 } }, /* */
	{ MP_QSTR_flow, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } }, /* */
	{ MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 100 } }, /* */
	{ MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } }, /* */
	{ MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = RX_BUFFER_MAX_SIZE } }, /* */
	{ MP_QSTR_packet_mode, MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = 0 } }, /* */
	{ MP_QSTR_packet_end_char, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } }, /* */
	};

	// parse args
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// Baudrate
	mp_int_t baudrate = args[0].u_int;

	// bits
	mp_int_t bits = args[1].u_int;
	if (bits != 8)
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "unsupported bits value"));

	// parity
	mp_int_t parity;
	if (args[2].u_obj == mp_const_none) {
		parity = 0;
	} else {
		parity = mp_obj_get_int(args[2].u_obj);
	}

	// Stop bits
	mp_int_t stopBits = args[3].u_int;
	if (stopBits != 1 && stopBits != 2)
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "unsupported stop bits value"));

	// timeout
	uint32_t timeout = args[5].u_int;
	self->timeoutFirstChar = timeout;

	// timeout char
	self->timeoutBtwChars = args[6].u_int;

	// buffer len
	uint32_t size = args[7].u_int;
	if (size > RX_BUFFER_MAX_SIZE)
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "buffer size is too big"));
	self->bufferMaxSize = size;

	// packet mode
	self->bufferEnabled = args[8].u_bool;
	// end char for packet mode
	uint8_t endChar = args[9].u_int;

	LPC_USART_T *pUART = self->pUART;
	Chip_UART_SetBaud(pUART, baudrate);

	uint32_t config = UART_LCR_WLEN8;
	if (stopBits == 2)
		config |= UART_LCR_SBS_2BIT;
	else
		config |= UART_LCR_SBS_1BIT;

	switch (parity) {
	case 1:
		config |= UART_LCR_PARITY_ODD;
		break;
	case 2:
		config |= UART_LCR_PARITY_EVEN;
		break;
	case 3:
		config |= UART_LCR_PARITY_F_0;
		break;
	case 4:
		config |= UART_LCR_PARITY_F_1;
		break;
	default:
		config |= UART_LCR_PARITY_DIS;
	}
	Chip_UART_ConfigData(pUART, config);

	if (self->bufferEnabled) {
		uint8_t *buf = uart_buffer[self->uartNumber];
		RingBuffer_Init(&self->rbuf, buf, sizeof(uint8_t), size);
		self->havePacket = FALSE;
	}

	return mp_const_none;
}

STATIC mp_obj_t pyb_uart_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
	return pyb_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
STATIC bool uart_rx_any(pyb_uart_obj_t *self) {
	uint32_t any = self->bufferEnabled ? self->havePacket : Chip_UART_ReadLineStatus(self->pUART) & UART_LSR_RDR;
	return any;
}

STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
	pyb_uart_obj_t *self = self_in;

	uint32_t any = uart_rx_any(self);

	if (any) {
		return mp_const_true;
	} else {
		return mp_const_false;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

// Waits at most timeout milliseconds for at least 1 char to become ready for
// reading (from buf or for direct reading).
// Returns true if something available, false if not.
STATIC bool uart_rx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
	uint32_t start = mp_hal_get_milliseconds();
	for (;;) {
		if (uart_rx_any(self)) {
			return true; // have at least 1 char ready for reading
		}
		if (mp_hal_get_milliseconds() - start >= timeout) {
			return false; // timeout
		}
		__WFI();
	}
	return false;
}
STATIC int32_t uart_rx_char(pyb_uart_obj_t* self) {
	if (Chip_UART_ReadLineStatus(self->pUART) & UART_LSR_RDR) {
		return (int32_t) Chip_UART_ReadByte(self->pUART);
	}
	return -1;
}

/// \method readchar()
/// Receive a single character on the bus.
/// Return value: The character read, as an integer.  Returns -1 on timeout.
STATIC mp_obj_t pyb_uart_readchar(mp_obj_t self_in) {
	pyb_uart_obj_t *self = self_in;
	if (uart_rx_wait(self, self->timeoutFirstChar)) {
		return MP_OBJ_NEW_SMALL_INT(uart_rx_char(self));
	} else {
		// return -1 on timeout
		return MP_OBJ_NEW_SMALL_INT(-1);
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_readchar_obj, pyb_uart_readchar);

/// \method writechar(char)
/// Write a single character on the bus.  `char` is an integer to write.
/// Return value: `None`.
STATIC mp_obj_t pyb_uart_writechar(mp_obj_t self_in, mp_obj_t char_in) {
	pyb_uart_obj_t *self = self_in;

	// get the character to write (might be 9 bits)
	uint8_t data = mp_obj_get_int(char_in);
	Chip_UART_SendByte(self->pUART, data);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_uart_writechar_obj, pyb_uart_writechar);

// Read function. used by streams
STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
	pyb_uart_obj_t *self = self_in;
	byte *buf = buf_in;

	// make sure we want at least 1 char
	if (size == 0) {
		return 0;
	}

	if (self->bufferEnabled == 0) {
		// wait for first char to become available
		if (!uart_rx_wait(self, self->timeoutFirstChar)) {
			// we can either return 0 to indicate EOF (then read() method returns b'')
			// or return EAGAIN error to indicate non-blocking (then read() method returns None)
			return 0;
		}

		// read the data
		byte *orig_buf = buf;
		for (;;) {
			int data = uart_rx_char(self);
			*buf++ = data;
			if (--size == 0 || !uart_rx_wait(self, self->timeoutBtwChars)) {
				// return number of bytes read
				return buf - orig_buf;
			}
		}
	} else {
		// packet mode
		uint32_t packetSize = RingBuffer_GetCount(&self->rbuf);
		uint32_t i;
		for (i = self->currentIndex; i < packetSize; i++) {
			RingBuffer_Pop(&self->rbuf, buf++);
			size--;
			if (size == 0)
				break;
		}

		if (i != packetSize) {
			// reading not finished. save current index for next reading
			self->currentIndex = i;
		} else {	// packet cleaning
			self->currentIndex = 0;
			RingBuffer_Flush(&self->rbuf);
		}

		return i;
	}
}

// write function. used by streams
STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
	pyb_uart_obj_t *self = self_in;
	LPC_USART_T *pUART = self->pUART;
	int delay = self->timeoutBtwChars;

	if (delay == 0)
		return Chip_UART_Send(LPC_USART0, buf_in, (int) size);

	uint32_t i = 0;
	while (size > 0) {
		uint8_t data =
		Chip_UART_SendByte(LPC_USART0, *((uint8_t*)buf_in++), 1);
		mp_hal_milli_delay(delay);
		size--;
		i++;
	}
	return size;
}

// method deinit
// disable uart. not implented
STATIC mp_obj_t pyb_uart_deinit(mp_obj_t self_in) {
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_deinit_obj, pyb_uart_deinit);

//_______________________________________________________________________________________________________________

STATIC const mp_map_elem_t pyb_uart_locals_dict_table[] = { //
		///
				{ MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t) &pyb_uart_init_obj },
				///
				{ MP_OBJ_NEW_QSTR(MP_QSTR_any), (mp_obj_t) &pyb_uart_any_obj },
				///
				{ MP_OBJ_NEW_QSTR(MP_QSTR_readchar), (mp_obj_t) &pyb_uart_readchar_obj },
				///
				{ MP_OBJ_NEW_QSTR(MP_QSTR_writechar), (mp_obj_t) &pyb_uart_writechar_obj },
				/// \method read([nbytes])
				{ MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t) &mp_stream_read_obj },
				/// \method readall()
				{ MP_OBJ_NEW_QSTR(MP_QSTR_readall), (mp_obj_t) &mp_stream_readall_obj },
				/// \method readline()
				{ MP_OBJ_NEW_QSTR(MP_QSTR_readline), (mp_obj_t) &mp_stream_unbuffered_readline_obj },
				/// \method readinto(buf[, nbytes])
				{ MP_OBJ_NEW_QSTR(MP_QSTR_readinto), (mp_obj_t) &mp_stream_readinto_obj },
				/// \method write(buf)
				{ MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t) &mp_stream_write_obj },
				// deinit
				{ MP_OBJ_NEW_QSTR(MP_QSTR_deinit), (mp_obj_t) &pyb_uart_deinit_obj }, };

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

STATIC const mp_stream_p_t uart_stream_p = { //
		//
				.read = pyb_uart_read, //
				.write = pyb_uart_write, //
				//.ioctl = pyb_uart_ioctl, //
				.is_text = false, //
		};

const mp_obj_type_t pyb_uart_type = {
//
		{ &mp_type_type }, //
		.name = MP_QSTR_UART, //
		.print = pyb_uart_print, //
		.make_new = pyb_uart_make_new, //
		.stream_p = &uart_stream_p, //
		.locals_dict = (mp_obj_t) &pyb_uart_locals_dict, //
		};
