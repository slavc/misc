#!/usr/bin/python

import sys
import os.path
import string

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
    for path in sys.argv[1:]:
        varname = path2varname(path)
        with open(path, 'rb') as f:
            sys.stdout.write('static const char %s[] = {' % varname)
            data = f.read()
            i = 0
            for c in data:
                if i % 16 == 0:
                    sys.stdout.write('\n\t')
                i += 1
                sys.stdout.write('0x%02x, ' % ord(c))
            sys.stdout.write('\n};\n\n')

if __name__ == '__main__':
    main()

