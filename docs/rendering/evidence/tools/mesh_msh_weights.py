import os, struct, collections

MESH = r"E:\wyd\test-setup\client_748\mesh"
def fl(b,o): return struct.unpack_from('<f', b, o)[0]

msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
bad_gt1 = 0; bad_neg = 0; total = 0; worst = 0.0; zero_w = 0
dist = collections.Counter()
for f in msh:
    d = open(os.path.join(MESH,f),'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    off = 32 + pal*64 + pal*4
    nw = infl-1
    if nw == 0:
        continue
    for v in range(vc):
        base = off + v*sv
        w = [fl(d, base+12+4*k) for k in range(nw)]
        total += 1
        s = sum(w)
        if s > 1.0 + 1e-4: bad_gt1 += 1; worst = max(worst, s)
        if any(x < -1e-6 for x in w): bad_neg += 1
        dist[round(s,1)] += 1
print("vertices with >=1 float weight:", total)
print("sum(weights) > 1.0 :", bad_gt1, " worst:", round(worst,4))
print("negative weights   :", bad_neg)
print("distribution of round(sum,1):", dict(sorted(dist.items())))
