#!/usr/bin/env python3
"""
disasm_dx8.py -- minimal disassembler for the Direct3D 8/9 *legacy* shader
bytecode used by the WYD 748 client (vs_1_1 / ps_1_1).

Token model (validated byte-for-byte against the matching .vsh/.psh sources):

  dword 0 : version token. 0xFFFE0101 = vs_1_1, 0xFFFF0101 = ps_1_1.
  0x0000FFFE in low 16 bits -> COMMENT token, size in DWORDs in high 16 bits.
  bit 31 == 0  -> instruction token; opcode == bits 0..15, controls in 16..23.
  bit 31 == 1  -> parameter token:
        bits 0..10   register number
        bit  13      relative addressing (c[a0.x + n])
        bits 16..23  dest write mask (bits 16..19) + dest modifier (20..23),
                     or source swizzle (2 bits per component, x in 16..17)
        bits 24..27  source modifier / dest modifier
        bits 28..30  register type (see REGTYPE)

Usage: python disasm_dx8.py <file.bin> [more.bin ...]
"""
import sys, struct

REGTYPE = {0: 'r', 1: 'v', 2: 'c', 3: 'a0/t', 4: 'oPos/oFog', 5: 'oD', 6: 'oT'}

VS_OPS = {
    0x00: 'nop', 0x01: 'mov', 0x02: 'add', 0x03: 'sub', 0x04: 'mad', 0x05: 'mul',
    0x06: 'rcp', 0x07: 'rsq', 0x08: 'dp3', 0x09: 'dp4', 0x0A: 'min', 0x0B: 'max',
    0x0C: 'slt', 0x0D: 'sge', 0x0E: 'exp', 0x0F: 'log', 0x10: 'lit', 0x11: 'dst',
    0x12: 'lrp', 0x13: 'frc', 0x14: 'm4x4', 0x15: 'm4x3', 0x16: 'm3x4', 0x17: 'm3x3',
    0x18: 'm3x2', 0x1F: 'dcl', 0x20: 'pow', 0x21: 'crs', 0x22: 'sgn', 0x23: 'abs',
}
PS_OPS = dict(VS_OPS)
PS_OPS.update({
    0x40: 'texcoord', 0x41: 'texkill', 0x42: 'tex', 0x43: 'texbem',
    0x44: 'texbeml', 0x45: 'texreg2ar', 0x46: 'texreg2gb', 0x47: 'texm3x2pad',
    0x48: 'texm3x2tex', 0x49: 'texm3x3pad', 0x4A: 'texm3x3tex',
    0x4B: 'reserved_4b', 0x4C: 'texm3x3spec', 0x4D: 'texm3x3vspec',
    0x50: 'cnd', 0x51: 'def', 0x52: 'texreg2rgb', 0x53: 'texdp3tex',
    0x54: 'texm3x2depth', 0x55: 'texdp3', 0x56: 'texm3x3', 0x57: 'texdepth',
})

SRC_MOD = {0: '', 1: '_neg', 2: '_bias', 3: '_biasneg', 4: '_bx2', 5: '_bx2neg',
           6: '_x2', 7: '_x2neg', 8: '_dz', 9: '_dw'}
SWZ = 'xyzw'


def swizzle(tok):
    """Return swizzle string or None if identity."""
    v = (tok >> 16) & 0xFF
    comps = [(v >> (2 * i)) & 3 for i in range(4)]
    s = ''.join(SWZ[c] for c in comps)
    return None if s == 'xyzw' else s


def regnum(tok):
    return tok & 0x7FF


def regtype(tok):
    return (tok >> 28) & 7


def relflag(tok):
    return bool(tok & 0x2000)


def src(tok, is_ps):
    t = regtype(tok)
    n = regnum(tok)
    if t == 0:
        base = 'r%d' % n
    elif t == 1:
        base = 'v%d' % n
    elif t == 2:
        base = 'c%d' % n
        if relflag(tok):
            base = 'c[a0.x+%d]' % n
    elif t == 3:
        base = ('t%d' % n) if is_ps else 'a0'
    elif t == 4:
        base = ('%d' % n) if is_ps else ('oPos' if n == 0 else 'oFog')
    elif t == 5:
        base = ('%d' % n) if is_ps else 'oD%d' % n
    elif t == 6:
        base = 'oT%d' % n
    else:
        base = '?type%d[%d]' % (t, n)
    s = swizzle(tok)
    if s:
        base += '.' + s
    mod = (tok >> 24) & 0xF
    base += SRC_MOD.get(mod, '_mod%d' % mod)
    return base


