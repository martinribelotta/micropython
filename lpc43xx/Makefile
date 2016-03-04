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
INC += -I../lib/mp-readline
INC += -I../lib/timeutils
INC += -Imods
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

MOD_SRC_C = $(addprefix mods/,\
	modpyb.c \
	modpybrtc.c \
	modpybuart.c \
	modutime.c \
	)
	#modpybpin.c \
	modpybextint.c \
	modpybadc.c \
	modpybdac.c \
	modpybeeprom.c \
	modpybspi.c \
	modpybpwm.c \
	modpybswitch.c \
	modpybled.c \
	modpybkeyboard.c \
	modpyblcd44780u.c \
	bootSys.c  \
	#

STM_SRC_C = $(addprefix stmhal/,\
	bufhelper.c \
	builtin_open.c \
	import.c \
	input.c \
	irq.c \
	lexerfatfs.c \
	moduselect.c \
	moduos.c \
	pybstdio.c \
	)
	
LIB_C_SRC = $(addprefix lib/,\
	libc/string0.c \
	libm/math.c \
	libm/asinfacosf.c \
	libm/atanf.c \
	libm/atan2f.c \
	libm/fmodf.c \
	libm/roundf.c \
	libm/log1pf.c \
	libm/acoshf.c \
	libm/asinhf.c \
	libm/atanhf.c \
	libm/kf_rem_pio2.c \
	libm/kf_sin.c \
	libm/kf_cos.c \
	libm/kf_tan.c \
	libm/ef_rem_pio2.c \
	libm/erf_lgamma.c \
	libm/sf_sin.c \
	libm/sf_cos.c \
	libm/sf_tan.c \
	libm/sf_frexp.c \
	libm/sf_modf.c \
	libm/sf_ldexp.c \
	libm/sf_erf.c \
	libm/wf_lgamma.c \
	libm/wf_tgamma.c \
	fatfs/ff.c \
	fatfs/option/ccsbcs.c \
	mp-readline/readline.c \
	netutils/netutils.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	utils/printf.c \
	)

#$(addprefix lib/,\
	fatfs/ff.c \
	fatfs/option/ccsbcs.c \
	mp-readline/readline.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	)

SRC_C = \
	main.c \
	help.c \
	fatfs_port.c \
	lpc43_it.c \
	lpc43_flash.c \
	lpc43_storage.c \
	lpc43xx_mphal.c \
	$(LIB_SRC_C) \
	$(STM_SRC_C) \
	$(MOD_SRC_C) \
	$(LIB_C_SRC) \
	$(wildcard lpc_chip_43xx/src/*.c) \
	$(wildcard board/$(BOARD)/src/*.c) \
	$(wildcard board/common/src/*.c)

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
