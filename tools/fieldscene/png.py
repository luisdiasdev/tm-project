"""Minimal RGBA image, PNG reader/writer and bitmap font (stdlib only).

This module deliberately has no third-party dependencies.  It is only used for
the offline reports and the optional screenshot overlay; the interactive viewer
draws with the browser canvas instead.
"""

from __future__ import annotations

import struct
import zlib
from pathlib import Path


# ---------------------------------------------------------------------------
# Image
# ---------------------------------------------------------------------------


class Image:
    """Simple 8-bit RGBA image stored as a flat bytearray."""

    __slots__ = ("width", "height", "data")

    def __init__(self, width: int, height: int, fill=(0, 0, 0, 0)):
        self.width = max(0, int(width))
        self.height = max(0, int(height))
        if isinstance(fill, int):
            fill = (fill, fill, fill, 255)
        self.data = bytearray(bytes(fill) * (self.width * self.height))

    # -- basic access -----------------------------------------------------
    def get(self, x: int, y: int):
        if x < 0 or y < 0 or x >= self.width or y >= self.height:
            return (0, 0, 0, 0)
        i = (y * self.width + x) * 4
        d = self.data
        return (d[i], d[i + 1], d[i + 2], d[i + 3])

    def put(self, x: int, y: int, rgba):
        if x < 0 or y < 0 or x >= self.width or y >= self.height:
            return
        i = (y * self.width + x) * 4
        self.data[i : i + 4] = bytes(rgba)

    def crop(self, x: int, y: int, w: int, h: int) -> "Image":
        out = Image(w, h)
        for yy in range(h):
            sy = y + yy
            if sy < 0 or sy >= self.height:
                continue
            for xx in range(w):
                sx = x + xx
                if sx < 0 or sx >= self.width:
                    continue
                si = (sy * self.width + sx) * 4
                di = (yy * w + xx) * 4
                out.data[di : di + 4] = self.data[si : si + 4]
        return out

    # -- compositing ------------------------------------------------------
    def blit(self, src: "Image", dx: int, dy: int, opacity: int = 255):
        """Blend ``src`` at 1:1, clipping to this image."""
        if opacity <= 0:
            return
        for sy in range(src.height):
            ty = dy + sy
            if ty < 0 or ty >= self.height:
                continue
            for sx in range(src.width):
                tx = dx + sx
                if tx < 0 or tx >= self.width:
                    continue
                si = (sy * src.width + sx) * 4
                sr, sg, sb, sa = src.data[si : si + 4]
                if opacity != 255:
                    sa = sa * opacity // 255
                _blend(self.data, (ty * self.width + tx) * 4, sr, sg, sb, sa)

    def blit_scaled(
        self,
        src: "Image",
        dx: int,
        dy: int,
        dw: int,
        dh: int,
        opacity: int = 255,
    ):
        """Nearest-neighbour scale of ``src`` into a ``dw`` x ``dh`` box."""
        if dw <= 0 or dh <= 0 or src.width <= 0 or src.height <= 0:
            return
        if dw == src.width and dh == src.height:
            self.blit(src, dx, dy, opacity)
            return
        for oy in range(dh):
            ty = dy + oy
            if ty < 0 or ty >= self.height:
                continue
            sy = oy * src.height // dh
            for ox in range(dw):
                tx = dx + ox
                if tx < 0 or tx >= self.width:
                    continue
                sx = ox * src.width // dw
                si = (sy * src.width + sx) * 4
                sr, sg, sb, sa = src.data[si : si + 4]
                if opacity != 255:
                    sa = sa * opacity // 255
                _blend(self.data, (ty * self.width + tx) * 4, sr, sg, sb, sa)

    def fill_rect(self, x: int, y: int, w: int, h: int, rgba):
        if isinstance(rgba, int):
            rgba = (rgba, rgba, rgba, 255)
        sr, sg, sb, sa = rgba
        for yy in range(y, y + h):
            if yy < 0 or yy >= self.height:
                continue
            row = yy * self.width
            for xx in range(x, x + w):
                if xx < 0 or xx >= self.width:
                    continue
                _blend(self.data, (row + xx) * 4, sr, sg, sb, sa)

    def outline_rect(self, x: int, y: int, w: int, h: int, rgba, thickness: int = 1):
        if w <= 0 or h <= 0:
            return
        t = max(1, thickness)
        self.fill_rect(x, y, w, t, rgba)
        self.fill_rect(x, y + h - t, w, t, rgba)
        self.fill_rect(x, y, t, h, rgba)
        self.fill_rect(x + w - t, y, t, h, rgba)

    # -- helpers ----------------------------------------------------------
    def to_png(self) -> bytes:
        return encode_png(self)

    def save(self, path):
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        Path(path).write_bytes(encode_png(self))


