/*
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include <retarget.h>
#include "board.h"
#include "string.h"


/** @ingroup BOARD_BAMBINO
 * @{
 */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System configuration variables used by chip driver */
const uint32_t ExtRateIn = 0;
const uint32_t OscRateIn = 12000000;

void Board_Systick(void) {
	// TODO
}

void Board_UART_Init(LPC_USART_T *pUART)
{
	if (pUART == LPC_USART0) {
		Chip_SCU_PinMuxSet(0x6, 4, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC2));					/* P6.5 : UART0_TXD */
		Chip_SCU_PinMuxSet(0x6, 5, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2));/* P6.4 : UART0_RXD */
	}
	else if (pUART == LPC_UART1) {
		Chip_SCU_PinMuxSet(0x5, 6, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC4));				/* P5.6 : UART1_TXD */
		Chip_SCU_PinMuxSet(0x1, 14, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC1));	/* P1.14 : UART1_RX */
	} else if (pUART == LPC_USART2) {
		Chip_SCU_PinMuxSet(0x2, 10, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC2));				/* P2.10 : UART2_TXD */
		Chip_SCU_PinMuxSet(0x2, 11, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2));	/* P2.11 : UART2_RX */
	}
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
#if defined(DEBUG_UART)
	Board_UART_Init(DEBUG_UART);

	Chip_UART_Init(DEBUG_UART);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_ConfigData(DEBUG_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* Enable UART Transmit */
	Chip_UART_TXEnable(DEBUG_UART);
#endif
}

/* Sends a character on the UART */
void Board_UARTPutChar(char ch)
{
#if defined(DEBUG_UART)
	/* Wait for space in FIFO */
	while ((Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_THRE) == 0) {}
	Chip_UART_SendByte(DEBUG_UART, (uint8_t) ch);
#endif
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void)
{
#if defined(DEBUG_UART)
	if (Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(DEBUG_UART);
	}
#endif
	return EOF;
}

/* Outputs a string on the debug UART */
void Board_UARTPutSTR(char *str)
{
#if defined(DEBUG_UART)
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
#endif
}

static void Board_LED_Init()
{
	/* Set LEDs as output */
    Chip_SCU_PinMuxSet(LED1_PIN_PORT_NUM, LED1_PIN_BIT_NUM, (SCU_PINIO_FAST | LED1_MODE_FUNC));
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LED1_GPIO_PORT_NUM, LED1_GPIO_BIT_NUM);
    Chip_SCU_PinMuxSet(LED2_PIN_PORT_NUM, LED2_PIN_BIT_NUM, (SCU_PINIO_FAST | LED2_MODE_FUNC));
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LED2_GPIO_PORT_NUM, LED2_GPIO_BIT_NUM);

    /* Set initial states to off (true to disable) */
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, LED1_GPIO_PORT_NUM, LED1_GPIO_BIT_NUM, (bool) true);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, LED2_GPIO_PORT_NUM, LED2_GPIO_BIT_NUM, (bool) true);
}

void Board_LED_Set(uint8_t LEDNumber, bool On)
{
	if (LEDNumber == 0) {
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, LED1_GPIO_PORT_NUM, LED1_GPIO_BIT_NUM, (bool) !On);
	}
	else if (LEDNumber == 1) {
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, LED2_GPIO_PORT_NUM, LED2_GPIO_BIT_NUM, (bool) !On);
	}
}

bool Board_LED_Test(uint8_t LEDNumber)
{
	if (LEDNumber == 0) {
		return (bool) !Chip_GPIO_GetPinState(LPC_GPIO_PORT, LED1_GPIO_PORT_NUM, LED1_GPIO_BIT_NUM);
	}
	else if (LEDNumber == 1) {
		return (bool) !Chip_GPIO_GetPinState(LPC_GPIO_PORT, LED2_GPIO_PORT_NUM, LED2_GPIO_BIT_NUM);
	}

	return false;
}

void Board_LED_Toggle(uint8_t LEDNumber)
{
	Board_LED_Set(LEDNumber, !Board_LED_Test(LEDNumber));
}

void Board_Buttons_Init(void)
{
    Chip_SCU_PinMuxSet(BUTTONS_BUTTON1_PIN_PORT_NUM, BUTTONS_BUTTON1_PIN_BIT_NUM,
            (SCU_MODE_PULLUP | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | BUTTONS_BUTTON1_MODE_FUNC));
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, BUTTONS_BUTTON1_GPIO_PORT_NUM, (1 << BUTTONS_BUTTON1_GPIO_BIT_NUM));  // input
}

