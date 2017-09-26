include ../../py/mkenv.mk

BOARD=lpc_board_ciaa_edu_4337

MPY_CROSS=$(TOP)/mpy-cross/mpy-cross

-include boards/$(BOARD)/board.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Iboards/$(BOARD)
INC += -Iboards/$(BOARD)/inc
INC += -Ichips/$(CHIP)/inc

-include chips/$(CHIP)/chip.mk

OPENOCD = openocd
OPENOCD_CONFIG = boards/$(BOARD)/openocd.cfg
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mabi=aapcs -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
LDFLAGS = -nostdlib -Lboards/$(BOARD) -Lchips/$(CHIP) -T chips/$(CHIP)/linker.lds -Map=$@.map --cref --gc-sections

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

CFLAGS += $(BOARD_CFLAGS) $(CHIP_CFLAGS)

LIBS = $(shell $(CROSS_COMPILE)gcc $(CFLAGS_CORTEX_M4)  --print-libgcc-file-name)

SRC_C = \
	main.c \
	mphalport.c \
	$(wildcard boards/$(BOARD)/src/*.c) \
	$(wildcard chips/$(CHIP)/src/*.c) \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	$(BUILD)/_frozen_mpy.c

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

all: $(BUILD)/firmware.bin

$(MPY_CROSS):
	$(ECHO) "MISC creating mpy cross"
	$(Q)$(MAKE) -C $(dir $@)

$(BUILD)/frozentest.mpy: frozentest.py $(MPY_CROSS)
	$(ECHO) "MISC compile bytecode of $<"
	$(Q)$(MPY_CROSS) $< -o $@

$(BUILD)/_frozen_mpy.c: $(BUILD)/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware.bin $@

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board via DFU"
	$(Q)$(PYTHON) $(PYDFU) -u $<

deploy-openocd: $(BUILD)/firmware.bin
	$(ECHO) "Writing $< to the board via OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "lpc_flash $<"

# Run emulation build on a POSIX system with suitable terminal settings
run:
	stty raw opost -echo
	build/firmware.elf
	@echo Resetting terminal...
# This sleep is useful to spot segfaults
	sleep 1
	reset

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
