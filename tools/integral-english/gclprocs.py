"""Dump every proc of a stage script (GCL_LoadScript layout: BE32 proclen, proc table
of (id:BE16, offset:BE16) ending in a zero word, then proc bodies, then the script)."""
import sys, struct; import os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from optscan import stage
from gclparse import be16, be32, parse_block, Node
sys.path.insert(0,'.')
from gcldump import val, dump
sd, name = sys.argv[1], sys.argv[2]; want = [int(x,16) for x in sys.argv[3:]]
sect,tags,F,pay=stage(sd,name)
for k in F:
    if chr(tags[k][1])!='c' or tags[k][3]==0: continue
    scr=pay[k]
    body=next((p for p in range(4,len(scr)-3) if scr[p]==0x40 and be32(scr,p-4)==be16(scr,p+1)+1), None)
    if body is None: continue
    script=body-4
    datatop=next((t for t in range(0,script) if be32(scr,t)+t+4==script), None)
    print('== %s tag %d datatop %X body %X' % (name,k,datatop,body))
    pt=datatop+4; procs=[]
    while be32(scr,pt)!=0:
        procs.append((be16(scr,pt), be16(scr,pt+2))); pt+=4
    pbody=pt+4
    for pid,off in procs:
        if want and pid not in want: continue
        p=pbody+off; assert scr[p]==0x40, hex(p)
        L=be16(scr,p+1); n=Node('ARG',p,p+1+L)
        try: parse_block(scr,p+3,n.end,n.kids)
        except Exception as e: print('  parse error',e)
        print('-- PROC %04X @%X len %d' % (pid,p,L)); dump(scr,n)
