# 02 — Texture loading and texture management (WYD 748)

Purpose: a byte-exact, framework-independent specification of how the 748 client
finds, decodes, uploads, caches and samples every texture, so the same rendering
can be reproduced in OpenGL or Vulkan from this document alone.

Last updated: 2026-11-10

Target artifacts:

| Artifact | Path | Identity |
|---|---|---|
| Shipped client | `client_748/WYD.exe` | PE x86, SHA-256 `b545ea104de50641e820f00b6bc54e4b2b14583ed75c7dcec06f50ba5042619c` |
| Port source | `Projects/TMProject/TextureManager.cpp`, `TextureManager.h`, `TMPaths.h`, `RenderDevice.cpp` | D3D9 port of the above |
| Assets | `client_748/UI`, `client_748/mesh`, `client_748/Env`, `client_748/Effect` | English 748, 2007-04 |

Method: static decompilation of the shipped binary with Ghidra 12.1.3 (via REA),
cross-checked line by line against the port source, and exhaustive numeric
validation of every shipped texture file with PowerShell. Raw hexdumps are in
[`evidence/hex/`](evidence/hex/).

---

## Confidence summary

| Tag | Count |
|---|---:|
| `[OBSERVED]` | 96 |
| `[INFERRED]` | 12 |
| `[UNKNOWN]` | 4 |

Unresolved items (4, each tagged UNKNOWN at its site; Appendix C lists how to
resolve them):

1. **Why the `.wys` fourCC tag is `"2z73"`/`"7x0s"`.** The bytes are constant per
   codec but are not a substitution of `"DXT1"`/`"DXT3"` under any cipher I could
   find. Only the *first* byte matters to the client; only the original exporter
   could say why.
2. **Whether `D3DXCreateTextureFromFileInMemoryEx` honours the TGA descriptor
   origin bit** (all `.wyt` files are bottom-left origin). The *observable*
   requirement — the uploaded texture must be top-down — is stated as inferred.
3. **The 632-byte guild-mark buffer's inner image format** (D3DX sniffs it; the
   size does not correspond to an obvious BMP/TGA geometry).
4. **Whether the colour key is compared before or after format conversion**
   (only matters on a 16-bit destination).

Watch-list — stated as `[INFERRED]`/`[OBSERVED]` but worth an in-game check:

* `DYNAMIC_TEXTURE_WIDTH` = 256 vs 512, identified as
  `D3DCAPS.MaxTextureHeight > 256` (offset `0x5C`, plus the `"VOODOO"` branch).
* `m_ppDynamicTexture[72]` is never written by any code I could find in the 748
  binary; `GetDynamicTexture` therefore always returns `NULL`.
* `cAlpha == 'C'` (84 model textures) has no load-time effect when DXT is
  supported; its meaning in the 2007 art pipeline is unclear.
* The minimap's `GenerateTexture(4, …)` requires slot 4 to be still empty; the
  call ordering is inferred from the shipped call graph, not traced.

Notation: `file[N]` = byte offset N in the on-disk file; `pBuffer[N]` = offset N
in the in-memory buffer handed to D3DX. All integers are little-endian.

---

## Table of contents

