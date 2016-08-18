PORT_M = base board chip modules

PORT_MODULES =

PORT_EXTRA_SRC =

PORT_FLASH_WRITE="flash write_image erase unlock $^ 0x00000000 bin"

PORT_EXTRA_LIBS=port/$(PORT)/chip/libs/libpower_m4f_hard.a
