#!/usr/bin/python3
#
# Read a file from standard input and if it's size is less the specified size
# pad it writing the result to standard output.
#
# Example:
#    echo -n 'abc' | ./pad.py 16 0x50 | hexdump -C
#    00000000  61 62 63 50 50 50 50 50  50 50 50 50 50 50 50 50  |abcPPPPPPPPPPPPP|
#    00000010
#

import sys
import os
import getopt

READ_SIZE = 64 * 1024

def main(args):
    try:
        opts, args = getopt.getopt(args, 'h')
        for opt in opts:
            if opt[0] == '-h':
                usage()

        if len(args) not in (1, 2):
            usage()
        size = int(args[0])
        byte = 0
        if len(args) == 2:
            byte = int(args.pop(), 16)

        pad(size, byte)
    except Exception as e:
        fatal('%s' % str(e))

def pad(size, byte):
    n = 0
    while True:
        buf = sys.stdin.buffer.read(READ_SIZE)
        sys.stdout.buffer.write(buf)
        n += len(buf)
        if len(buf) < READ_SIZE:
            break
    if n < size:
        buf = bytes([byte for x in range(size - n)])
        sys.stdout.buffer.write(buf)
    sys.stdout.buffer.flush()

def usage():
    print('usage: %s <size> [<fill_byte>] < input > output' % sys.argv[0])
    print('Pad an input file to <size> using <fill_byte> (0x00 by default).')

def fatal(msg):
    sys.stderr.write('%s: %s\n' % (sys.argv[0], msg))
    sys.exit(-1)

if __name__ == '__main__':
    main(sys.argv[1:])
