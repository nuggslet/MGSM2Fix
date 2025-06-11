#!/usr/bin/env python

'''DraculaDominus.py: ROM extraction utility for M2ENGAGE [Castlevania Dominus Collection].'''

import os
import json

__author__ = 'nuggslet'
__license__ = 'MIT'

def dracula_unpack(src, dst):
    files = os.path.join(src, 'files.bin')
    files = open(files, 'rb')
    meta = os.path.join(src, 'files_info.psb.json')
    meta = open(meta, 'r')

    meta = json.load(meta)
    for info in meta['list']:
        files.seek(info['offset'])
        data = files.read(info['bytes'])
        path = info['path'].lstrip('/\\')
        path = os.path.join(dst, path)
        path = os.path.normpath(path)

        os.makedirs(os.path.dirname(path), exist_ok=True)
        file = open(path, 'wb')
        file.write(data)
        file.close()

def main():
    import argparse
    parser = argparse.ArgumentParser('DraculaDominus', description='ROM extraction utility for M2ENGAGE [Castlevania Dominus Collection]')

    parser.add_argument('src')
    parser.add_argument('dst')

    args = parser.parse_args()
    dracula_unpack(args.src, args.dst)

if __name__ == '__main__':
    main()
