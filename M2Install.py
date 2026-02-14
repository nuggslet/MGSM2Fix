#!/usr/bin/env python

'''M2Install.py: Installer utility for M2Fix.'''

import os
import sys
import yaml
import shutil
import zipfile

__author__ = 'nuggslet'
__license__ = 'MIT'

def main():
    import argparse
    parser = argparse.ArgumentParser('M2Install', description='Installer utility for M2Fix')

    parser.add_argument('file', nargs='?')
    parser.add_argument('arch', nargs='?')
    parser.add_argument('--package', action='store_true')
    args = parser.parse_args()

    rules = os.path.join(os.path.dirname(__file__), 'M2Install.yml')
    if not os.path.isfile(rules): return
    rules = open(rules, 'r', encoding='utf-8')
    rules = yaml.load(rules, Loader=yaml.Loader)

    if args.package:
        file = os.path.realpath(__file__)
        file = os.path.dirname(file)
        file = os.path.join(file, 'MGSM2Fix.zip')
        file = zipfile.ZipFile(file, 'r')
        for arch in rules:
            for rule in rules[arch]:
                if os.path.isfile(rule): os.remove(rule)
                rule = os.path.dirname(rule)
                file.extractall(rule)
        file.close()
    else:
        if args.arch not in rules: return
        for rule in rules[args.arch]:
            shutil.copy(args.file, rule)
            rule = os.path.dirname(rule)
            file = os.path.join(rule, 'MGSM2Fix32.asi')
            if os.path.isfile(file): os.remove(file)
            file = os.path.join(rule, 'MGSM2Fix64.asi')
            if os.path.isfile(file): os.remove(file)

if __name__ == "__main__":
    main()