def dst(tok, is_ps):
    t = regtype(tok)
    n = regnum(tok)
    mask = (tok >> 16) & 0xF
    dm = (tok >> 20) & 0xF
    if t == 0:
        base = 'r%d' % n
    elif t == 1:
        base = 'v%d' % n
    elif t == 2:
        base = 'c%d' % n
    elif t == 3:
        base = ('t%d' % n) if is_ps else 'a0'
    elif t == 4:
        base = ('%d' % n) if is_ps else ('oPos' if n == 0 else 'oFog')
    elif t == 5:
        base = ('r%d' % n) if is_ps else ('oD%d' % n)
    elif t == 6:
        base = 'oT%d' % n
    else:
        base = '?type%d[%d]' % (t, n)
    if mask and mask != 0xF:
        base += '.' + ''.join(SWZ[i] for i in range(4) if mask & (1 << i))
    if dm & 1:
        base += '_sat'
    return base


def disasm(path):
    data = open(path, 'rb').read()
    w = list(struct.unpack('<%dI' % (len(data) // 4), data[:len(data) // 4 * 4]))
    out = []
    ver = w[0]
    is_ps = (ver >> 16) == 0xFFFF
    out.append('; file=%s  bytes=%d  version=0x%08X (%s)'
               % (path, len(data), ver, 'ps_1_1' if is_ps else 'vs_1_1'))
    ops = PS_OPS if is_ps else VS_OPS
    i = 1
    while i < len(w):
        tok = w[i]
        if (tok & 0xFFFF) == 0xFFFE and not (tok & 0x80000000):
            n = tok >> 16
            blob = data[i * 4 + 4:(i + 1 + n) * 4]
            txt = ''.join(chr(c) if 32 <= c < 127 else '.' for c in blob)
            out.append('  ; COMMENT (%d dwords): %s' % (n, txt))
            i += 1 + n
            continue
        op = tok & 0xFFFF
        if op == 0xFFFF:
            out.append('  end')
            i += 1
            continue
        name = ops.get(op, 'op_%02x' % op)
        ctrl = (tok >> 16) & 0xFF
        # operand counts: number of parameter tokens following the opcode token
        # (dest + N sources, validated against the .vsh/.psh source text)
        nops = {
            'nop': 0, 'texkill': 0, 'end': 0,
            'mov': 2, 'rcp': 2, 'rsq': 2, 'exp': 2, 'log': 2, 'lit': 2, 'frc': 2,
            'abs': 2, 'sgn': 2, 'pow': 3, 'tex': 1, 'texcoord': 1,
            'dcl': 2,
            'add': 3, 'sub': 3, 'mul': 3, 'dp3': 3, 'dp4': 3, 'min': 3, 'max': 3,
            'slt': 3, 'sge': 3, 'dst': 3, 'crs': 3,
            'm4x4': 3, 'm4x3': 3, 'm3x4': 3, 'm3x3': 3, 'm3x2': 3,
            'mad': 4, 'lrp': 4, 'cnd': 4,
            'texm3x3pad': 2, 'texm3x3tex': 2, 'texm3x3vspec': 2,
            'texm3x3spec': 4, 'texm3x2pad': 3, 'texm3x2tex': 3,
            'texreg2ar': 2, 'texreg2gb': 2, 'texreg2rgb': 2,
            'texbem': 3, 'texbeml': 3, 'texdp3tex': 2, 'texdp3': 2, 'texm3x3': 2,
        }.get(name, 3)
        if name == 'def':
            d = w[i + 1]
            vals = struct.unpack('<4f', struct.pack('<4I', *w[i + 2:i + 6]))
            out.append('  %-8s %s, %s' % ('def', dst(d, is_ps),
                                          ', '.join('%.6g' % v for v in vals)))
            i += 6
            continue
        args = [name]
        if name == 'dcl':
            decl = w[i + 1]
            usage = decl & 0x1F
            uidx = (decl >> 16) & 0xF
            args.append('%s v%d' % (['position', 'blendweight', 'blendindices',
                                     'normal', 'psize', 'texcoord', 'tangent',
                                     'binormal', 'tessfactor', 'positiont',
                                     'color', 'fog', 'depth', 'sample'][usage]
                                    if usage < 14 else 'usage%d' % usage, regnum(w[i + 2])))
            if uidx:
                args[1] += ' (index %d)' % uidx
            out.append('  ' + ' '.join(args))
            i += 3
            continue
        for k in range(nops):
            t = w[i + 1 + k]
            if k == 0:
                args.append(dst(t, is_ps) if nops > 1 else src(t, is_ps))
            else:
                args.append(src(t, is_ps))
        s = '  %-8s %s' % (name, ', '.join(args[1:]))
        if ctrl:
            s += '   ; ctrl=0x%03x' % ctrl
        out.append(s)
        i += 1 + nops
    return '\n'.join(out)


if __name__ == '__main__':
    for p in sys.argv[1:]:
        print(disasm(p))
        print()
