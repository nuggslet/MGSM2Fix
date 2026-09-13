"""Both recaps in English, in one grown `preope` stage.

The 796-byte ceiling I had been working under was self-imposed.  The stage is
not a fixed blob: LoadCacheSection ends with

    size = (current_ptr + tag->size) - buffer;
    GV_SplitMemory(GV_NORMAL_MEMORY, buffer, size);

so the stage keeps exactly what its tags declare and returns the rest of the
block.  Growing the script therefore only needs

  * tag[6].size raised (it is the ext=0xFF "fake tag" holding the GCL script),
  * the sound files after it shifted to the next sector boundary,
  * DATACNF.size raised from 87 to 88 sectors  <- the step the earlier
    growth attempts missed, which is why they broke, and
  * the stage parked somewhere with room, since it no longer fits its slot.

Metal Gear is wrapped at 45 and Metal Gear 2 at 46 characters; both stay under
what the recap texture can show.  No wording is changed - only page counts and
line breaks.
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
from workdir import WORK, require_decomp
import struct, sys
from gclparse import parse_script, containers_over, be16, be32
from gcldec import chain_at
def pad(x,a=2048): return (x+a-1)//a*a

REF=open(WORK + '/int1_stage.dir','rb').read()
US =open(WORK + '/us1_stage.dir','rb').read()
OVL=open(require_decomp() + '/obj/preope.bin','rb').read()
IB,UB=0x6A71*2048,0x7659*2048
scr_off=2048+pad(24911)+pad(113716)          # 0x23000
body=IB+scr_off+0x172
SCRIPT=21732

def rewrap(src, lo, hi, width):
    raw=[src[k][2].decode('latin1') for k in range(lo,hi)]
    paras=[]; cur=''
    for line in raw:
        if not line.strip():
            if cur: paras.append(cur); cur=''
            paras.append(None); continue
        if not cur: cur=line
        elif cur.endswith('-'): cur+=line
        else: cur+=' '+line
    if cur: paras.append(cur)
    out=[]
    for p in paras:
        if p is None: out.append(''); continue
        ln=''
        for w in p.split(' '):
            if not ln: ln=w
            elif len(ln)+1+len(w)<=width: ln+=' '+w
            else: out.append(ln); ln=w
        if ln: out.append(ln)
    while out and not out[-1].strip(): out.pop()
    return out

ci=chain_at(REF,IB+scr_off+0x1B8); cu=chain_at(US,UB+scr_off+0x1B8)
# rect.w is 128 in PreMet1_800C4E40, so font_set_kcb gives c_width
# (128 * 4) / 12 = 42 characters.  Anything longer wraps to a second
# line that c_height = 1 then clips, so 42 is the real limit.
mg1=rewrap(cu,4,95,42);  mg2=rewrap(cu,95,228,42)
P1,P2=(len(mg1)+7)//8,(len(mg2)+7)//8
S1,S2=P1*8,P2*8
print('MG1: %d lines -> %d pages (%d slots), longest %d'%(len(mg1),P1,S1,max(map(len,mg1))))
print('MG2: %d lines -> %d pages (%d slots), longest %d'%(len(mg2),P2,S2,max(map(len,mg2))))
assert (P1,P2)==(13,19), 'page counts must match the overlay build (13, 19)'

def rec(r): return bytes([0x07,r[1]])+r[2]+bytes(1)
def mk(s): b=s.encode('latin1'); return bytes([0x07,len(b)+1])+b+bytes(1)
def block(lines,slots):
    return b''.join(mk(l) for l in lines)+bytes([0x07,0x01,0x00])*(slots-len(lines))
new = b''.join(rec(ci[k]) for k in range(0,4)) + block(mg1,S1) + block(mg2,S2) \
    + b''.join(rec(ci[k]) for k in range(188,191))
span_i=sum(2+r[1] for r in ci); D=len(new)-span_i
NEWSCRIPT=SCRIPT+D
print('chain %d -> %d (D=%+d);  script %d -> %d, padded %d -> %d (+%d sectors)'
      %(span_i,len(new),D,SCRIPT,NEWSCRIPT,pad(SCRIPT),pad(NEWSCRIPT),(pad(NEWSCRIPT)-pad(SCRIPT))//2048))

# ---- rebuild the whole stage file with the new layout -------------------
old=bytearray(REF[IB:IB+87*2048])
ver,pdb,sect=struct.unpack('<BBh',old[:4])
tags=[list(struct.unpack('<HBBi',old[4+8*k:12+8*k])) for k in range(9)]
assert [chr(t[1])+chr(t[2]) for t in tags][:2]==['sb','nd']
assert tags[6][2]==0xFF and tags[6][3]==SCRIPT, tags[6]

# file-resident tags in order, with their retail file offsets
FILE=[0,1,6,7,8]
offs,o={},2048
for k in FILE: offs[k]=o; o+=pad(tags[k][3])
assert o==87*2048, o
payload={k:bytes(old[offs[k]:offs[k]+tags[k][3]]) for k in FILE}
payload[0]=OVL                                   # new overlay (12+17 pages)

# splice the new chain into the script payload
s=bytearray(payload[6])
root,slen=parse_script(REF,body)
cov=containers_over(root,IB+scr_off+0x1B8,IB+scr_off+0x1B8+span_i)
cs=0x1B8
s[cs:cs+span_i]=new
for c in cov:
    off=c.size_at-(IB+scr_off)
    if c.size_bits==32: struct.pack_into('>I',s,off,be32(s,off)+D)
    else:               struct.pack_into('>H',s,off,be16(s,off)+D)
payload[6]=bytes(s)
tags[0][3]=len(OVL); tags[6][3]=len(payload[6])

hdr=bytearray(2048)
newsect=(2048+sum(pad(len(payload[k])) for k in FILE))//2048
struct.pack_into('<BBh',hdr,0,ver,pdb,newsect)
for k,t in enumerate(tags): struct.pack_into('<HBBi',hdr,4+8*k,*t)
stage=bytearray(hdr)
for k in FILE: stage+=payload[k]+bytes(pad(len(payload[k]))-len(payload[k]))
assert len(stage)==newsect*2048
print('stage %d -> %d sectors (%d bytes); overlay %d, script %d'
      %(87,newsect,len(stage),len(OVL),len(payload[6])))

# ---- verify by re-parsing the rebuilt stage -----------------------------
V=bytes(stage); vscr=2048+pad(len(OVL))+pad(len(payload[1]))
assert vscr==2048+pad(len(OVL))+114688
r2,s2=parse_script(V,vscr+0x172); c2=chain_at(V,vscr+0x1B8)
assert s2==slen+D and r2.kids[0].end==vscr+0x172+s2
assert len(c2)==4+S1+S2+3, len(c2)
print('VERIFY: re-parse PASS, %d records (4 + %d + %d + 3), script_len %d -> %d'
      %(len(c2),S1,S2,slen,s2))
print('  MG1 first %r'%c2[4][2]);      print('  MG1 last  %r'%c2[3+len(mg1)][2])
print('  MG2 first %r'%c2[4+S1][2]);   print('  MG2 last  %r'%c2[3+S1+len(mg2)][2])
open(WORK + '/preope_en.bin','wb').write(bytes(stage))
print('wrote %s/preope_en.bin' % WORK)
