#!/usr/bin/env python3
"""
texture_extract.py - convert WYD texture containers to standard formats.

Supported:
  .wyt -> .tga   drop the 4-byte "WT10" magic and append the TGA 2.0 signature
  .wyp -> .png   delta-decrypt the file (same cipher as .pane)
  .wys -> .dds   raw copy for now; the game repairs the DDS header in memory

Usage:
  python texture_extract.py <file.wyt|file.wyp|file.wys> [output]
"""

import os
import sys

TGA_SIGNATURE = b"TRUEVISION-XFILE.\0"


def decrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    for i in range(n - 1, 0, -1):
        b[i] = (b[i] - b[i - 1] - (i % 14)) & 0xFF
    if n:
        b[0] = (b[0] - 14) & 0xFF
    return bytes(b)


def convert(path: str) -> tuple[bytes, str]:
    ext = os.path.splitext(path)[1].lower()
    raw = open(path, "rb").read()

    if ext == ".wyt":
        if raw[:4] != b"WT10":
            raise ValueError("not a WYT file (missing WT10 magic)")
        # raw[4:] is a TGA header + pixel data + 8 zero bytes.
        return raw[4:] + TGA_SIGNATURE, ".tga"

    if ext == ".wyp":
        dec = decrypt(raw)
        if not dec.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ValueError("WYP did not decrypt to a PNG")
        return dec, ".png"

    if ext == ".wys":
        # The game repairs this in memory (branch 2 of FUN_004aea40).
        # Keep the raw bytes so the header repair can be implemented later.
        return raw, ".dds"

    raise ValueError("unsupported extension: " + ext)


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    src = argv[1]
    data, ext = convert(src)
    dst = argv[2] if len(argv) > 2 else os.path.splitext(src)[0] + ext
    with open(dst, "wb") as f:
        f.write(data)
    print(f"{src} -> {dst} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
