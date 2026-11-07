"""748 UI asset readers: texture list, texture sets and WYT/TGA images."""

from __future__ import annotations

import os
import re
import struct
from dataclasses import dataclass
from pathlib import Path

from .png import Image


@dataclass(frozen=True)
class TextureCoord:
    tex: int
    sx: int
    sy: int
    w: int
    h: int
    dx: int
    dy: int


@dataclass
class TextureSet:
    index: int
    name: str
    coords: list


def normalize_asset_name(name: str) -> str:
    """``UI\\\\MainBox2.wyt`` -> ``UI/MainBox2.wyt``."""
    n = name.strip().replace("\\", "/")
    n = re.sub(r"/+", "/", n)
    while n.startswith("./"):
        n = n[2:]
    return n


class TextureList:
    """``UI/UITextureList.bin``: 512 records of 264 bytes."""

    RECORD = 264

    def __init__(self, path: Path | str):
        self.path = Path(path)
        self.filenames: list[str] = []
        self.alphas: list[str] = []
        data = self.path.read_bytes() if self.path.exists() else b""
        count = len(data) // self.RECORD
        for i in range(count):
            rec = data[i * self.RECORD : (i + 1) * self.RECORD]
            name = rec[:255].split(b"\0", 1)[0].decode("latin-1")
            self.filenames.append(normalize_asset_name(name))
            self.alphas.append(chr(rec[255]) if len(rec) > 255 else "?")

    def __len__(self):
        return len(self.filenames)

    def filename(self, index: int) -> str:
        if 0 <= index < len(self.filenames):
            return self.filenames[index]
        return ""

    def alpha(self, index: int) -> str:
        if 0 <= index < len(self.alphas):
            return self.alphas[index]
        return "?"


class TextureSetList:
    """``UI/UITextureSetList.txt``."""

    def __init__(self, path: Path | str):
        self.path = Path(path)
        self.sets: dict[int, TextureSet] = {}
        self.warnings: list[str] = []
        if not self.path.exists():
            return
        lines = [line.strip() for line in self.path.read_text(encoding="latin-1").splitlines()]
        i = 0
        while i < len(lines):
            m = re.match(r"SetIndex:\s*(-?\d+)", lines[i])
            if not m:
                i += 1
                continue
            index = int(m.group(1))
            name = lines[i - 1].strip().strip("[]") if i > 0 else "?"
            count = 0
            if i + 1 < len(lines):
                mc = re.match(r"ItemCount:\s*(\d+)", lines[i + 1])
                if mc:
                    count = int(mc.group(1))
            coords = []
            j = i + 2
            while j < len(lines) and re.match(r"^-?\d+,", lines[j]):
                try:
                    parts = [int(x) for x in lines[j].split(",")]
                except ValueError:
                    break
                if len(parts) >= 7:
                    coords.append(TextureCoord(*parts[:7]))
                j += 1
            if len(coords) != count:
                self.warnings.append(
                    "texture set %d (%s) says %d items, found %d"
                    % (index, name, count, len(coords))
                )
            self.sets[index] = TextureSet(index, name, coords)
            i = j

    def __len__(self):
        return len(self.sets)

    def get(self, index: int):
        return self.sets.get(index)


