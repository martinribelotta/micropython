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

#include "board.h"

/* The System initialization code is called prior to the application and
   initializes the board for run-time operation. Board initialization
   includes clock setup and default pin muxing configuration. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Structure for initial base clock states */
struct CLK_BASE_STATES {
	CHIP_CGU_BASE_CLK_T clk;	/* Base clock */
	CHIP_CGU_CLKIN_T clkin;	/* Base clock source, see UM for allowable souorces per base clock */
	bool autoblock_enab;/* Set to true to enable autoblocking on frequency change */
	bool powerdn;		/* Set to true if the base clock is initially powered down */
};

/* Initial base clock states are mostly on */
STATIC const struct CLK_BASE_STATES InitClkStates[] = {
	{CLK_BASE_PHY_TX, CLKIN_ENET_TX, true, false},
#if defined(USE_RMII)
	{CLK_BASE_PHY_RX, CLKIN_ENET_TX, true, false},
#else
	{CLK_BASE_PHY_RX, CLKIN_ENET_RX, true, false},
#endif

	/* Clocks derived from dividers */
	{CLK_BASE_LCD, CLKIN_IDIVC, true, false},
	{CLK_BASE_USB1, CLKIN_IDIVD, true, true}
};

/* SPIFI high speed pin mode setup */
STATIC const PINMUX_GRP_T spifipinmuxing[] = {
	{0x3, 3,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI CLK */
	{0x3, 4,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D3 */
	{0x3, 5,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D2 */
	{0x3, 6,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D1 */
	{0x3, 7,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D0 */
	{0x3, 8,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)}	/* SPIFI CS/SSEL */
};

STATIC const PINMUX_GRP_T pinmuxing[] = {
#if defined(USE_RMII)
	/* RMII pin group */
	{0x1, 19,
	 (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC0)},
	{0x0, 1,  (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC6)},
	{0x1, 18, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x1, 20, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x1, 17,
	 (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0xC, 1,  (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x1, 16,
	 (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC7)},
	{0x1, 15,
	 (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x0, 0,
	 (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2)},
#else
	/* MII pin group */
	{0x1, 19, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC0)},
	{0x0, 1,  (SCU_MODE_INACT | SCU_MODE_FUNC6)},
	{0x1, 18, (SCU_MODE_INACT | SCU_MODE_FUNC3)},
	{0x1, 20, (SCU_MODE_INACT | SCU_MODE_FUNC3)},
	{0x1, 17, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC3)},
	{0xC, 1,  (SCU_MODE_INACT | SCU_MODE_FUNC3)},
	{0x1, 16, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC7)},
	{0x1, 15, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC3)},
	{0x0, 0,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC2)},
	{0x9, 4,  (SCU_MODE_INACT | SCU_MODE_FUNC5)},
	{0x9, 5,  (SCU_MODE_INACT | SCU_MODE_FUNC5)},
	{0xC, 0,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC3)},
	{0x9, 0,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC5)},
	{0x9, 1,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC5)},
	{0x9, 6,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC5)},
	{0x9, 3,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC5)},
	{0x9, 2,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC5)},
	{0xC, 8,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC4)},
