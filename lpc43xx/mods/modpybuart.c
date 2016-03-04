/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "pybioctl.h"

#include <chip.h>
#include <board.h>
#include <modpybuart.h>

/// \moduleref pyb
/// \class UART - duplex serial communication bus
///
/// UART implements the standard UART/USART duplex serial communications protocol.  At
/// the physical level it consists of 2 lines: RX and TX.  The unit of communication
/// is a character (not to be confused with a string character) which can be 8 or 9
/// bits wide.
///
/// UART objects can be created and initialised using:
///
///     from pyb import UART
///
///     uart = UART(1, 9600)                         # init with given baudrate
///     uart.init(9600, bits=8, parity=None, stop=1) # init with given parameters
///
/// Bits can be 8 or 9.  Parity can be None, 0 (even) or 1 (odd).  Stop can be 1 or 2.
///
/// A UART object acts like a stream object and reading and writing is done
/// using the standard stream methods:
///
///     uart.read(10)       # read 10 characters, returns a bytes object
///     uart.readall()      # read all available characters
///     uart.readline()     # read a line
///     uart.readinto(buf)  # read and store into the given buffer
///     uart.write('abc')   # write the 3 characters
///
/// Individual characters can be read/written using:
///
///     uart.readchar()     # read 1 character and returns it as an integer
///     uart.writechar(42)  # write 1 character
///
/// To check if there is anything to be read, use:
///
///     uart.any()               # returns True if any characters waiting

typedef LPC_USART_T USART_TypeDef;

typedef struct
{
  uint32_t BaudRate;
  uint32_t WordLength;
  uint32_t StopBits;
  uint32_t Parity;
  uint32_t Mode;
  uint32_t HwFlowCtl;
} UART_InitTypeDef;

typedef struct
{
  USART_TypeDef            *Instance;        /*!< UART registers base address        */
  UART_InitTypeDef         Init;             /*!< UART communication parameters      */
  RINGBUFF_T read_buffer;
} UART_HandleTypeDef;

struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    UART_HandleTypeDef uart;            // this is 17 words big
    pyb_uart_t uart_id : 8;
    bool is_enabled : 1;
    byte char_width;                    // 0 for 7,8 bit chars, 1 for 9 bit chars
    uint16_t char_mask;                 // 0x7f for 7 bit, 0xff for 8 bit, 0x1ff for 9 bit
    uint16_t timeout;                   // timeout waiting for first char
    uint16_t timeout_char;              // timeout waiting between chars
};

STATIC mp_obj_t pyb_uart_deinit(mp_obj_t self_in);

void uart_init0(void) {
    for (int i = 0; i < MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all)); i++) {
        MP_STATE_PORT(pyb_uart_obj_all)[i] = NULL;
    }
}

// unregister all interrupt sources
void uart_deinit(void) {
    for (int i = 0; i < MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all)); i++) {
        pyb_uart_obj_t *uart_obj = MP_STATE_PORT(pyb_uart_obj_all)[i];
        if (uart_obj != NULL) {
            pyb_uart_deinit(uart_obj);
        }
    }
}

STATIC const struct {
	LPC_USART_T *pUART;
	mp_uint_t irq;
} usart_map[] = {
	{ LPC_USART0, USART0_IRQn },
	{ LPC_UART1, UART1_IRQn },
	{ LPC_USART2, USART2_IRQn },
	{ LPC_USART3, USART3_IRQn },
};

STATIC inline int Chip_UART_CheckLineStatus(LPC_USART_T *pUART, uint32_t flags) {
	return (Chip_UART_ReadLineStatus(pUART) & flags) == flags;
}

// assumes Init parameters have been set up correctly
STATIC bool uart_init2(pyb_uart_obj_t *uart_obj) {
    USART_TypeDef *UARTx = usart_map[uart_obj->uart_id].pUART;
    uart_obj->uart.Instance = UARTx;
    Board_UART_Init(uart_obj->uart.Instance);
    Chip_UART_Init(uart_obj->uart.Instance);
	Chip_UART_SetupFIFOS(uart_obj->uart.Instance, UART_FCR_FIFO_EN | UART_FCR_TRG_LEV0);
	Chip_UART_TXEnable(uart_obj->uart.Instance);
	Chip_UART_ConfigData(uart_obj->uart.Instance,
			(uart_obj->uart.Init.WordLength | uart_obj->uart.Init.StopBits | uart_obj->uart.Init.Parity));
    uart_obj->is_enabled = true;
    return true;
}

