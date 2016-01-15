# LPC43XX support

Support for NXP LPC43XX MCU

### Build binaries

#### Require
  - GNU arm embedded gcc:
  - GNU make
  - python3 with pyserial (`pip install pyserial`)
  - OpenOCD 0.8+

## Usage mode
- Execute `make clean all download` to compile and download the program to board.
- Open a terminal on serial USB converter 2 (/dev/ttyUSB1 usualy)
- A python REPL interpreter is available via USB UART (DEBUG connector)

##### Keybindings
###### Fancy REPL (aka `>>>` promt)
- CTRL+D restart interpreter (not board) aka soft reset
- CTRL+C cancel actual command
- CTRL+E enter paste mode (for paste code to editor)
- CTRL+A enter RAW REPL (for code sending)

###### RAW REPL (aka `>` promt)
- CTRL+B exit RAW REPL

###### PASTE MODE (aka `===` promt)
- CTRL+D finish and process paste mode
- CTRL+C cancel paste mode

## Support libraries

#### Base system packages and functionality

  - Full micropython implementation
  - FileIO interface with embedded flash filesystem (open, close, read)
  - collections
  - ustruct as sctruct
  - ubinascii as binascii
  - ure as re
  - uzlib as zlib
  - ujson as json
  - uheap as heap
  - uhashlib as hashlib
  - uos as os and uos
  - io

#### Hardware support (pyb module)
  - LED boards `pyb.LED`
  - Switchs `pyb.Switch`
  - UART `pyb.UART` (RS485 capable)
  - GPIO `pyb.GPIO`
  - DAC `pyb.DAC`
  - Timers (no PWM) `pyb.TIMER`
  - SCT PWM `pyb.PWM`
  - ADC `pyb.ADC`
  - EEPROM `pyb.EEPROM`
  - SPI `pyb.SPI`
  - RTC `pyb.RTC`

> Note: Some boards support more modules, see specific README.md on board/* directory