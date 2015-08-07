#!/usr/bin/python3
#
# Read a plain-text description of a data structure from standard input and
# serialize it to standard output.
#
# An input line consists of the following fields:
#   <field_name> <type> <value> [<count>]
# where:
#   <field_name> -- is annotation for humans which is not used by the tool.
#   <type> -- is one of:
#       le32 -- little-endian integer;
#       be32 -- big-endian integer;
#       byte -- byte.
#   <value> -- is a decimal or hexadecimal number.
#   <count> -- is optional: how many times to add this field (1 by default).
# Comments are started with '//' sequence and span till the end of line.
#
# EBNF for input:
#   input = {line}
#   line = [space], (field_spec | (field_spec, comment) | comment | EMPTY)
#   field_spec = field_name, space, type, space, value, [space, repeat_count]
#   field_name = NONSPACE_CHARACTER, {NONSPACE_CHARACTER}
#   type = "le32" | "be32" | "byte"
#   value = number
#   repeat_count = number
#   space = {" " | "\t"}
#   number = (("0x" | "0X"), HEXDIGIT, {HEXDIGIT}) | (["-"], DIGIT, {DIGIT})
#

import sys
import os
import getopt

MAX_TOKENS = 3 # max number of tokens (which are used by the tool) on a line

def main(args):
    opts, args = getopt.getopt(args, 'h')
    for opt in opts:
        if opt[0] == '-h':
            usage()
    genblob(sys.stdin, sys.stdout)

def usage():
    print('usage: %s < input.txt > output.bin' % sys.argv[0])
    print('Read plain-text description of a data structure from standard')
    print('input and serialize it to standard output.')
    sys.exit(0)

def genblob(fin, fout):
    line_no = 0
    for line in fin.readlines():
        line_no += 1
        try:
            line = line.split('//')[0] # remove comment if any
            line = line.strip()
            if not line: # empty line
                continue
            tokens = line.split()[1:] # field name is ignored: annotation for humans
            process(tokens, fout)
        except Exception as e:
            fatal('%s: parse error at line %d: %s' % (sys.argv[0], line_no, str(e)))
    sys.exit(0)

def fatal(msg):
    sys.stderr.write(msg + '\n')
    sys.exit(-1)

def process(tokens, fout):
    if len(tokens) > MAX_TOKENS:
        raise RuntimeError('unknown tokens: %s' % ' '.join(tokens[MAX_TOKENS:]))

    field_type = tokens[0]

    value = tokens[1]
    if value[:2] in ('0x', '0X'):
        value = int(value, 16)
    else:
        value = int(value)

    repeat_count = 1
    if len(tokens) > 2:
        repeat_count = int(tokens[2])

    while repeat_count > 0:
        repeat_count -= 1
        if field_type == 'le32':
            for i in range(4):
                os.write(fout.fileno(), bytes([(value >> (i * 8)) & 0xff]))
        elif field_type == 'be32':
            for i in range(4):
                os.write(fout.fileno(), bytes([(value >> ((4 - i - 1) * 8)) & 0xff]))
        elif field_type == 'byte':
            os.write(fout.fileno(), bytes([value & 0xff]))
        else:
            raise RuntimeError('%s: unknown field type' % field_type)

if __name__ == '__main__':
    main(sys.argv[1:])

