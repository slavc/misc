#!/bin/python

import sys, time

def main(args):
    f = open(args[1], 'r')
    s = f.read()
    f.close()
    start = time.clock()
    eval(s, {})
    end = time.clock()
    diff = end - start
    print diff

if __name__ == '__main__':
    main(sys.argv)
