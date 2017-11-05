#include "board.h"

#include <stdint.h>

/* System configuration variables used by chip driver */
const uint32_t ExtRateIn = 0;
const uint32_t OscRateIn = 12000000;

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
	/* RMII pin group */
	{0x1, 15, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x0, 0, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2)},
	{0x1, 16, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC7)},
	{0x0, 1,  (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC6)},
	{0x1, 19, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC0)},
	{0x1, 18, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x1, 20, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x1, 17, (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC3)},
	{0x2, 0,  (SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_INACT | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC7)},
	/* Board LEDs */
	{0x2, 11, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0)},
	{0x2, 12, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0)},
	/*  I2S  */
	{0x3, 0,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 0,  (SCU_PINIO_FAST | SCU_MODE_FUNC4)},
	{0x7, 2,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 2,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
	{0x7, 1,  (SCU_PINIO_FAST | SCU_MODE_FUNC2)},
	{0x6, 1,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},
};

void Board_UART_Init(LPC_USART_T *pUART)
{
    Chip_SCU_PinMuxSet(0x2, 10, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC2));                              /* P2.10 : UART2_TXD */
    Chip_SCU_PinMuxSet(0x2, 11, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC2)); /* P2.11 : UART2_RX */
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
   Board_UART_Init(CONSOLE_UART);
   
   Chip_UART_Init(CONSOLE_UART);
   Chip_UART_SetBaudFDR(CONSOLE_UART, 115200);
   Chip_UART_ConfigData(CONSOLE_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);
   /* Enable UART Transmit */
   Chip_UART_TXEnable(CONSOLE_UART);
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
   /* Sets up DEBUG UART */
   Board_Debug_Init();

   /* Initializes GPIO */
   Chip_GPIO_Init(LPC_GPIO_PORT);
}

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
	/* Setup system level pin muxing */
	Chip_SCU_SetPinMuxing(pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));

	/* SPIFI pin setup is done prior to setting up system clocking */
	Chip_SCU_SetPinMuxing(spifipinmuxing, sizeof(spifipinmuxing) / sizeof(PINMUX_GRP_T));
}

/* Set up and initialize clocking prior to call to main */
void Board_SetupClocking(void)
{
	int i;

	Chip_SetupCoreClock(CLKIN_CRYSTAL, MAX_CLOCK_FREQ, true);

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

	/* Setup system base clocks and initial states. This won't enable and
	   disable individual clocks, but sets up the base clock sources for
	   each individual peripheral clock. */
	for (i = 0; i < (sizeof(InitClkStates) / sizeof(InitClkStates[0])); i++) {
		Chip_Clock_SetBaseClock(InitClkStates[i].clk, InitClkStates[i].clkin,
								InitClkStates[i].autoblock_enab, InitClkStates[i].powerdn);
	}
}

/* Set up and initialize hardware prior to call to main */
void Board_SystemInit(void)
{
   /* Setup system clocking and memory. This is done early to allow the
      application and tools to clear memory and use scatter loading to
      external memory. */
   fpuInit();
   Board_SetupMuxing();
   Board_SetupClocking();
}
