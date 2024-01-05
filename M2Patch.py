#!/usr/bin/env python

'''M2Patch.py: Patcher tool for M2ENGAGE images [Metal Gear Solid].'''

import os
import re
import json
import struct
import shutil

import lz4.block

__author__ = 'nuggslet'
__license__ = 'MIT'

class M2Patch:
    def __init__(self, name=None, version=None):
        self.dev_name = name
        self.dev_version = version

        self.title = None
        if name:
            self.title = self.get_title(name)
            self.dev_id = int(self.title['dev_id'])

        self.version = None
        if version and self.title:
            self.version = self.get_version(self.dev_id, version)

        self.memories = ['rom']
        self.sets = ['PS', 'PS_SWAP', 'NX', 'XBOX', 'STEAM', 'PS5']
        self.accelerators = {
            0x7C1C0604: 'mgs_r3000_vr_eu',
            0x852CC462: 'mgs_r3000_vr_us',
            0x8020ADBB: 'mgs_r3000_vr_jp',
            0x7FD6F607: 'mgs_r3000_int',
            0x63B21C41: 'mgs_r3000_es',
            0x66AEFE85: 'mgs_r3000_de',
            0x67AEE2C9: 'mgs_r3000_it',
            0x65AF35FD: 'mgs_r3000_fr',
            0x5FB57AB9: 'mgs_r3000_uk',
            0xB20744CD: 'mgs_r3000_us',
            0x77C8ED13: 'mgs_r3000_jp',
        }

    def get_titles(self):
        schema = json.load(open('system/config/system_prof.psb.json', 'rb'))['root']
        for title in schema['title_list']:
            if 'dev_name_list' in title: return title['dev_name_list']
        return None

    def get_title(self, name):
        schema = json.load(open('system/config/system_prof.psb.json', 'rb'))['root']
        for title in schema['title_list']:
            if 'dev_name_list' in title: continue
            if title['dev_name'] == name: return title
        return None

    def get_versions(self, id):
        schema = json.load(open('%03d/config/title_prof.psb.json' % id, 'rb'))['root']
        return list(schema['m2epi']['version'].keys())

    def get_version(self, id, name):
        schema = json.load(open('%03d/config/title_prof.psb.json' % id, 'rb'))['root']
        if name not in schema['m2epi']['version']:
            return None
        return schema['m2epi']['version'][name]

    def read_archive_lz4a(self, file):
        blob = open('system/%s' % file, 'rb').read()
        body = blob.find(b'lz4a', blob.find(b'lz4a') + 1) + len(b'lz4a')
        header = dict(zip(['csz', 'dsz', 'fsz'], struct.unpack_from('<2IB', blob, body)))
        image = body + struct.calcsize('<2IB') + header['fsz']
        image = blob[image: image + header['csz']]
        return lz4.block.decompress(image, header['dsz'])

    def read_archive(self, file):
        magic = open('system/%s' % file, 'rb').read(struct.calcsize('4s'))
        magic = struct.unpack('4s', magic)[0]
        if magic == b'lz4a': return self.read_archive_lz4a(file)
        raise ValueError(magic)

    def acc_r3000_sum(self, memory):
        return sum(struct.unpack_from('<256I', memory, 0x10000)) & 0xFFFFFFFF

    def accelerate(self):
        for memory in self.memories:
            data = self.read_archive(self.version[memory])

            sum = self.acc_r3000_sum(data)
            print('ACC[r3000]: 0x%08X (%s)' % (sum, self.accelerators.get(sum, 'Unknown')))

    def memory_defines(self):
        schema = json.load(open('%03d/config/title_prof.psb.json' % self.dev_id, 'rb'))['root']['memory_define']
        if self.dev_version in schema: schema = schema[self.dev_version]
        for key, value in schema.items():
            if type(value) is list: # Hierarchy?!
                for i in range(len(value)):
                    print('MEM[%s][%d]: 0x%08X' % (key, i, value[i]))
            else:
                print('MEM[%s]: 0x%08X' % (key, value))

    def read(self):
        for memory in self.memories:
            data = self.read_archive(self.version[memory])
            file = open('system/%s.bin' % os.path.splitext(self.version[memory])[0], 'wb')
            file.write(data)
            file.close()

    def get_library(self, file):
        data = self.read_archive(file)
        size = len(max(re.findall(b'\xFF+', data[0x10000:]), key=len))
        offset = data.find(b'\xFF' * size, 0x10000)
        name = 'system/%s.lib' % os.path.splitext(file)[0]
        if not os.path.isfile(name): return (offset, size, None)
        return (offset, size, open(name, 'rb').read())

    def reconstruct(self):
        for memory in self.memories:
            data = self.read_archive(self.version[memory])
            lib_offset, lib_size, lib_data = self.get_library(self.version[memory])
            if lib_data: data = data[0 : lib_offset] + lib_data + data[lib_offset + lib_size:]

            header = data[0xB070: 0xB070 + 0x800]
            size = struct.unpack_from('<I', header, 0x1C)[0]
            image = data[0x10000: 0x10000 + size]

            file = open('system/%s.exe' % os.path.splitext(self.version[memory])[0], 'wb')
            file.write(header + image)
            file.close()

    def patch(self, set=None):
        memories = []
        for memory in self.memories:
            file = open('system/%s' % os.path.splitext(self.version[memory])[0], 'wb')
            file.write(self.read_archive(self.version[memory]))
            memories.append(file)

        disks = []
        for disk in self.version['disk'] if type(self.version['disk']) is list else [self.version['disk']]:
            shutil.copyfile('system/%s' % disk, 'system/%s' % os.path.splitext(disk)[0])
            disks.append(open('system/%s' % os.path.splitext(disk)[0], 'r+b'))

        if not set: return
        if set == 'Default': set = None

        schema = json.load(open('%03d/config/title_patchdata.psb.json' % self.dev_id, 'rb'))
        if 'patchs' not in schema: patches = schema[self.dev_version]['patchs']
        else: patches = schema['patchs']

        for patch in patches:
            offset = patch.get('offset')
            disc   = patch.get('disc', None)
            memory = patch.get('memory', None)
            file   = patch.get('file', None)
            data   = patch.get('data', None)

            if file:
                file = '%03d/patch/%s' % (self.dev_id, file)
                if not os.path.isfile(file): file = '%s_%s.bin' % (file, set)
                if not os.path.isfile(file) and not set: continue
                data = open(file, 'rb').read()
            data = bytes(data)

            if disc is not None:
                disks[disc].seek(offset)
                disks[disc].write(data)

            if memory is not None:
                memories[memory].seek(offset)
                memories[memory].write(data)

            block = 'IM%d' % memory if disc is None else 'CD%d' % disc
            print('%s[0x%08X]: %s' % (block, offset, data))

        for memory in memories:
            memory.close()

        for disk in disks:
            disk.close()

def main():
    import argparse
    parser = argparse.ArgumentParser('M2Patch', description='Patcher tool for M2ENGAGE images [Metal Gear Solid]')

    epi = M2Patch()

    parser.add_argument('--patch', choices=epi.sets + ['Default'])

    parser.add_argument('--name', choices=epi.get_titles())
    args, _args = parser.parse_known_args()
    if args.name is None:
        parser.error('the following arguments are required: --name')

    epi = M2Patch(args.name)

    parser.add_argument('--version', choices=epi.get_versions(epi.dev_id))
    parser.parse_args(_args, namespace=args)
    if args.name is None or args.version is None:
        parser.error('the following arguments are required: --version')

    epi = M2Patch(args.name, args.version)
    epi.read()
    epi.accelerate()
    epi.memory_defines()
    epi.patch(args.patch)
    epi.reconstruct()

if __name__ == '__main__':
    main()
