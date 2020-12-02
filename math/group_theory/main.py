#!/usr/bin/python3

from group import *

def main():
    print_group_info([1, 2, 4, 5, 7, 8], 'lambda a, b: (a * b) % 9')

if __name__ == '__main__':
    main()
