import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
def fl(b,o): return struct.unpack_from('<f', b, o)[0]
msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
cnt = collections.Counter()
sample = []
for f in msh:
    d = open(os.path.join(MESH,f),'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    if infl < 2: continue
    off = 32 + pal*64 + pal*4
    nw = infl-1
    for v in range(vc):
        base = off + v*sv
        w0 = fl(d, base+12)
        if w0 < 0.999: continue
        packed = struct.unpack_from('<I', d, base+12+4*nw)[0]
        by = [(packed >> (8*k)) & 0xFF for k in range(4)]
        cnt['rigid_vertices'] += 1
        if by[2] == 0: cnt['byte2_zero'] += 1
        if by[0] == 0: cnt['byte0_zero'] += 1
        if by[1] == 0: cnt['byte1_zero'] += 1
        if by[3] == 0: cnt['byte3_zero'] += 1
        if len(sample) < 12: sample.append((f, infl, pal, hex(packed), by))
print(dict(cnt))
for s in sample: print(s)
