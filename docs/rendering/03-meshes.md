# 03 — Static Mesh Loading (`.msh`, `.msa`, `.bon`, mesh registries)

Purpose: byte-exact specification of how the WYD 748 client turns mesh/bone files on disk
into GPU buffers, so the same geometry can be recreated in OpenGL or Vulkan without reading
the C++ source or the shipped executable.

Last updated: 2026-11-10

Scope note (important, read first): in this code base the extension names are **swapped
relative to their usual meaning**:

* `*.msh` = **skinned** mesh, loaded by `CMesh::LoadMesh` — one texture, up to 4 bone
  influences per vertex, 64-byte palette matrices in the header. `[OBSERVED] CMesh.cpp:794-931`
* `*.msa` = **static / "common"** mesh, loaded by `TMMesh::LoadMsa` — one to eighteen
  sub-meshes, each with its own texture, no bones. `[OBSERVED] TMMesh.cpp:481-728`

Animation *playback* (`.ani` sampling, `FrameMove`) is out of scope; this document stops at
the rest pose, the hierarchy and the matrix palette that the vertex shader consumes.

---

## Confidence summary

| Tag | Count | Meaning |
|---|---|---|
| `[OBSERVED]` | 96 | read in `Projects/TMProject/*` or measured in `client_748` |
| `[INFERRED]` | 8 | deduced from evidence, stated inline |
| `[UNKNOWN]` | 7 | unresolved, with the experiment that would settle it |

Structural facts that are *proved* by arithmetic over the whole asset set (not sampled):

* **657 / 657** `.msh` files satisfy `32 + numPalette*64 + numPalette*4 + vertexCount*sizeVertex + indexCount*2 == filesize` exactly. `[OBSERVED]` (`evidence/analysis/mesh-proof-msh-sizing.txt`)
* **1661 / 1661** `.msa` files satisfy `12 + attribCount*20 + attribCount*11 + 4 + ibBytes + 4 + vbBytes == filesize` exactly. `[OBSERVED]`
* **0** vertices (over all 271 610 weighted vertices of all 657 files) reference a palette slot
  `>= numPalette`, and **0** index values are `>= vertexCount`. `[OBSERVED]`
* **1585** non-empty records in `Mesh\MeshTextureList.bin` = 2048 × 264 bytes, remainder 0. `[OBSERVED]`

Biggest unknowns:

1. `[UNKNOWN]` The 3-token records at the head of every `Shader\skinmeshN.bin` open with
   token `0x0000001F` (D3DSIO_DCL in the D3D9 opcode numbering), which is not legal for the
   `vs_1_1` version token that follows. The *body* of the shaders decodes cleanly and to the
   last token, so only these declaration-like records are unexplained. Experiment: run
   `D3DXDisassembleShader` (D3DX9) on the blob and compare, or diff against a
   `Shader\skinmeshN.bin` produced by `D3DXAssembleShader` from reconstructed source.
2. `[UNKNOWN]` semantics of `m_sLegendType` 116..125 (item glow tiers) — the texture index
   bases are known (275/288/301/314/327/340/353/366/425/392) but what the ten tiers *mean*
   in game terms is data-driven and not documented anywhere in the source.
3. `[UNKNOWN]` `Mesh\tn.bon` (8 777 247 B) and `Mesh\ed.bon` (7 178 971 B) are not bone
   lists (sizes are not multiples of 8, both start with the same encrypted byte pattern as
   `object.bin`). They are never opened: `BoneAni4.txt` does not list them, while
   `TMPaths.h:61-62` references non-existent `Mesh\tn.dat` / `Mesh\ed.dat` instead.
   Experiment: run the `object.bin` key over them and look for a header.

---

## Table of contents

