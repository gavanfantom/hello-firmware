# hello
## the mini badge

This is where you will find the firmware for the hello mini badge.

You will need the following software installed to build:

* a working gcc-arm-embedded toolchain.
* GNU make

You will also need the following software installed to flash the firmware:

* lpc21isp
* Python

## Building the firmware

* Just type "make"

## Flashing the firmware on Linux

* Connect the hello badge to USB
* Type `make flash`

This defaults to writing to ttyUSB0. If the badge is connected to a different tty, let's say ttyUSB1, type:

* `make flash DEVICE=/dev/ttyUSB1`

## Flashing the firmware manually on other operating systems

If you are using Windows, use FlashMagic to program the firmware.

Before you can flash the firmware, you need to run:

* `hello-uart.py --bootloader`

After flashing the firmware, you need to run:

* `hello-uart.py --application`

## File formats

### Image files

The badge can display an image file that is at least 128 pixels wide and exactly 64 pixels high.

A script will convert a PNG or similar file into a suitable file format.

### Video files

The badge can display a video file that is exactly 128x64 pixels.

A script will convert a series of PNG or similar files into a suitable file format.

## Command line and file transfer

Connect your badge with USB and then open up a serial terminal such as minicom or HyperTerminal.

The connection parameters are:

* Any baud rate up to 1Mbps
* 8 data bits
* No parity
* 1 stop bit
* Hardware flow control

The first character received by the badge is used to set the baud rate. It *must* be an odd-valued byte, such as the letter 'a'. If a different character is sent, the badge must be disconnected from USB before communication can be attempted again.

Once communication is established, the badge will present a command line interface. A list of commands can be obtained by typing `help`.

Files can be placed onto the badge using ZMODEM. Simply initiate a transfer from your terminal program.

Files can be listed with the `ls` command and deleted with the `rm` command. You can find out how much space is used with the `free` command.

Do not use the `format` command unless you have no choice. It will not ask you for confirmation before destroying all your data.

## Display

Under normal circumstances the display will show whichever media has been selected. In the absence of any other configuration, this will default to the first file on the badge, in filesystem order.

If a media file can not be read, an exclamation mark will be displayed instead.

The display on the badge will change when the command line is active. When at least one character is in the command line buffer, a chevron > will be displayed.

When a file transfer is in progress, a progress bar will be displayed. If the proportion of the file(s) transferred is known, the bar will indicate this. If not, the inside of the bar will show a grey pattern to indicate that the progress is unknown.


## Navigation button

In normal display mode, the navigation button will do the following:

* Left/Right - Next/previous file
* Up/Down - Adjust display brightness
* Push - Enter menu

The navigation button can also be used to navigate within the menu.

# Third party code

This project uses libraries which are licensed separately. Refer to the licences in the library directories for details.
Note that the LICENSE file does not apply to third party code.

* LPCopen
* LittleFS
