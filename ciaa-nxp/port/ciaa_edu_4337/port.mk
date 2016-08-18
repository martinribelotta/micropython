PORT_M = base board chip modules

PORT_MODULES =
PORT_EXTRA_SRC =

PORT_FLASH_WRITE="flash write_image erase unlock $^ 0x1A000000 bin"
