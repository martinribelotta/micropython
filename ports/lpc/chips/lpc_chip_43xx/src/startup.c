
// The following block of code manually resets as
// much of the peripheral set of the LPC43 as possible. This is
// done because the LPC43 does not provide a means of triggering
// a full system reset under debugger control, which can cause
// problems in certain circumstances when debugging.
//
// TODO: Perhaps it's cleaner to have RGU/NVIC drivers
// and put each reset/clear routines there.
void Reset_Chip(void)
{
    // Disable interrupts
    __asm volatile ("cpsid i");

    unsigned int *RESET_CONTROL = (unsigned int *) 0x40053100;
    // LPC_RGU->RESET_CTRL0 @ 0x40053100
    // LPC_RGU->RESET_CTRL1 @ 0x40053104

    // Write to LPC_RGU->RESET_CTRL0
    *(RESET_CONTROL + 0) = 0x10DF1000;
    // GPIO_RST|AES_RST|ETHERNET_RST|SDIO_RST|DMA_RST|
    // USB1_RST|USB0_RST|LCD_RST|M0_SUB_RST

    // Write to LPC_RGU->RESET_CTRL1
    *(RESET_CONTROL + 1) = 0x01DFF7FF;
    // M0APP_RST|CAN0_RST|CAN1_RST|I2S_RST|SSP1_RST|SSP0_RST|
    // I2C1_RST|I2C0_RST|UART3_RST|UART1_RST|UART1_RST|UART0_RST|
    // DAC_RST|ADC1_RST|ADC0_RST|QEI_RST|MOTOCONPWM_RST|SCT_RST|
    // RITIMER_RST|TIMER3_RST|TIMER2_RST|TIMER1_RST|TIMER0_RST

    // Clear all pending interrupts in the NVIC
    volatile unsigned int *NVIC_ICPR = (unsigned int *) 0xE000E280;
    unsigned int irqpendloop;
    for (irqpendloop = 0; irqpendloop < 8; irqpendloop++) {
        *(NVIC_ICPR + irqpendloop) = 0xFFFFFFFF;
    }

    // Reenable interrupts
    __asm volatile ("cpsie i");
}
