# hello
## the mini badge
### firmware

## Prerequisites

You will need the following software installed to build the firmware:

* a working gcc-arm-embedded toolchain
* GNU make

You will also need the following software installed to flash the firmware:

* lpc21isp
* Python
* PyUSB

## Building the firmware

* Just type `make`

## Flashing the firmware on Linux

* Connect the hello badge to USB
* Type `make flash`

This defaults to writing to ttyUSB0. If the badge is connected to a different tty, let's say ttyUSB1, type:

* `make flash DEVICE=/dev/ttyUSB1`

## Flashing the firmware manually on other operating systems

If you are using Windows, use FlashMagic to program the firmware. The file `hello.bin` is what you need to flash.

Before you can flash the firmware, you need to run:

* `hello-uart.py --bootloader`

After flashing the firmware, you need to run:

* `hello-uart.py --application`

