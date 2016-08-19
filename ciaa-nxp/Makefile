include ../py/mkenv.mk

PORT?=ciaa_edu_4337

include port/$(PORT)/port.mk

PORT_LD = port/$(PORT)/linker.ld
OOCD_SCRIPT=port/$(PORT)/oocd.cfg

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
INC += $(foreach d, $(PORT_M), -Iport/$(PORT)/$(d)/inc)
INC += -Iport/$(PORT)/modules
INC += -Iponchos/inc
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

LDFLAGS = $(ARCH_FLAGS) -nostdlib -T $(PORT_LD) -Wl,-Map=$@.map -Wl,--cref
LIBS = $(PORT_EXTRA_LIBS)

SRC_M = \
	moduos.c \
	modutime.c \
	$(foreach m, $(PORT_MODULES), port/$(PORT)/modules/$(m))

SRC_C = \
	main.c \
	system.c \
	ciaa_help.c \
	bootSys.c  \
	diskio.c \
	ffconf.c \
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
	$(SRC_M) \
	$(foreach m, $(PORT_M), $(wildcard port/$(PORT)/$(m)/src/*.c)) \
	$(wildcard ponchos/src/*.c) \
	$(PORT_EXTRA_SRC)

SRC_S = \
#	startup_stm32f40xx.s \
	gchelper.s \

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

all: $(BUILD)/firmware.elf

# Frozen ####################################################################

OBJ += $(BUILD)/frozen-files.o
MAKE_FROZEN = tools/make-frozen.py

ifeq ($(FROZEN_DIR),)
FROZEN_DIR = frozen
endif

$(BUILD)/frozen-files.o: $(BUILD)/frozen-files.c
	$(call compile_c)

$(BUILD)/frozen-files.c: $(shell find ${FROZEN_DIR} -type f)
	@$(ECHO) "Creating $@"
	$(Q)$(PYTHON) $(MAKE_FROZEN) $(FROZEN_DIR) > $@

# Frozen ####################################################################

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) -A -x $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(CROSS_COMPILE)objcopy -v -O binary $< $@

$(BUILD)/firmware.hex: $(BUILD)/firmware.elf
	$(Q)$(CROSS_COMPILE)objcopy -O ihex $< $@;

PHONY: debug openocd download erase erase_fs

download: $(BUILD)/firmware.bin
	$(Q)openocd -f $(OOCD_SCRIPT) \
		-c "init" \
		-c "halt 0" \
		-c $(PORT_FLASH_WRITE) \
		-c "reset run" \
		-c "shutdown"

#fsdownload: $(BUILD)/fs.bin
#	@openocd -f $(OOCD_SCRIPT) \
#		-c "init" \
#		-c "halt 0" \
#		-c "flash write_image erase unlock $^ 0x1B000000 bin" \
#		-c "reset run" \
#		-c "shutdown"

erase:
	$(Q)openocd -f $(OOCD_SCRIPT) \
		-c "init" -c "halt 0" -c "flash erase_sector 0 0 last" -c "shutdown"

erase_fs:
	$(Q)openocd -f $(OOCD_SCRIPT) \
		-c "init" -c "halt 0" -c "flash erase_sector 1 0 last" -c "shutdown"

openocd:
	$(Q)openocd -f $(OOCD_SCRIPT)

debug: $(BUILD)/firmware.elf
	$(Q)$(CROSS_COMPILE)gdb $(BUILD)/firmware.elf -ex "target remote :3333" -ex "mon reset halt" -ex "load"

test: CFLAGS += -DTESTING

test: export MP_INCLUDE_TESTS = true

test: all

include ../py/mkrules.mk
