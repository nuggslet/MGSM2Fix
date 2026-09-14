"""Inventory remaining text in both main discs and the VR disc.

Reads the actual PPFs (including relocated stages), scans GCL STRING records,
overlay pointer tables and LUI/ADDIU or LUI/ORI references. Candidates are not
automatically translations: mappings and reachability still require evidence.
No game files are written. JSON includes counts, samples, hashes and failures.
"""
from collections import defaultdict
from pathlib import Path
import argparse
import json
import re
import struct
from iso import Disc
from portio import (INTEGRAL_IMAGES, USA_IMAGES, USA_VR_IMAGE, entries, stage,
                    read_ppf, sha256)
from workdir import GAME, WORK


def game_text(data):
    """Readable representation and unresolved font glyphs, not language proof."""
    out, p, japanese = [], 0, False
    while p < len(data):
        c = data[p]
        if 32 <= c < 127:
            out.append(chr(c)); p += 1
        elif 0x80 <= c <= 0xDF and p+1 < len(data):
            code = ((c << 8) | data[p+1]) & 0x9FFF
            # font.c strips these style flags before selecting a glyph.
            if 0x8020 <= code < 0x807F:
                out.append(chr(code & 255))
            elif code == 0x9001:
                out.append(' ')
            else:
                out.append('<%04X>' % code)
                japanese = True
            p += 2
        elif c in (1,10,13):
            out.append('<%02X>' % c); p += 1
        else:
            return None, False
    return ''.join(out), japanese


def gcl_strings(data):
    """Framing scan, including overflowing OPTION payloads; no silent skip."""
    out, p = [], 0
    while p+3 <= len(data):
        n = data[p+1]
        if data[p] == 7 and n and p+2+n <= len(data) and data[p+1+n] == 0:
            raw = data[p+2:p+1+n]
            value, japanese = game_text(raw)
            if value is not None:
                out.append(dict(offset=p,hex=raw.hex(),text=value,
                                unresolved_glyphs=japanese, evidence='framing candidate'))
                p += 2+n
                continue
        p += 1
    return out


def references(data, base):
    refs = defaultdict(set)
    highs = {}
    for p in range(0,len(data)-3,4):
        w = struct.unpack_from('<I',data,p)[0]
        if base <= w < base+len(data):
            refs[w-base].add('pointer')
        op,rs,rt,imm = w>>26,(w>>21)&31,(w>>16)&31,w&65535
        if op == 15:
            highs[rt] = (imm<<16,p)
        elif op in (9,13) and rs in highs:
            high,where = highs[rs]
            low = imm-65536 if op==9 and imm>=32768 else imm
            address = (high+low if op==9 else high|low) & 0xFFFFFFFF
            if p-where <= 24 and base <= address < base+len(data):
                refs[address-base].add('lui')
            highs.pop(rt,None)
        elif op in (2,3) or (op==0 and (w&63) in (8,9)):
            highs.clear()
        elif op in (8,10,11,12,14,32,33,35,36,37):
            highs.pop(rt,None)
    out = []
    for p,kinds in sorted(refs.items()):
        end = data.find(b'\0',p,min(p+256,len(data)))
        if end <= p:
            continue
        # Known false positive: a function pointer to addiu sp,sp,-N.
        if p+4 <= len(data) and struct.unpack_from('<I',data,p)[0]&0xFFFF0000 == 0x27BD0000:
            continue
        raw = data[p:end]
        text,jp = game_text(raw)
        if text and jp and len(raw)>=4:
            out.append(dict(offset=p,kinds=sorted(kinds),hex=raw.hex(),text=text))
    return out


def patch_index(paths):
    indexed = defaultdict(list)
    for path in paths:
        for off,data in read_ppf(path):
            sector,within = divmod(off,2352)
            assert 24 <= within and within+len(data)<=2072
            indexed[sector].append((within-24,data))
    return indexed


