#include <chip.h>
#include <mphalport.h>


void UART0_IRQHandler() {
	uart_irq_handler(0);
}

void UART1_IRQHandler() {
	uart_irq_handler(1);
}

void UART2_IRQHandler() {
	uart_irq_handler(2);
}

void UART3_IRQHandler() {
	uart_irq_handler(3);
}