def _blend(dst: bytearray, i: int, sr: int, sg: int, sb: int, sa: int):
    if sa <= 0:
        return
    da = dst[i + 3]
    if sa >= 255 or da == 0:
        dst[i] = sr
        dst[i + 1] = sg
        dst[i + 2] = sb
        dst[i + 3] = sa
        return
    out_a = sa + da * (255 - sa) // 255
    if out_a == 0:
        dst[i] = dst[i + 1] = dst[i + 2] = dst[i + 3] = 0
        return
    dst[i] = (sr * sa + dst[i] * da * (255 - sa) // 255) // out_a
    dst[i + 1] = (sg * sa + dst[i + 1] * da * (255 - sa) // 255) // out_a
    dst[i + 2] = (sb * sa + dst[i + 2] * da * (255 - sa) // 255) // out_a
    dst[i + 3] = out_a


# ---------------------------------------------------------------------------
# PNG encode / decode
# ---------------------------------------------------------------------------


def _chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def encode_png(img: Image) -> bytes:
    w, h = img.width, img.height
    raw = bytearray()
    stride = w * 4
    for y in range(h):
        raw.append(0)  # filter: none
        raw += img.data[y * stride : (y + 1) * stride]
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(bytes(raw), 6))
        + _chunk(b"IEND", b"")
    )


def read_png(path) -> Image:
    """Decode a non-interlaced 8-bit RGB/RGBA PNG (enough for screenshots)."""
    data = Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG: %s" % path)
    off = 8
    width = height = bit_depth = color_type = interlace = 0
    idat = bytearray()
    while off + 8 <= len(data):
        length = struct.unpack_from(">I", data, off)[0]
        kind = data[off + 4 : off + 8]
        payload = data[off + 8 : off + 8 + length]
        off += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
        elif kind == b"IDAT":
            idat += payload
        elif kind == b"IEND":
            break
    if bit_depth != 8 or interlace != 0 or color_type not in (2, 6):
        raise ValueError(
            "unsupported PNG (depth=%d color=%d interlace=%d)"
            % (bit_depth, color_type, interlace)
        )
    channels = 4 if color_type == 6 else 3
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    out = Image(width, height)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        ftype = raw[pos]
        pos += 1
        row = bytearray(raw[pos : pos + stride])
        pos += stride
        _unfilter_row(row, prev, ftype, channels)
        # convert to RGBA
        di = y * width * 4
        if channels == 4:
            out.data[di : di + width * 4] = row
        else:
            for x in range(width):
                out.data[di + x * 4 : di + x * 4 + 4] = bytes(
                    (row[x * 3], row[x * 3 + 1], row[x * 3 + 2], 255)
                )
        prev = row
    return out


def _unfilter_row(row: bytearray, prev: bytearray, ftype: int, bpp: int):
    n = len(row)
    if ftype == 0:
        return
    if ftype == 1:  # Sub
        for i in range(bpp, n):
            row[i] = (row[i] + row[i - bpp]) & 0xFF
    elif ftype == 2:  # Up
        for i in range(n):
            row[i] = (row[i] + prev[i]) & 0xFF
    elif ftype == 3:  # Average
        for i in range(n):
            left = row[i - bpp] if i >= bpp else 0
            row[i] = (row[i] + ((left + prev[i]) >> 1)) & 0xFF
    elif ftype == 4:  # Paeth
        for i in range(n):
            a = row[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            p = a + b - c
            pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
            if pa <= pb and pa <= pc:
                pr = a
            elif pb <= pc:
                pr = b
            else:
                pr = c
            row[i] = (row[i] + pr) & 0xFF
    else:
        raise ValueError("unknown PNG filter %d" % ftype)


# ---------------------------------------------------------------------------
# 5x7 bitmap font (uppercase only, enough for IDs and labels)
# ---------------------------------------------------------------------------

_GLYPHS = {
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "C": ("01110", "10001", "10000", "10000", "10000", "10001", "01110"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "E": ("11111", "10000", "10000", "11110", "10000", "10000", "11111"),
    "F": ("11111", "10000", "10000", "11110", "10000", "10000", "10000"),
    "G": ("01110", "10001", "10000", "10111", "10001", "10001", "01111"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "J": ("00111", "00010", "00010", "00010", "00010", "10010", "01100"),
    "K": ("10001", "10010", "10100", "11000", "10100", "10010", "10001"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
    "Q": ("01110", "10001", "10001", "10001", "10101", "10010", "01101"),
    "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
    "S": ("01111", "10000", "10000", "01110", "00001", "00001", "11110"),
    "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
    "U": ("10001", "10001", "10001", "10001", "10001", "10001", "01110"),
    "V": ("10001", "10001", "10001", "10001", "10001", "01010", "00100"),
    "W": ("10001", "10001", "10001", "10101", "10101", "10101", "01010"),
    "X": ("10001", "10001", "01010", "00100", "01010", "10001", "10001"),
    "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
    "Z": ("11111", "00001", "00010", "00100", "01000", "10000", "11111"),
    "0": ("01110", "10001", "10011", "10101", "11001", "10001", "01110"),
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "3": ("11111", "00010", "00100", "00010", "00001", "10001", "01110"),
    "4": ("00010", "00110", "01010", "10010", "11111", "00010", "00010"),
    "5": ("11111", "10000", "11110", "00001", "00001", "10001", "01110"),
    "6": ("00110", "01000", "10000", "11110", "10001", "10001", "01110"),
    "7": ("11111", "00001", "00010", "00100", "01000", "01000", "01000"),
    "8": ("01110", "10001", "10001", "01110", "10001", "10001", "01110"),
    "9": ("01110", "10001", "10001", "01111", "00001", "00010", "01100"),
    " ": ("00000",) * 7,
    "-": ("00000", "00000", "00000", "11111", "00000", "00000", "00000"),
    "_": ("00000", "00000", "00000", "00000", "00000", "00000", "11111"),
    ".": ("00000", "00000", "00000", "00000", "00000", "00110", "00110"),
    ":": ("00000", "00110", "00110", "00000", "00110", "00110", "00000"),
    "/": ("00001", "00001", "00010", "00100", "01000", "10000", "10000"),
    "#": ("01010", "01010", "11111", "01010", "11111", "01010", "01010"),
    "?": ("01110", "10001", "00001", "00010", "00100", "00000", "00100"),
    "(": ("00010", "00100", "01000", "01000", "01000", "00100", "00010"),
    ")": ("01000", "00100", "00010", "00010", "00010", "00100", "01000"),
    "[": ("01110", "01000", "01000", "01000", "01000", "01000", "01110"),
    "]": ("01110", "00010", "00010", "00010", "00010", "00010", "01110"),
    "+": ("00000", "00100", "00100", "11111", "00100", "00100", "00000"),
    "=": ("00000", "00000", "11111", "00000", "11111", "00000", "00000"),
    ",": ("00000", "00000", "00000", "00000", "00110", "00110", "01100"),
    "%": ("11001", "11010", "00010", "00100", "01000", "01011", "10011"),
}


def text_width(text: str, scale: int = 1) -> int:
    return max(0, len(text) * 6 * scale - scale)


def draw_text(img: Image, x: int, y: int, text: str, rgba, scale: int = 1):
    """Draw ``text`` with the built-in font.  Lowercase is folded to uppercase."""
    if isinstance(rgba, int):
        rgba = (rgba, rgba, rgba, 255)
    r, g, b, a = rgba
    px = x
    for ch in str(text):
        glyph = _GLYPHS.get(ch.upper(), _GLYPHS["?"])
        for row_idx, bits in enumerate(glyph):
            for col_idx, bit in enumerate(bits):
                if bit != "1":
                    continue
                for oy in range(scale):
                    ty = y + row_idx * scale + oy
                    if ty < 0 or ty >= img.height:
                        continue
                    for ox in range(scale):
                        tx = px + col_idx * scale + ox
                        if tx < 0 or tx >= img.width:
                            continue
                        _blend(img.data, (ty * img.width + tx) * 4, r, g, b, a)
        px += 6 * scale
    return px


def draw_text_shadow(img: Image, x: int, y: int, text: str, rgba, scale: int = 1):
    draw_text(img, x + scale, y + scale, text, (0, 0, 0, 200), scale)
    draw_text(img, x, y, text, rgba, scale)