#endif
	/* External data lines D0 .. D15 */
	{0x1, 7,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 8,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 9,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 10,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 11,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 12,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 13,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 14,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x5, 4,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 5,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 6,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 7,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 0,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 1,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 2,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x5, 3,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	/* Address lines A0 .. A23 */
	{0x2, 9,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 10,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 11,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 12,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 13,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 0,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x1, 1,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x1, 2,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x2, 8,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 7,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x2, 6,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x2, 2,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x2, 1,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x2, 0,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x6, 8,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC1)},
	{0x6, 7,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC1)},
	{0xD, 16,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0xD, 15,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0xE, 0,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xE, 1,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xE, 2,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xE, 3,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xE, 4,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xA, 4,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	/* EMC control signals */
	{0x1, 4,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 6,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC1)},
	{0xD, 13,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0xD, 10,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0x6, 9,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 6,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 4,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 5,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 11,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 12,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x6, 10,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0xD, 0,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2)},
	{0xE, 13,
	 (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC3)},
	{0x1, 3,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	{0x1, 4,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	{0x6, 6,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	{0x1, 5,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	/* LCD interface, 24bpp */
	{0x7, 7, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x4, 7, ( SCU_MODE_PULLUP | SCU_MODE_FUNC0)},
	{0x4, 5, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x4, 6, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x7, 6, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x4, 1, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x4, 4, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x4, 2, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x8, 7, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x8, 6, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x8, 5, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x8, 4, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x7, 5, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x4, 8, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x4, 10, (SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x4, 9, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x8, 3, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0xB, 6, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0xB, 5, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0xB, 4, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x7, 4, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x7, 2, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x7, 1, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0xB, 3, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0xB, 2, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0xB, 1, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0xB, 0, ( SCU_MODE_PULLUP | SCU_MODE_FUNC2)},
	{0x7, 0, ( SCU_MODE_PULLUP | SCU_MODE_FUNC3)},
	{0x4, 4, ( SCU_MODE_PULLUP | SCU_MODE_FUNC0)},
	{0x7, 3, ( SCU_MODE_PULLUP | SCU_MODE_FUNC0)},
	{0x4, 1, ( SCU_MODE_PULLUP | SCU_MODE_FUNC0)},
	/* Board LEDs */
	{0x8, 1, ( SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0)},
	{0xE, 6, ( SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC4)},	/* GPIO7.6, green */
	{0xE, 8, ( SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC4)},	/* GPIO7.8, blue */
	{0xE, 5, ( SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC4)},	/* GPIO7.5, red */
	/* Board ADC */
	{0xF, 9, ( SCU_MODE_INACT | SCU_MODE_FUNC7)},
	/*  I2S  */
	{0x3, 0,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 0,  (SCU_PINIO_FAST | SCU_MODE_FUNC4)},
	{0x7, 2,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 2,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	{0x7, 1,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 1,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	/*  CCAN  */
	{0x3, 2,   (SCU_MODE_INACT | SCU_MODE_FUNC2)},	/* PE.3 CAN TD1 | SCU_MODE_FUNC1) */
	{0x3, 1,   (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC2)},	/* PE.2	CAN RD1 | SCU_MODE_FUNC1) */
};

/* Pin clock mux values, re-used structure, value in first index is meaningless */
STATIC const PINMUX_GRP_T pinclockmuxing[] = {
	{0, 0,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC0)},
	{0, 1,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC0)},
	{0, 2,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC0)},
	{0, 3,  (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC0)},
};


/* EMC clock delay */
#define CLK0_DELAY 7

/* Hitex SDRAM timing and chip Config */
STATIC const IP_EMC_DYN_CONFIG_T IS42S16400_config = {
	EMC_NANOSECOND(64000000 / 4096),	/* Row refresh time */
	0x01,	/* Command Delayed */
	EMC_NANOSECOND(20),
	EMC_NANOSECOND(60),
	EMC_NANOSECOND(63),
	EMC_CLOCK(0x05),
	EMC_CLOCK(0x05),
	EMC_CLOCK(0x04),
	EMC_NANOSECOND(63),
	EMC_NANOSECOND(63),
	EMC_NANOSECOND(63),
	EMC_NANOSECOND(14),
	EMC_CLOCK(0x02),
	{
		{
			EMC_ADDRESS_DYCS0,	/* Hitex Board uses DYCS0 for SDRAM */
			3,	/* RAS */

			EMC_DYN_MODE_WBMODE_PROGRAMMED |
			EMC_DYN_MODE_OPMODE_STANDARD |
			EMC_DYN_MODE_CAS_3 |
			EMC_DYN_MODE_BURST_TYPE_SEQUENTIAL |
			EMC_DYN_MODE_BURST_LEN_8,

			EMC_DYN_CONFIG_DATA_BUS_16 |
			EMC_DYN_CONFIG_LPSDRAM |
			EMC_DYN_CONFIG_4Mx16_4BANKS_12ROWS_8COLS |
			EMC_DYN_CONFIG_MD_SDRAM
		},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	}
};

/* Hitex Static RAM timing and chip Config */
STATIC const IP_EMC_STATIC_CONFIG_T IS62WV25616_config = {
	2,
	EMC_STATIC_CONFIG_MEM_WIDTH_16 |
	EMC_STATIC_CONFIG_CS_POL_ACTIVE_LOW |
	EMC_STATIC_CONFIG_BLS_HIGH /* |
							      EMC_CONFIG_BUFFER_ENABLE*/,

	EMC_NANOSECOND(0),
	EMC_NANOSECOND(30),
	EMC_NANOSECOND(90),
	EMC_NANOSECOND(55),
	EMC_NANOSECOND(55),
	EMC_NANOSECOND(55)
};

/* Hitex NorFlash timing and chip Config */
STATIC const IP_EMC_STATIC_CONFIG_T SST39VF320_config = {
	0,
	EMC_STATIC_CONFIG_MEM_WIDTH_16 |
	EMC_STATIC_CONFIG_CS_POL_ACTIVE_LOW |
	EMC_STATIC_CONFIG_BLS_HIGH /* |
							      EMC_CONFIG_BUFFER_ENABLE*/,

	EMC_NANOSECOND(0),
	EMC_NANOSECOND(35),
	EMC_NANOSECOND(70),
	EMC_NANOSECOND(70),
	EMC_NANOSECOND(40),
	EMC_CLOCK(4)
};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
	int i;

	/* Setup system level pin muxing */
	Chip_SCU_SetPinMuxing(pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));

	/* Clock pins only, group field not used */
	for (i = 0; i < (sizeof(pinclockmuxing) / sizeof(pinclockmuxing[0])); i++) {
		Chip_SCU_ClockPinMuxSet(pinclockmuxing[i].pinnum, pinclockmuxing[i].modefunc);
	}

	/* SPIFI pin setup is done prior to setting up system clocking */
	Chip_SCU_SetPinMuxing(spifipinmuxing, sizeof(spifipinmuxing) / sizeof(PINMUX_GRP_T));
}

/* Setup external memories */
void Board_SetupExtMemory(void)
{
	/* Setup EMC Delays */
	/* Move all clock delays together */
	LPC_SCU->EMCDELAYCLK = ((CLK0_DELAY) | (CLK0_DELAY << 4) | (CLK0_DELAY << 8) | (CLK0_DELAY << 12));

	/* Setup EMC Clock Divider for divide by 2 - this is done in both the CCU (clocking)
	   and CREG. For frequencies over 120MHz, a divider of 2 must be used. For frequencies
	   less than 120MHz, a divider of 1 or 2 is ok. */
	Chip_Clock_EnableOpts(CLK_MX_EMC_DIV, true, true, 2);
	LPC_CREG->CREG6 |= (1 << 16);

	/* Enable EMC clock */
	Chip_Clock_Enable(CLK_MX_EMC);

	/* Init EMC Controller -Enable-LE mode */
	Chip_EMC_Init(1, 0, 0);
	/* Init EMC Dynamic Controller */
	Chip_EMC_Dynamic_Init((IP_EMC_DYN_CONFIG_T *) &IS42S16400_config);
	/* Init EMC Static Controller CS2 */
	Chip_EMC_Static_Init((IP_EMC_STATIC_CONFIG_T *) &IS62WV25616_config);
	/* Init EMC Static Controller CS0 */
	Chip_EMC_Static_Init((IP_EMC_STATIC_CONFIG_T *) &SST39VF320_config);

	/* Enable Buffer for External Flash */
	LPC_EMC->STATICCONFIG0 |= 1 << 19;
}

/* Set up and initialize clocking prior to call to main */
void Board_SetupClocking(void)
{
	int i;

	Chip_Clock_SetBaseClock(CLK_BASE_SPIFI, CLKIN_IRC, true, false);	// change SPIFI to IRC during clock programming
	LPC_SPIFI->CTRL |= SPIFI_CTRL_FBCLK(1);								// and set FBCLK in SPIFI controller

	Chip_SetupCoreClock(CLKIN_CRYSTAL, MAX_CLOCK_FREQ, true);

	/* Setup system base clocks and initial states. This won't enable and
	   disable individual clocks, but sets up the base clock sources for
	   each individual peripheral clock. */
	for (i = 0; i < (sizeof(InitClkStates) / sizeof(InitClkStates[0])); i++) {
		Chip_Clock_SetBaseClock(InitClkStates[i].clk, InitClkStates[i].clkin,
								InitClkStates[i].autoblock_enab, InitClkStates[i].powerdn);
	}

	/* Reset and enable 32Khz oscillator */
	LPC_CREG->CREG0 &= ~((1 << 3) | (1 << 2));
	LPC_CREG->CREG0 |= (1 << 1) | (1 << 0);

	/* Setup a divider E for main PLL clock switch SPIFI clock to that divider.
	   Divide rate is based on CPU speed and speed of SPI FLASH part. */
#if (MAX_CLOCK_FREQ > 180000000)
	Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, 5);
#else
	Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, 4);
#endif
	Chip_Clock_SetBaseClock(CLK_BASE_SPIFI, CLKIN_IDIVE, true, false);

	/* Attach main PLL clock to divider C with a divider of 2 */
	Chip_Clock_SetDivider(CLK_IDIV_C, CLKIN_MAINPLL, 2);
}

/* Set up and initialize hardware prior to call to main */
void Board_SystemInit(void)
{
	/* Setup system clocking and memory. This is done early to allow the
	   application and tools to clear memory and use scatter loading to
	   external memory. */
	Board_SetupMuxing();
	Board_SetupClocking();
	Board_SetupExtMemory();
}






