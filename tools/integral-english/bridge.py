"""Long-lived sqdbg bridge: reads expressions from cmd/*.sq, writes replies to out/*.txt."""
import socket, os, sys, time, glob

HERE = os.path.dirname(os.path.abspath(__file__))
CMD  = os.path.join(HERE, 'sqcmd'); OUT = os.path.join(HERE, 'sqout')
os.makedirs(CMD, exist_ok=True); os.makedirs(OUT, exist_ok=True)
LOG = open(os.path.join(HERE, 'bridge.log'), 'a', buffering=1)

def log(m): LOG.write('%s %s\n' % (time.strftime('%H:%M:%S'), m))

s = None
for attempt in range(600):
    try:
        s = socket.create_connection(('127.0.0.1', 27615), timeout=5)
        break
    except OSError:
        time.sleep(0.5)
if not s:
    log('CONNECT FAILED'); sys.exit(1)
log('connected')
s.settimeout(30)
f = s.makefile('r', encoding='latin1', newline='\n')
s.send(b'rd\n')
log('sent ready')

while True:
    files = sorted(glob.glob(os.path.join(CMD, '*.sq')))
    if not files:
        time.sleep(0.25); continue
    p = files[0]
    expr = open(p, 'r', encoding='utf-8').read().replace('\r', '').replace('\n', ' ').strip()
    os.remove(p)
    name = os.path.basename(p)[:-3]
    log('EVAL %s: %s' % (name, expr[:160]))
    try:
        s.send(b'ev:' + expr.encode('latin1') + b'\x00\n')
        reply = f.readline()
    except Exception as e:
        reply = 'ERROR %r' % (e,)
    open(os.path.join(OUT, name + '.txt'), 'w', encoding='utf-8').write(reply)
    log('REPLY %s: %s' % (name, reply.strip()[:300]))