def read(image, lba, size, patches):
    out = bytearray(image.read(lba,size))
    for sector in range(lba,lba+(size+2047)//2048):
        for within,data in patches.get(sector,[]):
            start = (sector-lba)*2048+within
            if start < size:
                out[start:min(size,start+len(data))] = data[:max(0,size-start)]
    return bytes(out)


def inventory(image, paths, base):
    patches = patch_index(paths)
    files = {n.upper():(l,s) for n,l,s,d in image.walk() if not d}
    lba,size = files['/MGS/STAGE.DIR;1']
    directory = read(image,lba,size,patches)
    result, errors = {}, []
    for name,(sector,_) in entries(directory).items():
        try:
            header = read(image,lba+sector,2048,patches)
            count = struct.unpack_from('<h',header,2)[0]
            assert 0<count<4096
            data = read(image,lba+sector,count*2048,patches)
            tags,payloads,_ = stage(data)
            strings,refs = [],[]
            for k,payload in payloads.items():
                mode,ext = tags[k][1:3]
                if mode == ord('c') and ext == 0xFF:
                    # Cache tags hold offsets into the single c/FF blob, not
                    # individual file lengths (libfs/cdstage.c).
                    for j,t in enumerate(tags[:k]):
                        if t[1] == ord('c') and t[2] == ord('g'):
                            end = tags[j+1][3]
                            for r in gcl_strings(payload[t[3]:end]):
                                r['offset'] += t[3]
                                strings.append(dict(tag=k,**r))
                if mode == ord('s') and ext == ord('b') and base is not None:
                    refs.extend(dict(tag=k,**r) for r in references(payload,base))
            result[name] = dict(sha256=sha256(data),gcl=strings,references=refs)
        except (AssertionError,ValueError,IndexError,struct.error) as e:
            errors.append(dict(stage=name,error=repr(e)))
    return result,errors


def find_vr(container):
    """Discover the SLPM_862.49 image from PVDs and ISO paths."""
    with container.open('rb') as f:
        offset,tail = 0,b''
        while block := f.read(8*1024*1024):
            data = tail+block
            for m in re.finditer(b'\x01CD001\x01',data):
                base = offset-len(tail)+m.start()-24-16*2352
                if base < 0:
                    continue
                try:
                    image = Disc(container,base)
                    found = any('SLPM_862.49' in n for n,_,_,_ in image.walk())
                    image.f.close()
                    if found:
                        return base
                except (AssertionError,IndexError,struct.error):
                    continue
            offset += len(block)
            tail = data[-6:]
    raise ValueError('Integral VR disc not found')


def save_titles(work):
    out = {}
    for name in ('int1.exe','us1.exe'):
        data = (work/name).read_bytes()
        matches = []
        for token in ('\uff2d\uff27\uff33','\uff24\uff4f\uff43\uff4b','\uff3b\uff2e\uff2d\uff3d'):
            pattern = token.encode('cp932')
            start = 0
            while (p := data.find(pattern,start)) >= 0:
                end = data.find(b'\0',p,p+128)
                if end >= 0:
                    raw = data[p:end]
                    matches.append(dict(offset=hex(p),hex=raw.hex(),cp932=raw.decode('cp932',errors='replace')))
                start = p+len(pattern)
        out[name] = dict(sha256=sha256(data),strings=matches)
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, default=Path(GAME) if GAME else None)
    parser.add_argument('--executables', type=Path, default=Path(WORK))
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    if args.game is None:
        parser.error('Cannot find the Master Collection MGS1 directory; '
                      'pass --game, or set INTEGRAL_ENGLISH_GAME (see workdir.py).')
    japan = args.game/'windata/dlc/dlc_japan.bin'
    usa = args.game/'windata/alldata.bin'
    vr = find_vr(japan)
    report = dict(method='candidate inventory; no reachability or translation inferred',
                  integral_vr_base=hex(vr),save_titles=save_titles(args.executables),discs={})
    sets = [('disc1',INTEGRAL_IMAGES[0],USA_IMAGES[0],'INTEGRAL/INTEGRAL/0',0x800C3208,0x800C5968),
            ('disc2',INTEGRAL_IMAGES[1],USA_IMAGES[1],'INTEGRAL/INTEGRAL/1',0x800C3208,0x800C5968),
            ('vr',vr,USA_VR_IMAGE,'INTEGRAL/VR-DISK',None,None)]
    for name,ib,ub,folder,ibase,ubase in sets:
        ipaths = sorted((args.game/'mods'/folder).glob('*.ppf'))
        images = [Disc(japan,ib),Disc(usa,ub)]
        try:
            # VR overlay bases are not established here; report GCL coverage
            # and leave references explicitly unscanned instead of guessing.
            i,ie = inventory(images[0],ipaths,ibase)
            u,ue = inventory(images[1],[],ubase)
            report['discs'][name] = dict(integral=i,usa=u,errors=ie+ue,
                references_scanned=ibase is not None,
                patches={p.name:sha256(p.read_bytes()) for p in ipaths},
                shared_stages=len(i.keys() & u.keys()),integral_only=sorted(i.keys()-u.keys()))
            print(name,'stages',len(i),len(u),'errors',len(ie)+len(ue),flush=True)
        finally:
            for image in images:
                image.f.close()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
    print(args.output,flush=True)


if __name__ == '__main__':
    main()
