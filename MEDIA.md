# hello
## the mini badge
### media

## Prerequisites

The following software will be useful for creating and converting media:

* Python
* Python Image Library (PIL / Pillow)
* Open Sans font (fonts-open-sans)

## Converting media

### Text

The conversion script can produce a scrolling image from text. You can do it like this;

`./hello-convert.py --reverse --text 'hello world' helloworld.img`

### Image files

The badge can display an image file that is at least 128 pixels wide and exactly 64 pixels high.

PNG and similar file formats can be converted like this:

`./hello-convert.py image.png image.img`

If you want to reverse the scrolling direction at the end (ideal for short text such as names), us this:

`./hello-convert.py --reverse image.png image.img`

You can use any image editor to create your image. Once you've created an image, save it as a PNG without
an alpha channel. Stick to black and white, as that's all the display is capable of.

### Video files

The badge can display a video file that is exactly 128x64 pixels.

Suppose you have a series of image files (in PNG or similar format) named `videoframe0000.png`, `videoframe0001.png`, and so on. You can convert them to a video like this:

`./hello-convert.py --video videoframe video.vid`

If you only have a video file which is not yet split into individual images, you can split it into frames using ffmpeg:

`ffmpeg -i video.mkv -vf scale=w=128:h=64:force_original_aspect_ratio=increase,crop=128:64 videoframe%04d.png`

If you know the frame rate of your video source, you can set it like this:

`./hello-convert.py --video --framerate 25 videoframe video.vid`

You can use any animation or video editor to create your video. Once you've created a video, if your software is
able to, save it directly as a series of PNG files without an alpha channel. Stick to black and white, as that's
all the display is capable of. If your software is unable to export as PNG files, use ffmpeg to convert, as shown
above.


## Command line and file transfer

### Prerequisites

You will need a working serial terminal program and working ZMODEM.

On Linux or MacOS, install:

* minicom
* lrzsz

On Windows, install one of:

* HyperTerminal
* Tera Term

### Connecting

Connect your badge with USB and then open up your serial terminal.

The connection parameters are:

* Any baud rate up to 1Mbps
* 8 data bits
* No parity
* 1 stop bit
* Hardware flow control

The first character received by the badge is used to set the baud rate. It *must* be an odd-valued byte, such as the letter 'a'. If a different character is sent, the badge must be disconnected from USB before communication can be attempted again.

### Command line

Once communication is established, the badge will present a command line interface. A list of commands can be obtained by typing `help`.

Files can be listed with the `ls` command and deleted with the `rm` command. You can find out how much space is used with the `free` command.

Do not use the `format` command unless you have no choice. It will not ask you for confirmation before destroying all your data.

### Sending files

#### The quick and easy way

Make sure that lrzsz is installed, and then:

`./send <files>`

This defaults to /dev/ttyUSB0. If your badge is connected to a different tty:

`DEVICE=/dev/ttyUSBwhatever ./send <files>`

#### The traditional way

Files can be placed onto the badge using ZMODEM. Simply initiate a transfer from your terminal program.

If you're using minicom, that's Ctrl-A followed by S, or Meta-S depending on your operatinng system. Select zmodem then select your file. Go into a directory by pressing the space bar twice, and select and send a file by pressing the space bar once, and then enter.

## Display

Under normal circumstances the display will show whichever media has been selected. In the absence of any other configuration, this will default to the first file on the badge, in filesystem order.

If a media file can not be read, an exclamation mark will be displayed instead.

The display on the badge will change when the command line is active. When at least one character is in the command line buffer, a chevron > will be displayed.

When a file transfer is in progress, a progress bar will be displayed. If the proportion of the file(s) transferred is known, the bar will indicate this. If not, the inside of the bar will show a grey pattern to indicate that the progress is unknown.

