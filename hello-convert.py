#!/usr/bin/python3

from PIL import Image
import os
import sys
import argparse
import glob

parser = argparse.ArgumentParser(description='Convert image files for the hello badge')
parser.add_argument('--video', action='store_true')
parser.add_argument('--framerate', default=50, type=int)
parser.add_argument('--scrollrate', default=1, type=int)
parser.add_argument('--reverse', action='store_true')
parser.add_argument('--leftblank', default=None, type=int)
parser.add_argument('--rightblank', default=None, type=int)
parser.add_argument('--leftblankpattern', default=0, type=int)
parser.add_argument('--rightblankpattern', default=0, type=int)
parser.add_argument('--leftpause', default=None, type=int)
parser.add_argument('--rightpause', default=None, type=int)
parser.add_argument('--start', default=0, type=int)
parser.add_argument('input')
parser.add_argument('output')
args = parser.parse_args()

class imageheader:
    def __init__(self,
            width, height,
            frame_rate = 50,
            scroll_rate = 1,
            action = 2,
            left_blank = 0,
            right_blank = 0,
            left_blank_pattern = 0,
            right_blank_pattern = 0,
            left_pause = 0,
            right_pause = 0,
            start_point = 0):
        self.width = width
        self.height = height
        self.frame_rate = frame_rate
        self.scroll_rate = scroll_rate
        self.action = action
        self.left_blank = left_blank
        self.right_blank = right_blank
        self.left_blank_pattern = left_blank_pattern
        self.right_blank_pattern = right_blank_pattern
        self.left_pause = left_pause
        self.right_pause = right_pause
        self.start_point = start_point

    def header(self):
        header = bytearray()
        version = 0
        data = 64
        header.extend("helloimg".encode('ascii'))
        header.extend(version.to_bytes(4, byteorder='little'))
        header.extend(data.to_bytes(4, byteorder='little'))
        header.extend(self.width.to_bytes(4, byteorder='little'))
        header.extend(self.height.to_bytes(4, byteorder='little'))
        header.extend(self.frame_rate.to_bytes(4, byteorder='little'))
        header.extend(self.scroll_rate.to_bytes(4, byteorder='little'))
        header.extend(self.action.to_bytes(4, byteorder='little'))
        header.extend(self.left_blank.to_bytes(4, byteorder='little'))
        header.extend(self.right_blank.to_bytes(4, byteorder='little'))
        header.extend(self.left_blank_pattern.to_bytes(4, byteorder='little'))
        header.extend(self.right_blank_pattern.to_bytes(4, byteorder='little'))
        header.extend(self.left_pause.to_bytes(4, byteorder='little'))
        header.extend(self.right_pause.to_bytes(4, byteorder='little'))
        header.extend(self.start_point.to_bytes(4, byteorder='little'))
        return header


class videoheader:
    def __init__(self,
            width, height,
            frames,
            frame_rate = 50,
            action = 1,
            start_point = 0):
        self.width = width
        self.height = height
        self.frames = frames
        self.frame_rate = frame_rate
        self.action = action
        self.start_point = start_point

    def header(self):
        header = bytearray()
        version = 0
        data = 40
        header.extend("hellovid".encode('ascii'))
        header.extend(version.to_bytes(4, byteorder='little'))
        header.extend(data.to_bytes(4, byteorder='little'))
        header.extend(self.width.to_bytes(4, byteorder='little'))
        header.extend(self.height.to_bytes(4, byteorder='little'))
        header.extend(self.frames.to_bytes(4, byteorder='little'))
        header.extend(self.frame_rate.to_bytes(4, byteorder='little'))
        header.extend(self.action.to_bytes(4, byteorder='little'))
        header.extend(self.start_point.to_bytes(4, byteorder='little'))
        return header


class image:
    def __init__(self, name, threshold=64):
        self.im = Image.open(name)
        self.threshold = threshold

    def width(self):
        return self.im.size[0]

    def height(self):
        return self.im.size[1]

    def data(self):
        data = bytearray()
        rows = int(self.height() / 8)

        for y in range(rows):
            for x in range(self.width()):
                byte = 0
                for bit in range(rows):
                    pixel = self.im.getpixel((x, y*8+bit))
                    if not isinstance(pixel, int):
                        pixel = pixel[0]
                    if pixel >= self.threshold:
                        byte |= (1<<bit)
                data.append(byte);
        return data

action = 1
if args.reverse:
    action = 2

if args.video:
    files = sorted(glob.glob(args.input + "*"))
    if not files:
        print("No matching files")
        sys.exit(1)
    data = bytearray()
    for file in files:
        print("Processing file {}".format(file))
        im = image(file)
        if im.height() != 64:
            print("Height must be exactly 64 pixels")
            sys.exit(1)

        if im.width() != 128:
            print("Width must be exactly 128 pixels")
            sys.exit(1)

        data.extend(im.data())

    header = videoheader(128, 64, len(files),
                         frame_rate = args.framerate,
                         action = action,
                         start_point = args.start)

else:
    image = image(args.input)

    if image.height() != 64:
        print("Height must be exactly 64 pixels")
        sys.exit(1)

    if image.width() < 128:
        print("Width must be at least 128 pixels")
        sys.exit(1)

    if args.leftblank is None:
        args.leftblank = 0 if args.reverse else 128

    if args.rightblank is None:
        args.rightblank = 0 if args.reverse else 128

    if args.leftpause is None:
        args.leftpause = 20 if args.reverse else 0

    if args.rightpause is None:
        args.rightpause = 20 if args.reverse else 0

    header = imageheader(image.width(), image.height(),
                         frame_rate = args.framerate,
                         scroll_rate = args.scrollrate,
                         action = action,
                         left_blank = args.leftblank,
                         right_blank = args.rightblank,
                         left_blank_pattern = args.leftblankpattern,
                         right_blank_pattern = args.rightblankpattern,
                         left_pause = args.leftpause,
                         right_pause = args.rightpause,
                         start_point = args.start)

    data = image.data()

f = open(args.output, 'wb')
f.write(header.header())
f.write(data)
f.close()