uint32_t Buttons_GetStatus(void)
{
	uint8_t ret = NO_BUTTON_PRESSED;
	if (Chip_GPIO_GetPinState(LPC_GPIO_PORT, BUTTONS_BUTTON1_GPIO_PORT_NUM, BUTTONS_BUTTON1_GPIO_BIT_NUM) == 0) {
		ret |= BUTTONS_BUTTON1;
	}
	return ret;
}

void Board_Joystick_Init(void)
{}

uint8_t Joystick_GetStatus(void)
{
	return NO_BUTTON_PRESSED;
}

/* Returns the MAC address assigned to this board */
void Board_ENET_GetMacADDR(uint8_t *mcaddr)
{
	uint8_t boardmac[] = {0x00, 0x21, 0xA3, 0x12, 0x34, 0x56};

	memcpy(mcaddr, boardmac, 6);
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
	/* Sets up DEBUG UART */
	DEBUGINIT();

	/* Updates SystemCoreClock global var with current clock speed */
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO_PORT);

	/* Setup GPIOs for USB demos */
	Chip_SCU_PinMuxSet(0x2, 6, (SCU_MODE_PULLUP | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC4));			/* P2_6 USB1_PWR_EN, USB1 VBus function */
	Chip_SCU_PinMuxSet(0x2, 5, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2));	/* P2_5 USB1_VBUS, MUST CONFIGURE THIS SIGNAL FOR USB1 NORMAL OPERATION */
	Chip_SCU_PinMuxSet(0x1, 7, (SCU_MODE_PULLUP | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC4));			/* P1_7 USB0_PWR_EN, USB0 VBus function Xplorer */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 6);							/* GPIO5[6] = USB1_PWR_EN */
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, 5, 6, true);							/* GPIO5[6] output high */

	/* Initialize LEDs */
	Board_LED_Init();
#if defined(USE_RMII)
	Chip_ENET_RMIIEnable(LPC_ETHERNET);
#else
	Chip_ENET_MIIEnable(LPC_ETHERNET);
#endif
}

void Board_I2C_Init(I2C_ID_T id)
{
	if (id == I2C1) {
		/* Configure pin function for I2C1*/
		Chip_SCU_PinMuxSet(0x2, 3, (SCU_MODE_ZIF_DIS | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC1));		/* P2.3 : I2C1_SDA */
		Chip_SCU_PinMuxSet(0x2, 4, (SCU_MODE_ZIF_DIS | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC1));		/* P2.4 : I2C1_SCL */
	}
	else {
		Chip_SCU_I2C0PinConfig(I2C0_STANDARD_FAST_MODE);
	}
}

void Board_SDMMC_Init(void)
{
	Chip_SCU_PinMuxSet(0x1, 9, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.9 connected to SDIO_D0 */
	Chip_SCU_PinMuxSet(0x1, 10, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.10 connected to SDIO_D1 */
	Chip_SCU_PinMuxSet(0x1, 11, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.11 connected to SDIO_D2 */
	Chip_SCU_PinMuxSet(0x1, 12, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.12 connected to SDIO_D3 */
	Chip_SCU_PinMuxSet(0x1, 13, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.13 connected to SD_CD */
	Chip_SCU_ClockPinMuxSet(2, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC4));	/* CLK2 connected to SDIO_CLK */
	Chip_SCU_PinMuxSet(0x1, 6, (SCU_PINIO_FAST | SCU_MODE_FUNC7));	/* P1.6 connected to SDIO_CMD */
}

void Board_SSP_Init(LPC_SSP_T *pSSP)
{
	if (pSSP == LPC_SSP1) {
		/* Set up clock and power for SSP1 module */
		/* Configure SSP1 pins*/
		/* SCLK comes out pin CLK0 */
		Chip_SCU_ClockPinMuxSet(0, (SCU_PINIO_FAST | SCU_MODE_FUNC6));		/* CLK0 connected to CLK	SCU_MODE_FUNC6=SSP1 CLK1  */
		//Chip_SCU_PinMuxSet(0xF, 4, (SCU_PINIO_FAST | SCU_MODE_FUNC0));			/* PF.4 connected to nCS	SCU_MODE_FUNC0=SSP1 CLK BAM200 REV A */
		Chip_SCU_PinMuxSet(0x1, 5, (SCU_PINIO_FAST | SCU_MODE_FUNC5));			/* P1.5 connected to nCS	SCU_MODE_FUNC5=SSP1 SSEL1 */
		Chip_SCU_PinMuxSet(0x1, 3, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC5));/* P1.3 connected to SO		SCU_MODE_FUNC5=SSP1 MISO1 */
		Chip_SCU_PinMuxSet(0x1, 4, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC5));/* P1.4 connected to nSI	SCU_MODE_FUNC5=SSP1 MOSI1 */
	}
	else {
		return;
	}
}

#if 0
static void delay(uint32_t i) {
	while (i--) {}
}
#endif

/**
 * @}
 */
