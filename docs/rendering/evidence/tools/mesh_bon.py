import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
bon = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.bon')])
print("bon files:", len(bon))
bad = []
roots = 0
for f in bon:
    d = open(os.path.join(MESH,f),'rb').read()
    if len(d) % 8:
        bad.append((f, len(d), len(d) % 8)); continue
    n = len(d)//8
    pairs = [struct.unpack_from('<Ii', d, 8*i) for i in range(n)]
    ids = [p[1] for p in pairs]
    ok_ids = (len(set(ids)) == len(ids)) and max(ids) == n-1 and min(ids) == 0
    parents_ok = all((-1 <= p[0] < n) and p[0] != p[1] for p in pairs)
    np_roots = sum(1 for p in pairs if p[0] == -1)
    roots += np_roots
    if not (ok_ids and parents_ok) and f not in ('tn.bon','ed.bon'):
        bad.append((f, len(d), 'ids_ok=%s parents_ok=%s roots=%d' % (ok_ids, parents_ok, np_roots)))
print("files whose size is not a multiple of 8:", [b for b in bad if isinstance(b[1], int) and isinstance(b[2], int) and b[2] != 0])
print("structural anomalies (non tn/ed):", [b for b in bad if not (isinstance(b[2], int) and b[2] != 0)])
print("total root bones across all files:", roots)
print("\nch01.bon first 10 (parent,id):", [struct.unpack_from('<Ii', open(os.path.join(MESH,'ch01.bon'),'rb').read(), 8*i) for i in range(10)])
print("ch02.bon first 10 (parent,id):", [struct.unpack_from('<Ii', open(os.path.join(MESH,'ch02.bon'),'rb').read(), 8*i) for i in range(10)])
for f in ('tn.bon','ed.bon','BL01.bon','mi010101.bon','mi01.bon'):
    d = open(os.path.join(MESH,f),'rb').read()
    print("%-14s size=%-8d size%%8=%d  head=%s" % (f, len(d), len(d)%8, d[:32].hex(' ')))
