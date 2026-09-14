"""Dump a stage script's command tree with decoded values."""
import sys, struct; import os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from optscan import stage
from gclparse import parse_script, be16, be32, walk_tree
def val(d, n):
    k=n.kind; p=n.pos
    if k=='VAR': return '$%08X' % be32(d,p)
    if k=='SHORT': return '%d' % struct.unpack('>h', d[p+1:p+3])[0]
    if k=='BYTE': return '%s%d' % ({2:'b',3:'c',4:'f'}[d[p]], d[p+1])
    if k=='STRID': return '%s%04X' % ('str' if d[p]==6 else 'proc', be16(d,p+1))
    if k=='INT': return '%d' % struct.unpack('>i', d[p+1:p+5])[0]
    if k=='STRING': return repr(bytes(d[n.pos+2:n.end]))
    if k=='OPTION':
        L=d[p+2]; inner=[]; q=p+3
        from gclparse import parse_values
        try: parse_values(d, q, n.end, inner)
        except Exception: pass
        parts=[]
        for x in inner:
            if x.kind=='END': continue
            if x.kind=='ARG':
                sub=[]
                for y in x.kids:
                    if y.kind=='COMMAND':
                        kk=[z for z in y.kids if z.kind not in ('OFS','END')]
                        sub.append('CMD %04X[%s]' % (be16(d,y.pos+3), ' '.join(val(d,z) for z in kk)))
                    elif y.kind=='PROC': sub.append('PROC %04X[%s]' % (be16(d,y.pos+2), ' '.join(val(d,z) for z in y.kids if z.kind!='END')))
                    elif y.kind=='EXPR': sub.append('expr[%s]' % d[y.pos+2:y.end].hex())
                parts.append('{ ' + ' ; '.join(sub) + ' }')
            else: parts.append(val(d,x))
        return '-%c(%s)' % (d[p+1], ' '.join(parts))
    if k=='EXPR': return 'expr[%s]' % d[p+2:n.end].hex()
    if k=='ARRAY': return 'arr'
    return k
def dump(d, root):
    for n, depth in walk_tree(root):
        if n.kind=='COMMAND':
            kids=[x for x in n.kids if x.kind not in ('OFS','END')]
            print('  '*depth+'CMD %04X @%X: %s' % (be16(d,n.pos+3), n.pos, ' '.join(val(d,x) for x in kids)))
        elif n.kind=='PROC':
            print('  '*depth+'PROC %04X @%X: %s' % (be16(d,n.pos+2), n.pos, ' '.join(val(d,x) for x in n.kids if x.kind!='END')))
        elif n.kind=='ARG' and depth>1:
            print('  '*depth+'{ @%X' % n.pos)
sd, name = sys.argv[1], sys.argv[2]
sect,tags,F,pay=stage(sd,name)
for k in F:
    if chr(tags[k][1])!='c' or tags[k][3]==0: continue
    scr=pay[k]
    # find body: script_len BE32 then 0x40
    body=None
    for p in range(4,len(scr)-3):
        if scr[p]==0x40 and be32(scr,p-4)==be16(scr,p+1)+1: body=p; break
    if body is None: print('== %s tag %d: no body'%(name,k)); continue
    print('== %s tag %d len %d body @%X' % (name,k,len(scr),body))
    root,slen=parse_script(scr, body); dump(scr, root)