/* obsolete and unused
bool uart_init(pyb_uart_obj_t *uart_obj, uint32_t baudrate) {
    UART_HandleTypeDef *uh = &uart_obj->uart;
    memset(uh, 0, sizeof(*uh));
    uh->Init.BaudRate = baudrate;
    uh->Init.WordLength = UART_WORDLENGTH_8B;
    uh->Init.StopBits = UART_STOPBITS_1;
    uh->Init.Parity = UART_PARITY_NONE;
    uh->Init.Mode = UART_MODE_TX_RX;
    uh->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uh->Init.OverSampling = UART_OVERSAMPLING_16;
    return uart_init2(uart_obj);
}
*/

mp_uint_t uart_rx_any(pyb_uart_obj_t *self) {
    int buffer_bytes = RingBuffer_GetCount(&self->uart.read_buffer);
    if (buffer_bytes) {
        return buffer_bytes;
    } else {
        return Chip_UART_CheckLineStatus(self->uart.Instance, UART_LSR_RDR) != 0;
    }
}

// Waits at most timeout milliseconds for at least 1 char to become ready for
// reading (from buf or for direct reading).
// Returns true if something available, false if not.
STATIC bool uart_rx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout) {
        if (!RingBuffer_IsEmpty(&self->uart.read_buffer) ||
        		Chip_UART_CheckLineStatus(self->uart.Instance, UART_LSR_RDR) != 0) {
            return true; // have at least 1 char ready for reading
        }
        __WFI();
    }
    return false;
}

// assumes there is a character available
int uart_rx_char(pyb_uart_obj_t *self) {
	if (RingBuffer_GetCount(&self->uart.read_buffer) > 0) {
		uint8_t data;
		RingBuffer_Pop(&self->uart.read_buffer, &data);
		return data;
	} else {
        return Chip_UART_ReadByte(self->uart.Instance);
    }
}

// Waits at most timeout milliseconds for TX register to become empty.
// Returns true if can write, false if can't.
STATIC bool uart_tx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start < timeout)) {
        if (Chip_UART_CheckBusy(self->uart.Instance) == RESET) {
            return true; // tx register is empty
        }
        __WFI();
    }
    return false; // timeout
}

/**
  * @brief  HAL Status structures definition
  */
typedef enum
{
  HAL_OK       = 0x00,
  HAL_ERROR    = 0x01,
  HAL_BUSY     = 0x02,
  HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;

STATIC /* Transmit a byte array through the UART peripheral (blocking) with timeout */
HAL_StatusTypeDef Chip_UART_SendBlockingTout(LPC_USART_T *pUART, const void *data, int numBytes, int tout)
{
	uint8_t *p8 = (uint8_t *) data;

	while (numBytes > 0) {
		/* Wait for space in FIFO */
		uint32_t start = HAL_GetTick();
		while ((Chip_UART_ReadLineStatus(pUART) & UART_LSR_THRE) == 0) {
			if ((HAL_GetTick() - start) > tout)
				return HAL_TIMEOUT;
		}
		Chip_UART_SendByte(pUART, (uint8_t) *p8++);
		numBytes--;
	}

	return HAL_OK;
}

STATIC HAL_StatusTypeDef uart_tx_data(pyb_uart_obj_t *self, uint8_t *data, uint16_t len) {
    // The timeout specified here is for waiting for the TX data register to
    // become empty (ie between chars), as well as for the final char to be
    // completely transferred.  The default value for timeout_char is long
    // enough for 1 char, but we need to double it to wait for the last char
    // to be transferred to the data register, and then to be transmitted.
	return Chip_UART_SendBlockingTout(self->uart.Instance, data, len, 2 * self->timeout_char);
}

STATIC void uart_tx_char(pyb_uart_obj_t *uart_obj, int c) {
    uint8_t ch = c;
    uart_tx_data(uart_obj, &ch, 1);
}

void uart_tx_strn(pyb_uart_obj_t *uart_obj, const char *str, uint len) {
    uart_tx_data(uart_obj, (uint8_t*)str, len);
}

void uart_tx_strn_cooked(pyb_uart_obj_t *uart_obj, const char *str, uint len) {
    for (const char *top = str + len; str < top; str++) {
        if (*str == '\n') {
            uart_tx_char(uart_obj, '\r');
        }
        uart_tx_char(uart_obj, *str);
    }
}

// this IRQ handler is set up to handle RXNE interrupts only
void uart_irq_handler(mp_uint_t uart_id) {
    // get the uart object
    pyb_uart_obj_t *self = MP_STATE_PORT(pyb_uart_obj_all)[uart_id];
    if (self == NULL) {
        // UART object has not been set, so we can't do anything, not
        // even disable the IRQ.  This should never happen.
        return;
    }
    Chip_UART_RXIntHandlerRB(self->uart.Instance, &self->uart.read_buffer);
}

STATIC int config_to_nbits(int config) {
	switch(config) {
	case UART_LCR_WLEN5: return 5;
	case UART_LCR_WLEN6: return 6;
	case UART_LCR_WLEN7: return 7;
	case UART_LCR_WLEN8: return 8;
	default: return 0;
	}
}

/******************************************************************************/
/* Micro Python bindings                                                      */

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = self_in;
    if (!self->is_enabled) {
        mp_printf(print, "UART(%u)", self->uart_id);
    } else {
        mp_int_t bits = config_to_nbits(self->uart.Init.WordLength);
        mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=",
            self->uart_id, self->uart.Init.BaudRate, bits);
        if (self->uart.Init.Parity == UART_LCR_PARITY_DIS) {
            mp_print_str(print, "None");
        } else {
            mp_printf(print, "%u", self->uart.Init.Parity == UART_LCR_PARITY_EVEN ? 0 : 1);
        }
        mp_printf(print, ", stop=%u, timeout=%u, timeout_char=%u, read_buf_len=%u)",
            self->uart.Init.StopBits == UART_LCR_SBS_1BIT ? 1 : 2,
            self->timeout, self->timeout_char,
            RingBuffer_GetSize(&self->uart.read_buffer));
    }
}

