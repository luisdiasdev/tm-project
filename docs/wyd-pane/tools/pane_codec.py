#!/usr/bin/env python3
"""
WYD .pane codec (also used for .wyp)

The on-disk .pane is a UTF-16LE text file whose bytes have been run through a
position-dependent delta transform by WYD.exe::FUN_00407580 (0x407580).

Decrypt (byte-wise, all arithmetic mod 256):
    P[0] = C[0] - 14
    P[i] = C[i] - C[i-1] - (i % 14)          for i >= 1

Encrypt is the exact inverse:
    C[0] = P[0] + 14
    C[i] = P[i] + C[i-1] + (i % 14)          for i >= 1

Note: the previous byte used on the right-hand side is always the *input*
byte (C[i-1] when decrypting, P[i-1] when encrypting), so both passes are
single, left/right-sequential loops.

Usage:
    python pane_codec.py decode <file.pane> [output.txt]
    python pane_codec.py encode <file.txt>  [output.pane]
    python pane_codec.py dump   <file.pane>
"""

import sys


def decrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    for i in range(n - 1, 0, -1):
        b[i] = (b[i] - b[i - 1] - (i % 14)) & 0xFF
    if n:
        b[0] = (b[0] - 14) & 0xFF
    return bytes(b)


def encrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    if n:
        b[0] = (b[0] + 14) & 0xFF
    for i in range(1, n):
        b[i] = (b[i] + b[i - 1] + (i % 14)) & 0xFF
    return bytes(b)


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cmd, src = argv[1], argv[2]
    raw = open(src, "rb").read()

    if cmd == "decode":
        text = decrypt(raw)
        dst = argv[3] if len(argv) > 3 else src + ".txt"
        open(dst, "wb").write(text)
        print(f"decoded {len(raw)} bytes -> {dst}")
        return 0

    if cmd == "encode":
        enc = encrypt(raw)
        dst = argv[3] if len(argv) > 3 else src + ".pane"
        open(dst, "wb").write(enc)
        print(f"encoded {len(raw)} bytes -> {dst}")
        return 0

    if cmd == "dump":
        text = decrypt(raw).decode("utf-16-le")
        print(text.lstrip("\ufeff"))
        return 0

    print("unknown command:", cmd)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
