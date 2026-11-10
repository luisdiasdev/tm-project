import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
def fl(b,o): return struct.unpack_from('<f', b, o)[0]
msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
pat = collections.Counter(); maxes = collections.Counter()
samples=[]
for f in msh:
    d = open(os.path.join(MESH,f),'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    if infl not in (2,4): continue
    off = 32 + pal*64 + pal*4; nw = infl-1
    for v in range(vc):
        base = off + v*sv
        w0 = fl(d, base+12)
        if not (0.05 < w0 < 0.95): continue
        packed = struct.unpack_from('<I', d, base+12+4*nw)[0]
        by = [(packed >> (8*k)) & 0xFF for k in range(4)]
        key = (infl, tuple(1 if b else 0 for b in by))
        pat[key] += 1
        for k in range(4): maxes[(infl,k)] = max(maxes[(infl,k)], by[k])
        if len(samples)<10: samples.append((f,infl,round(w0,3),hex(packed),by,pal))
for k,c in sorted(pat.items()): print(k, c)
print("max byte value per position:", dict(maxes))
for s in samples: print(s)
