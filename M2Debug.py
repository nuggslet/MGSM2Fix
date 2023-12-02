#!/usr/bin/env python

'''M2Debug.py: Debugger for M2ENGAGE [Metal Gear Solid].'''

import socket

__author__ = 'nuggslet'
__license__ = 'MIT'

class M2Debug:
    def __init__(self, socket):
        self.s = socket

    def ready(self):
        msg = b'rd\n'
        self.s.send(msg)

    def evaluate(self, closure):
        closure = closure.replace('\r', '').replace('\n', '')
        msg = b'ev:%s\0\n' % closure.encode('ascii')
        r = self.s.makefile('r')
        self.s.send(msg)
        return r.readline()

def main():
    import argparse
    parser = argparse.ArgumentParser('M2Debug', description='Debugger for M2ENGAGE [Metal Gear Solid]')

    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=27615)
    args = parser.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((args.host, args.port))
    s = M2Debug(s)

    s.ready()
    while True:
        print(s.evaluate(input('> ')))

if __name__ == "__main__":
    main()