class AssetLibrary:
    """Resolves a control's texture-set index to a cropped RGBA image."""

    def __init__(self, asset_root: Path | str):
        self.root = Path(asset_root)
        self.texture_list = TextureList(self.root / "UI" / "UITextureList.bin")
        self.set_list = TextureSetList(self.root / "UI" / "UITextureSetList.txt")
        self.warnings: list[str] = list(self.set_list.warnings)
        self._image_cache: dict[str, Image | None] = {}
        self._crop_cache: dict[tuple, Image | None] = {}
        self._warned: set[str] = set()

    # -- names ------------------------------------------------------------
    def set_name(self, set_index: int) -> str:
        s = self.set_list.get(set_index)
        return s.name if s else ""

    def set_filename(self, set_index: int, frame: int = 0) -> str:
        """WYT filename used by a set frame (for cross-version matching)."""
        info = self.frame_info(set_index, frame, decode=False)
        return info.get("file", "") if info else ""

    def set_basename(self, set_index: int, frame: int = 0) -> str:
        name = self.set_filename(set_index, frame)
        return os.path.basename(name).lower() if name else ""

    # -- frames -----------------------------------------------------------
    def resolved_set_index(self, tex: int):
        """Return the set index for a positive/``< -2`` texture id."""
        if tex >= 0:
            return tex
        if tex < -2:
            return -tex
        return None

    def frame_info(self, tex: int, frame: int = 0, decode: bool = True):
        """Return metadata for a control texture id + frame index."""
        set_index = self.resolved_set_index(tex)
        if set_index is None:
            return {
                "set": None,
                "frame": frame,
                "solid": -1 if tex == -1 else -2,
                "name": "",
                "file": "",
                "rect": None,
                "dest": None,
                "image": None,
            }
        texture_set = self.set_list.get(set_index)
        if texture_set is None:
            self._warn("missing texture set %d" % set_index)
            return None
        if not texture_set.coords:
            self._warn("empty texture set %d (%s)" % (set_index, texture_set.name))
            return None
        if frame < 0:
            frame = 0
        if frame >= len(texture_set.coords):
            frame = len(texture_set.coords) - 1
        coord = texture_set.coords[frame]
        filename = self.texture_list.filename(coord.tex)
        info = {
            "set": set_index,
            "frame": frame,
            "solid": None,
            "name": texture_set.name,
            "file": filename,
            "rect": [coord.sx, coord.sy, coord.w, coord.h],
            "dest": [coord.dx, coord.dy],
            "image": None,
        }
        if decode:
            info["image"] = self.frame_image(set_index, frame)
        return info

    def frame_image(self, tex: int, frame: int = 0) -> Image | None:
        key = (tex, frame)
        if key in self._crop_cache:
            return self._crop_cache[key]
        info = self.frame_info(tex, frame, decode=False)
        if not info or info.get("solid") is not None:
            self._crop_cache[key] = None
            return None
        coord = None
        texture_set = self.set_list.get(info["set"])
        if texture_set and info["frame"] < len(texture_set.coords):
            coord = texture_set.coords[info["frame"]]
        if coord is None:
            self._crop_cache[key] = None
            return None
        image = self.texture_image(coord.tex)
        if image is None:
            self._crop_cache[key] = None
            return None
        crop = image.crop(coord.sx, coord.sy, max(0, coord.w), max(0, coord.h))
        self._crop_cache[key] = crop
        return crop

    def texture_image(self, tex_index: int) -> Image | None:
        name = self.texture_list.filename(tex_index)
        if not name:
            self._warn("texture %d has no filename" % tex_index)
            return None
        return self.load_texture(name)

    def load_texture(self, name: str) -> Image | None:
        name = normalize_asset_name(name)
        if name in self._image_cache:
            return self._image_cache[name]
        path = self.root / name
        if not path.exists():
            self._warn("missing texture file %s" % name)
            self._image_cache[name] = None
            return None
        suffix = path.suffix.lower()
        if suffix == ".wys":
            # DDS/DXT is irrelevant for FieldScene2 except one mob portrait.
            self._warn("DDS texture not decoded: %s" % name)
            self._image_cache[name] = None
            return None
        if suffix not in (".wyt", ".tga"):
            self._warn("unsupported texture format: %s" % name)
            self._image_cache[name] = None
            return None
        try:
            img = decode_wyt(path.read_bytes(), has_magic=(suffix == ".wyt"))
        except Exception as exc:  # noqa: BLE001 - surface as an asset warning
            self._warn("cannot decode %s: %s" % (name, exc))
            img = None
        self._image_cache[name] = img
        return img

    # -- warnings ---------------------------------------------------------
    def _warn(self, text: str):
        if text not in self._warned:
            self._warned.add(text)
            self.warnings.append(text)

    def unique_warnings(self):
        return list(dict.fromkeys(self.warnings))