STATIC int nbits_to_config(int n) {
	switch(n) {
	case 5: return UART_LCR_WLEN5;
	case 6: return UART_LCR_WLEN6;
	case 7: return UART_LCR_WLEN7;
	case 8: return UART_LCR_WLEN8;
	default: return-1;
	}
}

/// \method init(baudrate, bits=8, parity=None, stop=1, *, timeout=1000, timeout_char=0, read_buf_len=64)
///
/// Initialise the UART bus with the given parameters:
///
///   - `baudrate` is the clock rate.
///   - `bits` is the number of bits per byte, 7, 8 or 9.
///   - `parity` is the parity, `None`, 0 (even) or 1 (odd).
///   - `stop` is the number of stop bits, 1 or 2.
///   - `timeout` is the timeout in milliseconds to wait for the first character.
///   - `timeout_char` is the timeout in milliseconds to wait between characters.
///   - `read_buf_len` is the character length of the read buffer (0 to disable).
STATIC mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_flow, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0 } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 64} },
    };

    // parse args
    struct {
        mp_arg_val_t baudrate, bits, parity, stop, flow, timeout, timeout_char, read_buf_len;
    } args;
    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, (mp_arg_val_t*)&args);

    // set the UART configuration values
    memset(&self->uart, 0, sizeof(self->uart));
    UART_InitTypeDef *init = &self->uart.Init;

    // baudrate
    init->BaudRate = args.baudrate.u_int;

    // parity
    mp_int_t bits = args.bits.u_int;
    if (args.parity.u_obj == mp_const_none) {
        init->Parity = UART_LCR_PARITY_DIS;
    } else {
        mp_int_t parity = mp_obj_get_int(args.parity.u_obj);
        init->Parity = (parity & 1) ? UART_LCR_PARITY_ODD : UART_LCR_PARITY_EVEN;
    }

    // number of bits
    init->WordLength = nbits_to_config(bits);
    if (init->WordLength == -1) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "unsupported combination of bits"));
    }

    // stop bits
    switch (args.stop.u_int) {
        case 1: init->StopBits = UART_LCR_SBS_1BIT; break;
        default: init->StopBits = UART_LCR_SBS_2BIT; break;
    }

    // flow control
    init->HwFlowCtl = args.flow.u_int;

    // extra config (not yet configurable)
    init->Mode = 0;

    // init UART (if it fails, it's because the port doesn't exist)
    if (!uart_init2(self)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", self->uart_id));
    }

    // set timeout
    self->timeout = args.timeout.u_int;

    // set timeout_char
    // make sure it is at least as long as a whole character (13 bits to be safe)
    self->timeout_char = args.timeout_char.u_int;
    uint32_t min_timeout_char = 13000 / init->BaudRate + 1;
    if (self->timeout_char < min_timeout_char) {
        self->timeout_char = min_timeout_char;
    }

    // setup the read buffer
    if (self->uart.read_buffer.data)
    	m_del(byte, self->uart.read_buffer.data, RingBuffer_GetSize(&self->uart.read_buffer));
    if (args.read_buf_len.u_int <= 0) {
        RingBuffer_Init(&self->uart.read_buffer, NULL, 0, 0);
        Chip_UART_IntDisable(self->uart.Instance, UART_IER_BITMASK);
        NVIC_DisableIRQ(usart_map[self->uart_id].irq);
    } else {
    	int irq = usart_map[self->uart_id].irq;
        byte *buffer = m_new(byte, args.read_buf_len.u_int);
    	RingBuffer_Init(&self->uart.read_buffer, buffer, sizeof(byte), args.read_buf_len.u_int);
        Chip_UART_IntEnable(self->uart.Instance, UART_IER_RBRINT | UART_IER_RLSINT);
        NVIC_SetPriority(irq, 1);
        NVIC_EnableIRQ(irq);
    }

    // compute actual baudrate that was configured
    // (this formula assumes UART_OVERSAMPLING_16)
    uint32_t actual_baudrate = Chip_UART_SetBaud(self->uart.Instance, init->BaudRate);

    // check we could set the baudrate within 5%
    //uint32_t baudrate_diff;
    //if (actual_baudrate > init->BaudRate) {
    //    baudrate_diff = actual_baudrate - init->BaudRate;
    //} else {
    //    baudrate_diff = init->BaudRate - actual_baudrate;
    //}
    init->BaudRate = actual_baudrate; // remember actual baudrate for printing
    //if (20 * baudrate_diff > init->BaudRate) {
    //    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "set baudrate %d is not within 5%% of desired value", actual_baudrate));
    //}

    return mp_const_none;
}

