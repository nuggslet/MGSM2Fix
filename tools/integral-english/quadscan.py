"""Report brf_800C983C(prim, tex_id, poly, xl, yt, xr, yb, ...) arguments,
labelling each call by the resource-name string loaded just before it."""
import struct
CALLER_SAVED=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,24,25]
def scan(blob, base, drawfn):
    W=list(struct.unpack('<%dI'%(len(blob)//4), blob[:len(blob)//4*4]))
    jal=0x0C000000 | ((drawfn>>2)&0x03FFFFFF)
    reg=[None]*32; reg[0]=0; hi={}; stack={}; name=None; out=[]; pending=None
    src={}   # register -> address of the instruction that last set it
    def setr(r,v):
        if r: reg[r]=v
    def cstr(a):
        o=a-base
        if not (0<=o<len(blob)): return None
        e=blob.find(b'\0',o)
        try: return blob[o:e].decode('latin1')
        except Exception: return None
    for i,w in enumerate(W):
        a=base+4*i
        op=w>>26; rs,rt,rd=(w>>21)&31,(w>>16)&31,(w>>11)&31
        sa,f6=(w>>6)&31,w&0x3F
        imm=w&0xFFFF; simm=imm-0x10000 if imm>=0x8000 else imm
        x,y=reg[rs],reg[rt]
        if op==0x0F: hi[rt]=imm<<16; setr(rt,imm<<16); src[rt]=a
        elif op==9:
            if rs in hi and rt==rs:
                s=cstr(hi[rs]+simm)
                if s and s.startswith('br_'): name=s
                hi.pop(rs,None); setr(rt,None)
            else:
                setr(rt, None if x is None else x+simm); src[rt]=a
        elif op==0x0D: setr(rt, None if x is None else x|imm); src[rt]=a
        elif op==0x0C: setr(rt, None if x is None else x&imm)
        elif op==0x0E: setr(rt, None if x is None else x^imm)
        elif op==0x2B and rs==29: stack[simm]=y
        elif op==0:
            if f6 in (0x20,0x21): setr(rd, None if (x is None or y is None) else x+y); src[rd]=a
            elif f6 in (0x22,0x23): setr(rd, None if (x is None or y is None) else x-y)
            elif f6==0x25: setr(rd, None if (x is None or y is None) else x|y)
            elif f6==0x24: setr(rd, None if (x is None or y is None) else x&y)
            elif f6==0x00: setr(rd, None if y is None else (y<<sa)&0xFFFFFFFF)
            elif f6==0x02: setr(rd, None if y is None else (y&0xFFFFFFFF)>>sa)
            elif f6==0x03: setr(rd, None if y is None else y>>sa)
            elif f6 in (8,9): pass
            else: setr(rd,None)
        elif op in (0x20,0x21,0x23,0x24,0x25,0x0A,0x0B): setr(rt,None)
        if pending is not None:
            out.append((pending,name,reg[7],stack.get(16),stack.get(20),stack.get(24),src.get(7)))
            pending=None
            for r in CALLER_SAVED: reg[r]=None
        if w==jal: pending=a
    return out