1. [File inventory, extensions and case sensitivity](#1-file-inventory-extensions-and-case-sensitivity)
2. [`.msh` — skinned mesh format](#2-msh--skinned-mesh-format)
3. [`.msa` — static / common mesh format](#3-msa--static--common-mesh-format)
4. [`.bon` — bone hierarchy (+ `BoneAni4.txt`, `ValidIndex.bin`)](#4-bon--bone-hierarchy)
5. [Registry files and their index spaces](#5-registry-files-and-their-index-spaces)
6. [`object.bin` — the object footprint mask](#6-objectbin--the-object-footprint-mask)
7. [From file bytes to GPU buffers](#7-from-file-bytes-to-gpu-buffers)
8. [Draw-time state: `SetMaterial`, texturing, alpha, culling](#8-draw-time-state)
9. [Multi-texture / legend / multi-type / god2 flags](#9-multi-texture--legend--multi-type--god2-flags)
10. [Naming conventions](#10-naming-conventions)
11. [Decompiler artefacts and landmines](#11-decompiler-artefacts-and-landmines)
12. [Reimplementation notes (OpenGL / Vulkan)](#12-reimplementation-notes-opengl--vulkan)
13. [Evidence index](#13-evidence-index)

---

## 1. File inventory, extensions and case sensitivity

Measured in `E:\wyd\test-setup\client_748\mesh` (4741 files). `[OBSERVED]`

| Ext | Count | Meaning | Loaded by |
|---|---|---|---|
| `.msa` | 1661 | static mesh (also 37 more in `Effect\`) | `TMMesh::LoadMsa` |
| `.wys` | 1618 | model texture (1-byte-prefixed DDS, or 4-byte-prefixed TGA) | `TextureManager::LoadModelTexture` |
| `.ani` | 718 | one animation clip per file | animation agent |
| `.msh` | 657 | skinned mesh | `CMesh::LoadMesh` |
| `.bon` | 69 | bone hierarchy (2 are not bones, see §4) | `MeshManager::InitBoneAnimation` |
| `.txt` | 7 | registries / legends | see §5 |
| `.bin` | 6 | `MeshTextureList.bin`, `ItemList.bin`, `SkillData.bin`, `UITextureList.bin`, `ValidIndex.bin`, `cdata.bin` | various |
| `.dat` | 5 | terrain attribute maps / field placement | other docs |

Root of `client_748` contains `object.bin` (524 292 B) and nothing else mesh-related.
There is **no** `MeshList.txt`, `CommonMeshList.txt` or `MeshTextureList.txt` at the root —
the only copies are inside `mesh\`. `[OBSERVED]` (directory listing)

**Case sensitivity.** The client mixes cases freely and relies on Win32 case-insensitive
lookups:

* `TMPaths.h:51-55` — `MeshTextureList_Path = "Mesh\\MeshTextureList.bin"`,
  `MeshList_Path = "Mesh\\MeshList.txt"`, `BoneAni4_Path = "Mesh\\BoneAni4.txt"`,
  `ValidIndex_Path = "Mesh\\ValidIndex.bin"` (directory `Mesh`, capital M).
* `TMPaths.h:68-69` — `MeshTexture_FileFormat_Path = "mesh\\%s.wys"` (lowercase `mesh`).
* `Mesh\MeshList.txt` lines contain literal **double** backslashes: `1 mesh\\sky001.msa`.
  `fscanf(fp, "%d %s\r\n", ...)` copies that verbatim into `stCommonMeshList.szFileName`
  (`MeshManager.cpp:62-65`) and it is passed straight to `fopen`. Win32 collapses the
  doubled separator, so it works — but a POSIX/GL port must normalise it. `[OBSERVED]`
  `evidence/hex/mesh-meshlist-txt-head.txt`
* One bone file is `BL01.bon` (uppercase) and one duplicate is `mi010101.bon`; the skin-mesh
  loader builds `mesh\BL010101.msh` from `BoneAni4.txt` entry `mesh\BL01`
  (`BoneAni4.txt` line for index 44 is `44 5 4 mesh\bl01` — lowercase `bl01`, while the file
  on disk is `BL01.bon`). So the `.bon` open **depends on case-insensitivity**. `[OBSERVED]`

**.wyt is a virtual extension for meshes.** `TMSkinMesh.cpp:179-192` builds texture names as
`"%s%02d%02d.wyt"` but there are **zero** `.wyt` files under `mesh\` (only `UI\*.wyt`
exists). The lookup still succeeds because `TextureManager::GetModelTextureIndex`
(`TextureManager.cpp:956-973`) overwrites the last three characters of the *query* string
with each table entry's last three characters before `strcmp`:

```c
int nStrLen  = strlen(szTextureName);
int nStrLen2 = strlen(m_stModelTextureList[i].szFileName);
sprintf((char*)&szTextureName[nStrLen - 3], (char*)&m_stModelTextureList[i].szFileName[nStrLen2 - 3]);
if (!strcmp(szTextureName, m_stModelTextureList[i].szFileName)) return i;
```

Consequences (all `[OBSERVED]` by reading the loop):

1. The extension is effectively ignored — `.wyt` queries resolve to `.wys` entries.
2. The first match wins (the table contains duplicates, see §5.2).
3. The call **mutates the caller's string tail** (a bug that is also a load-bearing behaviour).
4. `sprintf` with `nStrLen - 3` underflows if the query is shorter than 3 characters.

---

## 2. `.msh` — skinned mesh format

### 2.1 Header (32 bytes, little-endian, no padding)

`[OBSERVED] CMesh.cpp:821-830` (read order), verified against 657 files.

| Offset | Type | Field (decompiled name) | Meaning |
|---|---|---|---|
| 0x00 | `uint32` | `dwParentID` | parent bone id. **Read into a local and never used** (`CMesh.cpp:821-822`) — the hierarchy comes from `.bon`. `[OBSERVED]` |
| 0x04 | `uint32` | `m_dwID` | id of the bone this whole mesh is rigidly attached to (becomes `CMesh::m_dwID`, used for `CFrame::FindFrame` lookup and weapon/part special cases) |
| 0x08 | `uint32` | `m_dwFVF` | D3D FVF of the vertex block. Observed values: `0x1116`, `0x1118`, `0x111A`, `0x111C`, and one `0x12` |
| 0x0C | `uint32` | `m_sizeVertex` | bytes per vertex (36 / 40 / 44 / 48; 24 for the `0x12` file) |
| 0x10 | `uint32` | `m_numFaceInflunce` | **number of bone influences per vertex**, 1..4 (the name is a long-standing typo for "face influence") |
| 0x14 | `uint32` | `m_numPalette` | number of palette (bone) matrices that follow, 0..26 |
| 0x18 | `uint32` | `AttRange[0].VertexCount` | vertex count |
| 0x1C | `uint32` | `AttRange[0].FaceCount` | **index count** (i.e. `3 × triangle count`); the loader stores `m_numFaces = FaceCount / 3` |

`m_dwAttCount` is never read from the file: it stays **0** for a `.msh` (only
`m_AttRange[0].VertexCount/FaceCount` are filled), which is exactly why a `.msh` cannot be
drawn through `TMMesh::RenderDraw` (it loops `i < m_dwAttCount`) and is drawn by
`CMesh::RenderMesh` instead. `VertexStart`/`FaceStart` stay 0 from the constructor
`memset` (`TMMesh.cpp:33`). There is **no** per-face texture id and **no** per-face material
or alpha flag anywhere in the file — a `.msh` is one texture and one draw call. `[OBSERVED]`

### 2.2 Body layout

| Offset | Size | Content |
|---|---|---|
| 32 | `numPalette * 64` | `numPalette` × `D3DXMATRIX` (16 floats, row-major, only rows 0..2 used — the 4th column is 0, the 4th row is 0,0,0,1) |
| `32 + 64*numPalette` | `numPalette * 4` | `numPalette` × `uint32` **bone id** for each palette slot (`m_dwNames`) |
| … | `vertexCount * sizeVertex` | vertex block, layout depends on influence count (§2.3) |
| … | `indexCount * 2` | `uint16` triangle list indices, 0-based into the vertex block |

Total size (the identity checked on all 657 files):

```
filesize == 32 + 64*numPalette + 4*numPalette + vertexCount*sizeVertex + indexCount*2
```

The palette matrix read is `fread(m_pBoneMatrix, m_numPalette << 6, 1, handle)` i.e.
`numPalette * 64` bytes; the names read is `fread(m_dwNames, 4, m_numPalette, handle)`.
Note `malloc(160)` for names — a latent overflow if `numPalette > 40`; max observed is 26.
`[OBSERVED] CMesh.cpp:837-842`

### 2.3 Vertex layouts

FVF decoding (`D3DFVF_*`): `0x1116 = XYZB1|NORMAL|TEX1|LASTBETA_UBYTE4`,
`0x1118 = XYZB2|…`, `0x111A = XYZB3|…`, `0x111C = XYZB4|…`. `D3DFVF_LASTBETA_UBYTE4 = 0x1000`
turns the **last** blend weight into a packed 4-byte value. `[OBSERVED]` (FVF values in the
files) + `[INFERRED]` (the FVF bit meaning; the layout is then proved by the D3D9 vertex
declarations in `RenderDevice.cpp:770-912`, §7.1)

`n = numFaceInfluence`, `S = sizeVertex = 32 + 4n`:

| Offset | Type | n=1 | n=2 | n=3 | n=4 | Meaning |
|---|---|---|---|---|---|---|
| +0 | `float3` | 0 | 0 | 0 | 0 | position (object-space, rest pose) |
| +12 | `float` | – | +12 | +12 | +12 | blend weight 0 |
| +16 | `float` | – | – | +16 | +16 | blend weight 1 |
| +20 | `float` | – | – | – | +20 | blend weight 2 |
| `12+4(n-1)` | `uint32` | +12 | +16 | +20 | +24 | packed palette indices, byte *k* = slot for influence *k* |
| `16+4(n-1)` | `float3` | +16 | +20 | +24 | +28 | normal (object space) |
| `28+4(n-1)` | `float2` | +28 | +32 | +36 | +40 | texture coordinate set 0 |
| | | 36 | 40 | 44 | 48 | total |

There is **no** second UV set and **no** vertex colour in `.msh`. `[OBSERVED]`

Empirical proof of the index packing: because the value is declared as `D3DDECLTYPE_D3DCOLOR`
(`0xAARRGGBB`), the shader applies a `.zyxw` swizzle to undo the BGRA permutation
(§7.3), and the *file* byte order is plain little-endian: byte 0 = influence 0 … byte 3 =
influence 3. Measured over all 271 610 weighted vertices: for 2-influence meshes bytes 2 and 3
are **always 0** and bytes 0/1 reach 23/24; for 4-influence meshes all four bytes are used
(max 25). `[OBSERVED]` (`evidence/analysis/mesh-proof-msh-vertices.txt`)

Weights: all in `[0,1]`, and `sum(stored weights) <= 1.0` in **all** 271 610 weighted
vertices — the final weight is implicit: `w_last = 1 - sum(others)`. This is exactly what
the vertex shaders compute (§7.3). `[OBSERVED]`

### 2.4 Worked example 1 — `mesh\ch010264.msh`, 214 bytes

`evidence/hex/mesh-msh-ch010264-full.txt`

```
00000000  00 00 00 00 2D 00 00 00 16 11 00 00 24 00 00 00
00000010  01 00 00 00 01 00 00 00 03 00 00 00 03 00 00 00
00000020  53 4B 00 B4 6C F7 BC 35 FF FF 7F BF 00 00 00 00   <- palette matrix (64 B)
...
00000060  08 00 00 00                                        <- palette bone id = 8
00000064  67 CF A7 40 78 C9 E2 BD 07 9A 91 3F | 00 00 00 00
00000074  82 5F DF 3E 75 F4 49 BF E3 98 DD 3E | EF 57 D3 3B 16 53 F2 3E
000000D0  00 00 01 00 02 00                                 <- 3 indices = 1 triangle
```

| Field | Value | Check |
|---|---|---|
| dwParentID | 0 | |
| dwID | 45 (`0x2D`) | valid bone in `ch01.bon` (47 bones, ids 0..46) |
| FVF | `0x1116` | XYZB1 + LASTBETA_UBYTE4 → 36 B |
| sizeVertex | 36 | matches FVF |
| influences | 1 | |
| numPalette | 1 | |
| vertexCount | 3 | |
| indexCount | 3 | 1 triangle |
| **size** | **32 + 64 + 4 + 108 + 6 = 214** | **= filesize 214** ✔ |

First vertex at 0x64: pos = (5.2441, −0.1107, 1.1375), index dword = 0 (slot 0),
normal = (0.436, −0.789, 0.433), uv = (0.00645, 0.4734).
Palette matrix row 0 = (−4.6e−7, 1.4e−6, −0.99999, 0), row 1 = (−0.0871, −0.9969, −1.3e−6, 0),
row 2 = (0.9969, −0.0871, −2.4e−7, 0), row 3 (translation) = (−0.9917, −0.0232, 5.2361, 1).
`[OBSERVED]`

### 2.5 Worked example 2 — `mesh\or020101.msh`, 142 084 bytes

`evidence/hex/mesh-msh-or020101-header.txt`

| Field | Value |
|---|---|
| dwParentID | 0 |
| dwID | 1 |
| FVF | `0x111C` (XYZB4 + LASTBETA_UBYTE4) |
| sizeVertex | 48 |
| influences | 4 |
| numPalette | 25 |
| vertexCount | 2680 |
| indexCount | 5856 (= 1952 triangles) |

```
size = 32 + 64*25 + 4*25 + 2680*48 + 5856*2
     = 32 + 1600 + 100 + 128640 + 11712 = 142084   == filesize 142084  ✔
```

First vertex at `32 + 1700 = 1732 (0x6C4)`: pos = (−0.3851, −2.0737, 0.0586),
`w0 = 1.0`, `w1 = 0.0`, `w2 = 0.0` (bytes `00 00 80 3F | 00 00 00 00 | 00 00 00 00`),
index dword = `0x0000000E` → influence0 slot 14 (valid, 25 palettes), influences 1..3 = 0,
normal = (−0.2742, 0.6428, −0.7158), uv = (0.5346, 0.4077).
Palette ids (first 8): `34, 33, 37, 35, 16, 15, 20, 28` — all < 55 (`or02.bon`). `[OBSERVED]`

### 2.6 Cross-file facts

* All 657 `.msh` have a same-stem-prefix `.bon` (first 4 characters + `.bon`, e.g.
  `ch010264.msh` → `ch01.bon`, `BL010101.msh` → `BL01.bon`), and every palette id and every
  `dwID` is a valid bone id of that `.bon`. `[OBSERVED]`
* Influence count histogram across the 657 files: 1 → 56, 2 → 198, 3 → 162, 4 → 240, and one
  file with 0 influences (the `bt010201.msh` case below). `[OBSERVED]`
* `mesh\bt010201.msh` (296 B) is the only non-skinned `.msh`: FVF `0x12`
  (XYZ|NORMAL, **no texture coordinate**), 24 B/vertex, `numFaceInfluence = 0`,
  `numPalette = 0`, 9 vertices, 24 indices. `CMesh::RenderMesh` rejects
  `m_numFaceInflunce < 1` (`CMesh.cpp:608-609`) and returns without drawing, so this asset is
  dead. `[OBSERVED]` `evidence/hex/mesh-msh-bt010201-full.txt`

---

## 3. `.msa` — static / common mesh format

### 3.1 Layout

`[OBSERVED] TMMesh.cpp:494-497, 513-621, 627-670`; verified on all 1661 files.

| Offset | Type | Field | Meaning |
|---|---|---|---|
| 0x00 | `uint32` | `m_dwFVF` | `0x112` (1636 files), `0x142` (24 files), `0x12` (1 file: `grid.msa`) |
| 0x04 | `uint32` | `m_sizeVertex` | 32 for `0x112`, 24 for `0x142`/`0x12` |
| 0x08 | `uint32` | `m_dwAttCount` | number of sub-meshes, observed 1..18 (`m_AttRange[32]` is not bounds-checked) |
| 0x0C | `attribCount × 20` | `D3DXATTRIBUTERANGE[]` | 5 × `uint32`: `AttribId, FaceStart, FaceCount, VertexStart, VertexCount` |
| … | `attribCount × 11` | texture name per attribute | 11 raw ASCII bytes, NUL-padded (max observed length 10, so always terminated) |
| … | `uint32` | index buffer size (bytes) | |
| … | that many bytes | `uint16` indices | shared index/vertex space across all sub-meshes |
| … | `uint32` | vertex buffer size (bytes) | |
| … | that many bytes | vertex block | |

```
filesize == 12 + 20*attribCount + 11*attribCount + 4 + ibBytes + 4 + vbBytes
```

**`FaceCount` in `.msa` is a triangle count, not an index count** — the opposite of `.msh`.
Proof: `TMMesh::RenderDraw` calls
`DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, VertexStart, VertexCount, 3*FaceStart, FaceCount)`
(`TMMesh.cpp:221-227`), and `sum(FaceCount)*3 == ibBytes/2` holds for all 1661 files.
`[OBSERVED]`

Everything is consistent across the asset set: `max(VertexStart+VertexCount) <= vertexCount`,
`max(index) < vertexCount`, `sum(FaceCount)*3 == indexCount`. `[OBSERVED]`

### 3.2 Vertex layouts and the load-time conversion

`0x112` → `XYZ|NORMAL|TEX1` (pos 12, normal 12, uv 8 = 32 B). At load, for every file whose
FVF is not `322`, `TMMesh::LoadMsa` does (`TMMesh.cpp:644-670`):

```
m_dwFVF  += 0x100              ; 0x112 -> 0x212  (TEXCOORDCOUNT 1 -> 2)
m_sizeVertex += 8              ; 32 -> 40
for each vertex: copy 32 bytes, leaving 8 bytes of (uninitialised) space after it
```

The 8 extra bytes are later filled by the animated-UV writer (§8.3) as `tu2/tv2`. The
resulting in-memory vertex is `RDVERTEX2` (`TMMesh.h:5-13`):

| Offset | Type | Field |
|---|---|---|
| 0 | `float3` | position |
| 12 | `float3` | normal |
| 24 | `float2` | uv set 0 |
| 32 | `float2` | uv set 1 (runtime only) |
| = 40 B | | |

`0x142` → `XYZ|DIFFUSE|TEX1` (pos 12, `D3DCOLOR` diffuse 4, uv 8 = 24 B). **No conversion
happens**: the second UV is not added, so the animated-UV path is skipped for these files
(`TMMesh::Render` requires `m_dwFVF == 530`). All 2582 vertices in all 24 such files have a
diffuse dword of exactly `0x00000000`, i.e. the channel is vestigial — treat it as padding.
`[OBSERVED]`

`grid.msa` has FVF `0x12` (XYZ|NORMAL, no UV) with 24-byte vertices; the "+8 bytes, +0x100"
rule is applied to it anyway, producing 32-byte in-memory vertices whose two UVs are garbage.
`[OBSERVED]` `evidence/hex/mesh-msa-grid-full.txt`

### 3.3 Texture name resolution

For each attribute the 11-byte name is stripped at the last `\` and at the first `.`
(`TMMesh.cpp:516-548`) and re-extended:

```
"mesh\\%s.wys"     if the mesh path's directory is not "effect"
"Effect\\%s.wys"   if it is
```

then resolved with `TextureManager::GetModelTextureIndex` / `GetEffectTextureIndex`.
If that returns −1, the loader retries with the **mesh file's own stem** as the texture name
(`TMMesh.cpp:568-598`) and gives up (logs `"%s , %s"` and returns 0) only if that also fails.
Measured: 2819 of 2885 texture references in `.msa` files resolve directly against
`MeshTextureList.bin` (97.7 %); the 66 misses are exactly the cases the fallback exists for.
`[OBSERVED]`

The `aa` special case (`TMMesh.cpp:513-556`): if the *mesh file name* starts with `aa`, the
11-byte texture name is read but then ignored and replaced by the mesh's own stem. No `.msa`
in the 748 data set starts with `aa`, so this path is dead here. `[OBSERVED]`

### 3.4 Worked example — `mesh\sp0001.msa`, 159 bytes

`evidence/hex/mesh-msa-sp0001-full.txt`

```
00000000  42 01 00 00 18 00 00 00 01 00 00 00            FVF=0x142 sizeVertex=24 attribs=1
0000000C  00 00 00 00 00 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00
          |attribId=0|faceStart=0|faceCount=2|vertexStart=0|vertexCount=4|
00000020  73 70 30 30 35 2E 74 67 61 00                  "sp0005.tga\0"  (11 B)
0000002B  0C 00 00 00                                      ibBytes = 12 (6 indices)
0000002F  00 00 01 00 02 00 03 00 02 00 01 00              2 triangles
0000003B  60 00 00 00                                      vbBytes = 96 (4 × 24)
0000003F  ... 96 bytes of vertices ...
```

```
size = 12 + 20*1 + 11*1 + 4 + 12 + 4 + 96 = 159  == filesize 159  ✔
```

Texture `sp0005.tga` → stem `sp0005` → `mesh\sp0005.wys` → row of `MeshTextureList.bin`.

### 3.5 Multi-sub-mesh example — `mesh\aef040.msa`

`evidence/hex/mesh-msa-aef040-header.txt` — FVF `0x112`, 32 B/vertex, 4 attributes,
vbBytes 33 568 (1049 vertices), ibBytes 4752 (2376 indices):

| # | AttribId | FaceStart | FaceCount | VertexStart | VertexCount | Texture |
|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 558 | 0 | 744 | `jwal02.tga` |
| 1 | 1 | 558 | 72 | 744 | 124 | `aef004.tga` |
| 2 | 2 | 630 | 144 | 868 | 165 | `aef001.tga` |
| 3 | 3 | 774 | 18 | 1033 | 16 | `dun011.tga` |

`(558+72+144+18)*3 = 2376` indices ✔; `744+124+165+16 = 1049` vertices ✔. Draw order is the
array order, one `DrawIndexedPrimitive` per attribute. `[OBSERVED]`

---

## 4. `.bon` — bone hierarchy

### 4.1 Format

Trivially simple: an array of 8-byte records, `numBone = filesize / 8`
(`MeshManager.cpp:115-123`).

| Offset | Type | Field |
|---|---|---|
| +0 | `int32` | parent bone id; `-1` (`0xFFFFFFFF`) for the root |
| +4 | `uint32` | this bone's id |

No names, no local transforms, no rest pose, no inverse-bind matrices — **none**. The rest
pose comes from frame 0 of the `.ani` clips (animation agent), and the palette matrices that
bind vertices to bones live in each `.msh` header. `[OBSERVED]`

`evidence/hex/mesh-bon-ch01-full.txt` (376 B = 47 bones):

```
00000000  FF FF FF FF 00 00 00 00   parent=-1, id=0   (root)
00000008  00 00 00 00 01 00 00 00   parent= 0, id=1
00000010  01 00 00 00 02 00 00 00   parent= 1, id=2
00000018  01 00 00 00 03 00 00 00   parent= 1, id=3
00000020  03 00 00 00 04 00 00 00   parent= 3, id=4   ...
```

`numBone` per file: 5 (`tr13/14/15.bon`) … 70 (`BL01.bon`); 20 for a small tree (`tr01`),
35 for an orc (`or01`), 47/48 for the two player rigs (`ch01`/`ch02`), 57..62 for large props
(`sp01`, `sq01`). `[OBSERVED]`

### 4.2 How it is turned into a tree

`TMSkinMesh::RestoreDeviceObjects` (`TMSkinMesh.cpp:119-153`), per bone *i* in file order:

```c
parent_ID  = pBone[2*i];            // int; -1 means root
parent_raw = parent_ID;             // kept for CFrame::m_dwParentID
if (parent_ID == -1) parent_ID = 0; // -1 is rewritten to 0 for the lookup
my_ID      = pBone[2*i+1];
tmp = new CFrame(my_ID);
tmp->m_dwParentID = parent_raw;     // note: the RAW value (-1), not the patched one
m_pframeToAnimate[my_ID] = tmp;     // <- id is used as an ARRAY INDEX
parent = m_pRoot->FindFrame(parent_ID);
if (parent) parent->AddFrame(tmp);
```

Consequences a reimplementation must copy:

* Bone ids must be dense `0..n-1` and `< MAX_FRAME_TO_ANIMATE` (100) or the array write
  walks off `m_pframeToAnimate[100]` (`TMSkinMesh.h:9-10, 60`).
* The synthetic root `CFrame(0)` is created first, so bone id 0 is a *child* of the real
  root; the root bone's `m_dwParentID` is `-1`.
* `mi01.bon` and `mi010101.bon` are **byte-identical** 280-byte / 35-bone duplicates (same
  SHA-256) — two accepted spellings of the same rig, because the mesh-stem rule turns
  `mi010101.msh` into `mi01.bon` but some editor tooling emitted the 8-character form.
  `[OBSERVED]`

### 4.3 `Mesh\BoneAni4.txt` — which `.bon` belongs to which rig, and how many parts

Text, one line per rig (`MeshManager.cpp:104-113`):

```
<rigIndex> <numAniTypeCount> <numParts> <path-without-.bon>
```

```
0 186 8 mesh\ch01
1 149 8 mesh\ch02
2 55 8 mesh\or01
...
44 5 4 mesh\bl01
...
87 1 2 mesh\tr21
```

* `rigIndex` → `MeshManager::m_BoneAnimationList[rigIndex]`, index space `0..99`
  (`MAX_BONE_ANIMATION_LIST = 100`). It is *also* the `nBoneAniIndex` passed all over the
  engine (`TMSkinMesh` ctor) — 0 = male player, 1 = female player, 2.. = monsters,
  20.. = props/NPCs, 60 = ship, 61 = leaf/grass, 62 = flower, 63.. = trees, 85 = mantle,
  86 = wings/familiar, 87 = special tree. `[OBSERVED]` (`BoneAni4.txt` + call sites)
* `numAniTypeCount` = number of `.ani` files for this rig (also the size of the
  `m_stValidAniList[rigIndex][*]` sub-array).
* `numParts` = **number of `.msh` parts** drawn per rig; the loader loops `i < numParts` and
  builds `"%s%02d%02d.msh"` with `(i+1)` and the per-part "look" value (§10).
* `BoneAni3.txt` is the previous generation of the same file (no `rigIndex` column —
  `<numAni> <numParts> <path>`); it is not loaded by this client. `[OBSERVED]`

`Mesh\ValidIndex.bin` (74 400 B) is read with a single
`fread(m_stValidAniList, sizeof(m_stValidAniList), 1, fp)` (`MeshManager.cpp:92`):
`stValidAniList = int nI` × `[100][186]` = 100*186*4 = 74 400 ✔. It maps file-order `.ani`
number → "valid index", whose decimal digits are decoded as `weapon = idx/100-1`,
`motion = idx%100-1` when filling `m_sAnimationArray[class][weapon][motion]`
(`MeshManager.cpp:189-192`). That indexing is animation-domain; see the animation document.

---

## 5. Registry files and their index spaces

### 5.1 `Mesh\MeshList.txt` — common-mesh index space (the big one)

Text, `fscanf(fp, "%d %s\r\n", &nIndex, szMeshName)` (`MeshManager.cpp:62-65`).

```
1 mesh\\sky001.msa
2 mesh\\even01.msa
3 mesh\\bnsh01.msa
...
2887 mesh\\shout.msa
```

Measured `[OBSERVED]`: 1703 lines, all parse as `<int> <path>`; indices 1..2887; 1701 unique
(two duplicated indices, 1710 and 1789 — the **later** line overwrites the earlier one because
the store is a plain array write); 1186 index values in 1..2887 are *not* present. Directory
mix: 1667 `mesh\`, 36 `effect\`. Extensions: 100 % `.msa`.

That index is `m_dwObjType` for map objects (`TMObject::Render` →
`GetCommonMesh(m_dwObjType, 0, 3_min)`, `TMObject.cpp:253`), `n3DObjIndex` for placed objects
and items (`TMObjectContainer.cpp:919`, `SGrid.cpp:697`, `ItemList.bin` records), and a few
hard-coded numbers (`704/705`, `703`, `2964+`). The array is
`stCommonMeshList m_stCommonMesh[3048]` and `GetCommonMesh` rejects `nIndex >= 3048`
(`MeshManager.cpp:465`). `[OBSERVED]`

Missing files: 28 entries point at `.msa` files that do not exist on disk (e.g. index 490/657
→ `mesh\abox01.msa`, 1001 → `mesh\ch010201.msa`); conversely 27 `.msa` files on disk are not
listed at all. Both are harmless — the loader logs and leaves a null mesh. `[OBSERVED]`

`stCommonMeshList` (`MeshManager.h:5-14`) — one entry per slot in *both* arrays:

| Field | Meaning |
|---|---|
| `char szFileName[64]` | path from `MeshList.txt` (common list) or the name passed to `CMesh::LoadMesh` (skin list) |
| `TMMesh* pMesh` | the loaded mesh; `nullptr` = empty/evictable slot |
| `D3DXMATRIX* m_pBoneMatrix` | skin list only: `numPalette` × 64 B (copied out of the `.msh` header) |
| `DWORD* m_dwNames` | skin list only: the palette bone ids |
| `DWORD m_dwID` | skin list only: the `.msh` `dwID` (attach bone) |
| `DWORD m_numFaceInflunce` | skin list only: influences per vertex |
| `DWORD m_numPalette` | skin list only: palette size |

### 5.2 `Mesh\MeshTextureList.bin` — model-texture table

`TextureManager::InitModelTextureList` does a single
`fread(m_stModelTextureList, sizeof(m_stModelTextureList), 1, fp)` (`TextureManager.cpp:727`),
i.e. the file is a **raw memory image** of `stTextureListInfo m_stModelTextureList[2048]`
(`TextureManager.h:39-45, 110`).

Record = 264 bytes (`[OBSERVED]`, 540 672 / 264 = 2048 remainder 0):

| Offset | Type | Field |
|---|---|---|
| 0x00 | `char[255]` | `szFileName` — full path, e.g. `mesh\bird0101.wys` |
| 0xFF | `char` | `cAlpha` — one of `N`, `A`, `C` (and `a`, handled in code but absent here) |
| 0x100 | `uint32` | `dwLastUsedTime` — **filler `0xCDCDCDCD`** (never initialised by the tool), not a time |
| 0x104 | `uint32` | `dwShowTime` — same filler `0xCDCDCDCD` |

Both DWORDs are runtime-only scratch (they are overwritten by `GetModelTexture`), so the on-disk
value must be ignored. `[OBSERVED]` `evidence/hex/mesh-texturelist-bin-record0.txt`

Measured: 1585 non-empty slots (highest used index 1662), all `.wys`; 2585 empty slots still
carry `cAlpha = 'N'`. Alpha histogram over non-empty slots: `N` 1283, `A` 218, `C` 84.
Duplicates (same path in two slots): `mesh\wh010101.wys`, `mesh\ch010219.wys`,
`mesh\ag010101.wys`. `[OBSERVED]`

The table index is what `CMesh::m_nTextureIndex` and `TMMesh::m_nTextureIndex[i]` store.
Hard-coded slots used by the port: `204` (UI 3-D icons, `RenderDevice.cpp:3391`) and
`1840..1843` (LB/LK mantles, `TMSkinMesh.cpp:306-329`) — **the latter are empty slots in the
748 table** (highest used index is 1662), so those mantles render untextured. `[OBSERVED]`

`Mesh\MeshTextureList.txt` (34 573 B, 1382 lines, format `"<idx> <path> <alpha> "` with a
trailing space) is a **stale human-readable dump** of the same table: the binary has 1585
entries, the text 1382. Nothing reads the `.txt`. Same for `Mesh\CommonMeshList.txt`
(887 lines, indices 0..1975, 64 of which disagree with `MeshList.txt` at the same index) and
`Mesh\Mesh.txt` (a Korean naming legend, see §10). `[OBSERVED]`

### 5.3 Skin-mesh index space

`CMesh::LoadMesh` keeps a 1024-entry cache of *already loaded* `.msh` files keyed by path
(`MAX_SKIN_MESH_LIST = 1024`, `MeshManager.h:35`):

```c
idx = GetSkinMeshIndex(path);              // linear strcmp scan of the 1024 paths
if (idx == -1) idx = GetAvailableSkinMeshIndex();   // first slot with pMesh == nullptr
if (idx < 0) return 0;                     // cache full -> mesh silently does not load
```

`[OBSERVED]` `CMesh.cpp:794-811`, `MeshManager.cpp:482-505`

Eviction: **none**. A skin mesh is freed only by `MeshManager::ReleaseMesh` (at shutdown).
The common-mesh list, by contrast, is evicted (§5.4).

### 5.4 `GetCommonMesh` and the LRU rule

```c
TMMesh* GetCommonMesh(int nIndex, int bEffect, DWORD showTime) {
    if (nIndex < 0 || nIndex >= 3048) return 0;                 // MAX_COMMON_MESH
    if (!m_stCommonMesh[nIndex].pMesh) LoadCommonMesh(nIndex, bEffect);  // Sleep(0), new, LoadMsa
    if (!m_stCommonMesh[nIndex].pMesh) return 0;
    TMMesh* pMesh = m_stCommonMesh[nIndex].pMesh;
    if (pMesh->m_bEffect != bEffect) pMesh->m_bEffect = bEffect; // effect vs model texture space
    pMesh->m_dwLastUsedTime = timeGetTime();
    pMesh->m_dwShowTime     = showTime;
    return pMesh;
}
```

`[OBSERVED]` `MeshManager.cpp:442-480`

`showTime` is a **per-call lease in milliseconds**, not an insertion counter: map objects pass
`3_min` (180 000 ms), items in the inventory grid `180000`, quest/UI previews `3_min`.
The sweeping is done by `ReleaseNotUsingMesh` (`MeshManager.cpp:377-395`), called once per
frame from `NewApp::RenderScene` (`NewApp.cpp:739`), which itself rate-limits to one pass per
1200 ms and deletes `pMesh` when `now - m_dwLastUsedTime > m_dwShowTime`. Deletion is
straight `SAFE_DELETE` (drops the VB/IB) — there is **no LRU ordering**, just per-mesh expiry,
which is why a mesh touched by two different callers with different leases flaps between
loaded and unloaded. `[OBSERVED]`

If `LoadMsa` fails the code keeps the object but clears `m_bEffect`
(`MeshManager.cpp:457-458`), so a failed load is retried on the next access.

---

## 6. `object.bin` — the object footprint mask

### 6.1 Content

`MeshManager::ReadObjectMask` (`MeshManager.cpp:397-440`):

```
struct: 2048 masks × 16 rows × 16 bytes  (524 288 B)  +  4-byte checksum
which is exactly  char m_aObjectMask[2048][16][16]      (MeshManager.h:42-44, 72)
```

The read is `fread(m_aObjectMask[MaskNum][MaskY], 16, 1, fp)` for all 2048×16 rows, then the
4-byte checksum is read **after** decryption. `[OBSERVED]`

### 6.2 Encryption (present, and required)

The 524 288 mask bytes are stored obfuscated with a 170-byte key held as a 172-element
`unsigned char[172]` whose last two bytes are `0x00` so that `strlen()` == 170
(`MeshManager.h:78-91`). The key is **85 Korean characters in cp949/EUC-KR** — the lyrics of
the children's songs *금강산 찾아가자 / 새나라의 어린이 / 무궁화* — used as a **byte pad**:

```
금강산찾아가자일만이천봉볼수록아름답고신비하구나우리나라좋은나라새나라의어린이는
일찍일어납니다잠꾸러기없는나라우리나라좋은나라무궁화무궁화우리나라꽃삼천리강산에
우리나라꽃
```

```c
int nLen = strlen(ObjectMaskEncKeys);           // 170
for (j = 0; j < sizeof(m_aObjectMask); ++j)
    pTemp[j] = pTemp[j] - ObjectMaskEncKeys[j % nLen] - j;      // 8-bit wrap
```

Both the key index and the absolute byte offset `j` participate — so decryption is
position-dependent and cannot be done per-row. `[OBSERVED]` `MeshManager.cpp:423-427`

Verification: the first 16 bytes of `object.bin` are
`B1 DE B2 B0 BF EF C9 AA C6 CF BA AC CC E7 CE DE`, and decrypting them yields sixteen zero
bytes, consistent with `evidence/hex/mesh-object-bin-first256.txt` beginning with a run of
empty masks. `[OBSERVED]`

### 6.3 Checksum (over the *encrypted* bytes)

```c
// computed while reading, BEFORE decryption
for (MaskNum 0..2047) for (MaskY 0..15) for (i 0..15)
    nCheckSum += (signed char)m_aObjectMask[MaskNum][MaskY][i] % 4 + 2*MaskY + 5*MaskNum;
// ...then decryption, then
fread(&nRCheckSum, 4, 1, fp);
if (nCheckSum != nRCheckSum) MessageBoxA(0, "Object.bin File Crashed", "CheckSum Error", MB_OK);
```

The stored value at offset `0x80000` is `-1604083155` (`0xA063A22D` when read as `uint32`),
and recomputing the formula with C truncating `%` on *signed* bytes and 32-bit wraparound
reproduces it exactly. `[OBSERVED]` (`evidence/analysis/mesh-proof-objectbin.txt`)

### 6.4 What the mask is for: terrain height + obstacle occupancy (not rendering)

`TMObject::RegisterMask` (`TMObject.cpp:607-654`) rotates the 16×16 mask by the object's yaw
and writes it into the terrain's **height/mask grid**:

```c
int nTemp = (int)((m_fHeight / 0.1f) + 3 * cTempMask[y][i]);
if (m_fHeight > 0.0f) ++nTemp;
if (nTemp > 127) nTemp = 127;
if ((char)nTemp > pGround->m_pMaskData[y][128*nBaseY + i + nBaseX])
    pGround->m_pMaskData[y][128*nBaseY + i + nBaseX] = nTemp;
```

with `nBaseX = (int)fX - 7`, `nBaseY = (int)fY - 7`, i.e. a 16×16 stamp centred on the object.
The terrain mask is a per-tile `char[128][128]` grid, filled with terrain heights in
`TMGround.cpp:2598-2664` and saturated to `127` for water/void. So an object's footprint makes
those cells report the object's *height* (in 0.1-unit steps) instead of the terrain height,
capped at 127. `[OBSERVED]`

Consumers (all `[OBSERVED]`, `TMScene::GroundGetMask` at `TMScene.cpp:1827-1848`):

* **Ground height for spawning / effects / drops / projectiles** — the dominant use:
  `y = GroundGetMask(pos) * 0.1f` appears in ~40 places (`TMFieldScene.cpp:6442`, `TMHuman.cpp`
  ~2198/3792/3836/12605, `TMDrop.cpp:100`, `TMRain.cpp:181`, skill projectiles, …).
* **Occupancy / camera blocking** — `TMObject::isCamPos` (`TMObject.cpp:692`) and
  `TMObject.cpp:765` treat `mask >= 126` (and `> 125`) as "this cell is inside an obstacle",
  which drives the camera's zoom-in instead of clipping through buildings and large props.
* **Water tests** — `TMScene.cpp:2062-2089` passes `GroundGetMask(pos) * 0.1f` into
  `IsInWater`.

So: the mask is a **terrain height + obstacle footprint field**. It is never read by the
renderer, no mesh is culled or occluded with it, and no per-pixel work uses it.
The decrypted value set is consistent with that: `0` (510 076 bytes), small values `1..31`
(3 605 bytes) and `127` (10 607 bytes) — see `evidence/analysis/mesh-proof-objectbin.txt`.
`[OBSERVED]` for the histogram, `[INFERRED]` for "127 = solid/full-height sentinel".

`m_nMaskIndex` per object comes from the terrain `.dat` placement record
(`TMObjectContainer.cpp:921`, `TMObject::Save`), not from the mesh.
`[OBSERVED]`

---

## 7. From file bytes to GPU buffers

### 7.1 `TMMesh` — what it actually is

`TMMesh` does **not** wrap `ID3DXMesh` and never calls `DrawSubset`. It owns a plain
`IDirect3DVertexBuffer9*` + `IDirect3DIndexBuffer9*` plus the attribute ranges, and draws them
itself (`TMMesh.h:36-58`, `TMMesh.cpp:215-232`). `[OBSERVED]`

| Field | Meaning |
|---|---|
| `m_bEffect` | `1` → textures come from the effect table (`GetEffectTexture`), else model table |
| `m_bDummy` | set when the file could not be opened; `CMesh::LoadMesh` then refuses the mesh |
| `m_sizeVertex` | bytes per vertex actually uploaded (32/40 for `.msa`, 24/36/40/44/48 for `.msh`) |
| `m_dwFVF` | D3D FVF, used for `.msa` (`SetFVF`); for `.msh` it is only used to create the buffer, the vertex *declaration* wins |
| `m_dwAttCount`, `m_AttRange[32]` | sub-mesh table: **0** for `.msh` (CMesh draws directly), 1..18 for `.msa` |
| `m_nTextureIndex[32]` | one model/effect texture index per attribute |
| `m_dwLastUsedTime`, `m_dwShowTime` | LRU lease (§5.4) |
| `m_nMeshType`, `m_fScaleH/V`, `m_vecCenter`, `m_fRadius`, `m_fMaxX..m_fMinZ` | runtime bounds/scale (bounds computed at load, see §7.5) |
| `m_bFirst` | one-shot flag used by scroll modes 6/101 |

Upload paths:

`.msa` (`TMMesh::LoadMsa`): `CreateVertexBuffer(vbBytes [+8*nVerts], 8=D3DUSAGE_WRITEONLY,
m_dwFVF, D3DPOOL_MANAGED)` → `Lock` → `fread` directly into the lock → `Unlock`;
`CreateIndexBuffer(ibBytes, 8, D3DFMT_INDEX16, MANAGED)` → same. For the `+8` case the copy is
done vertex-by-vertex with a stride hole (`TMMesh.cpp:666-667`), so the second UV set is
**uninitialised memory** until first written.
`.msh` (`CMesh::LoadMesh`): `CreateVertexBuffer(sizeVertex*VertexCount, 0, m_dwFVF,
D3DPOOL_MANAGED)` and a single `fread` of the whole vertex block; index buffer is
`2*FaceCount` bytes of `D3DFMT_INDEX16` (`CMesh.cpp:852-908`). No conversion of any kind:
the file bytes **are** the GPU vertex data (positions/normals/UVs are in the model's rest
pose and are transformed on the GPU, never on the CPU).

### 7.2 Vertex declarations (`.msh`)

`RenderDevice::InitVertexShader` (`RenderDevice.cpp:768-963`) builds four declarations and
eight shaders. `CMesh::RenderMesh` picks `m_pVertexDeclaration[m_numFaceInflunce - 1]` — or
`[m_numFaceInflunce + 3]` when `m_nBoneAniIndex == 61` (leaves/grass) — and the matching
`m_pVertexShader[]` (`CMesh.cpp:610-623`). Because the declaration array is built with
`VertexDecl[i % 4]`, shader slots 4..7 reuse declarations 0..3. `[OBSERVED]`

`D3DDECLTYPE`: 0=FLOAT1, 1=FLOAT2, 2=FLOAT3, 4=D3DCOLOR, 17=UNUSED.
`D3DDECLUSAGE`: 0=POSITION, 1=BLENDWEIGHT, 2=BLENDINDICES, 3=NORMAL, 5=TEXCOORD.

**1 influence** (`VertexDecl1`, `.msh` sizeVertex 36):

| Stream | Offset | Type | Usage | Bytes |
|---|---|---|---|---|
| 0 | 0 | FLOAT3 | POSITION | 12 |
| 0 | 12 | D3DCOLOR | BLENDINDICES | 4 |
| 0 | 16 | FLOAT3 | NORMAL | 12 |
| 0 | 28 | FLOAT2 | TEXCOORD0 | 8 |

**2 influences** (sizeVertex 40): POSITION@0, FLOAT1 BLENDWEIGHT@12, D3DCOLOR BLENDINDICES@16,
NORMAL@20, TEXCOORD0@32.
**3 influences** (44): POSITION@0, FLOAT1@12, FLOAT1@16, BLENDINDICES@20, NORMAL@24, TEXCOORD0@36.
**4 influences** (48): POSITION@0, FLOAT1@12, 16, 20, BLENDINDICES@24, NORMAL@28, TEXCOORD0@40.

This matches the FVF-derived layout of §2.3 byte for byte, which is the second independent proof
of that table. `[OBSERVED]`

For `.msa` the FVF is used directly (`SetFVF(m_dwFVF)` in `TMMesh::Render`), so there is no
declaration: `0x212` = XYZ|NORMAL|2×TEXCOORD(2D) for the converted meshes and `0x142` =
XYZ|DIFFUSE|TEXCOORD for the 24 legacy ones. `[OBSERVED]`

Index format everywhere: `D3DFMT_INDEX16` (16-bit), `D3DPT_TRIANGLELIST`, no strips/fans.

### 7.3 The skinning vertex shaders (`Shader\skinmesh1..8.bin`)

Each file is a compiled `vs_1_1` blob (`version token 0xFFFE0101`), 464..784 bytes, loaded
verbatim and passed to `CreateVertexShader` (`RenderDevice.cpp:936-963`). I disassembled them
from the token stream — the bodies decode end-to-end into valid D3D9 shader instructions, so
the reconstruction below is `[OBSERVED]`; only the four/five 3-token declaration records at
the head remain `[UNKNOWN]` (see Confidence summary).

Constant registers used (matches what the C++ writes):

| Register | Written by | Content |
|---|---|---|
| `c0` | `CMesh::SetMaterial` (`CMesh.cpp:993-1036`) | `(1.0, material.Power, fProgress, 765.01001)` |
| `c1` | `RenderDevice` block 1 / `CMesh::RenderMesh` | light direction (`normalize(-1,1,1)` for skinned meshes) |
| `c2..c5` | `RenderDevice.cpp:1848-1851` | `transpose(m_matProj)`, refreshed by `SetProjectionMatrix()` |
| `c6` | `SetMaterial` | fog: `(1.0, fogEnd, 1/(fogEnd-fogStart), 0)` |
| `c7` | `SetMaterial` | ambient×0.25 + emissive (a colour) |
| `c8` | `SetMaterial` | `D3DMATERIAL9.Diffuse` (first row of the material struct) |
| `c9 + 3*i` | `RenderMesh` (`CMesh.cpp:625-631`) | palette matrix *i*: 3 rows of `m_pBoneMatrix[i] * m_pBoneOffset[i] * view`, transposed |
| `c92..c95` | `RenderMesh` (`CMesh.cpp:637-640`) | `transpose(inverse(view))` |

Palette budget: 3 registers per bone from `c9`; `c92` is the ceiling ⇒ at most 27 bones,
and the largest `numPalette` in the data is 26 (last matrix at `c84..c86`). `[OBSERVED]`

Decoded body (slot *n* = influence count):

```
; index -> constant-register addressing
r1   = v2.zyxw * c0.w                  ; c0.w = 765.01 ~= 3*255.0033
; v2 is BLENDINDICES as D3DCOLOR (0xAARRGGBB), so .zyxw = (byte0, byte1, byte2, byte3)
; each component is byte/255, therefore r1.x = 3*byte0, r1.y = 3*byte1, ...

a0.x = r1.x                            ; relative-addressing offset for influence 0
pos  = m4x3(v0, c9[a0.x])              ; v0 = POSITION
nrm  = m3x3(v3, c9[a0.x])              ; v3 = NORMAL
if n >= 2: pos *= v1.x ; nrm *= v1.x ; a0.x = r1.y ; pos += m4x3(v0,c9[a0.x]) * (1 - v1.x) ...
if n >= 3: ... weights v1.x, v1.y, last = 1 - (v1.x + v1.y)     (skinmesh3: dp3 with c0.xxzz)
if n >= 4: ... weights v1.x, v1.y, v1.z, last = 1 - (v1.x + v1.y + v1.z)  (dp3 with c0.xxxx)
pos.w = c0.x (=1) ; oPos = m4x4(pos, c2)      ; clip-space
; normal
except in skinmesh1: nrm = normalize(m3x3(nrm, c92))   ; -> world space
oD0  = min(c7 + lit(dot(nrm,c1)) * c8, c0.x)  ; per-vertex lighting, slots 1..4
oD0  = min(c7 + c8, c0.x)                     ; flat, slots 5..8 (unlit variant)
oD1  = c0.y                                   ; specular = material.Power (0 in practice)
; fog -- two DIFFERENT formulas in the shipped blobs:
;   skinmesh2/3/4 (depth fog):
oFog = clamp((c6.y - pos.z) * c6.z, c6.w, c6.x)
;   skinmesh1 (range fog, distance to camera):
oFog = min(max((c95.y - length(c6 - pos)) * c95.z, c95.w), c0.x)
oT0  = v4                                     ; TEXCOORD0
oT1  = v3.x + c0.z, ...                       ; TEXCOORD1 = uv + fProgress  (animated UV)
```

The `skinmesh1` range-fog formula reads `c95.y/z/w`, but in this code base `c92..c95` is
`transpose(inverse(view))` and **nothing ever loads fog parameters into it**
(`CMesh.cpp:1046` writes them to `c6`; the only writers of `c92` are `CMesh.cpp:640` and
`RenderDevice.cpp:1851`). So single-influence skinned meshes get a nonsense fog factor in this
port. `[OBSERVED]` for the bytecode and the constant writes, `[INFERRED]` that this is a
755/760-shader-vs-748-constant-layout mismatch. A port should use the depth-fog formula for
every influence count.

Two quirks worth keeping: shader slot 0 (1 influence, `skinmesh1.bin`) skips the `c92`
normal transform (so single-bone props are lit in object space), and slots 5..8 drop the
directional term entirely (flat `c7 + c8`). `[OBSERVED]` (`evidence/analysis/mesh-vs-skinmesh{1,2,3,4,5}.disasm.txt`)

Note: this fills the gap explicitly flagged as `[UNKNOWN]` in `01-overview.md` §9 item 10.

### 7.4 Per-draw call sequence for a `.msh`

`CMesh::RenderMesh` (`CMesh.cpp:598-774`), condensed:

```c
SetStreamSource(0, m_pMesh->m_pVB, 0, m_pMesh->m_sizeVertex);
SetIndices(m_pMesh->m_pIB);
SetVertexDeclaration(decl[infl-1 (+3 if rig==61)]);
SetVertexShader(shader[infl-1 (+3 if rig==61)]);
for (i < m_numPalette)  SetVertexShaderConstantF(3*i+9, transposed(palette[i]*offset[i]*view), 3);
SetVertexShaderConstantF(1, normalize(-1,1,1), 1);
SetVertexShaderConstantF(92, transpose(inverse(view)), 4);
SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
DrawIndexedPrimitive(TRIANGLELIST, 0, /*MinIndex*/0, NumVertices=AttRange[0].VertexCount,
                     /*StartIndex*/0, PrimitiveCount=m_numFaces);
SetVertexShader(NULL);
```

`[OBSERVED]`

### 7.5 Bounding volumes (computed, never stored)

`TMMesh` keeps a box + a crude radius, filled **once at load** by scanning the vertex
positions:

* `.msa`: `TMMesh::LoadMsa` (`TMMesh.cpp:672-722`) seeds from vertex **1** and loops
  `i < nVerts-1` (so vertices 0 and n−1 are skipped), then
  `m_fRadius = max(|maxX|,|minX|,|maxY|,|minY|,|maxZ|,|minZ|)` and `m_vecCenter` = box centre.
* `.msh`: `CMesh::LoadMesh` (`CMesh.cpp:865-894`) scans `i < nVerts-1`, same radius rule, but
  **does not set `m_vecCenter`**.
* Initial values are `m_fMax* = 1.175494f` (`FLT_MIN`, used as "smallest positive") and
  `m_fMin* = 3.402823f` (`FLT_MAX`), `m_fRadius = 1.0f` (`TMMesh.cpp:22-29`), so a mesh whose
  real coordinates stay inside ±1.0 keeps `m_fRadius = 1.0`.

Consequence: the bounding box always ignores the **last** vertex (and, for `.msa`, also the
first) of the position stream. `m_fRadius` is what `TMObject::IsVisible` culls with
(`TMObject.cpp:351`), so the off-by-one can only change culling by one vertex — harmless in
practice, but replicate it if you want identical pop-in behaviour. Note also that `m_fRadius`
is `max(|component|)` of the axis-aligned box, **not** a real bounding-sphere radius: it
over-estimates badly for long thin meshes (e.g. a wall), which makes objects stay visible
longer than they should. `[OBSERVED]`

---

## 8. Draw-time state

### 8.1 `CMesh::SetMaterial(char cAlpha)`

`[OBSERVED] CMesh.cpp:982-1051`. In this D3D9 port it touches **no** `D3DRS_*` state at all —
it exists purely to fill the vertex-shader constant file:

```c
D3DMATERIAL9 materials = m_pParentSkin->m_materials;     // copied, then modified locally
g_pDevice->m_pd3dDevice->SetMaterial(&materials);         // FFP material (only matters if no shader)
fProgress = (serverTime % 10000) / 10000.0f;
vConst = {1.0f, 0.0f, fProgress, 765.01001f};
ambEmm = D3DXColorModulate(materials.Ambient, {0.25,0.25,0.25,1});   // = Ambient * 0.25
if (m_sLegendType >= 116 && m_sLegendType <= 125 && m_sMultiType > 0) {
    Emi = materials.Emissive; Emi.a = 0.89;
    Emi.rgb = max(Emi.rgb, 0.55);
    materials.Diffuse.a = 0.89; materials.Diffuse.rgb = min(rgb, 0.1);
    ambEmm += Emi;
} else ambEmm += materials.Emissive;
vConst.y = materials.Power;
SetVertexShaderConstantF(8, &materials, 1);   // c8 = diffuse row only
SetVertexShaderConstantF(7, &ambEmm, 1);      // c7
SetVertexShaderConstantF(0, &vConst, 1);      // c0
fEnd = (rig == 61) ? m_fFogEnd * 1.4f : m_fFogEnd;
fDist = (fEnd - m_fFogStart) ?: 0.01f;
fog = {1.0f, fEnd, 1/fDist, 0.0f};
SetVertexShaderConstantF(6, &fog, 1);         // c6
```

The `cAlpha` argument is **unused** — a decompiler artefact (the parameter is honoured only by
the caller, `CMesh::Render`, which uses it for `D3DRS_ALPHATESTENABLE`). Do not confuse it with
material state.

The legend-116..125 branch is the only place where the material is modified
(emissive floor 0.55, diffuse ceiling 0.1, alpha 0.89) — it produces the blown-out glow of
high-tier items.

### 8.2 Render states set around a skinned mesh (`CMesh::Render`)

`[OBSERVED] CMesh.cpp:36-60, 168-181, 536-538, 644-656, 725-753`

| When | State | Value | GL/Vulkan equivalent |
|---|---|---|---|
| entry | `D3DRS_ALPHATESTENABLE` | `1` | enable alpha test (`GL_ALPHA_TEST` / discard in shader) |
| `cAlpha == 'C'` | `D3DRS_ALPHATESTENABLE` | `0` | disable alpha test (glow/additive) |
| `cAlpha != 'C'` on glow paths | `D3DTSS_COLOROP` stage 0 | `4 = MODULATE` | `col = tex × diffuse` |
|  | `D3DTSS_COLOROP` stage 1 | `11 = ADDSMOOTH` | `col += tex1 × (1−col)` |
| `cAlpha == 'C'` | stage 1 | `18 = MODULATEALPHA_ADDCOLOR` | `col += tex1.rgb × tex1.a` |
| no multi / no legend | stage-0 `COLOROP` | `4 = MODULATE`, stage-1 texture = `NULL` | single-texture path |
| glow paths | `D3DRS_CULLMODE` | `2 = CW` → then restored to `3 = CCW` | flip winding; see §12 |
| base draw | `D3DRS_CULLMODE` | `D3DCULL_NONE` (set twice, `CMesh.cpp:647-648`) | cull disabled |
| multi-type > 0 | `D3DRS_DESTBLEND` | `6 = INVSRCALPHA` if `m_cDefaultAlpha==1` | standard alpha blend |
| hover highlight | `D3DRS_TEXTUREFACTOR`, `D3DTS_SHADEMODE=1` (FLAT), `SRCBLEND=5`(SRCALPHA), `DESTBLEND=6`(INVSRCALPHA) | selection edge | second pass with flat shading |

Alpha reference values in effect when a skinned mesh is drawn (set by
`SetRenderStateBlock(1)`, `RenderDevice.cpp:1691-1705`): `D3DRS_ALPHAREF = 0xDD` (221) for a
32-bit back buffer, `0x0D` (13) for 16-bit, `0xFF000000` for VGA-id 1 at 32-bit. `D3DRS_ALPHAFUNC`
is `7 = D3DCMP_GREATEREQUAL`, so the test is `texel.a >= ref`. Whether a driver honours the
upper 24 bits of `0xFF000000` is `[UNKNOWN]`; treat it as "always pass". `[OBSERVED]` for the
values, `[INFERRED]` for the D3DCMP mapping.

### 8.3 Static meshes: fixed-function lighting + CPU-side UV animation

`.msa` meshes are drawn with `SetFVF` and **no** vertex shader, i.e. D3D fixed-function
lighting from state block 1 (`RenderDevice.cpp:1632-1667`): two directional lights,
`m_light[0].Direction = (1,-1,-1)` (colour = `m_colorLight`), `m_light[1].Direction = (0,0,1)`
(colour = `m_colorBackLight`), `D3DRS_AMBIENT = 0x33FFFFFF`, `D3DRS_SPECULARENABLE = 0`,
`D3DRS_NORMALIZENORMALS = 1`, `D3DRS_LIGHTING = 1`. Materials come from the object
(`TMObject::Render`, `TMObject.cpp:104-145`): diffuse 0.7, emissive 0.3, specular 0.7, power 0
in normal scenes; dungeon scenes derive diffuse/emissive from the ground colour.
`[OBSERVED]`

The 3D-icon path in the UI (`RenderDevice.cpp:3373-3395`) instead forces `LIGHTING=0`,
`ALPHABLENDENABLE=1`, `SRCBLEND = 5 = SRCALPHA`, `DESTBLEND = 2 = ONE` (additive), and
*overwrites the vertex diffuse of the cached VB* with `n3DObjIndex - 2964` and the texture index
with `204`. This mutates shared cached data — a reimplementation must copy-on-write or the
icons will corrupt the same mesh used by the world. `[OBSERVED]`

Animated UVs (`.msa` only, `TMMesh::Render` `TMMesh.cpp:52-157`): when the caller passes
`cMulti != 0` **and** `m_dwFVF == 530` (the converted 2-UV format), the VB is locked and the
second UV set is rewritten per attribute range every frame. `fProgress = (time % 4000)/4000`
(`8000` for `cMulti == 100`):

| `cMulti` | Effect on `tu2/tv2` |
|---|---|
| 1 | `tu2 = tu1 + p`, `tv2 = tv1 + p` (scroll diagonally) |
| 2 | `tu2 = tu1 + p` |
| 3 | `tv2 = tv1 + p` |
| 4 | `tu2 = tu1 - p` |
| 5 | `tv2 = tv1 - p` |
| 6 | first frame: `tv2 = tv1`; afterwards `tv1 = tv2 - p` (accumulating) |
| 7 | `tu1 = tu2 - 3p` |
| 100 | `tu1 = tu2 - 2p` (with the 8000 ms period) |
| 101 | first frame: `tv2 = tv1`; afterwards `tv1 = tv2 - p`, `tu1 = tu2 + p` |

The skinned-mesh equivalent is done in the vertex shader (`oT1 = uv + c0.z`), so it never
touches memory.

### 8.4 Fixed-function texture stage defaults

Exact stage state of **state block 1** (the world pass, `RenderDevice.cpp:1738-1758`):

| Stage | State | Value | Meaning |
|---|---|---|---|
| 0 | `COLOROP` | `4 = MODULATE` | `rgb = tex0 × diffuse` |
| 0 | `COLORARG1` / `COLORARG2` | `2 = TEXTURE` / `0 = DIFFUSE` | |
| 0 | `ALPHAOP` | `2 = SELECTARG1` | **alpha = tex0.a only** (the diffuse alpha is *not* multiplied in) |
| 0 | `ALPHAARG1` / `ALPHAARG2` | `2 = TEXTURE` / `1 = CURRENT` | |
| 0 | `MINFILTER/MAGFILTER/MIPFILTER` | `2 = LINEAR` / `2` / `2 = LINEAR` | trilinear |
| 0 | `TEXCOORDINDEX` | `0` | uv set 0 |
| 1 | `COLOROP` / `ALPHAOP` | `1 = DISABLE` / `2` | stage 1 off by default |
| 1 | `TEXCOORDINDEX` | `1` | ready for the multi/glow pass (uv set 1) |
| 2 | `COLOROP` | `1 = DISABLE` | |

Block 1 also sets `D3DRS_DIFFUSEMATERIALSOURCE = 1` (`D3DMCS_COLOR1`, i.e. "use the vertex
colour when the vertex format has one"), `D3DRS_SPECULARMATERIALSOURCE = 1`,
`D3DRS_FOGVERTEXMODE = 3` (VERTEX), and fog start/end = `m_fFogStart/End + cameraSightLength -
8 / -15`. State block 0 (UI) is the same but uses `COLOROP = 2 = SELECTARG1` (texture only,
no lighting). `[OBSERVED]` `RenderDevice.cpp:1606-1631, 1730-1758`

So the default world equation for a static mesh is

```
rgb   = tex0.rgb × litVertexColour      (ffp lighting: ambient·mat + Σ N·L·light·mat)
alpha = tex0.a
```

and the two-light FFP lighting of §8.3 is what produces `litVertexColour`. `[OBSERVED]`

---

## 9. Multi-texture / legend / multi-type / god2 flags

All of these are **runtime-only** `CMesh` fields (`CMesh.h:30-42`) — none come from the file.
They are set by `TMSkinMesh::RestoreDeviceObjects` from the character "look"/"sanc" bytes
(`TMSkinMesh.cpp:298-345`) and by costume code.

| Field | Type | Set from | Effect |
|---|---|---|---|
| `m_sMultiType` | `short` | `SANC_INFO.Sanc<i>` clamped to 0..15 (`TMSkinMesh.cpp:298-334`) | which *second* texture (the "multi"/glow overlay) is used: `GetEffectTexture(base + m_sMultiType)`, and the stage-0 colour op: `<=6` → MODULATE(4), `7..9` → MODULATE2X(5), `>9` → MODULATE4X(6). Clamped to 12 before use. `0` = no overlay |
| `m_sLegendType` | `short` | `SANC_INFO.Legend<i>` (0..15) and `m_Color.Sanc<i>` 116..125 | selects the overlay *family* and the base effect-texture block: `4` → 179+x, `5..8` → 153/166/246/260 + x, `9..12` → 153/166/246/440 + x, `116..125` → 275/288/301/314/327/340/353/366/425/392 + x − 1, else `233 + x` (mount: `440 + x`) |
| `m_nTextureIndex` | `int` | texture look-up of the `.wyt`/`.wys` name | slot in `m_stModelTextureList[2048]`; `>= 0` is a valid texture. `< 0` with `m_bSheild` triggers the shield path |
| `m_bHead` | `bool` | `i == 1 && look.Mesh1 == 40`, or any costume (`m_nCosType != 0`) | changes the *texture-coordinate source* of the glow pass: with head/shield, stages 0/1 both use `D3DTSS_TEXCOORDINDEX = 0` and `COLOROP=MODULATE(2)`, stage1 `ADDSMOOTH(8)`; without, a 3-term `MULTIPLYADD(0x18)`/`ADD(4)` combination using the diffuse as arg |
| `m_bMount` | `bool` | `TMSkinMesh` mount flag | overlay texture comes from the *mount* block (`440 + x`, or `456` when `x >= 13`) instead of `233 + x` |
| `m_bSheild` | `int` | set to 1 when `m_nTextureIndex < 0` in the 116..125 branch | forces the shield shading path even without a base texture |
| `m_god2cos` | `bool` | `look[2i] == '_' / 'a' / 'N' / 'L'` | "god2 costume": part uses **one** texture for all variants (`God2Exception`, `TMSkinMesh.cpp:1947`) |
| `m_god2Alpha/Red/Green/Brue` | `int` | ctor 255/50/50/50, never read in `CMesh` | dead in this port — **flag as unused** (`CMesh.cpp:26-29` sets them; nothing reads them) |

`[OBSERVED]` for all rows; `[INFERRED]` for "which visual effect each family produces"
(the constants are observed, the artistic intent is not documented).

Hard-coded special cases in `CMesh::Render` (`CMesh.cpp:95-252`) for **weapon-in-hand**
meshes: when the mesh's `dwID` equals `g_dwHandIndex[rig][0|1]` (the hand bone for that
animation set) and the player's `LOOK_INFO.Mesh6/Mesh7` names a common mesh in
`2889..2901`, `2906..2909` or `2912..2923`, the code draws **a common `.msa` mesh instead of
the skinned one**, with the glow overlay taken from effect textures `153/166/246/260 + multi`
(or `440 + multi`) and the second UV scrolled **in the VB** every frame. `[OBSERVED]`

---

## 10. Naming conventions

### 10.1 `Mesh\Mesh.txt` — the original authors' own legend

The file is **EUC-KR / cp949** Korean text (272 non-ASCII bytes). It decodes cleanly as
cp949; it must never be re-saved as UTF-8. Full decoded text:
`[OBSERVED]` (decoded with Python `bytes.decode('cp949')`, 1022 characters)

```
ch010101.msh
ch : 케릭터                       (character)
01  : class
     01 : 기사, 02 : 마법사, 03 : 소환사, 04 : 헌터
     (knight / wizard / summoner / hunter)
01 : parts
     01 : 머리, 02 : 투구, 03 : 윗도리, 04 : 바지,
     05 : 장갑, 06 : 장화, 07 : 왼쪽, 08 : 오른쪽
     (head / helmet / jacket / pants / gloves / boots / left / right)
01 : shape
     01:기본 02:가죽1 03:가죽2 04:가죽3 05:링 06, 07, 08, 09,
     (basic, leather1..3, ring)

ch010101.ani
ch : 캐릭터
01 : sex      01 : 남자, 02 : 여자              (male / female)
01 : 남자 무기  (male weapons)  01:기본.어쎄신류의무기 02:한손검.도끼 03:방패
                04:한손검과방패 05:양손검 06:중간검.큰일본도 07:큰검.지그프리트검
                08:낫스.해머.청룡언월도 09:창 10:던지는창.작은창 11:던지는단검.단검
02 : 여자 무기  (female weapons) 01:기본 공격 02:한손검.도끼 03:방패 04:한손검과방패
                05:양손검 06:중간검.큰일본도 07:활 08:마법지팡이
                09:마법지팡이와 방패 10:큰 마법지팡이
01 : 동작      (motions) 01:제자리1 02:제자리2 03:걷기 04:뛰기 05:칼쌈1 06:칼쌈2
                07:칼쌈3 08:스킬1 09:스킬2 10:스킬3 11:타격 12:죽기 13:죽음
                14:앉기 15:레벨업
                 (idle1/2, walk, run, fight1/2/3, skill1/2/3, hit, dying, death, sit, levelup)
01 : 동작(여자) (female motions) 01..04 idle/walk/run, 05:칼쌈1 06:칼쌈2 07:마법1
                08:마법2 09:스킬1 10:스킬2 11:타격 12:죽기 13:죽음 14:앉기 15:레벨업

ch01.bon   01 : 남자   02 : 여자                      (male / female)

큰도끼 : x 축 18도, z축 45도    (big axe: 18° about X, 45° about Z — authoring note)

무기 mesh   (weapon mesh)
wp0101.msa
wp : 무기                        (weapon)
01 : 공격무기, 02 : 방어 무기     (attack weapon / defensive weapon)
01 : 무기 순서...                (weapon order ...)
texture는 같은 이름으로...        (textures use the same name ...)
```

The legend's field layout has to be read against what the code actually builds
(`"%s%02d%02d.msh"`, `TMSkinMesh.cpp:167`):

| Field | Legend says | What the engine does | Verdict |
|---|---|---|---|
| prefix (2) | entity family (`ch`, `wp`, …) | used verbatim as the `.bon` stem prefix | consistent |
| field 2 (2) | `class` = knight/mage/summoner/hunter | the *rig* name from `BoneAni4.txt`: `mesh\ch01` / `mesh\ch02`, i.e. **male / female** | legend is **stale** — only `ch01` (238 meshes) and `ch02` (182) exist; there is no `ch03`/`ch04`, and `ch01.bon`'s own legend says `01 남자 / 02 여자` |
| field 3 (2) | `parts` 1..8 = head/helmet/jacket/pants/gloves/boots/left/right | part index `i+1`, loop bounded by `numParts` = 8 for `ch01`/`ch02` | consistent |
| field 4 (2) | `shape` (기본/가죽1..3/링) | `look[2*i] + 20*m_bExpand + 1` — a 0-based *look* value offset by the "expand" tier | consistent (the legend just lists the low values) |

So: `<prefix><rig><part><variant>.<ext>`, and the "class" of a character is **not** in the
mesh name at all — all four classes share the male and female bodies, and class identity comes
from the equipment look values (`LOOK_INFO.Mesh0..Mesh7`) and the texture variant.
`[OBSERVED]` (file listing + `TMSkinMesh.cpp:161-192` + `BoneAni4.txt`),
`[INFERRED]` for "class is purely look/texture driven".

### 10.2 The name-building formulas (exact)

`TMSkinMesh::RestoreDeviceObjects` (`TMSkinMesh.cpp:161-215`), for part `i` = 0..numParts-1:

```c
look = (unsigned short*)&m_Look;            // LOOK_INFO = 8 × (short Mesh, short Skin)  [Structures.h:268-286]
sanc = (unsigned char*)&m_Sanc;             // SANC_INFO = 8 × Sanc + 8 × Legend        [Structures.h:288-306]

god2cos = (look[2*i] == '_' || look[2*i] == 'a' || look[2*i] == 'N' || look[2*i] == 'L');

meshName = sprintf("%s%02d%02d.msh", rigName, i+1, look[2*i] + 20*m_bExpand + 1);
// rigs 45,46,53,54 ignore the look value: variant is always 01

texName  = sprintf("%s%02d%02d.wyt", rigName, i+1,
                   (look[2*i+1] & 0xFFF) + look[2*i] + 20*m_bExpand + 1);
// God2Exception()/MantleException() paths use part 1 and the low 12 bits of look[2*i+1]
```

Consequences to replicate: part numbers are printed `%02d`, so `>= 100` silently changes the
name; the variant field is `look[2*i]` plus 20 per "expand" tier, so the set of variants that
actually exist depends entirely on the character/item data — which is why the numbering is
sparse (`ch010101.msh`, `ch010106.msh`, `ch010107.msh`, `ch010108.msh`, `ch010113.msh`, …);
and rigs 45/46/53/54 always use variant `01`. For characters, parts 7 and 8 are the **left and
right hand** (`Mesh.txt` parts legend: `07 : 왼쪽, 08 : 오른쪽`), which is why
`TMSkinMesh.cpp:381-391` attaches parts `i==6` and `i==7` to the hand bones via
`g_dwHandIndex[rig][0..1]` instead of loading them as normal body parts. `[OBSERVED]`

### 10.3 Entity vs texture naming

* Mesh files are lowercase on disk except the `BL01*` mantle meshes (`BL010101.msh` …). The
  client's own path strings mix `Mesh\` and `mesh\` (§1), so any case-insensitive filesystem
  view is acceptable — but keep the on-disk names.
* Textures for meshes are `mesh\<stem>.wys`; the code *asks* for `.wyt` and relies on the
  extension-masking lookup (§1).
* Effect meshes live in `Effect\` (37 `.msa`) and use `Effect\<stem>.wys` (413 files).
* Common-mesh entries in `MeshList.txt` may live in either directory, and that directory is
  what decides whether the texture comes from the model or effect table
  (`TMMesh.cpp:539-548` uses `!stricmp(szPathName, "effect")` on the *mesh's* directory).

---

## 11. Decompiler artefacts and landmines

| Location | Issue |
|---|---|
| `CMesh.h:27` / `CMesh.cpp:826` | `m_numFaceInflunce` — misspelling of "influences per vertex"; a *face* has no influence here. `[OBSERVED]` |
| `CMesh.cpp:822` | `dwParentID` is read and discarded; the `.msh` "parent id" field is unused. `[OBSERVED]` |
| `CMesh.cpp:982` | `SetMaterial(char cAlpha)` never uses `cAlpha`. `[OBSERVED]` |
| `CMesh.h:37-41` | `m_god2cos/m_god2Alpha/m_god2Red/m_god2Green/m_god2Brue` (note the typo "Brue" for Blue) are written in the ctor and never read. `[OBSERVED]` |
| `TMMesh.cpp:68` | `if (!cMulti || m_dwFVF != 530) return RenderDraw(...)` — the CPU UV animation is *silently* skipped for any mesh that is not the converted 2-UV format. `[OBSERVED]` |
| `TMMesh.cpp:694` | `.msa` bounds scan starts at vertex **1** and stops at `nVerts-1` (skips the first and last vertex). `[OBSERVED]` |
| `CMesh.cpp:865` | `.msh` bounds scan stops at `nVerts-1` too. `[OBSERVED]` |
| `CMesh.cpp:838` | `malloc(160)` for `numPalette` bone ids: overflow if `numPalette > 40`. `[OBSERVED]` |
| `TMMesh.cpp:493`/`m_AttRange[32]`, `m_nTextureIndex[32]` | `.msa` `attribCount` is not bounds-checked (max observed 18, so safe today). `[OBSERVED]` |
| `TextureManager.cpp:956-973` | `GetModelTextureIndex` writes into the caller's string and ignores the extension (§1). `[OBSERVED]` |
| `TextureManager.cpp:915` | `if (nIndex < 0 || nIndex > MAX_MODEL_TEXTURE)` — off-by-one on a 2048-entry array. `[OBSERVED]` |
| `RenderDevice.cpp:3391` | The UI 3-D icon path rewrites the cached VB's diffuse and the shared `m_nTextureIndex[0]`. `[OBSERVED]` |
| `TMSkinMesh.cpp:306-329` | Hard-coded texture slots 1840..1843 that are **empty** in the 748 table (max used index 1662) — 755/760 leftovers. `[OBSERVED]` |
| `MeshManager.cpp:62` | `fscanf("...%s\r\n")` on `Mesh\\MeshList.txt` with literal double backslashes; works only through Win32 path normalisation. `[OBSERVED]` |
| `MeshManager.h:78` | `ObjectMaskEncKeys[172]` — the array is 2 bytes longer than the 170-byte string; `strlen` is what matters. `[OBSERVED]` |
| `TMMesh.cpp:483-485` | `LoadMsa` returns 0 for an empty file name, otherwise it trusts the whole file (no header magic, no version). Corruption ⇒ garbage buffers, not an error. `[OBSERVED]` |

Note also: **neither format has a magic number or version field.** A `.msh` and a `.msa` are
distinguished only by their FVF/count plausibility, and the loader `MeshList.txt` is the only
thing that decides which loader runs. `[OBSERVED]`

---

## 12. Reimplementation notes (OpenGL / Vulkan)

1. **Two mesh pipelines, not one.** `.msa` gets a plain static pipeline (one VB, one IB,
   1..18 draw calls with per-draw texture); `.msh` needs a *skinning* pipeline (palette
   uniforms, per-vertex indices/weights). Do not try to unify them at load time.

2. **Pre-bake the `.msa` UV conversion at import.** The load-time transform is
   `FVF 0x112 → 0x212`, `sizeVertex 32 → 40`, second UV = copy of the first (then written per
   frame). In a modern renderer, upload 32-byte vertices and animate `uv1` with a
   *uniform* (`uv1 = uv0 + vec2(t,t)`) instead of rewriting the buffer — that reproduces
   modes 1..5 and 100 exactly, and modes 6/101 need a per-attribute accumulated offset uniform
   (they integrate deltas, so the value depends on frames rendered, not on time — `[INFERRED]`
   from `m_bFirst` and the `tu1 = tu2 - p` structure; the original is frame-rate dependent).

3. **`.msh` vertex fetch.** Bind 4 attributes: `vec3 pos @0`, `n × float weight @12`,
   `4 × uint8 indices @12+4n` (as `R8G8B8A8_UINT`, *not* normalized float — the shader needs
   the raw 0..25 values; if you use D3DCOLOR-style normalized bytes multiply by 255 first),
   `vec3 normal @16+4n`, `vec2 uv0 @28+4n`. Vertex stride `32 + 4n`.

4. **Skinning math** (row-vector D3D convention, `v' = v · M`):
   `p = Σ w_k · (pos · P[idx_k])`, `n = normalize(Σ w_k · (normal · P[idx_k]))`, with
   `w_last = 1 − Σ(preceding w)`. Then `clip = p · (P_viewProj)`, and for GL/VK use the
   *column-vector* form by transposing every matrix (`p^T = M^T · p^T`) — geometry is
   identical, only the storage convention changes. Palette matrices are
   `file_palette[i] · animation_frame_matrix[bone_i]` and are already view-premultiplied in
   the original; you can keep only the world part and let the vertex shader multiply by the
   view-projection.

5. **Palette registers.** 3 constant registers per bone starting at `c9`; the hardware budget
   ends at `c91` (83 registers ⇒ 27 bones), and the 748 data never exceeds 26. If you switch
   to a UBO/SSBO you can drop the limit, but do not change *which* palette slot each vertex
   references.

6. **Lighting.** Skinned: `lit = clamp(N·L, 0, 1)` with `L = normalize(-1,1,1)` in *object*
   space for the 1-bone shader and in *world* space (after `inverse(view)`) for 2..4 bones;
   `colour = min(ambient·0.25 + emissive + lit·diffuse, 1)`. Static: D3D fixed-function with
   two directional lights `(1,-1,-1)` and `(0,0,1)`, global ambient `0x33FFFFFF`, no specular.
   Reproduce it as a small analytic function; do not "improve" it with a PBR model.

7. **Alpha test values matter.** `ref = 0xDD/255 ≈ 0.867` (32-bit) with `GREATEREQUAL`;
   `0x0D/255 ≈ 0.051` if you target a 16-bit back buffer. Textures flagged `'N'` get a
   *colour key* (`0xFF000000` = black → transparent) instead of an alpha channel, `'A'`/`'C'`
   keep alpha, `'C'` additionally disables the alpha test and uses additive stage ops.

8. **Winding.** The client renders skinned meshes with `D3DCULL_NONE` (and temporarily
   `D3DCULL_CW` for some glow paths); static meshes inherit `D3DCULL_CCW` from state block 1.
   D3D's front face is CW in a left-handed system with a Y-flipped clip space; if you render
   with an unflipped Vulkan/GL pipeline you must flip the front-face convention *and* the
   projection's Y, or simply disable culling for skinned meshes exactly as the original does.

9. **Bone hierarchy load order.** Iterate the `.bon` array in file order and attach to
   `FindFrame(parent)` *after* the parent exists — the files are ordered parent-before-child
   in all 67 real bone files, so a single pass works, but a defensive second pass is cheap.
   Bone ids are dense and are used as array indices, so validate them.

10. **Cache semantics are visible.** Common meshes expire per-caller (`showTime` 3 min for
    world objects, 180 s for inventory icons) on a 1.2 s sweep; skin meshes never expire and
    the 1024-slot table silently refuses to load a 1025th distinct `.msh`. If you fix the
    eviction, expect different texture/memory behaviour but identical pixels; if you must be
    bug-compatible (e.g. to reproduce a "mesh disappears" artifact), keep the lease.

11. **`object.bin`** must be decrypted (170-byte pad, `−key[j%170] − j`) and checksum-verified
    (`signed byte % 4`, 32-bit wrap) or the client pops a modal and refuses to start. The
    masks themselves are only needed for *gameplay*: ground-height lookups (`y = mask * 0.1`)
    for spawns, effects, drops and projectiles, and the `mask >= 126` camera-blocking test.
    They never touch the render path.

12. **Path handling**: normalise `\\` → `\`, match case-insensitively, and resolve texture
    names by *stem* (ignore the extension, first table match wins) — three separate quirks of
    the original that all show up in asset resolution.

---

## 13. Evidence index

Hex dumps (`docs/rendering/evidence/hex/`, all prefixed `mesh-`):

| File | Content |
|---|---|
| `mesh-msh-ch010264-full.txt` | complete 214-byte `.msh` (3 verts, 1 palette) |
| `mesh-msh-ch010101-header.txt` | 256 bytes of a 2-influence character part |
| `mesh-msh-or020101-header.txt` | 512 bytes of a 25-palette, 4-influence mesh |
| `mesh-msh-bt010201-full.txt` | the only non-skinned `.msh` (FVF `0x12`) |
| `mesh-msa-sp0001-full.txt` | complete 159-byte `.msa` (FVF `0x142`) |
| `mesh-msa-hs0001-header.txt` | `.msa` header with several attributes |
| `mesh-msa-aef040-header.txt` | 4-sub-mesh `.msa` with full attribute table |
| `mesh-msa-grid-full.txt` | the FVF `0x12` outlier |
| `mesh-bon-ch01-full.txt` | complete 47-bone hierarchy |
| `mesh-bon-ch02-header.txt` | first 16 bones of the female rig |
| `mesh-boneani4-txt-head.txt` | `BoneAni4.txt` head |
| `mesh-meshlist-txt-head.txt` | `MeshList.txt` head (double backslashes) |
| `mesh-texturelist-bin-record0.txt` | `MeshTextureList.bin` records 0..1 with the `0xCD` filler |
| `mesh-object-bin-first256.txt`, `mesh-object-bin-tail4.txt` | encrypted mask head + the checksum dword |

Analysis logs (`docs/rendering/evidence/analysis/`):

| File | Proves |
|---|---|
| `mesh-proof-msh-sizing.txt` | 657 `.msh` size identity + a per-file `OK` table, FVF histogram |
| `mesh-proof-msa-sizing.txt` | 1661 `.msa` size identity + a per-file `OK` table, FVF/attribute histograms |
| `mesh-proof-msh-vertices.txt` | palette-index range, index range, weight range, index-byte order (4 sub-runs) |
| `mesh-proof-bon.txt` | `.bon` sizes, root/parent sanity, the two non-bone files |
| `mesh-proof-texturelist.txt` | 2048 × 264 layout, alpha histogram, `.msa` texture resolution rate |
| `mesh-proof-meshlist.txt` | `MeshList.txt` / `CommonMeshList.txt` / `MeshTextureList.txt` shape and gaps |
| `mesh-proof-objectbin.txt` | decryption, checksum reproduction, mask value histogram |
| `mesh-vs-skinmesh{1,2,3,4,5}.disasm.txt` | decoded `vs_1_1` bytecode of the skinning shaders |

Reproduction scripts: `docs/rendering/evidence/tools/mesh_*.py` (pure Python 3, read-only).

