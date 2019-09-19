#!/usr/bin/env python3
# To create the icon header file run this script as follows.
# python3 tools/embed_c.py -o icon.c -p icon.h -n icondata assets/icon.bmp

import sys


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Create c/h file from a resource to embed inside a program.')
    parser.add_argument('input', nargs=argparse.OPTIONAL, default=sys.stdin, type=argparse.FileType('rb'),
                        help='Input file to embed (default is stdin')
    parser.add_argument('--output', '-o', type=argparse.FileType('w'), default=sys.stdout,
                        help='Output C source file (default is stdout)')
    parser.add_argument('--header', '-p', default=None,
                        help='Output H header file  (default is to not generate it)')
    parser.add_argument('--name', '-n', nargs=argparse.OPTIONAL, default='data',
                        help='Variable name of the generated variable')

    if not sys.version_info >= (3, ):
        parser.error('This script requires python 3+')

    ns = parser.parse_args()

    # Count the number of bytes while we are reading.
    # We can't do seek because the input can be stdin which is not seekable.
    data_length = 0

    # Get the buffer of the stream to read binary data
    inputfile = ns.input
    if hasattr(inputfile, 'buffer'):
        inputfile = inputfile.buffer

    # Write the C file
    cout = ns.output
    if ns.header:
        cout.write('#include "{}"\n'.format(ns.header))
        cout.write('\n')
    cout.write('const char {}[] = {{\n'.format(ns.name))
    while True:
        line = inputfile.read(8)
        data_length += len(line)
        if not line:
            break
        cout.write("\t{}\n".format(' '.join('0x{:02x},'.format(b) for b in line)))
    cout.write('};\n')
    cout.close()

    # Write the H file
    if ns.header:
        guard_trans = str.maketrans('.@,-=+', '______')
        guard_name = '_{}_'.format(ns.header.upper().translate(guard_trans))

        with open(ns.header, 'w') as hout:
            hout.write('#ifndef {}\n'.format(guard_name))
            hout.write('#define {}\n'.format(guard_name))
            hout.write('\n')
            hout.write('extern const char {}[{}];\n'.format(ns.name, data_length))
            hout.write('\n')
            hout.write('#endif // {}\n'.format(guard_name))
    return 0


if __name__ == '__main__':
    sys.exit(main())
