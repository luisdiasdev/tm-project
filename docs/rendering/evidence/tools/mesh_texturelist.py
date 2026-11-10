import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
ROOT = r"E:\wyd\test-setup\client_748"

# --- MeshTextureList.bin : 2048 records of stTextureListInfo
d = open(os.path.join(MESH,'MeshTextureList.bin'),'rb').read()
SZ = 264
n = len(d)//SZ
print("MeshTextureList.bin size=%d  records=%d  remainder=%d" % (len(d), n, len(d)%SZ))
names = []
alpha = collections.Counter()
for i in range(n):
    rec = d[i*SZ:(i+1)*SZ]
    name = rec[:255].split(b'\x00')[0].decode('latin1')
    a = rec[255:256]
    names.append(name)
    if name: alpha[a] += 1
    if i < 3 or i in (2046,2047):
        print("  rec[%d] name=%r alpha=%r bytes255..263=%s" % (i, name, a, rec[255:264].hex(' ')))
print("alpha histogram (non-empty records):", {k.decode('latin1'):v for k,v in alpha.items()})
used = [x for x in names if x]
print("non-empty records:", len(used))
ext = collections.Counter(os.path.splitext(x)[1].lower() for x in used)
print("extensions:", dict(ext))
print("first 5:", used[:5])
print("last 5:", used[-5:])
dups = collections.Counter(used)
print("duplicate names (count>1):", [(k,v) for k,v in dups.items() if v>1][:10])

idx = {}
for i,x in enumerate(names):
    idx.setdefault(x, i)

# --- .msa texture names -> lookup
miss = collections.Counter(); hit = 0; tot = 0
for f in os.listdir(MESH):
    if not f.lower().endswith('.msa'): continue
    b = open(os.path.join(MESH,f),'rb').read()
    fvf, sv, att = struct.unpack_from('<3I', b, 0)
    off = 12 + att*20
    for i in range(att):
        raw = b[off:off+11]; off += 11
        base = raw.split(b'\x00')[0].decode('latin1')
        stem = base.split('.')[0]
        key = 'mesh\\%s.wys' % stem
        tot += 1
        if key in idx: hit += 1
        else: miss[key] += 1
print("\n.msa texture refs: total=%d resolved=%d (%.2f%%)  unresolved distinct=%d" % (tot, hit, 100.0*hit/tot, len(miss)))
print("unresolved samples:", list(miss)[:10])