def decode_wyt(data: bytes, has_magic: bool = True) -> Image:
    """Decode a 748 ``.wyt``: ``WT10`` + TGA image (+ 8 trailing bytes)."""
    base = 0
    if has_magic:
        if data[:4] != b"WT10":
            raise ValueError("bad WYT magic %r" % data[:4])
        base = 4
    if len(data) < base + 18:
        raise ValueError("short TGA header")
    tga = data[base:]
    id_length = tga[0]
    color_map_type = tga[1]
    image_type = tga[2]
    width = tga[12] | (tga[13] << 8)
    height = tga[14] | (tga[15] << 8)
    bpp = tga[16]
    descriptor = tga[17]
    if color_map_type != 0:
        raise ValueError("colour-mapped TGA is not supported")
    if image_type not in (2, 10):
        raise ValueError("unsupported TGA image type %d" % image_type)
    if bpp not in (16, 24, 32):
        raise ValueError("unsupported TGA depth %d" % bpp)
    if width <= 0 or height <= 0 or width > 4096 or height > 4096:
        raise ValueError("bad TGA size %dx%d" % (width, height))
    off = 18 + id_length
    pixel_count = width * height
    pixels = bytearray(pixel_count * 4)
    bytes_pp = bpp // 8

    def read_rgba(pos: int):
        if bpp == 24:
            b, g, r = tga[pos], tga[pos + 1], tga[pos + 2]
            return r, g, b, 255
        if bpp == 32:
            b, g, r, a = tga[pos], tga[pos + 1], tga[pos + 2], tga[pos + 3]
            return r, g, b, a
        v = tga[pos] | (tga[pos + 1] << 8)
        r = ((v >> 10) & 0x1F) * 255 // 31
        g = ((v >> 5) & 0x1F) * 255 // 31
        b = (v & 0x1F) * 255 // 31
        a = 255 if not (descriptor & 0x0F) or (v & 0x8000) else 0
        return r, g, b, a

    if image_type == 2:
        need = pixel_count * bytes_pp
        if off + need > len(tga):
            raise ValueError("truncated TGA data")
        for i in range(pixel_count):
            r, g, b, a = read_rgba(off + i * bytes_pp)
            j = i * 4
            pixels[j : j + 4] = bytes((r, g, b, a))
    else:  # RLE true-colour
        out = 0
        while out < pixel_count:
            if off >= len(tga):
                raise ValueError("truncated RLE packet")
            packet = tga[off]
            off += 1
            count = (packet & 0x7F) + 1
            if packet & 0x80:
                r, g, b, a = read_rgba(off)
                off += bytes_pp
                for _ in range(count):
                    if out >= pixel_count:
                        break
                    j = out * 4
                    pixels[j : j + 4] = bytes((r, g, b, a))
                    out += 1
            else:
                for _ in range(count):
                    if out >= pixel_count or off + bytes_pp > len(tga):
                        break
                    r, g, b, a = read_rgba(off)
                    off += bytes_pp
                    j = out * 4
                    pixels[j : j + 4] = bytes((r, g, b, a))
                    out += 1

    img = Image(width, height)
    img.data = pixels
    if not (descriptor & 0x20):  # TGA default is bottom-left origin
        flip_vertical(img)
    return img


def flip_vertical(img: Image):
    stride = img.width * 4
    top = bytearray()
    bottom = bytearray()
    for y in range(img.height // 2):
        y2 = img.height - 1 - y
        top[:] = img.data[y * stride : (y + 1) * stride]
        bottom[:] = img.data[y2 * stride : (y2 + 1) * stride]
        img.data[y * stride : (y + 1) * stride] = bottom
        img.data[y2 * stride : (y2 + 1) * stride] = top
