#include <stdint.h>

#include "fpu_init.h"
#include "board.h"

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;

void Reset_Handler(void) __attribute__((naked));
void Reset_Handler(void) {

    // The following block of code manually resets as
    // much of the peripheral set of the LPC43 as possible. This is
    // done because the LPC43 does not provide a means of triggering
    // a full system reset under debugger control, which can cause
    // problems in certain circumstances when debugging.
    //

    // Disable interrupts
    //__asm volatile ("cpsid i");

    Chip_RGU_TriggerResetAll();
    NVIC_ClearAll();

    // Reenable interrupts
    //__asm volatile ("cpsie i");

    #if defined(__FPU_PRESENT) && __FPU_PRESENT == 1
    fpuInit();
    #endif

    Board_SystemInit();

    // set stack pointer
    __asm volatile ("ldr sp, =_estack");
    // copy .data section from flash to RAM
    for (uint32_t *src = &_sidata, *dest = &_sdata; dest < &_edata;) {
        *dest++ = *src++;
    }
    // zero out .bss section
    for (uint32_t *dest = &_sbss; dest < &_ebss;) {
        *dest++ = 0;
    }
    // SCB->CCR: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    *((volatile uint32_t*)0xe000ed14) |= 1 << 9;

    // now that we have a basic system up and running we can call main
    int main(void);
    main();

    // we must not return
    for (;;) {
    }
}

void Default_Handler(void) {
    for (;;) {
    }
}

const uint32_t isr_vector[] __attribute__((section(".isr_vector"))) = {
    (uint32_t)&_estack,
    (uint32_t)&Reset_Handler,
    (uint32_t)&Default_Handler, // NMI_Handler
    (uint32_t)&Default_Handler, // HardFault_Handler
    (uint32_t)&Default_Handler, // MemManage_Handler
    (uint32_t)&Default_Handler, // BusFault_Handler
    (uint32_t)&Default_Handler, // UsageFault_Handler
    0,
    0,
    0,
    0,
    (uint32_t)&Default_Handler, // SVC_Handler
    (uint32_t)&Default_Handler, // DebugMon_Handler
    0,
    (uint32_t)&Default_Handler, // PendSV_Handler
    (uint32_t)&Default_Handler, // SysTick_Handler
};
