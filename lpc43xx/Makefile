include ../py/mkenv.mk

BOARD ?= edu_ciaa

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include ../py/py.mk
LD=$(CROSS_COMPILE)gcc

CROSS_COMPILE = arm-none-eabi-

INC =  -I.
INC += -I..
INC += -I../stmhal
INC += -I../teensy
INC += -I../lib/mp-readline
INC += -I../lib/timeutils
INC += -Iboard/$(BOARD)/inc/ -Ilpc_chip_43xx/inc/ -Iboard/common/inc/
INC += -I$(BUILD)

COPT += -DCORE_M4 -D__USE_LPCOPEN -D__USE_NEWLIB

ARCH_FLAGS = -mthumb -mtune=cortex-m4 -mabi=aapcs-linux -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard 
CFLAGS_CORTEX_M4 = $(ARCH_FLAGS) -fsingle-precision-constant -Wdouble-promotion -fshort-enums
CFLAGS = $(INC) -Wall -Wpointer-arith -Werror -ansi -std=gnu99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
endif

OOCD_CFG = board/$(BOARD)/openocd.cfg

LDFLAGS = $(ARCH_FLAGS) -nostdlib -T lpc4337.ld -Wl,-Map=$@.map -Wl,--cref
LIBS =

SRC_C = \
	main.c \
	sysinit.c \
	system.c \
	cr_startup_lpc43xx.c \
	lpc43xx_mphal.c \
	help.c \
	bootSys.c  \
	lpc43_diskio.c \
	lpc43_flash.c \
	lpc43_storage.c \
	ffconf.c \
	moduos.c \
	lib/fatfs/ff.c \
	lib/fatfs/option/ccsbcs.c \
	lib/mp-readline/readline.c \
	lib/timeutils/timeutils.c \
	lib/utils/pyexec.c \
	stmhal/file.c \
	stmhal/import.c \
	stmhal/input.c \
	stmhal/lexerfatfs.c \
	stmhal/pybstdio.c \
	stmhal/bufhelper.c \
	$(wildcard lpc_chip_43xx/src/*.c) \
	$(wildcard board/$(BOARD)/src/*.c) \
	$(wildcard board/common/src/*.c) \
	modpyb.c \
	#modpybpin.c \
	modpybuart.c \
	modpybextint.c \
	modpybadc.c \
	modpybdac.c \
	modpybeeprom.c \
	modpybtimer.c \
	modpybspi.c \
	modpybrtc.c \
	modpybpwm.c \
	modpybswitch.c \
	modpybled.c \
	modpybkeyboard.c \
	modpyblcd44780u.c \
	#

-include board/$(BOARD)/board.mk

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

all: $(BUILD)/firmware.elf

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) -A -x $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(CROSS_COMPILE)objcopy -v -O binary $< $@

PHONY: debug openocd download erase

download: $(BUILD)/firmware.bin
	@openocd -f $(OOCD_CFG) \
		-c "init" \
		-c "halt 0" \
		-c "flash write_image erase unlock $^ 0x1A000000 bin" \
		-c "reset run" \
		-c "shutdown"

erase:
	@openocd -f $(OOCD_CFG) \
		-c "init" -c "halt 0" -c "flash erase_sector 0 0 last" -c "shutdown"

openocd:
	@openocd -f $(OOCD_CFG)

debug: $(BUILD)/firmware.elf
	$(CROSS_COMPILE)gdb $(BUILD)/firmware.elf -ex "target remote :3333" -ex "mon reset halt" -ex "load"

include ../py/mkrules.mk
