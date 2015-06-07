#!/usr/bin/python

import sys
import os.path
import string
import getopt

cflag = 0 # clean output: just the hexdump

def path2varname(path):
    path = os.path.basename(path)
    s = ''
    for c in path:
        if c in string.ascii_letters or c in string.digits:
            s += c
        else:
            s += '_'
    return s

def main():
    global cflag

    opts, args = getopt.getopt(sys.argv[1:], "c")

    for (x, y) in opts:
        if x == "-c":
            cflag += 1

    for path in args:
        varname = path2varname(path)
        with open(path, 'rb') as f:
            if not cflag:
                sys.stdout.write('static const char %s[] = {' % varname)
            data = f.read()
            i = 0
            for c in data:
                if i % 16 == 0:
                    sys.stdout.write('\n')
                    if not cflag:
                        sys.stdout.write('\t')
                i += 1
                sys.stdout.write('0x%02x, ' % ord(c))
            if not cflag:
                sys.stdout.write('\n};')
            sys.stdout.write('\n')

if __name__ == '__main__':
    main()

