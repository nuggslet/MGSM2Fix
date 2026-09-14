"""Report the right-hand submenu rows: top edge of each band and the gap
between consecutive tops (the row pitch)."""
import sys, glob
from PIL import Image
def bands(path,x0,x1,y0,y1,thr=170,minpx=8,minrun=4):
    im=Image.open(path).convert('RGB').crop((x0,y0,x1,y1)); w,h=im.size; px=im.load()
    rows=[[x for x in range(w) if px[x,y][1]>thr and px[x,y][1]>px[x,y][0]+30 and px[x,y][1]>px[x,y][2]+30] for y in range(h)]
    out=[];cur=None
    for y in range(h):
        if len(rows[y])>=minpx:
            if cur is None: cur=[y,y,[]]
            cur[1]=y; cur[2].extend(rows[y])
        else:
            if cur and cur[1]-cur[0]+1>=minrun: out.append(cur)
            cur=None
    if cur and cur[1]-cur[0]+1>=minrun: out.append(cur)
    return [(min(c[2])+x0,max(c[2])+x0,c[0]+y0,c[1]+y0) for c in out]
for p in sys.argv[1:]:
    # right panel only; the divider sits at ~2100 (mine) / ~1950 (USA)
    # find the vertical divider (a column green for many rows) and start after it
    im=Image.open(p).convert('RGB'); px=im.load()
    col=None
    for x in range(1900, 2200):
        n=sum(1 for y in range(620,1700,4) if px[x,y][1]>150 and px[x,y][1]>px[x,y][0]+30)
        if n > 180: col=x
    x0 = (col+14) if col else 1990
    b=[r for r in bands(p,x0,3400,600,1750) if r[1]-r[0] > 60]
    tops=[r[2] for r in b]
    gaps=[tops[i+1]-tops[i] for i in range(len(tops)-1)]
    print('%s  %d rows' % (p.split('/')[-1], len(b)))
    for i,(a,c,t,bo) in enumerate(b):
        g = ('  pitch %+4d' % gaps[i-1]) if i else ''
        print('     top %4d  h %-3d  x %4d..%-4d w %-5d%s' % (t,bo-t+1,a,c,c-a+1,g))
