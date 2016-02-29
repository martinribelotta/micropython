#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"

#include "lib/utils/pyexec.h"
#include "lib/fatfs/ff.h"
#include "extmod/fsusermount.h"

#include "modpyb.h"

#include "chip.h"
#include "board.h"
#include "lpc43_storage.h"

// maximum heap for device with 32k RAM o RAM2 (40K block)
static char *stack_top;
//static __DATA(RAM2) char heap[32*1024];

#define HEAP_START ((uint8_t*) 0x20000000)
#define HEAP_SIZE  (64*1024)
#define HEAP_END   (HEAP_START + HEAP_SIZE)

// static FATFS fatfs0;

fs_user_mount_t fs_user_mount_flash;

static const char fresh_main_py[] =
"# main.py -- put your code here!\r\n"
;

static const char fresh_readme_txt[] =
"print('Welcome to Micropython on EDU-CIAA-NXP');\r\n"
"import os\r\n"
"import sys\r\n"
"import pyb\r\n"
"import gc\r\n"
;
#if 1
void init_flash_fs(uint reset_mode) {
    // init the vfs object
    fs_user_mount_t *vfs = &fs_user_mount_flash;
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->flags = 0;
    pyb_flash_init_vfs(vfs);

    // put the flash device in slot 0 (it will be unused at this point)
    MP_STATE_PORT(fs_user_mount)[0] = vfs;

    // try to mount the flash
    FRESULT res = f_mount(&vfs->fatfs, vfs->str, 1);

    if (reset_mode == 3 || res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one

        res = f_mkfs("/flash", 0, 0);
        if (res == FR_OK) {
            // success creating fresh LFS
        } else {
            printf("PYB: can't create flash filesystem\n");
            MP_STATE_PORT(fs_user_mount)[0] = NULL;
            return;
        }

        // set label
        f_setlabel("/flash/pybflash");

        // create empty main.py
        FIL fp;
        f_open(&fp, "/flash/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);
#if 0
        // create .inf driver file
        f_open(&fp, "/flash/pybcdc.inf", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_pybcdc_inf, sizeof(fresh_pybcdc_inf) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
#endif

        // create readme file
        f_open(&fp, "/flash/README.txt", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_readme_txt, sizeof(fresh_readme_txt) - 1 /* don't count null terminator */, &n);
        f_close(&fp);

    } else if (res == FR_OK) {
        // mount sucessful
    } else {
        printf("PYB: can't mount flash\n");
        MP_STATE_PORT(fs_user_mount)[0] = NULL;
        return;
    }

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    f_chdrive("/flash");

    // Make sure we have a /flash/boot.py.  Create it if needed.
    FILINFO fno;
#if _USE_LFN
    fno.lfname = NULL;
    fno.lfsize = 0;
#endif
    res = f_stat("/flash/boot.py", &fno);
    if (res == FR_OK) {
        if (fno.fattrib & AM_DIR) {
            // exists as a directory
            // TODO handle this case
            // see http://elm-chan.org/fsw/ff/img/app2.c for a "rm -rf" implementation
        } else {
            // exists as a file, good!
        }
    } else {
#if 0
        // doesn't exist, create fresh file

        // LED on to indicate creation of boot.py
        // led_state(PYB_LED_R2, 1);
        uint32_t start_tick = HAL_GetTick();

        FIL fp;
        f_open(&fp, "/flash/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);

        // keep LED on for at least 200ms
        sys_tick_wait_at_least(start_tick, 200);
        // led_state(PYB_LED_R2, 0);
#endif
    }
}
#else
void init_flash_fs(uint reset_mode) {
    // try to mount the flash
    FRESULT res = f_mount(&fatfs0, "/flash", 1);

    if (reset_mode == 3 || res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one

        res = f_mkfs("/flash", 0, 0);
        if (res == FR_OK) {
            // success creating fresh LFS
        } else {
            __BKPT(0);
        }

        // set label
        f_setlabel("/flash/pybflash");

        // create empty main.py
        FIL fp;
        f_open(&fp, "/flash/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);

        // keep LED on for at least 200ms
        mp_hal_milli_delay(200);
    } else if (res == FR_OK) {
        // mount sucessful
    } else {
        __BKPT(0);
    }

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    f_chdrive("/flash");
}
#endif

int main(int argc, char **argv) {
    int stack_dummy;
soft_reset:
    stack_top = (char*)&stack_dummy;
    // memset(heap, 0, sizeof(heap));
    // gc_init(heap, heap + sizeof(heap));
    memset(HEAP_START, 0, HEAP_SIZE);
    gc_init(HEAP_START, HEAP_END);

    mp_init();
    mp_hal_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    mp_obj_list_init(mp_sys_argv, 0);

    readline_init0();
    //pin_init0();
    //extint_init0();
    //timer_init0();
    uart_init0();

    // Define MICROPY_HW_UART_REPL to be PYB_UART_6 and define
    // MICROPY_HW_UART_REPL_BAUD in your mpconfigboard.h file if you want a
    // REPL on a hardware UART as well as on USB VCP
#if defined(MICROPY_HW_UART_REPL)
    {
        mp_obj_t args[2] = {
            MP_OBJ_NEW_SMALL_INT(MICROPY_HW_UART_REPL),
            MP_OBJ_NEW_SMALL_INT(MICROPY_HW_UART_REPL_BAUD),
        };
        MP_STATE_PORT(pyb_stdio_uart) = pyb_uart_type.make_new((mp_obj_t)&pyb_uart_type, MP_ARRAY_SIZE(args), 0, args);
    }
#else
    MP_STATE_PORT(pyb_stdio_uart) = NULL;
#endif

    init_flash_fs(0);

    if (!pyexec_file("/flash/Main.py")) {
    	mp_hal_stdout_tx_str("\nFATAL ERROR:\n");
    }

    // Main script is finished, so now go into REPL mode.
    // The REPL mode can change, or it can request a soft reset.
    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    printf("soft reboot\n");
    goto soft_reset;
    return 0;
}


void gc_collect(void) {
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    //gc_dump_info();
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught exception %p\n", val);
    __fatal_error("");
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

/*
<<<<<<< HEAD

#if 0
int _lseek() {return 0;}
int _read() {return 0;}
int _write() {return 0;}
int _close() {return 0;}
void _exit(int x) {for(;;){}}
int _sbrk() {return 0;}
int _kill() {return 0;}
int _getpid() {return 0;}
int _fstat() {return 0;}
int _isatty() {return 0;}
#endif

#if 0
void *malloc(size_t n) {return NULL;}
void *calloc(size_t nmemb, size_t size) {return NULL;}
void *realloc(void *ptr, size_t size) {return NULL;}
void free(void *p) {}
int printf(const char *m, ...) {return 0;}
void *memcpy(void *dest, const void *src, size_t n) {return NULL;}
int memcmp(const void *s1, const void *s2, size_t n) {return 0;}
void *memmove(void *dest, const void *src, size_t n) {return NULL;}
void *memset(void *s, int c, size_t n) {return NULL;}
int strcmp(const char *s1, const char* s2) {return 0;}
int strncmp(const char *s1, const char* s2, size_t n) {return 0;}
size_t strlen(const char *s) {return 0;}
char *strcat(char *dest, const char *src) {return NULL;}
char *strchr(const char *dest, int c) {return NULL;}
#include <stdarg.h>
int vprintf(const char *format, va_list ap) {return 0;}
int vsnprintf(char *str,  size_t  size,  const  char  *format, va_list ap) {return 0;}

#undef putchar
int putchar(int c) {return 0;}
int puts(const char *s) {return 0;}

void _start(void) {main(0, NULL);}
#endif



=======
>>>>>>> master
*/

