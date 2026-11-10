"""Decode D3D8/D3D9 vs_1_1 vertex-shader blobs (Shader\\skinmesh*.bin).

Token layout as observed in the shipped WY D 748 blobs:
  opcode token : bits 0..15 opcode, bits 16..23 control (shader <2.0), bit 31 = 0
  param  token : bits 0..10 register number, bits 11..12 register type,
                 bit 13 relative address flag (c#[a0.x]), bits 16..23
                 write mask (dest) or swizzle (source), bits 24..27 source
                 modifier, bits 28..30 = register type + 0? (see note), bit 31 = 1

The register type is stored in bits 28..30: 0=r(temp), 1=v(input), 2=c(constant),
3=a(address), 4=oPos, 5=oD(0/1=colour, 1 also used as oFog), 6=oT(texture coord).
Bit 31 is set on every parameter token and carries no information.

Caveat: the first record of every file opens with opcode 0x1F (D3DSIO_DCL in the
D3D9 numbering) which is not legal for the vs_1_1 version token that follows; those
records are printed as "dcl?" and their exact meaning is unknown.
"""
import struct, sys

OPS = {
 0:('nop',0), 1:('mov',2), 2:('add',3), 3:('sub',3), 4:('mad',4), 5:('mul',3),
 6:('rcp',2), 7:('rsq',2), 8:('dp3',3), 9:('dp4',3), 10:('min',3), 11:('max',3),
 12:('slt',3), 13:('sge',3), 14:('expp',2), 15:('logp',2), 16:('lit',2), 17:('dst',2),
 18:('lrp',4), 19:('frc',2), 20:('m4x4',3), 21:('m4x3',3), 22:('m3x4',3), 23:('m3x3',3),
 24:('m3x2',3), 25:('call',1), 26:('callnz',2), 27:('loop',1), 28:('ret',0),
 29:('endloop',0), 30:('label',1), 31:('dcl?',2),
}
RT = {0:'r', 1:'v', 2:'c', 3:'a', 4:'oPos', 5:'oD', 6:'oT'}
SW = 'xyzw'

def reg(tok, dest):
    num  = tok & 0x7FF
    t    = (tok >> 28) & 0x7
    body = (tok >> 16) & 0xFF
    mod  = (tok >> 24) & 0xF
    rel  = (tok >> 13) & 1
    s = '%s%d' % (RT.get(t, 'T%d' % t), num)
    if dest:
        m = body & 0xF
        if m != 0xF:
            s += '.' + ''.join(SW[i] for i in range(4) if m & (1 << i))
    else:
        swz = ''.join(SW[(body >> (2*i)) & 3] for i in range(4))
        if swz != 'xyzw':
            s += '.' + swz
        if rel:
            s += '[a0.x]'
    if mod:
        s = {1:'-', 2:'_bias', 3:'-bias', 4:'_x2', 5:'-x2',
             6:'_dz', 7:'_dw', 8:'|', 9:'-|'}.get(mod, '_m%d' % mod) + s
    return s

def disasm(path, out):
    d = open(path, 'rb').read()
    toks = list(struct.unpack('<%dI' % (len(d)//4), d))
    out("== %s  (%d bytes, %d DWORDs)" % (path.split('\\')[-1], len(d), len(toks)))
    out("   version token 0x%08X -> vs_%d_%d" % (toks[0], (toks[0] >> 8) & 0xFF, toks[0] & 0xFF))
    i = 1
    while i < len(toks):
        t = toks[i]
        op = t & 0xFFFF
        if op == 0xFFFF:
            out("   end")
            break
        if op == 0xFFFE:                              # comment
            ln = (t >> 16) & 0x7FFF
            body = toks[i+1:i+1+ln]
            out("   ; " + b''.join(struct.pack('<I', x) for x in body).split(b'\0')[0].decode('latin1'))
            i += 1 + ln
            continue
        name, np = OPS.get(op, ('UNKNOWN_0x%X' % op, 0))
        ps = [reg(x, k == 0) for k, x in enumerate(toks[i+1:i+1+np])]
        if op == 31:
            out("   dcl?    usage=%d idx=%d -> %s   ; opcode 0x1F, unexplained for vs_1_1"
                % (toks[i+1] & 0xF, (toks[i+1] >> 16) & 0xF, ps[1]))
        elif np == 0:
            out("   %s" % name)
        else:
            out("   %-7s %s" % (name, ', '.join(ps)))
        i += 1 + np
    out("")

if __name__ == '__main__':
    emit = sys.stdout.write
    for p in sys.argv[1:]:
        disasm(p, lambda s: emit(s + "\n"))