1. [The complete texture-loading surface](#1-the-complete-texture-loading-surface)
2. [Format `.wyt` — 4-byte magic + raw TGA 2.0](#2-format-wyt--4-byte-magic--raw-tga-20)
3. [Format `.wys` — DDS with a mutated header](#3-format-wys--dds-with-a-mutated-header)
4. [Formats that are *not* loadable](#4-formats-that-are-not-loadable)
5. [Decode pseudocode (self-contained)](#5-decode-pseudocode-self-contained)
6. [Texture index spaces and list files](#6-texture-index-spaces-and-list-files)
7. [UI texture sets (`ControlTextureSet`)](#7-ui-texture-sets-controltextureset)
8. [Loading: format selection, colour key, mip policy, pool](#8-loading-format-selection-colour-key-mip-policy-pool)
9. [Caching and eviction](#9-caching-and-eviction)
10. [Generated textures: dynamic, guild mark, font page, render targets](#10-generated-textures-dynamic-guild-mark-font-page-render-targets)
11. [UV conventions, filtering, wrapping](#11-uv-conventions-filtering-wrapping)
12. [Names, paths, case sensitivity, missing files](#12-names-paths-case-sensitivity-missing-files)
13. [Reimplementation notes (OpenGL / Vulkan)](#13-reimplementation-notes-opengl--vulkan)
14. [Appendix A — constants](#appendix-a--constants)
15. [Appendix B — measured statistics](#appendix-b--measured-statistics)
16. [Appendix C — unresolved items](#appendix-c--unresolved-items)

---

## 1. The complete texture-loading surface

Every texture the client ever uploads comes from **one** function — the D3DX9
entry point `D3DXCreateTextureFromFileInMemoryEx` — reached from exactly nine
call sites in `WYD.exe`. `[OBSERVED]` (REA: `xrefs(0x560eab)` → 9 references)

| Call site | Containing function (748 address) | Client method | Calls |
|---|---|---|---:|
| `0x4b8759` | `0x4b8333` | `TextureManager::LoadUITexture` | 1 |
| `0x4b9190`, `0x4b9209` | `0x4b8d26` | `TextureManager::LoadEffectTexture` | 2 |
| `0x4b9944`, `0x4b99bd` | `0x4b9478` | `TextureManager::LoadModelTexture` | 2 |
| `0x4ba203`, `0x4ba27c` | `0x4b9d99` | `TextureManager::LoadEnvTexture` | 2 |
| `0x4baac9` | `0x4baa4d` | `TextureManager::LoadGuildTexture` | 1 |
| `0x42245b` | `0x421b8f` | `TMFont2` glyph page | 1 |

Two more upload paths do not go through D3DX-from-file:

* `TextureManager::GenerateTexture` (`0x4ba3ca`) creates a blank texture with
  `D3DXCreateTexture` and blits into it (see §10.1). `[OBSERVED]`
* The font page is created with `IDirect3DDevice9::CreateTexture`
  (vtable `+0x50`) on Savage hardware, otherwise from the file buffer, and is
  then fully overwritten from a GDI bitmap (see §10.3). `[OBSERVED]`

Consequence for a port: there is no BMP/DDS/JPG/PNG/TGA loader to reimplement —
there are exactly **two** on-disk texture formats.

---

## 2. Format `.wyt` — 4-byte magic + raw TGA 2.0

### 2.1 On-disk layout

```
offset 0        : "WT10"                    4 bytes    (57 54 31 30)
offset 4        : TGA header                18 bytes
offset 22       : pixel data                W*H*Bpp/8 bytes (uncompressed)
offset len-8    : 00 00 00 00 00 00 00 00   first 8 bytes of the TGA 2.0 footer
```

`[OBSERVED]` — `UI/cursor.wyt`, 262174 bytes:
`4 + 18 + 262144 (=256*256*4) + 8`. Hexdump:
[`evidence/hex/tex-wyt-cursor-head.txt`](evidence/hex/tex-wyt-cursor-head.txt)

```
00000000  57 54 31 30 00 00 02 00 00 00 00 00 00 00 00 00  |WT10............|
00000010  00 01 00 01 20 08 00 00 00 00 00 00 00 00 00 00  |.... ...........|
          ^^^^^ ^^^^^ ^^ ^^
          width height |  +-- file[21] descriptor = 0x08 (32 bpp, 8 alpha bits, bottom-up)
                       +----- file[20] bits per pixel = 0x20 = 32
          file[16..17] = 0x0100 = 256 (width)
          file[18..19] = 0x0100 = 256 (height)
```

### 2.2 TGA header field table (identical to TGA 1.0/2.0)

| file off | size | field | observed in 748 |
|---:|---:|---|---|
| 4 | 1 | ID length | **0** in all 221 files |
| 5 | 1 | colour-map type | **0** in all 221 files |
| 6 | 1 | image type | **2** (uncompressed true colour) in all 221 files |
| 7 | 2 | colour-map origin | 0 |
| 9 | 2 | colour-map length | 0 |
| 11 | 1 | colour-map entry size | 0 |
| 12 | 2 | x origin | 0 |
| 14 | 2 | y origin | 0 |
| 16 | 2 | **width** | 64 … 1024 |
| 18 | 2 | **height** | 64 … 600 |
| 20 | 1 | **bits per pixel** | 16 (2 files), 24 (180), 32 (39) |
| 21 | 1 | image descriptor | `0x00` (180), `0x01` (2), `0x08` (39) |

`[OBSERVED]` (all 221 `.wyt` under `client_748/`, script output in
[`evidence/hex/tex-wyt-scan-summary.txt`](evidence/hex/tex-wyt-scan-summary.txt))

Descriptor decoding: bits 0–3 = alpha bits, bit 4 = right-to-left, bits 5–6 =
origin (0 = **bottom-left**, 1 = top-left). Every shipped `.wyt` has bit 5 = 0,
i.e. **all pixel rows are stored bottom-up**; `0x00` = 24/32 bpp with no alpha,
`0x01` = 16 bpp with a 1-bit alpha, `0x08` = 32 bpp with 8 alpha bits. `[OBSERVED]`

There is **no RLE** (no image type 10) and **no mip chain**: `len - 30` equals
`W*H*Bpp/8` exactly for all 221 files. `[OBSERVED]`

### 2.3 How the client turns it into a texture

```cpp
read(handle, magic, 4);                 // discard "WT10"
nLength = filesize - 4;
buf = new char[nLength + 18];
read(handle, buf, nLength);             // bytes 4..end, incl. the 8 zero bytes
memcpy(buf + nLength, m_szTGAHeader, 18);
D3DXCreateTextureFromFileInMemoryEx(..., buf, nLength + 18, ...);
```

`m_szTGAHeader` is the 18-byte string `"TRUEVISION-XFILE" "." 0x00`
(`.wyt` files already carry the footer's leading 8 zero bytes, so the
concatenation reconstructs the full 26-byte TGA 2.0 footer
`8 zero bytes | "TRUEVISION-XFILE" | "." | 0`). `[OBSERVED]`
(748 `TextureManager` ctor at `0x4b7780`: `m_szTGAHeader` at `this+0x304`,
`[0x314]=0x2E`, `[0x315]=0`; loader at `0x4b8333`)

Port equivalence: `TextureManager.cpp:15-19, 269-276`. A `.wyt` can therefore be
converted losslessly with `tga = wyt[4:] + b"TRUEVISION-XFILE.\x00"`. `[OBSERVED]`

---

## 3. Format `.wys` — DDS with a mutated header

### 3.1 What the file really is

`file[0..3]` is `"WS10"`, then **one extra space byte** `file[4] = 0x20`, then a
standard 128-byte DDS header *minus its 4-byte magic* starting at `file[5]`
(`dwSize = 124`), then the DXT block data at `file[129]`.

Reconstruction used by the client (`0x4b8333`, DDS branch):

```cpp
read(handle, junk, 1);                       // skip ONE byte, not four
nLength = filesize - 1;
buf = new char[nLength];
read(handle, buf, nLength);                  // buf = file[1..]
memcpy(buf, m_szDDSHeader, 3);               // m_szDDSHeader = "DDS" (3 bytes)
// => buf == "DDS " + file[5:]   (length = filesize-1)
if (buf[0x54] == '2') memcpy(buf+0x54, "DXT1", 4);
else                  memcpy(buf+0x54, "DXT3", 4);
```

`[OBSERVED]` Fully consistent arithmetic: `filesize - 129` equals the exact
DXT1-or-DXT3 size for the declared dimensions and mip count for **2336 of 2336**
shipped `.wys` files (script output:
[`evidence/hex/tex-wys-scan-summary.txt`](evidence/hex/tex-wys-scan-summary.txt)).

Two equivalently valid one-liners for a port:

```
dds = b"DDS " + file[5:]                  # 4 + (len-5) = len-1 bytes
dds = bytearray(file[1:]); dds[0:3] = b"DDS"
```

Worked example — `mesh/sword01.wys` (2177 bytes, 64×64, DXT1, no mips):

```
file offset  bytes                             means
00000000     57 53 31 30                       "WS10"
00000004     20                                the DDS magic's trailing space
00000005     7C 00 00 00                       dwSize = 124
00000009     00 07 10 00                       dwFlags = 0x1007
0000000D     00 40 00 00                       dwHeight = 64
00000011     00 40 00 00                       dwWidth  = 64
00000015..0x4C                                pitch/depth/mips/reserved = 0
0000004D     20 00 00 00                       ddspf.dwSize = 32
00000051     04 00 00 00                       ddspf.dwFlags = DDPF_FOURCC
00000055     32 7A 37 33                       fourCC tag "2z73"  (-> "DXT1")
00000059..0x6C                                bpp = 0, all four masks = 0
0000006D     00 10 00 00                       dwCaps = 0x1000
00000071..0x80                                 caps2..reserved2 = 0
00000081     D0 83 EF 7B DD 5D DD 5D ...      first DXT1 block (8 bytes)
```
`[OBSERVED]` — `2177 - 129 = 2048 = 16*16 blocks * 8 bytes`.
Full dump: [`evidence/hex/tex-wys-dxt1-sword01-head.txt`](evidence/hex/tex-wys-dxt1-sword01-head.txt).
A mipmapped DXT3 example (`Effect/arrow.wys`, 128×128, 4 levels, tag `"7x0s"`,
`dwCaps = 0x401008`, `21760` bytes of blocks) is in
[`evidence/hex/tex-wys-dxt3-arrow-head.txt`](evidence/hex/tex-wys-dxt3-arrow-head.txt).

**Port check — the port is correct here.** `TextureManager.cpp:295-302` does
exactly what the client does: `nLength = filesize - 1`,
`_read(handle, szHeader, 1)` (skips the single `'W'`), then
`memcpy(pBuffer, m_szDDSHeader, 3)` with `m_szDDSHeader = "DDS"`
(`TextureManager.cpp:16`). Because `pBuffer` starts at `file[1]`, the result is
`"DDS" + file[4..]` = `"DDS "` followed by a valid header. `[OBSERVED]`

Do **not** "fix" the `- 4` that appears at `TextureManager.cpp:271` — that line is
the `.wyt`/TGA branch, where skipping the 4-byte `"WT10"` magic is correct. All
four loaders use the `- 1` form in their DDS branch:
`LoadUITexture` :295, `LoadEffectTexture` :580, `LoadModelTexture` :789,
`LoadEnvTexture` :1052. `[OBSERVED]`

(compare `0x4b8333`: `FUN_0058f8c9(local_28, local_c, 1)` and
`local_10 = filesize - 1`)

### 3.2 Reconstructed DDS header

Offsets below are in the reconstructed buffer `pBuffer`, with the equivalent
on-disk offset in the right column (`pBuffer[k] == file[k+1]`).

| pBuffer off | file off | size | field | observed |
|---:|---:|---:|---|---|
| 0 | 1 | 4 | magic `"DDS "` | written by the client |
| 4 | 5 | 4 | `dwSize` | **124** (all 2336) |
| 8 | 9 | 4 | `dwFlags` | `0x1007` (1085 files) or `0x21007` (1251) |
| 12 | 13 | 4 | `dwHeight` | 4 … 256 (mostly powers of two; also 8, 16, 32, 64, 128, 256) |
| 16 | 17 | 4 | `dwWidth` | 1 … 512 (mostly powers of two; also 18 and 62) |
| 20 | 21 | 4 | `dwPitchOrLinearSize` | **0** (all) |
| 24 | 25 | 4 | `dwDepth` | **0** (all) |
| 28 | 29 | 4 | `dwMipMapCount` | `0` (1085), `3` (146), `4` (1105) |
| 32 | 33 | 44 | `dwReserved1[11]` | 0 |
| 76 | 77 | 4 | `ddspf.dwSize` | **32** (all) |
| 80 | 81 | 4 | `ddspf.dwFlags` | **4** = `DDPF_FOURCC` (all) |
| 84 | 85 | 4 | `ddspf.dwFourCC` | opaque tag, see §3.3 |
| 88 | 89 | 4 | `ddspf.dwRGBBitCount` | **0** (all) |
| 92 | 93 | 4 | `ddspf.dwRBitMask` | **0** |
| 96 | 97 | 4 | `ddspf.dwGBitMask` | **0** |
| 100 | 101 | 4 | `ddspf.dwBBitMask` | **0** |
| 104 | 105 | 4 | `ddspf.dwABitMask` | **0** |
| 108 | 109 | 4 | `dwCaps` | `0x1000` (no mips) or `0x401008` |
| 112 | 113 | 4 | `dwCaps2` | **0** |
| 116 | 117 | 4 | `dwCaps3` | 0 |
| 120 | 121 | 4 | `dwCaps4` | 0 |
| 124 | 125 | 4 | `dwReserved2` | 0 |
| 128 | 129 | … | DXT block data | |

`dwCaps = 0x401008` = `DDSCAPS_COMPLEX|DDSCAPS_TEXTURE|DDSCAPS_MIPMAP`.
`dwFlags = 0x21007` = `CAPS|HEIGHT|WIDTH|PIXELFORMAT|MIPMAPCOUNT`.
`[OBSERVED]`

Note `DDPF_ALPHAPIXELS` (0x1) is **not** set anywhere; for a fourCC format it is
irrelevant, but see the DXT1 alpha trap in §13.

### 3.3 The fourCC tag — the one non-standard field

The fourCC slot does **not** contain `"DXT1"`. It contains one of two constant
4-character tags:

| raw tag (file[85..88]) | files | block size that validates | client rewrites to |
|---|---:|---:|---|
| `"2z73"` | 1980 | 8 bytes / 4×4 block → DXT1 | `"DXT1"` |
| `"7x0s"` | 356 | 16 bytes / 4×4 block → DXT3 | `"DXT3"` |

`[OBSERVED]` (tag census over all 2336 files; size validation 100 %)

The client only tests the **first byte**:

```cpp
if (pBuffer[0x54] == '2') fourcc = "DXT1"; else fourcc = "DXT3";
```

`[OBSERVED]` (`0x4b8333`, `0x4b8d26`, `0x4b9478`, `0x4b9d99`, all identical;
strings `"DXT1"` @ `0x5ba7d8`, `"DXT3"` @ `0x5ba7e0`)

Evidence that `"7x0s"` really is DXT3 (16-byte blocks, explicit 4-bit alpha) and
not DXT5:

* 69 % (512 330 / 745 853) of its blocks have the first 8 bytes equal to
  `FF FF FF FF FF FF FF FF` — DXT3's fully-opaque alpha encoding. The DXT5
  equivalent (endpoints `FF FF`, all-zero indices) appears in only 343 of those
  blocks (0.05 %), so the data is not DXT5-encoded. `[OBSERVED]`
* 73 % (1 222 024 / 1 674 314) of the non-`FF` alpha bytes have identical high
  and low nibbles — the fingerprint of an 8-bit→4-bit alpha conversion into DXT3
  nibbles. `[OBSERVED]`
* The client forces `"DXT3"`, and the shipped game renders those textures. `[OBSERVED]`

`[UNKNOWN]` Why the tags are `"2z73"`/`"7x0s"`. They are constant per codec and
are not an affine/XOR transform of `"DXT1"`/`"DXT3"` (checked byte-wise and as a
32-bit word). Resolving it needs the original `.wys` exporter, which is not in
the distribution (`UI/WYT3.CONVERT.exe`, 67 978 779 bytes, is shipped but is not
referenced by any string in `WYD.exe` `[OBSERVED]`, and no `"WYT3"` string
exists in the binary).

**DXT5 is never used.** Do not map `"7x0s"` to BC3.

### 3.4 Mip chains in `.wys`

`.wys` files carry full mip chains (`dwMipMapCount` 3 or 4, sometimes 0 meaning
one level only). They are stored contiguously in the standard DDS order (level 0
first). However, `LoadUITexture` always asks D3DX for **1 mip level**, so UI
textures ignore the chain; effect/model/env loading asks for 4 levels under the
quality rules of §8.4. `[OBSERVED]`

Sizes match `Σ_levels ceil(w/4)*ceil(h/4)*blocksize` with `blocksize` 8 (DXT1) or
16 (DXT3) and `w,h` halving per level (min 1) — this is what the 2336-file
validation used. `[OBSERVED]`

---

## 4. Formats that are *not* loadable

| Extension | Loadable by the 748 client? | Evidence |
|---|---|---|
| `.wyt` | yes (TGA branch) | §2 |
| `.wys` | yes (DDS branch) | §3 |
| `.dds` | **no** | name would fall into the TGA branch (extension test is `strcmp(suffix, ".wys") == 0`), which strips 4 bytes of a DDS magic and appends a TGA footer → unparseable. No `.dds` string exists in the binary. `[OBSERVED]` |
| `.tga` | **no** | same reasoning; the branch *assumes* a 4-byte magic before a TGA header. A real `.tga` would lose its first 4 header bytes. `[OBSERVED]` |
| `.bmp`, `.jpg`, `.png` | **no** | no such strings in the binary; `"ScreenShot\\Capture%04d.jpg"` is an *output* path only. `[OBSERVED]` |
| `.wyp` | **no** — 755+/2026 feature | the string `".wyp"` does not exist in `WYD.exe`; `"guimat"` does not exist either. `[OBSERVED]` |
| palette / `PALETTEENTRY` | **no** | every D3DX call passes `pPalette = NULL`; all TGA files have colour-map type 0. `[OBSERVED]` |

No encryption, base64, AES or MD5 is involved anywhere in the texture path:
`AES.h`, `Base64.h`, `desbase64.h`, `md5.h` exist in the source tree but are
referenced by **no** `.cpp`, and `"JPGLib.dll"` appears in `WYD.exe` at
`0x5b07ce` with **zero xrefs**. `[OBSERVED]`

`JPGLib.dll` (57 344 bytes, shipped next to the client) is named exactly once in
`WYD.exe` — as a plain string at `0x5b07ce` with **zero xrefs** — and does not
appear in the executable's import directory. `[OBSERVED]`

One caveat: `D3DXCreateTextureFromFileInMemoryEx` sniffs the buffer, so a
*correctly repaired* DDS (from `.wys`) also works; conversely a plain `.dds`
entry inserted into a list file would be decoded as garbage rather than
rejected. `[INFERRED]`

---

## 5. Decode pseudocode (self-contained)

```python
# ---------------------------------------------------------------- constants
TGA_SIG   = b"TRUEVISION-XFILE.\x00"   # 18 bytes appended by the client
DDS_MAGIC = b"DDS "
DXT_TAGS  = {0x32: b"DXT1",            # tag byte '2'  -> DXT1 / BC1
             0x37: b"DXT3"}            # tag byte '7'  -> DXT3 / BC2
FOURCC_DXT1, FOURCC_DXT3 = b"DXT1", b"DXT3"

def is_dds_suffix(name: str) -> bool:
    # the client scans for '.' then strcmp()s the remainder, case-SENSITIVE
    i = name.find(".")
    return i >= 0 and name[i:] == ".wys"

def load_texture_bytes(name: str, raw: bytes, mip_levels: int):
    """Returns (memory_buffer, colorkey_argb, mip_levels)."""
    if is_dds_suffix(name):
        buf = bytearray(raw[1:])            # NOTE: skip exactly ONE byte
        buf[0:3] = b"DDS"                   # -> "DDS " + raw[5:]
        tag = buf[0x54]
        buf[0x54:0x58] = FOURCC_DXT1 if tag == 0x32 else FOURCC_DXT3
        return buf, 0x00000000, mip_levels  # colorkey disabled
    else:
        buf = bytearray(raw[4:])            # drop "WT10"
        buf += TGA_SIG                      # rebuild the TGA 2.0 footer
        return buf, 0xFF000000, 1           # black is the colour key, 1 level

# ---------------------------------------------------------------- after upload
def apply_colorkey(argb_src, key):          # key 0 => disabled
    if key == 0:
        return argb_src
    kr, kg, kb = (key >> 16) & 0xFF, (key >> 8) & 0xFF, key & 0xFF
    out = argb_src
    for each texel (r, g, b, a):
        if (r, g, b) == (kr, kg, kb):
            out.texel = (0, 0, 0, 0)        # transparent black
    return out

def tga_rows_to_top_down(pixels, width, height, bpp, descriptor):
    if descriptor & 0x20:                    # top-left origin
        return pixels
    return flip_rows(pixels, width, height)  # bottom-left origin (all 748 files)
```

The `DXT1`/`DXT3` block decode itself is standard BC1/BC2 and is out of scope
for this document (any GPU or `bcdec`-class library reproduces it bit-exactly).
`[OBSERVED]` the data is textbook BC1/BC2 — e.g. `mesh/sword01.wys` block 0 is
`D0 83 EF 7B DD 5D DD 5D`: colour0 `0x83D0`, colour1 `0x7BEF`, then 4 index
bytes.

---

## 6. Texture index spaces and list files

### 6.1 The record: `stTextureListInfo`

`[OBSERVED]` (748 ctor `0x4b7780`: filename at `this+0x1328 + i*0x108`, alpha at
`+0x1427 + i*0x108` ⇒ stride `0x108` = 264, alpha at record offset 0xFF)

| field | type | offset | size | meaning |
|---|---|---:|---:|---|
| `szFileName` | `char[255]` | 0 | 255 | relative path, **NUL-terminated**, used verbatim |
| `cAlpha` | `char` | 255 | 1 | `'A'`, `'a'`, `'N'`, `'C'` (see §8.2) |
| `dwLastUsedTime` | `uint32` | 256 | 4 | `timeGetTime()` of last `Get*` call |
| `dwShowTime` | `uint32` | 260 | 4 | lifetime in ms requested by the caller |
| **total** | | | **264** | no padding |

File-size proof:

| file | records | × 264 | file size |
|---|---:|---:|---:|
| `UI/UITextureList.bin` | 512 | 135 168 | 135 168 ✓ |
| `Effect/EffectTextureList.bin` | 512 | 135 168 | 135 168 ✓ |
| `mesh/MeshTextureList.bin` | 2048 | 540 672 | 540 672 ✓ |
| `Env/EnvTextureList3.bin` | 512 | 135 168 | 135 168 ✓ |

`[OBSERVED]` Each is read with a single `fread(array, 0x21000|0x84000, 1, fp)`
(`0x4b827f`, `0x4b8c72`, `0x4b93c4`, `0x4b9ce5`) — the whole array at once, no
header, no count field, no checksum.

### 6.2 Two 748-specific gotchas

1. **`dwLastUsedTime`/`dwShowTime` in the shipped bins contain `0xCDCDCDCD`
   (3 452 816 845)** — the MSVC debug heap fill. They are meaningless; the
   client overwrites both on every `Get*`. `[OBSERVED]`
   (verified for records 0–2 of all four files;
   e.g. `UI/UITextureList.bin` record 0 bytes `256..263` = `CD CD CD CD CD CD CD CD`)
2. The unused portion of `szFileName` is also filled with `0xCD`, not `0x00`.
   A parser must stop at the first `0x00` (the client uses C strings). `[OBSERVED]`

### 6.3 The four index spaces

| List file (as coded in 748) | Array in `TextureManager` | Capacity | Live entries | Highest index |
|---|---|---:|---:|---:|
| `UI/UITextureList.bin` | `m_stUITextureList` | 512 | 239 | 261 |
| `Effect/EffectTextureList.bin` | `m_stEffectTextureList` | 512 | 418 | 438 |
| `mesh/MeshTextureList.bin` | `m_stModelTextureList` | 2048 | 1585 | 1662 |
| `Env/EnvTextureList3.bin` | `m_stEnvTextureList` | 512 | 376 | 395 |

`[OBSERVED]` — note the env list's first non-empty record is index 10.

The client builds each path by taking the **`.txt`** literal and overwriting its
last three characters with `"bin"`:

```cpp
sprintf(buf, "%s", "UI\\UITextureList.txt");        // 0x4b827f
strlen("UI\\UITextureList.txt");                    // = 21
memcpy(buf + (21 - 3), "bin", 3);                   // -> "UI\\UITextureList.bin"
fopen(buf, "rb");
```

`[OBSERVED]` (`0x4b827f`, string `"UI\\UITextureList.txt"` @ `0x5ba774`,
`"bin"` @ `0x5ba790`; identical pattern for the other three lists)

So the `.bin` name is derived, not literal. The port's `TMPaths.h` constants
already spell out the final `.bin` names, which is equivalent.

### 6.4 Path strings inside the bins (a real porting hazard)

| list | path style observed |
|---|---|
| UI | **mixed**: 52 entries use `UI\\name.wyt` (two `0x5C`), the rest use `UI\name.wyt` |
| Effect | all `Effect\name.wys` (single `\`), 4 entries are `mesh\sky*.wys` |
| Mesh | all `mesh\name.wys` (lowercase `mesh`) |
| Env | all `Env\Tile*.wys` / `Env\MTile*.wys` |

`[OBSERVED]` e.g. `UI/UITextureList.bin` record 0 is literally
`55 49 5C 5C 63 75 72 73 6F 72 2E 77 79 74 00` = `UI\\cursor.wyt`
(hexdump: [`evidence/hex/tex-uitexturelist-records-0-1.txt`](evidence/hex/tex-uitexturelist-records-0-1.txt)).

Windows collapses a doubled separator, so `_open("UI\\cursor.wyt")` works there.
A port running on a case-sensitive filesystem must normalise `\\` → `/` **and**
do case-insensitive matching — see §12.

### 6.5 Which list the port must use (duplicate files)

`client_748` ships two copies of two lists:

| file | SHA-256 | contents |
|---|---|---|
| `UI/UITextureList.bin` | `DC67D0CA…81F2D9` | 239 entries, max index 261 |
| `mesh/UITextureList.bin` | `DC67D0CA…81F2D9` | byte-identical duplicate |
| `mesh/MeshTextureList.bin` | `7FE770F9…CA65395` | 1585 entries, max 1662, alpha ∈ {A:218, C:84, N:1283} |
| `UI/MeshTextureList.bin` | `45C32EE1…92804B6` | **older/different**: 1190 entries, max 1308, alpha ∈ {A:124, N:1066}, no `'C'` |

`[OBSERVED]` The client opens `Mesh\\MeshTextureList.bin`, i.e. the `mesh/` copy.
Using the `UI/` copy silently loses 395 model textures.

### 6.6 `GetUITextureIndex` / `GetModelTextureIndex` / `GetEffectTextureIndex`

These do a linear scan of the whole array comparing the caller's string after
splicing in the last three characters of the candidate (`0x4b8844`):

```cpp
int nStrLen = strlen(szTextureName);
for (i = 0; i < MAX; ++i) {
    int n2 = strlen(list[i].szFileName);
    sprintf(&szTextureName[nStrLen - 3], &list[i].szFileName[n2 - 3]);  // ".wys"
    if (!strcmp(szTextureName, list[i].szFileName)) return i;
}
```

`[OBSERVED]` The function **modifies the caller's string** (it is a bug, but it
is observable behaviour). `GetModelTexture(char*)` (`0x4b9acf`) is a plain
`strcmp` loop over all 2048 slots, stopping at the first match, then behaves like
`GetModelTexture(int)`. `[OBSERVED]`

`GetUITexture`/`GetEffectTexture`/`GetEnvTexture`/`GetModelTexture` bounds checks
are strict: `0 <= i < 512` (or `< 2048`, `< 72`). The port uses
`i > MAX_*`, which allows the out-of-range index `== MAX_*` and reads one slot
past the array. `[OBSERVED]` (`0x4b87be`, `0x4b925c`, `0x4b9a4b`, `0x4ba2cf`,
`0x4ba9fc`, `0x4b9a10`) — flag this as a port bug, not a spec.

---

## 7. UI texture sets (`ControlTextureSet`)

### 7.1 Text format (`UI/UITextureSetList.txt`)

```
[<SetName>]
SetIndex: <int>
ItemCount: <int>
<texIndex>,<startX>,<startY>,<width>,<height>,<destX>,<destY>
... ItemCount rows ...
<blank line>
```

Parsed with exactly three format strings `[OBSERVED]` (`0x4b8928`):

```c
fscanf(fp, "%s\r\nSetIndex: %d\r\nItemCount: %d\r\n", name, &setIndex, &itemCount);   // 0x5ba804
fscanf(fp, "%d,%d,%d,%d,%d,%d,%d\r\n", &texIdx,&sx,&sy,&w,&h,&dx,&dy);                // 0x5ba850
```

The mandatory `\r\n` means the file must be CRLF; the shipped file is CRLF.
The loop terminates when the first `fscanf` returns `-1` (EOF).

* 446 sets, `SetIndex` 0…456 (not contiguous: 11 indices unused), 1851 declared
  items of which 1849 rows are actually present. `[OBSERVED]`
* Two sets declare more items than they list: `Yellow_Number` (SetIndex 139,
  ItemCount 12, 5 rows) and `chat_sel2` (SetIndex 198, ItemCount 5, 4 rows).
  The missing rows stay **all zero** (`ControlTextureCoord()` ctor `0x4bac10`
  zeroes the 7 ints), i.e. `nTextureIndex = 0` → the cursor atlas, rect 0×0,
  at destination `(0,0)`. `[OBSERVED]`
* The parse loop is `while (fscanf(...) != -1)`, so any short/failed read is
  treated as success and reuses the previous values — there is no validation.
  `[OBSERVED]`
* `SetIndex >= 512` is rejected with a log line and skipped; **the bound is 512,
  not 600**. `[OBSERVED]` (`0x4b8928`: `if (local_98 < 0x200)`; ctor
  `memset(this+0x328, 0, 0x1000)` = 512 × 8 bytes)

### 7.2 In-memory structures

```c
struct ControlTextureCoord {   // 28 bytes (0x1C)
    int nTextureIndex;  // offset  0  -> index into m_stUITextureList
    int nStartX;        // offset  4  -> source rect x in atlas pixels
    int nStartY;        // offset  8
    int nWidth;         // offset 12
    int nHeight;        // offset 16
    int nDestX;         // offset 20  -> ALWAYS 0 in 748 data
    int nDestY;         // offset 24  -> ALWAYS 0 in 748 data
};                              // 7 * sizeof(int) = 0x1C, confirmed by `operator_new(count*0x1c)`

struct ControlTextureSet {     // 8 bytes
    int nCount;                 // offset 0
    ControlTextureCoord* pTextureCoord;  // offset 4
};
```

`[OBSERVED]` (`0x4b8928`: `operator_new(local_14 * 0x1c)`, field writes at
`+0, +4, +8, +0xc, +0x10, +0x14, +0x18`; `ControlTextureCoord` ctor `0x4bac10`
zeroes the 7 ints)

### 7.3 Semantics

* `nTextureSetIndex` (on a UI control) selects a set; `nTextureIndex` (on the
  control) selects which `ControlTextureCoord` inside it (typically a frame
  number for animated/multi-state widgets).
* `GetUITextureSet(i)` returns `&m_UITextureSetList[i]` and only checks
  `0 <= i < 512` — a set that was never loaded returns a zeroed entry with
  `pTextureCoord == NULL`, which the draw code treats as "draw nothing".
  `[OBSERVED]`
* Drawing: `GetUITexture(coord.nTextureIndex, 2000)`, then the source rect
  `(nStartX, nStartY, nWidth, nHeight)` is blitted to `(nDestX + control.x,
  nDestY + control.y)` — and since `nDestX/nDestY` are always 0 in 748, the
  destination is simply the control origin. `[OBSERVED]`
  (`RenderDevice.cpp:2850-2862`; consistent with the 0-valued `destX/destY`
  census: min = max = 0 over 1849 rows)
* Measured source-rect statistics: width 0…1024, height 0…600, and every
  referenced `nTextureIndex` is ≤ 261 (0 references above 261). `[OBSERVED]`
* Negative set indices (used by the port to mean "text" / "solid fill") are a
  *renderer* convention (`RenderDevice`), not present in the file. `[OBSERVED]`

---

## 8. Loading: format selection, colour key, mip policy, pool

### 8.1 Common skeleton of all four loaders

```
if (m_ppXxxTexture[idx] != NULL) return 0;             // already resident
bDDS = (name ends with ".wys")                          // case-sensitive
fd = _open(list[idx].szFileName, _O_BINARY, 0)
if (fd == -1) return 0                                  // missing file -> failure
len = _filelength(fd)
fmt = g_pDevice->m_eFormat                               // backbuffer format
colorkey = 0xFF000000 ; plusLen = 18
... branch on bDDS ...                                   // see §5
D3DXCreateTextureFromFileInMemoryEx(
        device, buf, len_with_padding,
        D3DX_DEFAULT /*w*/, D3DX_DEFAULT /*h*/, mipLevels, 0 /*usage*/,
        fmt, D3DPOOL_MANAGED /*1*/, filter, mipFilter, colorkey,
        NULL /*info*/, NULL /*palette*/, &m_ppXxxTexture[idx]);
on failure: texture := NULL, free buf, return 0
```

`[OBSERVED]` The file name is taken from record `nSrcIndex` while the format
decision and `cAlpha` come from record `nIndex` — `LoadUITexture(nIndex,
nSrcIndex)` is the only loader with that split; every 748 caller passes
`nSrcIndex = -1`, which the loader maps to `nIndex`. `[OBSERVED]`

`D3DPOOL_MANAGED` (enum value 1) is used for **every** texture, including the
generated ones. `D3DUSAGE` is always 0. `[OBSERVED]`

### 8.2 `cAlpha` → format table (exact `D3DFORMAT` numbers)

`m_bVoodoo` is `device+0x2a390`, `m_dwBitCount` is `device+0x2a510`,
`m_eFormat` is `device+0x2a514`, `m_bDXT1`/`m_bDXT3` are `+0x2a5c0/+0x2a5c4`,
and `m_bDxt` is a static (`DAT_005b4924`, default 1). `[OBSERVED]`

**TGA / `.wyt` branch** (`plusLen = 18`, `colorkey = 0xFF000000`)

| `cAlpha` | if `m_bVoodoo==1 || m_dwBitCount==0x10` | else |
|---|---|---|
| `'A'` | `D3DFMT_A4R4G4B4` (26) | `D3DFMT_UNKNOWN` (0) |
| `'a'` | `D3DFMT_A1R5G5B5` (25) | `D3DFMT_UNKNOWN` (0) |
| anything else | `m_eFormat` (backbuffer) | `m_eFormat` |

**DDS / `.wys` branch** (`plusLen = 0`)

| condition | `colorkey` | format (high colour) | format (Voodoo / 16-bpp device) |
|---|---:|---|---|
| `m_bDXT1 && m_bDXT3 && m_bDxt` | 0 | `D3DFMT_UNKNOWN` (0) → keep the file's DXT1/DXT3 | same |
| `cAlpha == 'A' \|\| 'C'` | 0 | `A8R8G8B8` (21) | `A4R4G4B4` (26) |
| `cAlpha == 'a'` (effect / model / env only) | 0 | `A8R8G8B8` (21) | `A1R5G5B5` (25) |
| `cAlpha == 'a'` (UI loader) | 0 | `A8R8G8B8` (21) | `A4R4G4B4` (26) |
| `cAlpha == 'N'` | 0 | `X8R8G8B8` (22) | `X1R5G5B5` (24) |
| none of the above | 0 | `m_eFormat` | `m_eFormat` |

`[OBSERVED]` (`0x4b8333` UI, `0x4b8d26` effect, `0x4b9478` model, `0x4b9d99` env).
The **only** difference between the four loaders is the `'a'` row: the UI loader
groups `'a'` with `'A'`/`'C'` (so on a 16-bpp device a 1-bit-alpha UI texture
becomes `A4R4G4B4`), while the other three give it `A1R5G5B5`. The port
reproduces this.

Consequence: with a DXT-capable device (any D3D9 GPU, and `m_bDxt` defaults to
1) the `.wys` `cAlpha` has **no effect on the uploaded format**. `[OBSERVED]`
`cAlpha` only matters for (a) the non-DXT fallback path and (b) render-state
decisions such as `IsModelTextureAlpha(i) → cAlpha=='A' || cAlpha=='a'`
(`0x4b9c9d`). `[OBSERVED]`

Value census in the shipped lists: `'A'` 60/66/218/0, `'a'` 5/1/0/0,
`'C'` 0/0/84/0, `'N'` 174/351/1283/376 for UI/Effect/Mesh/Env. `[OBSERVED]`

### 8.3 Colour key = "black is transparent" (only for TGA)

`ColorKey = 0xFF000000` means A=255,R=0,G=0,B=0 → **any pure-black texel becomes
transparent black**. This is the mechanism that gives the 24-bpp `.wyt` atlases
(180 of 221 files, no alpha channel at all) their 1-bit transparency.
`[OBSERVED]` (parameter `local_18 = 0xff000000` in the TGA branch; 0 in the DDS
branch, and D3DX treats 0 as "no colour key")

Port trap: this also knocks out *legitimately black* pixels inside 32-bpp
`.wyt` files, and it does **not** apply to `.wys`. If your port forgets it, every
24-bpp UI atlas renders on an opaque black rectangle.

Concrete case: `UI\MessageBox2.wyt` is 256×128, 24 bpp, descriptor `0x00` — it has
**no alpha channel at all**, so the message-box panel's rounded corners and its
transparent border exist only because black is the colour key. `[OBSERVED]`

`[UNKNOWN]` Whether D3DX compares the colour before or after conversion to the
destination format (matters only if the destination is 16-bit). Resolving: load
a `.wyt` with 24 bpp and `cAlpha='N'` on a 16-bit device and look for
near-black survivors.

### 8.4 Mip policy and filter flags

| family | levels requested | filter / mipFilter | condition |
|---|---:|---|---|
| UI | **1** | `1` / `1` (`D3DX_FILTER_NONE`) | always |
| Effect | 1 or **4** | 1/1 or **5/5** (`D3DX_FILTER_BOX`) | 4 levels iff `D3DDevice::m_nMipMap > 9` |
| Model | 1 or **4** | 1/1 or 5/5 | 4 levels iff `m_nMipMap >= 50`, or `40..49 && index > 11`, or `30..39 && 11 < index < 400`; else 1 |
| Env | 1 or **4** | 1/1 or 5/5 | 4 levels iff `m_nMipMap >= 20` |
| Guild mark | **0 → full chain** | 1/1 | always |

`[OBSERVED]` (`0x4b8333` UI; `0x4b8d26` effect, `9 < DAT_005b4920`;
`0x4b9478` model, thresholds `0x32/0x28/0x1e` and index tests `0xb`, `0x190`;
`0x4b9d99` env, `DAT_005b4920 < 0x14`; `0x4baa4d` guild, `MipLevels = 0`)

`m_nMipMap` is the global at `0x5b4920`; its static initial value is **20**
(`0x14`). `[OBSERVED]` (data at `0x5b4910`: `1.0f`, `12`, `256`, `50`, `20`,
`1`). With the default, effect textures get 4 mips (BOX-filtered by D3DX) and
model/env/UI textures get one level.

Mip generation is done by D3DX at load time from level 0 (not read from the
file) — `MipFilter = 5` = `D3DX_FILTER_BOX`. `[OBSERVED]`

### 8.5 What is stored in the texture

For `.wys` with DXT support the texture is created in the file's own compressed
format (BC1/BC2) — the client never decompresses. For `.wyt` the texture is
RGBA8/XRGB (or 16-bit on low-colour devices) as the TGA dictates. `[OBSERVED]`

---

## 9. Caching and eviction

### 9.1 `Get*` = "fetch, stamp, return"

```c
IDirect3DTexture9* GetUITexture(int idx, DWORD showTime) {   // 0x4b87be
    if (idx < 0 || idx >= 512) return NULL;
    if (m_ppUITexture[idx] == NULL && !LoadUITexture(idx, -1)) return NULL;
    m_stUITextureList[idx].dwLastUsedTime = timeGetTime();
    m_stUITextureList[idx].dwShowTime     = showTime;
    return m_ppUITexture[idx];
}
```

`[OBSERVED]` identical shape for effect (`0x4b925c`), model (`0x4b9a4b` int /
`0x4b9acf` name) and env (`0x4ba2cf`).

So the texture list entry holds **runtime cache state** even though it was read
from a file: `dwShowTime` is not a property of the texture, it is "how long may
this stay resident after the last use", supplied by the *caller* on every draw.

Observed `showTime` values in the client: `2000` ms for UI elements
(`RenderDevice.cpp:2850, 2896, 2923`); `360000` ms (6 min) for effect textures and
`180000` ms (3 min) for model textures on an object draw — or, when a per-object
field (`this+0x324`) is at/above thresholds `0x57E41` / `0x2BF21`, that field's
value instead. `[OBSERVED]` (`0x4bb847`, `0x4bba7e`:
`GetEffectTexture(idx, 360000)` / `GetModelTexture(idx, 180000)`)

### 9.2 `ReleaseNotUsingTexture` (`0x4b7f0d`)

```c
DWORD now = timeGetTime();
if (m_LastLoadTime + 900 > now) return;         // at most ~1 sweep/second
m_LastLoadTime = now;
for each slot i in UI(512), Effect(512), Env(512), Model(2048):
    if (tex[i] && list[i].dwShowTime < now - list[i].dwLastUsedTime)
        tex[i]->Release(); tex[i] = NULL;
for each dynamic slot i in 0..71:
    if (tex[i] && 5000 < now - m_dwDynamicLastUsedTime[i])
        tex[i]->Release(); tex[i] = NULL;
```

`[OBSERVED]` Note the **5000 ms** dynamic timeout — the port uses 1000 ms
(`TextureManager.cpp:201`). Sweep order is UI, Effect, Env, Model, Dynamic.
`m_LastLoadTime` is at `this+0x324`.

Because unsigned arithmetic is used (`now - dwLastUsedTime`), a `dwLastUsedTime`
in the future (clock wrap) keeps a texture resident. `[OBSERVED]`

### 9.3 Bulk release

`ReleaseTexture` (`0x4b7ca2`) releases all five arrays via vtable `+8`
(`IUnknown::Release`) and NULLs the pointers; note it runs the 72-slot loop
**twice** (harmless double iteration, the second pass finds NULL).
`Release_GuildMarkList` (`0x4bab41`) does the same for 64 guild marks and resets
`nGuild = -1`. `[OBSERVED]`

### 9.4 What a "cache miss" looks like to the renderer

`Get*` returns `NULL` for a missing file, a decode failure, or an out-of-range
index. Callers either skip the draw (`RenderRect` checks `pTexture != nullptr`,
`RenderDevice.cpp:1905`) or draw untextured. There is **no fallback/placeholder
texture** anywhere in 748. `[OBSERVED]`

`IsValidModelTexture(i)` (`0x4b9a10`) and `IsValidDynamicTexture(i)`
(`0x4ba392`) just test the pointer; they never attempt a load. `[OBSERVED]`

---

## 10. Generated textures: dynamic, guild mark, font page, render targets

### 10.1 `GenerateTexture` (`0x4ba3ca`) — the only blit path

Signature: `GenerateTexture(dstUISlot, srcUISlot, dstX, dstY, srcX, srcY, w, h)`.

```c
if (DYNAMIC_TEXTURE_WIDTH == 256) {         // global, init 256, 512 on modern HW
    w /= 2; h /= 2; dstX /= 2; dstY /= 2;   // srcX/srcY are NOT divided
}
if (w > DYNAMIC_TEXTURE_WIDTH)  w = DYNAMIC_TEXTURE_WIDTH;
if (h > DYNAMIC_TEXTURE_HEIGHT) h = DYNAMIC_TEXTURE_HEIGHT;

if (!m_ppUITexture[src]) { LoadUITexture(src,-1); if (!m_ppUITexture[src]) return 0; }
srcSurf = tex[src]->GetSurfaceLevel(0);   srcSurf->GetDesc(&sd);  srcSurf->LockRect(&s,0,0);

if (!m_ppUITexture[dst])
    D3DXCreateTexture(dev, DYNAMIC_TEXTURE_WIDTH, DYNAMIC_TEXTURE_HEIGHT,
                      1, 0, sd.Format, D3DPOOL_MANAGED, &tex[dst]);
else if (format changed) { release and recreate as above; }
dstSurf = tex[dst]->GetSurfaceLevel(0);  dstSurf->LockRect(&d,0,0);

if (w + dstX >= DYNAMIC_TEXTURE_WIDTH)  w = DYNAMIC_TEXTURE_WIDTH  - dstX;
if (h + dstY >= DYNAMIC_TEXTURE_HEIGHT) h = DYNAMIC_TEXTURE_HEIGHT - dstY;

if (sd.Format is 16-bit (==23 || 24 || 25)) {           // 2 bytes/texel
    for (i=0;i<h;i++) for (j=0;j<w;j++)
        u16[dst + (dstY+i)*DYN_W + dstX + j]
            = u16[src + 2*sd.Width*(srcY+i) + 2*j + srcX];
} else {                                                 // 4 bytes/texel
    for (i=0;i<h;i++) for (j=0;j<w;j++)
        u32[dst + (dstY+i)*DYN_W + dstX + j]
            = u32[src + 2*sd.Width*(srcY+i) + 2*j + srcX];
}
```

`[OBSERVED]` (`0x4ba3ca`; indices are in *texels* of the respective surface)

Notes that matter:

* The destination is `m_ppUITexture[dstUISlot]` — **not** `m_ppDynamicTexture`.
  `m_ppDynamicTexture[72]` is only ever NULLed (`0x4ba353`), released
  (`0x4b7ca2`) and read (`0x4ba9fc`, `0x4ba392`); no code writes it.
  `GetDynamicTexture` therefore always returns `NULL` in 748. `[OBSERVED]`
* The 2×2 decimation: with `DYN_W == 256` the source is sampled every second
  texel in **both** x and y (`2*j`, `2*sd.Width*(...)`). Because `srcX/srcY` are
  not halved, the first sampled row is `2*srcY`. All shipped call sites pass
  `srcX = srcY = 0`, where the two conventions coincide. `[OBSERVED]` +
  `[INFERRED]` for other call sites.
* With `DYN_W == 512` (the value a modern adapter selects) the function performs
  a straight 1:1 copy of an `w×h` rect — no decimation, no halving.
* The only caller in the whole binary is the minimap composite builder
  (`0x534968`, one xref to `0x4ba3ca`): it walks a 3×3 neighbourhood, resolves
  `UI\m%02d%02d.wyt` through `GetUITextureIndex`, falls back to index 3
  (`UI\minimap2.wyt`) or 13 (`UI\minimap.wyt`), and calls
  `GenerateTexture(4, idx, tileX<<7, (2-tileY)*128, 0, 0, 128, 128)`.
  `[OBSERVED]` (= port `TMGround.cpp:3754-3762`)
* Destination slot 4 of the UI list is `UI\minimap.wyt` (128×128, 24 bpp) — the
  same file as slot 13. The composite therefore lives in a *separate* slot that
  only becomes a 512×512 sheet because `GenerateTexture` creates it empty first.
  If anything ever calls `GetUITexture(4, …)` before the first
  `GenerateTexture(4, …)`, the file is loaded into that slot (128×128), the blit
  reuses it and the 3×3 grid is clamped to a single 128×128 tile.
  `[INFERRED]` (the clamp logic is observed; the call ordering is not)
  With `DYN_W = 512` the 3×3 grid occupies 384×384 of the 512×512 sheet, which
  is exactly why the 512 path exists.

### 10.2 Guild marks (`LoadGuildTexture`, `0x4baa4d`)

```c
if (idx < 0 || idx > 64) return 0;                 // note: 64 allowed -> 65 slots used
if (!szMarkBuffer || m_stGuildMark[idx].pTexture) return 0;
return SUCCEEDED(D3DXCreateTextureFromFileInMemoryEx(
    dev, szMarkBuffer, /*size*/ 632,
    /*w*/ 16, /*h*/ 12, /*mips*/ 0, /*usage*/ 0,
    g_pDevice->m_eFormat, D3DPOOL_MANAGED,
    /*filter*/ 1, /*mipfilter*/ 1, /*colorkey*/ 0, 0, 0,
    &m_stGuildMark[idx].pTexture));
```

`[OBSERVED]`

* Buffer size is hard-coded **632 bytes**, decoded by D3DX's sniffers (so it is
  a self-describing image), forced to **16×12**, format = **backbuffer format**
  (so RGBA8/XRGB8), mip count 0 = full chain, no colour key.
* `m_stGuildMark[64]` at `this+4`, stride 12 bytes: `pTexture`, `nGuild`
  (init −1), `dwLastRenderTime`. `[OBSERVED]` (`0x4baae7`, `0x4bab41`)
* `[UNKNOWN]` The inner image format of the 632-byte buffer. 632 is not
  `14+40+16*12*3` (630) nor any 16×12 TGA size (594/786), so it is probably a
  TGA/BMP with an extra 2-byte or a differently padded row; the buffer arrives
  from a server packet whose layout is outside this document.
  Resolve by capturing a guild-mark packet in-game and running a format sniffer.

### 10.3 The font glyph page (context only)

`TMFont2` (`0x421b8f`) builds its glyph atlas itself:

1. If `m_bSavage`, `CreateTexture(256, 32, 1, 0, D3DFMT_A4R4G4B4, MANAGED)`.
2. Otherwise it loads `UI\minimap.wyt` through the *same* TGA trick (4-byte
   magic skip + `"TRUEVISION-XFILE"` append) and asks D3DX for a
   `256 × 32` `A4R4G4B4` texture with colour key `0xFF000000`. The file content
   is irrelevant — the texture is immediately overwritten.
3. It then locks the texture and writes, for every glyph pixel,
   `texel = (nibble << 12) | 0x0FFF`, where `nibble = (gdiPixel & 0xFF) >> 4`
   — i.e. white RGB, alpha from the GDI text bitmap.

`[OBSERVED]` (`0x421b8f`, string `"UI\\minimap.wyt"` @ `0x5b16ec`; the port's
`TMFont2.cpp:196-236` matches, including `MiniMap_Path`)

The glyph page size is the global `m_nFontTextureSize = 256` at `0x5b4918`
(height `256/8 = 32`). `[OBSERVED]`

### 10.4 Render-target textures — absent in 748

`TextureManager` in 748 has no render-target/bloom machinery:

* the log string `"Error in Init Render Target Texture"` does not exist in
  `WYD.exe`; `"Bloom"` does not exist either. `[OBSERVED]`
* the ctor (`0x4b7780`) does not allocate any `D3DUSAGE_RENDERTARGET` texture;
  the only render targets in the binary are the device's own back/stencil
  buffers. `[OBSERVED]`

What the (755/760) code that the port keeps behind `#if 0`
(`TextureManager.cpp:1446-1569`) would have done, had it been enabled: allocate
five full-screen `X8R8G8B8` textures in `D3DPOOL_MANAGED`
(`m_pOriginTexture`, `m_pAccumTexture`, `m_pRenderTexture`, plus two bloom
chains at `W>>2 × H>>2` and `W>>1 × H>>1`, and an optional multisampled
`CreateRenderTarget` for `m_pOriginRTT`), all for a bloom/glow post-process.
`[INFERRED]` from the port's commented-out source; it is dead code in 748 and
must stay dead — `D3DUSAGE_RENDERTARGET` with `D3DPOOL_MANAGED` is invalid in
D3D9 and makes device creation fail.

---

## 11. UV conventions, filtering, wrapping

### 11.1 Two drawing paths, two UV conventions

**Path A — `RenderRect`** (`RenderDevice.cpp:1893`), used for ordinary UI
images: an `ID3DXSprite::Draw` with a pixel-space `RECT` source:
`left = srcX, top = srcY, right = srcX + w, bottom = srcY + h`, destination
`(destX, destY)` top-left, rotation centre at the rect centre, no rotation.
`ID3DXSprite` maps the rect with `u = x / texWidth`, `v = y / texHeight`, with
the sprite's **top-left** origin convention and no half-texel inset.
`[OBSERVED]` for the call; `[INFERRED]` for D3DXSprite's internal formula
(D3DX is a black box; the convention is documented as top-left based).

**Path B — `RenderRectTex`** (`RenderDevice.cpp:2043`), used for effects,
stretch/clip and rotated quads: a hand-built quad with an explicit **half-texel
inset**:

```c
fTexWidth  = 512; fTexHeight = 512;                  // defaults if no texture
if (pTexture) pTexture->GetLevelDesc(0, &desc), fTexWidth = desc.Width,
                                                   fTexHeight = desc.Height;

tu0 = srcX / W + 0.5f / W;                 tv0 = srcY / H + 0.5f / H;
tu1 = (srcX + w) / W - 0.5f / W;           tv1 = (srcY + h) / H - 0.5f / H;
```

`[OBSERVED]` So a `w × h` source rect is mapped onto the *centres* of its first
and last texel: the sampled footprint is `(w-1)/W` wide, not `w/W`.
`[INFERRED]` consequence: path B de-magnifies by `(w-1)/w` — a 1-pixel border
tone difference versus a naive `w/W` mapping. Reproduce it if you want
pixel-identical output.

Two additional quirks of path B:

* `if (m_iVGAID == 1)` (3dfx) the upper UV is quantised to 1/10000 steps:
  `fEndX = trunc((srcX+w)/W * 10000) * (1/10000)`. `[OBSERVED]`
* `v = 0` is the **top** row of the texture in both paths. `[OBSERVED]` (the
  quad vertex order puts `tv0` at `destY`, `tv1` at `destY + h`).

### 11.2 The origin trap

All 221 `.wyt` files store rows bottom-up (TGA descriptor bit 5 = 0, §2.2).
Since the UI is authored and drawn top-down (rects, `ID3DXSprite`, screen-space
y-down) and renders correctly, the uploaded texture **must end up top-down**
(row 0 = top of the visible image). `[INFERRED]` (strong: two independent
top-down assumptions in the draw path plus a working shipped client)

Therefore: if your loader hands the TGA rows to the GPU in file order, every UI
element will be vertically mirrored. `[INFERRED]`

`[UNKNOWN]` Whether D3DX itself flips (honouring the descriptor) or the art was
authored upside-down. Both produce the same shipped result. Experiment to
resolve: convert `UI\MessageBox2.wyt` to `.tga`, open it in a viewer that honours
the descriptor, and confirm the panel's title bar is at the top.

### 11.3 Filtering and wrapping

| state | value | where |
|---|---|---|
| `D3DSAMP_MINFILTER` | `2` = `D3DTEXF_LINEAR` | 2-D state block (`RenderDevice.cpp:1626`) |
| `D3DSAMP_MAGFILTER` | `2` = `D3DTEXF_LINEAR` | same |
| `D3DSAMP_MIPFILTER` | `1` = `D3DTEXF_POINT` | same |
| 3-D blocks | `1/1/1` (POINT) at `1784-1786`, `2/2/2` (LINEAR) at `1744-1746`, `1816-1818` | per pass |
| `D3DSAMP_ADDRESSU/V` | **never set** → `D3DTADDRESS_WRAP` (D3D9 default) | — |
| `D3DRS_ALPHAREF` | `0xDD` (221) on 32-bpp devices, `0xD` (13) otherwise | every draw path, reset after each quad |
| `D3DRS_ALPHAFUNC` | `8` = `D3DCMP_ALWAYS` in the 2-D paths; `7` = `D3DCMP_GREATEREQUAL` in the 3-D pass | `ALPHATESTENABLE` is `0` in the transparent 2-D path and `1` in the 3-D pass |

`[OBSERVED]` — i.e. alpha *testing* on `.wyt` transparency is not done via
`ALPHAREF` in 2-D: the colour key already made those texels `(0,0,0,0)` and plain
alpha blending draws them. `[INFERRED]`

`[OBSERVED]` for the `SetSamplerState` calls (`RenderDevice.cpp:1626-1628,
1744-1756, 1784-1786, 1816-1818`); `[INFERRED]` for the WRAP default (no
`ADDRESSU`/`ADDRESSV` call exists in the port source and the port mirrors the
binary, so the client never changes the default).

Practical meaning: linear textures, nearest mip selection (only one mip for UI
anyway), and **no clamping** — a quad whose UVs touch exactly 0 or 1 can bleed
one texel from the opposite edge of the atlas because bilinear taps wrap.
`[INFERRED]`

### 11.4 Texture size vs screen size

Nothing is pre-scaled at load. A source rect is a pixel rect in the atlas;
the destination rect is a pixel rect on the (already scaled) screen; the quad
size does the scaling (`RENDER_IMAGE_STRETCH` computes
`fScaleX = control.nWidth / coord.nWidth`). UVs are derived from the *actual*
texture dimensions via `GetLevelDesc`, so non-power-of-two atlases
(800×600, 1024×512, 234×256 all exist) work without padding. `[OBSERVED]`

---

## 12. Names, paths, case sensitivity, missing files

| Topic | Observation |
|---|---|
| Separators in list records | `UI\\name.wyt` (52 records) *and* `UI\name.wyt` (single) coexist in `UITextureList.bin`; the other three lists use single `\` only |
| Case in list records | `mesh\…` (all lowercase) while the *directory* is `client_748/mesh` (lowercase) and the *path constant* is `Mesh\\MeshTextureList.bin` (capital M) |
| Case mismatches vs disk | 23 UI records (`UI\inven01.wyt` vs `Inven01.wyt` … `inven23.wyt`), 1 effect (`EFFect\re0012.wys`), 1 mesh (`mesh\bl010101.wys`) → 25 records only resolve on a case-insensitive filesystem |
| Case sensitivity of the format test | `strcmp(suffix, ".wys")` is case-sensitive: `FOO.WYS` would be treated as a TGA |
| Missing files | 1 UI (`UI\\cancel.wyt`), 1 mesh (`mesh\bird0101.wys` = model texture index **0**), 83 Env (`Env\Tile*x2/x3.wys` variants). All load attempts fail cleanly (return 0 → `NULL`) |
| Path resolution | the record string is passed to `_open` **verbatim** — no prefix, no extension guessing, no search path |
| File-name length | up to 254 characters + NUL |

`[OBSERVED]` (census over all four lists, cross-checked against the filesystem)

The client never falls back to another file or to a default texture. A port
should resolve names case-insensitively and collapse duplicate separators, or
25 textures will silently disappear: **all 23 inventory atlases**
(`UI\inven01.wyt` … `UI\inven23.wyt`, records 9–59), `Effect\re0012.wys` and
`mesh\bl010101.wys`. On a case-sensitive filesystem the inventory window would
therefore lose every item/panel graphic. `[OBSERVED]`

---

## 13. Reimplementation notes (OpenGL / Vulkan)

### 13.1 Pure format work (must be re-implemented bit-exactly)

1. **`.wyt` reader**: `file[4:] + b"TRUEVISION-XFILE.\x00"` is a 26-byte-footer
   TGA 2.0. Uncompressed, bottom-up rows, 16/24/32 bpp, no RLE, no mips.
   Convert to RGBA8 (16 bpp: expand A1R5G5B5 → 0/255 alpha; 24 bpp: alpha 255
   then apply the colour key; 32 bpp: straight BGRA→RGBA).
2. **`.wys` reader**: `dds = b"DDS " + file[5:]`, then patch
   `dds[0x54:0x58]` to `DXT1` if `dds[0x54] == '2'` else `DXT3`. Upload as
   BC1/BC2 (GL: `GL_COMPRESSED_RGB_S3TC_DXT1_EXT` /
   `GL_COMPRESSED_RGBA_S3TC_DXT3_EXT`; Vulkan: `VK_FORMAT_BC1_RGB_UNORM_BLOCK`
   / `VK_FORMAT_BC2_UNORM_BLOCK`). Reject nothing; the header is otherwise
   standard.
3. **Colour key** for `.wyt` only: RGB (0,0,0) → (0,0,0,0).
4. **List files**: 264-byte records, 512/512/2048/512, NUL-terminated names,
   `0xCD` filler, ignore the two trailing DWORDs.
5. **Set list**: CRLF text, `ControlTextureCoord` 28 bytes, 512-set bound.
6. **Mip policy**, exactly as §8.4 (this is what makes distant effect textures
   look the way they do).
7. **Priority order** of the four `cAlpha` tests for the non-DXT fallback path.

### 13.2 API work (translate, do not copy)

| D3D9 | Meaning | OpenGL | Vulkan |
|---|---|---|---|
| `D3DPOOL_MANAGED` | driver keeps a system copy, survives device loss | ordinary `glTexImage*` | `DEVICE_LOCAL` + host staging |
| `D3DXCreateTextureFromFileInMemoryEx` | decode + upload + format convert | your own decode → `glTexImage2D` | your own decode → `vkCmdCopyBufferToImage` |
| `D3DFMT_A4R4G4B4 / A1R5G5B5` | 16-bit packed | `GL_RGBA4` / `GL_RGB5_A1` | `VK_FORMAT_A4R4G4B4_UNORM_PACK16` / `A1R5G5B5_UNORM_PACK16` |
| `D3DFMT_A8R8G8B8` | BGRA8 | `GL_BGRA`/`GL_RGBA8` | `VK_FORMAT_B8G8R8A8_UNORM` |
| `D3DFMT_X8R8G8B8` | BGRX8 | `GL_RGB8` | `VK_FORMAT_B8G8R8A8_UNORM` (ignore A) |
| `D3DCOLOR` colour key | transparent-black replacement | do it in the decoder | do it in the decoder |
| `D3DTEXF_LINEAR` / `POINT` | min/mag filter | `GL_LINEAR` / `GL_NEAREST` | `VK_FILTER_LINEAR` / `NEAREST` |
| `D3DTADDRESS_WRAP` (default) | UV wrap | `GL_REPEAT` | `VK_SAMPLER_ADDRESS_MODE_REPEAT` |
| `ID3DXSprite::Draw` | top-left rect blit | your batcher, `u = x/W` | same |
| half-texel inset (`RenderRectTex`) | `+0.5/W`, `-0.5/W` | keep verbatim | keep verbatim |
| `ALPHAREF 0xDD` + `GREATER` | alpha cutout at 221/255 | `glAlphaFunc` / shader discard | shader discard |

### 13.3 Keep 1:1 (do not "improve")

* 1 mip level for UI; 4 for effects; the model/env index-dependent rule.
* BOX filtering when generating mips (a Kaiser/Lanczos box will change the look
  of distant effects).
* Colour-key black knockout on `.wyt` only.
* 512/512/2048/512 index spaces — art indices are baked into scene binaries.
* 512-entry set list, 72 dynamic slots, 64 guild marks, `16×12` guild size.
* `DYNAMIC_TEXTURE_WIDTH/HEIGHT`: 512 when the adapter allows textures larger
  than 256, else 256. Modern hardware always takes the 512 path — meaning a
  straight 1:1 blit, **not** the 2×2 decimation.
* `timeGetTime()`-based eviction (900 ms sweep gate, `showTime` per call,
  5000 ms for dynamic slots).
* Case-insensitive, duplicate-separator-tolerant path resolution.

### 13.4 May be modernised safely

* BC1/BC2 upload instead of D3DX's internal decompress-and-recompress: identical
  pixels, less memory. (The 748 client already kept them compressed.)
* Your own texture array/atlas allocator instead of 2048 separate objects.
* Streamed/async decode: the original is synchronous on the render thread.
* `vkImage` per texture with a descriptor index instead of `SetTexture`.

### 13.5 Specific traps that cause visible differences

| # | Trap | Symptom |
|---|---|---|
| 1 | Treating `.wys` as a normal DDS (4-byte magic) | garbage/black model & terrain textures |
| 2 | Mapping the `"7x0s"` tag to BC3/DXT5 | effects lose their hard alpha edges, colour noise |
| 3 | Mapping `"2z73"`/`"7x0s"` as *literal* fourCC strings | every `.wys` fails to load |
| 4 | Not flipping bottom-up TGA rows | all UI is vertically mirrored |
| 5 | No colour key on `.wyt` | every 24-bpp panel is an opaque black box |
| 6 | Colour key applied to `.wys` too | legitimate black pixels vanish from models |
| 7 | Enabling BC1 punch-through alpha (DXT1a) | `.wys` DXT1 gets holes it never had (DDPF_ALPHAPIXELS is clear) |
| 8 | Forcing power-of-two sizes | 800×600 and 1024×512 atlases get resampled/shifted |
| 9 | Using `w/W` instead of `(w-1)/W` in path B | 1-texel seams on rotated/stretched effects |
| 10 | Clamping samplers instead of WRAP | atlas edge bleeding changes on filter taps |
| 11 | Trusting `dwShowTime` from the bin (`0xCDCDCDCD`) | nothing is ever evicted, or everything is evicted instantly |
| 12 | Copying the port's `MB`/`nIndex > MAX` bounds checks | out-of-bounds read at index == capacity |
| 13 | Taking the UI copy of `MeshTextureList.bin` | 395 model textures missing |
| 14 | Case-sensitive path resolution | 25 textures missing |
| 15 | "Fixing" `TextureManager.cpp:271` (`nLength - 4`) — that is the `.wyt`/TGA branch; the `.wys` branch is already `- 1` | `.wys` stops loading |

---

## Appendix A — constants

`[OBSERVED]` (748 ctor `0x4b7780` unless noted)

| Constant | 748 value | Evidence |
|---|---:|---|
| `MAX_UI_TEXTURE` | 512 | `operator_new(0x800)`; loops `0x200` |
| `MAX_EFFECT_TEXTURE` | 512 | `operator_new(0x800)` |
| `MAX_MODEL_TEXTURE` | 2048 | `operator_new(0x2000)`; loops `0x800` |
| `MAX_ENV_TEXTURE` | 512 | `operator_new(0x800)` |
| `MAX_DYNAMIC_TEXTURE` | 72 | `operator_new(0x120)`; loops `0x48` |
| `MAX_GUILD_MARK` | 64 | loops `0x40`, stride 12 |
| `MAX_UI_TEXTURE_SET_LIST` | **512** (port says 600) | `memset(this+0x328,0,0x1000)`; `if (idx < 0x200)` in `0x4b8928` |
| `GUILD_MARK_WIDTH/HEIGHT` | 16 / 12 | `0x10`, `0xc` in `0x4baa4d` |
| `DYNAMIC_TEXTURE_WIDTH/HEIGHT` | 256 / 256 static, 512/512 when `D3DCAPS.MaxTextureHeight > 256` | `0x5ba694`, `0x5ba698`; `0x427119` |
| `TextureList_FileFormat` | `".wys"` | string @ `0x5ba7d0` |
| `m_szTGAHeader` | `"TRUEVISION-XFILE" 0x2E 0x00` (18 B) | `this+0x304`, `[16]=0x2E` |
| `m_szDDSHeader` | `"DDS"` + `0x00` (4 B) | `this+0x317`, string @ `0x5ba6b0` |
| D3DX filters | `MipFilter 1` = NONE, `5` = BOX | D3DX9 enum |
| D3D pool | `1` = `D3DPOOL_MANAGED` | every create call |
| `m_nMipMap` default | 20 | data @ `0x5b4920` |
| `m_bDxt` default | 1 | data @ `0x5b4924` |
| `m_nFontTextureSize` | 256 (height 32) | data @ `0x5b4918` |
| sweep interval | 900 ms | `0x4b7f0d` |
| dynamic timeout | 5000 ms | `0x4b7f0d` |

Object layout of `TextureManager` (from the ctor; useful if you compare dumps):

| offset | member |
|---:|---|
| 0x004 | `m_stGuildMark[64]` (12 B each) |
| 0x304 | `m_szTGAHeader[19]` |
| 0x317 | `m_szDDSHeader[4]` |
| 0x324 | `m_LastLoadTime` |
| 0x328 | `m_UITextureSetList[512]` (8 B each, 0x1000 total) |
| 0x1328 | `m_stUITextureList[512]` (0x21000) |
| 0x22328 | `m_stEffectTextureList[512]` |
| 0x43328 | `m_stModelTextureList[2048]` (0x84000) |
| 0xC7328 | `m_stEnvTextureList[512]` |
| 0xE8328 | `m_ppUITexture[512]` |
| 0xE832C | `m_ppEffectTexture[512]` |
| 0xE8330 | `m_ppModelTexture[2048]` |
| 0xE8334 | `m_ppEnvTexture[512]` |
| 0xE8338 | `m_ppDynamicTexture[72]` |
| 0xE833C | `m_dwDynamicLastUsedTime[72]` |

`[OBSERVED]` Note the 32-bit pointers: this is a 32-bit build; a 64-bit port's
layout will differ but the *contents* are as listed.

## Appendix B — measured statistics

`.wys` (2336 files: 1618 mesh, 305 Env, 413 Effect) `[OBSERVED]`

| property | values |
|---|---|
| `dwSize` | 124 (100 %) |
| `dwFlags` | `0x1007` 1085, `0x21007` 1251 |
| `ddspf.dwSize / dwFlags` | 32 / 4 (100 %) — always FOURCC |
| raw fourCC tag | `2z73` 1980, `7x0s` 356 |
| `dwMipMapCount` | 0 (1085), 3 (146), 4 (1105) |
| `dwCaps` | 0x1000 (1085), 0x401008 (1251) |
| height / width | height 4…256, width 1…512; almost all powers of two, but `18x32` and `62x32` exist — do not assume POT |
| tag by folder | Effect: 344 × `2z73`, 69 × `7x0s`; Env: 305 × `2z73`; mesh: 1331 × `2z73`, 287 × `7x0s` |
| size validation | 2336/2336 exact (`filesize-129 == Σ ceil(w/4)*ceil(h/4)*blocksize`) |

`.wyt` (221 files) `[OBSERVED]`

| property | values |
|---|---|
| magic | `WT10` (100 %) |
| image type / colour map / ID length | 2 / 0 / 0 (100 %) |
| bpp | 24 (180), 32 (39), 16 (2) |
| descriptor | `0x00` (180), `0x01` (2), `0x08` (39) — origin always bottom-left |
| mipmaps | none (size math exact) |
| dimensions | 128×128 (98), 256×256 (71), 256×512 (17), 512×256 (10), 64×128 (9), 800×600 (6), 256×128 (4), 234×256 (2), 512×512 (2), 1024×512 (1), 128×64 (1) — largest 1024×512, tallest 800×600 |

Lists `[OBSERVED]`

| list | capacity | live | max index | alpha census |
|---|---:|---:|---:|---|
| `UI/UITextureList.bin` | 512 | 239 | 261 | A 60, a 5, N 174 |
| `Effect/EffectTextureList.bin` | 512 | 418 | 438 | A 66, a 1, N 351 |
| `mesh/MeshTextureList.bin` | 2048 | 1585 | 1662 | A 218, C 84, N 1283 |
| `Env/EnvTextureList3.bin` | 512 | 376 | 395 | N 376 |

UI extensions: 211 × `.wyt`, 28 × `.wys` (`mesh\mt01xxxxUI.wys`).
Set list: 446 sets (0…456), 1851 declared items, 1849 present rows, `destX =
destY = 0` for all rows, rect widths 0…1024, heights 0…600.
Missing on disk: 1 UI, 1 mesh, 83 Env. Case-mismatched: 23 UI, 1 Effect, 1 mesh.

## Appendix C — unresolved items

| Item | Why it matters | How to resolve |
|---|---|---|
| Origin of the `"2z73"`/`"7x0s"` tags | a port must not hash them as real fourCCs | find the `.wys` exporter (not shipped); only the first byte is used |
| Whether D3DX flips bottom-up TGAs | if it does *not*, the art is authored upside-down and a naive flip breaks everything | export `UI\MessageBox2.wyt` to TGA, view it, and compare with `docs/port-748/screenshots/message-box-original-748.png` |
| Which `D3DCAPS` field gates 256 vs 512 | determines whether the minimap blit decimates | step through `0x427119` at runtime, or trust the offset-0x5C = `MaxTextureHeight` reading |
| Guild-mark buffer format (632 B) | needed to reproduce cloak emblems offline | capture a guild-mark packet and sniff the header |
| Writer of `m_ppDynamicTexture[]` | `GetDynamicTexture` is dead if nothing writes it | grep the binary for stores to `this+0xE8338`; none found here |
| `cAlpha == 'C'` semantics | unknown art-pipeline flag, 84 model textures | compare `'C'` vs `'A'` textures visually; both are usually soft-alpha |
| Ordering of minimap generation vs slot-4 load | minimap tiles truncate if the 128×128 file wins | in-game test of the HUD |
| Whether the GUI colour-key comparison happens pre- or post-format-conversion | affects 16-bit device output only | run on a 16-bit presentation format |
