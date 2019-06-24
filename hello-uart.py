#!/usr/bin/python3

import usb.core
import usb.util
import argparse

parser = argparse.ArgumentParser(description='Enable/disable bootloader mode')
group1 = parser.add_mutually_exclusive_group()
group1.add_argument("--bootloader", dest='bootloader', action="store_const", const=True, default=False, help='Switch to bootloader mode')
group1.add_argument("--application", dest='application', action="store_const", const=True, default=False, help='Switch to application mode')
group2 = parser.add_mutually_exclusive_group()
group2.add_argument("--read-config", dest='readconfig', metavar='filename', type=argparse.FileType('wb'), help='Read config into file')
group2.add_argument("--write-config", dest='writeconfig', metavar='filename', type=argparse.FileType('rb'), help='Write config file to chip (use with care)')
parser.add_argument("--unconfigured", dest='unconfigured', action='store_const', const=True, default=False, help='Match unconfigured devices (use with care)')
args = parser.parse_args()

class BootloaderSwitch:
    def __init__(self, **args):
        self.dev = usb.core.find(**args)
        if self.dev is None:
            raise ValueError('Device not found')

    @property
    def state(self):
        gpio = self.dev.ctrl_transfer(0xc1, 0xff, 0x00c2, 0, 1)
        return (gpio[0] & 0x20) != False

    @state.setter
    def state(self, state):
        value = 0x2020 if state else 0x0020
        self.dev.ctrl_transfer(0x41, 0xff, 0x37e1, value, [])

    def read_config(self):
        ret = self.dev.ctrl_transfer(0xc0, 0xff, 0x000e, 0, 2)
        size = ret[0] + (ret[1] << 8)
        ret = self.dev.ctrl_transfer(0xc0, 0xff, 0x000e, 0, size)
        return ret

    def write_config(self, config):
        ret = self.dev.ctrl_transfer(0xc0, 0xff, 0x000e, 0, 2)
        size = ret[0] + (ret[1] << 8)
        ret = self.dev.ctrl_transfer(0x40, 0xff, 0x370f, 0, config)
        return ret


def name(value):
    if value:
        return "Bootloader"
    else:
        return "Application"

def verify(config):
    if len(config) < 4:
        return False
    size = config[0] + (config[1] << 8)
    if (size != len(config)):
        return False
    sum1 = 0
    sum2 = 0
    for i in range(size-2):
        sum1 = (sum1 + config[i]) % 255
        sum2 = (sum2 + sum1) % 255
    if sum1 != config[size-1]:
        return False
    if sum2 != config[size-2]:
        return False
    return True

if args.unconfigured:
    switch = BootloaderSwitch(idVendor=0x10c4, idProduct=0xea60)
else:
    switch = BootloaderSwitch(idVendor=0x10c4, idProduct=0xea60, product='hello badge')

old_state = switch.state

if args.bootloader:
    switch.state = True
if args.application:
    switch.state = False

if args.bootloader or args.application:
    new_state = switch.state
    if new_state != old_state:
        print("New state: {} (was {})".format(name(new_state), name(old_state)))
    else:
        print("Current state: {} (unchanged)".format(name(old_state)))
else:
    print("Current state: {}".format(name(old_state)))

if args.readconfig:
    args.readconfig.write(switch.read_config())

if args.writeconfig:
    config = args.writeconfig.read()
    if verify(config):
        switch.write_config(config)
    else:
        print("Checksum mismatch")
