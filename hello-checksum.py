#!/usr/bin/python3

import argparse

parser = argparse.ArgumentParser(description='Update checksum')
parser.add_argument("--read", dest='read', metavar='filename', type=argparse.FileType('rb'), required=True, help='Read config from file')
parser.add_argument("--write", dest='write', metavar='filename', type=argparse.FileType('wb'), required=True, help='Write config to file')
args = parser.parse_args()

def update(config):
    if len(config) < 4:
        raise ValueError('Input too short')
    size = config[0] + (config[1] << 8)
    if (size != len(config)):
        raise ValueError('Length mismatch')
    config = config[:-2]
    sum1 = 0
    sum2 = 0
    for i in range(size-2):
        sum1 = (sum1 + config[i]) % 255
        sum2 = (sum2 + sum1) % 255
    return config + bytes([sum2, sum1])

args.write.write(update(args.read.read()))