/// \classmethod \constructor(bus, ...)
///
/// Construct a UART object on the given bus.  `bus` can be 1-6, or 'XA', 'XB', 'YA', or 'YB'.
/// With no additional parameters, the UART object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out port
    int uart_id = mp_obj_get_int(args[0]);
	if (uart_id < 0 || uart_id >= MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all))) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_id));
	}

    pyb_uart_obj_t *self;
    if (MP_STATE_PORT(pyb_uart_obj_all)[uart_id] == NULL) {
        // create new UART object
        self = m_new0(pyb_uart_obj_t, 1);
        self->base.type = &pyb_uart_type;
        self->uart_id = uart_id;
        MP_STATE_PORT(pyb_uart_obj_all)[uart_id] = self;
    } else {
        // reference existing UART object
        self = MP_STATE_PORT(pyb_uart_obj_all)[uart_id];
    }

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return self;
}

STATIC mp_obj_t pyb_uart_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

/// \method deinit()
/// Turn off the UART bus.
STATIC mp_obj_t pyb_uart_deinit(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    UART_HandleTypeDef *uart = &self->uart;
    Chip_UART_DeInit(uart->Instance);
    NVIC_DisableIRQ(usart_map[self->uart_id].irq);
    self->is_enabled = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_deinit_obj, pyb_uart_deinit);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    return MP_OBJ_NEW_SMALL_INT(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

// this table converts from HAL_StatusTypeDef to POSIX errno
const byte mp_hal_status_to_errno_table[4] = {
    [HAL_OK] = 0,
    [HAL_ERROR] = EIO,
    [HAL_BUSY] = EBUSY,
    [HAL_TIMEOUT] = ETIMEDOUT,
};

NORETURN void mp_hal_raise(HAL_StatusTypeDef status) {
    nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(mp_hal_status_to_errno_table[status])));
}

