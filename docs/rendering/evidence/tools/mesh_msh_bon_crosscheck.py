import os, struct, collections
MESH = r"E:\wyd\test-setup\client_748\mesh"
msh = sorted([f for f in os.listdir(MESH) if f.lower().endswith('.msh')])
bon_cache = {}
def bon(path):
    if path not in bon_cache:
        d = open(path,'rb').read()
        bon_cache[path] = [struct.unpack_from('<iI', d, 8*i) for i in range(len(d)//8)]
    return bon_cache[path]

stats = collections.Counter(); samples = []
for f in msh:
    d = open(os.path.join(MESH,f),'rb').read()
    parent, mid, fvf, sv, infl, pal, vc, fc = struct.unpack_from('<8I', d, 0)
    names = [struct.unpack_from('<I', d, 32+pal*64+4*k)[0] for k in range(pal)]
    base = f[:4]
    path = os.path.join(MESH, base + '.bon')
    if not os.path.exists(path):
        cand = [x for x in os.listdir(MESH) if x.lower() == (base+'.bon').lower()]
        path = os.path.join(MESH, cand[0]) if cand else None
    if not path:
        stats['no_bon_file'] += 1
        continue
    bones = bon(path); nb = len(bones)
    ids = set(b[1] for b in bones)
    stats['files_checked'] += 1
    if mid not in ids: stats['mesh_id_not_a_bone'] += 1
    if any(x not in ids for x in names): stats['palette_name_not_a_bone'] += 1
    if names and max(names) >= nb: stats['palette_name_ge_numbone'] += 1
    if len(samples) < 8:
        samples.append((f, 'dwParentID=%d' % parent, 'dwID=%d' % mid, 'numPalette=%d' % pal,
                        'names=%s' % names[:8], 'bones=%d' % nb))
print(dict(stats))
for s in samples: print(s)