/// \method writechar(char)
/// Write a single character on the bus.  `char` is an integer to write.
/// Return value: `None`.
STATIC mp_obj_t pyb_uart_writechar(mp_obj_t self_in, mp_obj_t char_in) {
    pyb_uart_obj_t *self = self_in;

    // get the character to write (might be 9 bits)
    uint16_t data = mp_obj_get_int(char_in);

    // write the character
    HAL_StatusTypeDef status;
    if (uart_tx_wait(self, self->timeout)) {
        status = uart_tx_data(self, (uint8_t*)&data, 1);
    } else {
        status = HAL_TIMEOUT;
    }

    if (status != HAL_OK) {
        mp_hal_raise(status);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_uart_writechar_obj, pyb_uart_writechar);

/// \method readchar()
/// Receive a single character on the bus.
/// Return value: The character read, as an integer.  Returns -1 on timeout.
STATIC mp_obj_t pyb_uart_readchar(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    if (uart_rx_wait(self, self->timeout)) {
        return MP_OBJ_NEW_SMALL_INT(uart_rx_char(self));
    } else {
        // return -1 on timeout
        return MP_OBJ_NEW_SMALL_INT(-1);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_readchar_obj, pyb_uart_readchar);

#if 0
// uart.sendbreak()
STATIC mp_obj_t pyb_uart_sendbreak(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    #if defined(MCU_SERIES_F7)
    self->uart.Instance->RQR = USART_RQR_SBKRQ; // write-only register
    #else
    self->uart.Instance->CR1 |= USART_CR1_SBK;
    #endif
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_sendbreak_obj, pyb_uart_sendbreak);
#endif

STATIC const mp_map_elem_t pyb_uart_locals_dict_table[] = {
    // instance methods

    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&pyb_uart_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit), (mp_obj_t)&pyb_uart_deinit_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_any), (mp_obj_t)&pyb_uart_any_obj },

    /// \method read([nbytes])
    { MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&mp_stream_read_obj },
    /// \method readall()
    { MP_OBJ_NEW_QSTR(MP_QSTR_readall), (mp_obj_t)&mp_stream_readall_obj },
    /// \method readline()
    { MP_OBJ_NEW_QSTR(MP_QSTR_readline), (mp_obj_t)&mp_stream_unbuffered_readline_obj},
    /// \method readinto(buf[, nbytes])
    { MP_OBJ_NEW_QSTR(MP_QSTR_readinto), (mp_obj_t)&mp_stream_readinto_obj },
    /// \method write(buf)
    { MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&mp_stream_write_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_writechar), (mp_obj_t)&pyb_uart_writechar_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readchar), (mp_obj_t)&pyb_uart_readchar_obj },

#if 0
    { MP_OBJ_NEW_QSTR(MP_QSTR_sendbreak), (mp_obj_t)&pyb_uart_sendbreak_obj },

    // class constants
    { MP_OBJ_NEW_QSTR(MP_QSTR_RTS), MP_OBJ_NEW_SMALL_INT(UART_HWCONTROL_RTS) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CTS), MP_OBJ_NEW_SMALL_INT(UART_HWCONTROL_CTS) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    byte *buf = buf_in;

    // check that size is a multiple of character width
    if (size & self->char_width) {
        *errcode = EIO;
        return MP_STREAM_ERROR;
    }

    // convert byte size to char size
    size >>= self->char_width;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // wait for first char to become available
    if (!uart_rx_wait(self, self->timeout)) {
        // return EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    byte *orig_buf = buf;
    for (;;) {
        int data = uart_rx_char(self);
        *buf++ = data;
        if (--size == 0 || !uart_rx_wait(self, self->timeout_char)) {
            // return number of bytes read
            return buf - orig_buf;
        }
    }
    return 0;
}

STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    const byte *buf = buf_in;

    // check that size is a multiple of character width
    if (size & self->char_width) {
        *errcode = EIO;
        return MP_STREAM_ERROR;
    }

    // wait to be able to write the first character
    if (!uart_tx_wait(self, self->timeout)) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    // write the data
    HAL_StatusTypeDef status = uart_tx_data(self, (uint8_t*)buf, size >> self->char_width);

    if (status == HAL_OK) {
        // return number of bytes written
        return size;
    } else {
        *errcode = mp_hal_status_to_errno_table[status];
        return MP_STREAM_ERROR;
    }
}

STATIC mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_IOCTL_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_IOCTL_POLL_RD) && uart_rx_any(self)) {
            ret |= MP_IOCTL_POLL_RD;
        }
        if ((flags & MP_IOCTL_POLL_WR) && Chip_UART_CheckBusy(self->uart.Instance)) {
            ret |= MP_IOCTL_POLL_WR;
        }
    } else {
        *errcode = EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = pyb_uart_print,
    .make_new = pyb_uart_make_new,
    .getiter = mp_identity,
    .iternext = mp_stream_unbuffered_iter,
    .stream_p = &uart_stream_p,
    .locals_dict = (mp_obj_t)&pyb_uart_locals_dict,
};
