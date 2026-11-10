# 04 — Animation loading & skeletal skinning (WYD 748 client)

> Byte-exact, framework-independent specification of how the 748 client loads animation data
> (`.ani`, `.bon`, `.msh`, `.msa`) and how it turns keyframes into skinned geometry, so that the
> same rendering can be reimplemented in OpenGL or Vulkan from this document alone.

Last updated: 2026-11-10

Source of truth used for this document:

* Decompiled client: `E:\wyd\test-setup\tmproject\Projects\TMProject\*.cpp|*.h`
* Shipped assets: `E:\wyd\test-setup\client_748` (English 748, 2007-04)
* Empirical tooling output: `docs/rendering/evidence/hex/ani-*.txt` (the 19 files prefixed `ani-` are
  the evidence for *this* document; the other files in that folder belong to the sibling documents
  `01`..`03` and may be referenced here where useful)

Tag legend used on every factual claim:

* `[OBSERVED]` — read directly in the source (`file:line`) or measured in the real asset files (byte offset given).
* `[INFERRED]` — deduced from observations; the reasoning is stated.
* `[UNKNOWN]` — not established; the experiment that would resolve it is stated.

---

## Confidence summary

| tag | occurrences in this document |
|---|---|
| `[OBSERVED]` | 116 |
| `[INFERRED]` | 16 |
| `[UNKNOWN]` | 7 |

(Counted mechanically with a regex over the finished text; the three tags in this table and in the
legend above are included in the numbers.)

Biggest unknowns (all of them are *optional* for a reimplementation, none block a port):

1. **Exact per-instruction arithmetic** of `Shader\skinmesh1..8.bin` (the blend *structure* is proven:
   2 relatively-addressed palette reads per influence at base `c9`, stride 3 — see §10.4; the exact
   order of the multiply/add chain, the specular formula and what the `c0.z` sawtooth modulates are
   not). A port should **write its own shader**; see §10.5.
2. **Whether D3D9 `CreateVertexShader` accepts the shipped `Shader\skinmesh*.bin`.** Their D3D9
   instruction-length nibble is zero. Resolve by running the current build and reading `WYD.log`
   ("Read VertexShader %d Error.." would be logged on failure, `RenderDevice.cpp:947`).
3. **Why `m_sAnimationArray[nCount][…+59][…]` writes index 60 in a `[6][60][56]` array**
   (out-of-bounds). Either a decompiler artefact or an original memory-layout hack. See §6.6.
4. **What `skinType == 61` needs the `skinmesh5..8` variants for** — the bytecode is a separate build
   with identical declarations and identical palette addressing; the visual difference is not
   established. Resolve by diffing the two shader families' constant usage per draw, or by running a
   type-61 object with each.
5. **Whether the 59 `.msa` files with FVF 322 (no normals) are intentionally unlit** (see §12.2) or
   should be lit with a flat normal. Resolve by comparing a screenshot of such an object with
   `D3DRS_LIGHTING` toggled.

Two facts that look like unknowns but are **not** (both `[OBSERVED]` and decisive):

* `.ani` files store full 4×4 matrices; there is **no** quaternion or Euler on disk. Quaternions are
  derived at load time for banks 0/1 only.
* `m_dwStartTime` is uninitialised for skin-mesh types < 60 (`TMSkinMesh.cpp:47-48`). It only shifts
  the animation phase by a constant per object, and the phase is re-normalised by the modulo, so it
  cannot cause divergence between two runs of the same build.

---

## Table of contents

1. [Conventions: coordinate system and matrix order](#1-conventions-coordinate-system-and-matrix-order)
2. [Asset inventory and how the pieces fit](#2-asset-inventory-and-how-the-pieces-fit)
3. [`.ani` — the keyframe file](#3-ani--the-keyframe-file)
4. [`.bon` — the bone hierarchy](#4-bon--the-bone-hierarchy)
5. [`.msh` — the skinned sub-mesh](#5-msh--the-skinned-sub-mesh)
6. [The animation tables](#6-the-animation-tables)
7. [Runtime structures and loading](#7-runtime-structures-and-loading)
8. [Sampling a pose at time t (`FrameMove`)](#8-sampling-a-pose-at-time-t-framemove)
9. [Bone matrices: local → world](#9-bone-matrices-local--world)
10. [The bone matrix palette and GPU skinning](#10-the-bone-matrix-palette-and-gpu-skinning)
11. [Attachments: weapons, shields, heads, mounts, mantles](#11-attachments-weapons-shields-heads-mounts-mantles)
12. [`.msa` — rigid/effect models and their "animation"](#12-msa--rigid-effect-models-and-their-animation)
13. [Non-skinned animation paths](#13-non-skinned-animation-paths)
14. [Everything else that affects animation rendering](#14-everything-else-that-affects-animation-rendering)
15. [Decompiled-name warning list](#15-decompiled-name-warning-list)
16. [Reimplementation notes (OpenGL / Vulkan)](#16-reimplementation-notes-opengl--vulkan)

---

## 1. Conventions: coordinate system and matrix order

Everything below depends on getting this right, so it comes first.

### 1.1 Storage and multiplication convention

* All matrices on disk and in memory are **D3DX `D3DXMATRIX`**: 16 floats, **row-major** memory
  layout, **row-vector** algebra: a point is transformed as `p' = p * M`, therefore the
  **translation lives in row 3** (`_41,_42,_43`) and the last column is `(0,0,0,1)`. `[OBSERVED]`
  Proof: in `or010101.ani` the first matrix is the identity and in `to010101.ani` the non-identity
  values sit at byte offset `+0x30..0x38` of the 64-byte block
  (`docs/rendering/evidence/hex/ani-to010101-full.txt`, offset `0x38` = `54 B8 1E 3F` = 0.62,
  `0x40` = `52 B8 0E 40` = 2.23 while the 4th column stays 0,0,0,1).
* Concatenation order is therefore **child-first**: `M_world = M_local * M_parent`, exactly as the
  code writes `D3DXMatrixMultiply(&m_matCombined, &m_matRot, matCur)` (`CFrame.cpp:282`). `[OBSERVED]`

### 1.2 Model space is Z-up, world space is Y-up

* A character's root matrix is built as
  `D3DXMatrixRotationYawPitchRoll(&matTemp, m_vAngle.y - 90°, m_vAngle.x - 90°, m_vAngle.z)`
  (`TMSkinMesh.cpp:651-655`), and the game object passes `SetAngle(0, m_fAngle, 0)`
  (`TMHuman.cpp:2173`), i.e. yaw = `m_fAngle - 90°`, pitch = `-90°`. The constant `-90°` pitch
  rotates the Z-up model into the Y-up world, and the `-90°` yaw rotates the model's "forward" axis
  onto world +X. `[OBSERVED]` code, `[INFERRED]` interpretation of the two ±90° offsets as axis
  conversion (corroborated by the bind matrices in §5.4 whose translations are ≈ (x, y, **2.0**),
  i.e. a ~2-unit tall character measured along model +Z).
* The rest of the engine positions objects in the XY ground plane; a character at 2D position
  `(x, y)` with height `h` gets `SetPosition(x, h, y)` (`TMHuman.cpp:2243-2245`, and the camera
  conversions `TMObject.cpp:348-349`). `[OBSERVED]`

### 1.3 What a port must do

Working in GL/Vulkan column-vector conventions, every D3DX matrix `M` becomes `Mᵀ`, and
`M_a * M_b` (row-vector) becomes `M_bᵀ * M_aᵀ` (column-vector). See §16 for the concrete palette
formula; it is the single most error-prone part of this document.

---

## 2. Asset inventory and how the pieces fit

Measured in `E:\wyd\test-setup\client_748`:

| file type | count | role |
|---|---|---|
| `.ani` | 718 (`mesh\`) | one keyframe clip for one skeleton bank (e.g. `ch010103.ani` = bank `ch01`, weapon row 01, motion 03) |
| `.bon` | 69 (`mesh\`) | skeleton: parent/id array, 8 bytes per bone |
| `.msh` | 657 (`mesh\`) | one skinned sub-mesh (body part, armour piece) with its own vertex/index buffers, bind matrices and bone-name list |
| `.msa` | 1661 (`mesh\`) + 37 elsewhere | rigid/effect model: mesh + attribute list + embedded texture names, **no** skeleton |
| `.wys` | 1618 (`mesh\`) / 2336 (whole client) | textures (the `.msa` files still name their textures `*.tga`, see §12.3) |
| `Mesh\BoneAni4.txt` | 1 | skeleton-bank table (index, clip count, part count, name) |
| `Mesh\BoneAni3.txt` | 1 | older variant of the same table (25 lines, no mounts) |
| `Mesh\ValidIndex.bin` | 74400 B | `int[100][186]`: for each bank, the list of clip ordinals that exist |
| `AniSound4.txt` | 40154 B | per-skin-type, per-motion: cut ordinal, playback speed, sound id |
| `Shader\skinmesh1..8.bin` | 8 | D3D9 vertex shaders, 1..4 influences × 2 variants |
| `Mesh\MeshList.txt` | 39092 B | `index name` → `.msa` file for the "common mesh" pool |

`[OBSERVED]` (file counts from `Get-ChildItem`; see also `docs/rendering/evidence/hex/ani-verify-*`).

Composition model, from `TMSkinMesh::RestoreDeviceObjects` (`TMSkinMesh.cpp:119-423`):

```
skin mesh instance (TMSkinMesh)
 ├─ one skeleton: MeshManager::m_BoneAnimationList[skinType].pBone[]  (from  .bon)
 │     → CFrame tree, root at id 0, m_pframeToAnimate[boneID] lookup table
 ├─ numParts child meshes ("parts", numParts from BoneAni4.txt, e.g. 8 for ch01)
 │     for part i (0-based):
 │       file    = "<bank><i+1:02d><Mesh_i+20*m_bExpand+1:02d>.msh"            (TMSkinMesh.cpp:167)
 │       texture = "<bank><i+1:02d><(Skin_i&0xFFF)+Mesh_i+20*m_bExpand+1:02d>.wyt"  (:186)
 │       each part → one CMesh → one .msh (own VB/IB, one texture)
 │     parts 7 and 8 (i == 6,7) for skinType < 19 are the two *hands*: no .msh is loaded,
 │       the CMesh only remembers g_dwHandIndex[skinType][0..1] and draws the equipped .msa
 └─ animation state: current cut, frame, sub-frame phase
```

`[OBSERVED]` `TMSkinMesh.cpp:161-419`, part naming at `:167-215`, hand special case at `:381-402`.

The seven-digit asset naming convention (`ch010103`) is:

```
ch 01 01 03
│  │  │  └── motion index + 1  (01 = STAND01, 02 = STAND02, 03 = WALK, …)
│  │  └───── weapon/skin colour row + 1  (01..16)
│  └──────── skeleton bank        (ch01 = male, ch02 = female, or01 = orc, …)
```

and for `.msh`/`.wyt` the third group is the **part number** (`ch0102**` = part 2). `[OBSERVED]`
(from the actual file names; e.g. `ch011602.ani` = weapon row 16, motion 02; see
`docs/rendering/evidence/hex/ani-validindex-bonemap.txt`).

---

## 3. `.ani` — the keyframe file

### 3.1 Header

| offset | type | size | meaning |
|---|---|---|---|
| 0x00 | `uint32` | 4 | **frame count** of this clip ("ani cut length") |
| 0x04 | `uint32` | 4 | **row count** = number of bones in the keyframe array |
| 0x08 | `D3DXMATRIX[frames][rows]` | `frames*rows*64` | keyframes, frame-major |

There is **no magic, no version, no name, no per-bone list, no translation/rotation split** — the
file is literally two `uint32` followed by a matrix array. `[OBSERVED]` `MeshManager.cpp:167-172`
reads exactly `4 + 4` bytes then `numAniFrame * (frames * 64)` bytes; and the byte-exact size proof
below.

Each element is one **local** bone transform (relative to its parent in the `.bon` hierarchy),
stored as a full 4×4 affine matrix. Not Euler. Not a quaternion (the loader *derives* quaternions for
two banks only, `MeshManager.cpp:178-181`). Not compressed, not scaled, no per-key time stamps —
**key `i` is exactly `i` frame intervals after key `i-1`; playback is a constant frame rate** (§8).
`[OBSERVED]`

### 3.2 Empirical size proof

For all 718 `.ani` files in `client_748\mesh`:

```
filesize == 8 + dword0 * dword1 * 64      (0 mismatches)
```

`[OBSERVED]` — full per-file table in `docs/rendering/evidence/hex/ani-verify-sizemath-ani.txt`
(the last line of that file states `718 files checked, mismatches = 0`).

Worked examples:

| file | size | dword0 (frames) | dword1 (rows/bones) | arithmetic |
|---|---|---|---|---|
| `or010101.ani` | 35848 | 16 | 35 | `8 + 16*35*64 = 8 + 35840 = 35848` ✔ |
| `or010107.ani` | 2248 | **1** | 35 | `8 + 1*35*64 = 8 + 2240 = 2248` ✔ |
| `to010101.ani` | 392 | 1 | 6 | `8 + 1*6*64 = 8 + 384 = 392` ✔ |
| `ch010103.ani` | 48136 | 16 | **47** | `8 + 16*47*64 = 8 + 48128 = 48136` ✔ |

**Why `or010101.ani` (35848 B) is 16× larger than `or010107.ani` (2248 B):** both have the *same*
35 bones; `or010107` contains a **single-frame pose** (a 1-key "hold" clip, used for e.g. seat/dead
poses), while `or010101` is a 16-frame looping clip. Bone count is identical; frame count differs
16×. `[OBSERVED]` header bytes: `10 00 00 00 23 00 00 00` (0x10=16, 0x23=35) vs `01 00 00 00 23 00 00 00`
(1, 35) — see `ani-or010101-header.txt` / `ani-or010107-header.txt`.

Distribution over the 718 clips `[OBSERVED]`:

* frames: 1×64, 8×3, 10×1, 11×1, **15×204, 16×346**, 18×15, 20×21, 21×54, 23×1, 26×2, 30×3, 31×3
* rows/bones: 47×198 (ch01), 48×169 (ch02), 35×76 (or01), 33×46, 36×41, 70×9, 64×22, 5×1 … (full
  histogram in `ani-verify-sizemath-ani.txt`)

### 3.3 Hex examples

`or010101.ani` (first 0x140 bytes) — `ani-or010101-header.txt`:

```
00000000  10 00 00 00 23 00 00 00 00 00 80 3F 00 00 00 00  |....#......?....|
                                   ^^ row0 = (1,0,0,0) of the identity = frame0/bone0
00000010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F
00000020  00 00 00 00 ...                                | ....|
00000040  00 00 00 00 00 00 80 3F CD CD CD CD CD CD CD CD   <- frame0/bone1 = 0xCD fill
```

Note the `0xCD` bytes: the *exporter* wrote an uninitialised MSVC debug pattern for bone 1 of frame 0.
Some bones in some clips are therefore garbage; they are harmless because those bones carry no mesh
vertices in the corresponding `.msh` (verified: bone1 of or01 has no palette entry in `or0101*.msh`).
`[OBSERVED]` bytes, `[INFERRED]` harmlessness.

`to010101.ani` (whole 392-byte file) — `ani-to010101-full.txt` — a real pose with translation:
`0x38: 54 B8 1E 3F` (=0.6239), `0x3C: 37 54 D1 B3`, `0x40: 52 B8 0E 40` (=2.2296) in row 3.

### 3.4 No root motion

`ch010103.ani` (WALK): bone 0 of every frame is the identity *and* its translation is exactly
`(0,0,0)`; the hand bone (id 19) keeps a constant local offset. Movement therefore comes from the
game object's world position, not from the clip. See
`docs/rendering/evidence/hex/ani-ch010103-root-motion.txt`. `[OBSERVED]`

Consequence for a port: **never accumulate a root translation from `.ani`** — that would double the
motion.

---

## 4. `.bon` — the bone hierarchy

| offset | type | size | meaning |
|---|---|---|---|
| 0x00 | `int32` | 4 | parent bone id; `0xFFFFFFFF` (= -1) for the entry that defines the root |
| 0x04 | `int32` | 4 | this bone's id |

`numBone = filesize / 8` (`MeshManager.cpp:123`). `[OBSERVED]`

Example `or01.bon` (280 B = 35 pairs) — `ani-bon-or01.txt`:

```
00000000  FF FF FF FF 00 00 00 00   pair[0] parent=-1 id=0
00000008  00 00 00 00 02 00 00 00   pair[1] parent=0  id=2
00000010  02 00 00 00 03 00 00 00   pair[2] parent=2  id=3
00000018  02 00 00 00 04 00 00 00   pair[3] parent=2  id=4
```

Loading (`TMSkinMesh.cpp:133-153`):

```
root = new CFrame(0)
for i in 0..numBone-1:
    parentID = (Int32)pBone[2i]; if (parentID == -1) parentID = 0
    myID     = (UInt32)pBone[2i+1]
    frame = new CFrame(myID)
    frame->m_dwParentID = rawParentID          // raw, may be -1
    m_pframeToAnimate[myID] = frame            // <-- indexed by *bone id*
    m_pRoot->FindFrame(parentID)->AddFrame(frame)
```

Two consequences that a port must replicate:

1. **`m_pframeToAnimate[]` is indexed by bone id, not by array position.** `.ani` row `k`
   animates the bone whose **id is `k`**. `[OBSERVED]` `TMSkinMesh.cpp:148` + `FrameMove`'s
   `m_pframeToAnimate[j]` loop with `j` = `.ani` row index.
2. Bone ids are *nearly* dense `0..n-1`. Gaps exist (`lf01`: ids `0,2,3,…,12`), and the `.ani`
   row count can be **less** than `maxId+1` (e.g. `dr01`: 52 bones, ids 0..51, but only 51 rows),
   in which case the highest-id bone simply receives **no** keyframes and keeps its identity
   matrix. `[OBSERVED]` table in `docs/rendering/evidence/hex/ani-verify-bon-vs-ani.txt`.
3. `AddFrame` reverses sibling order: a new frame is inserted *before* the current first child
   (`CFrame.cpp:247-256`), so the traversal order of siblings is the reverse of the `.bon` order.
   This only affects draw order, not transforms. `[OBSERVED]`

---

## 5. `.msh` — the skinned sub-mesh

### 5.1 Header (32 bytes)

| offset | type | size | meaning |
|---|---|---|---|
| 0x00 | `uint32` | 4 | parent id — **read and thrown away** (`CMesh.cpp:821-822`) |
| 0x04 | `uint32` | 4 | **`m_dwID`**: the bone this sub-mesh is attached to (only used to *place* the mesh in the CFrame tree and for the hand tests) |
| 0x08 | `uint32` | 4 | FVF (informational: encodes the influence count, not used for the draw) |
| 0x0C | `uint32` | 4 | vertex stride in bytes |
| 0x10 | `uint32` | 4 | **number of bone influences per vertex** (1..4) |
| 0x14 | `uint32` | 4 | **number of palette entries** (bone matrices referenced by this mesh) |
| 0x18 | `uint32` | 4 | vertex count |
| 0x1C | `uint32` | 4 | **index count** (`= 3 * triangleCount`, unlike `.msa` where the field is triangles) |
| 0x20 | `float[16] * numPalette` | `64*n` | per-palette-entry **bind (inverse-bind) matrix**, row-vector layout |
| 0x20+64n | `uint32 * numPalette` | `4*n` | per-palette-entry **bone id** (matched against the `.bon` ids) |
| … | vertices | `stride*vertexCount` | interleaved, layout chosen by `numInfluence` (see 5.2) |
| … | `uint16[]` | `2*indexCount` | triangle list |

`[OBSERVED]` `CMesh.cpp:794-931`; byte-exact size proof for **all 657** `.msh`:

```
filesize == 32 + 64*numPalette + 4*numPalette + stride*vertexCount + 2*indexCount   (0 mismatches)
```

Full table: `docs/rendering/evidence/hex/ani-verify-sizemath-msh.txt`.

Examples:

| file | size | infl | palette | verts | idx | arithmetic |
|---|---|---|---|---|---|---|
| `ch010101.msh` | 10668 | 2 | 5 | 222 | 708 | `32+320+20+8880+1416 = 10668` ✔ |
| `to010101.msh` | 4560 | 2 | 2 | 102 | 156 | `32+128+8+4080+312 = 4560` ✔ |

`to010101.msh` is **not** a rigid multi-part mesh (contrary to a natural guess from its small size):
it is a normal skinned mesh with 2 bones and 2 influences. `[OBSERVED]`
`docs/rendering/evidence/hex/ani-msh-to010101-header.txt`.

### 5.2 Vertex layout per influence count

The renderer does not use FVF for `.msh`; it binds a **vertex declaration** and a **vertex shader**
chosen by `numInfluence` (`CMesh.cpp:604-623`, declarations built in `RenderDevice.cpp:778-918`).
`[OBSERVED]`

| influences | stride | POSITION | BLENDINDICES | BLENDWEIGHT | NORMAL | TEXCOORD | shader |
|---|---|---|---|---|---|---|---|
| 1 | 36 | `FLOAT3` @0 | `D3DCOLOR` @12 | *(none — weight is implicit 1.0)* | `FLOAT3` @16 | `FLOAT2` @28 | `skinmesh1.bin` |
| 2 | 40 | `FLOAT3` @0 | `D3DCOLOR` @16 | `FLOAT1` @12 | `FLOAT3` @20 | `FLOAT2` @32 | `skinmesh2.bin` |
| 3 | 44 | `FLOAT3` @0 | `D3DCOLOR` @20 | `FLOAT2` @12 | `FLOAT3` @24 | `FLOAT2` @36 | `skinmesh3.bin` |
| 4 | 48 | `FLOAT3` @0 | `D3DCOLOR` @24 | `FLOAT3` @12 | `FLOAT3` @28 | `FLOAT2` @40 | `skinmesh4.bin` |

Notes:

* `BLENDINDICES` is a `D3DCOLOR`, i.e. **four bytes packed in one DWORD** — four palette indices,
  fetched in the shader as a `float4` (each byte/255, so the shader must scale back by 255).
  `[OBSERVED]` declaration (`RenderDevice.cpp:819-822`, type 4 = `D3DDECLTYPE_D3DCOLOR`) + FVF flag
  `D3DFVF_LASTBETA_UBYTE4` (§5.3). The scaling/rounding is `[INFERRED]` (standard D3D behaviour).
* Only `n-1` weights are stored; the last weight **must be computed as `1 - Σw`** by the shader.
  `[INFERRED]` — forced by the layout (no room for the n-th weight) and by the D3D9 blend-weight
  semantics; the observable data is exactly `n-1` floats (`ani-verify-sizemath-msh.txt` strides).
* Weights are **not** normalised anywhere in the CPU code; the source data is assumed normalised.
  `[OBSERVED]` (no normalisation code exists) 
* The stride histogram over all 657 files is exactly `{36:56, 40:198, 44:162, 48:240, 24:1}` and the
  influence histogram is `{1:56, 2:198, 3:162, 4:240, 0:1}` — i.e. **stride is fully determined by
  the influence count** (the single 24-byte/0-influence file is a degenerate/unused mesh).
  `[OBSERVED]`

### 5.3 FVF field: influence count encoded in the "position" bits

All 657 files use `FVF = 0x1116 | (2 * (infl-1))` = `0x1116, 0x1118, 0x111A, 0x111C` for 1..4
influences:

* `D3DFVF_XYZBn` position mask = `0x04 + 2*n`, i.e. "n blend weights" (`0x06`=XYZB1 … `0x0C`=XYZB4)
* `0x10` = `NORMAL`, `0x100` = one texture-coordinate set, `0x1000` = `LASTBETA_UBYTE4`

So the field is a *self-describing* copy of the influence count, redundant with the `numInfluence`
DWORD. `[OBSERVED]` values in `ani-verify-sizemath-msh.txt`, decoding by the D3DFVF bit layout.

### 5.4 Bind matrices ("palette" matrices in the file)

The `64*numPalette` block holds, per palette entry, the **inverse of the bind-pose bone world matrix**
(mesh space → bone space), in the same row-major, translation-in-row-3 layout. `[INFERRED]` — strong
evidence:

* The matrices' 3×3 part is orthonormal with determinant +1 (rotation only) and the 4th column is
  `(0,0,0,1)`; only `Mᵀ` gives an exact inverse under row-vector algebra.
* Their translations are ≈ `(±1.6, ±0.4, 2.0)` — `z ≈ 2.0` for *every* entry, which is exactly what
  `-b·R` looks like for bones of a ~2-unit-tall Z-up character.
* `CMesh.cpp:628` multiplies **file matrix × live bone matrix**, which only makes sense as
  `Bind⁻¹ · BoneWorld`.

Printed example (`ch010101.msh`, 5 entries, ids `8,7,14,10,13`) —
`docs/rendering/evidence/hex/ani-msh-ch010101-header-palette.txt`:

```
palette[0] (boneNameID=8):
    0.00000  -0.00000   1.00000   0.00000
    0.99619  -0.08715  -0.00000   0.00000
    0.08715   0.99619   0.00000   0.00000
   -1.61337   0.13170   1.99861   1.00000
```

The bone ids tie each palette entry to a `CFrame`:

```
CFrame::LinkBones (CFrame.cpp:258-275):
    for i in 0..m_numPalette-1:
        frame = root->FindFrame(m_dwNames[i])
        m_pBoneOffset[i] = &frame->m_matCombined     // live pointer, always current
```

`m_pBoneOffset[40]` bounds the palette: the data max is **26** (`ani-verify-sizemath-msh.txt`), the
array is 40, and the shader constant budget allows 27 (see §10.4). `[OBSERVED]`

### 5.5 Transmission of the palette between instances of the same mesh

`MeshManager::m_stSkinMeshList[1024]` caches `.msh` **geometry** by file name, and shares
`m_pBoneMatrix` / `m_dwNames` between all `CMesh` users of the same file (`CMesh.cpp:796-843`,
`:912-928`). Consequence: `m_pBoneOffset[]` is per-`CMesh` (per character), while the bind matrices
are shared. `[OBSERVED]`

---

## 6. The animation tables

### 6.1 Semantic hierarchy: class → type → motion

```
"class"   = animation *bank*  (0..5 in m_sAnimationArray, from BoneAni4.txt column 1)
            → determines the skeleton (.bon) and the clip file prefix (ch01, ch02, or01 …)
"type"    = the "weapon row" / colour row inside a bank (1..16 for ch01), stored 0-based
            → selects which *variant* of the same motion to play (different weapon poses)
"motion"  = the concrete action: STAND01, WALK, RUN, ATTACK1..3, SKILL01..03, DIE, … (56 slots)
"ani cut" = ONE .ani clip file = (bank, weaponRow, motion) → an *ordinal* in that bank's clip list
```

`[OBSERVED]` `MeshManager.cpp:189-192`:

```c
nArrayIndex = m_stValidAniList[nCount][nFI].nI + 1;    // e.g. 1602
nWeapon     = nArrayIndex / 100 - 1;                  // 0-based weapon row
nAnimation  = nArrayIndex % 100 - 1;                  // 0-based motion
m_sAnimationArray[nCount][nWeapon][nAnimation] = dwFileIndex;   // = ordinal of the cut
```

and the file name is `sprintf("%s%04d.ani", bankName, nI + 1)` (`MeshManager.cpp:127`), so
`nI+1` **is** the 4-digit number in the file name (`ch01` + `0103`). Therefore

```
cutOrdinal(weaponRow0, motion0) = 100*(weaponRow0+1) + (motion0+1)
```

`[OBSERVED]` — verified against the whole `ch01*` file set (186 files, blocks `01`..`16`).

**What "ani cut" means in the memory structures**: `stBoneAni::numAniCut[186]` stores, for each
loaded cut, **the number of frames of that cut** (`MeshManager.cpp:170`). It is an *array of frame
counts*, and its name is one of the worst decompilation artefacts in the project: it has nothing to
do with "cuts". The cut *index* (`m_nAniIndex`) is the ordinal 0..185. `[OBSERVED]`

### 6.2 `Mesh/BoneAni4.txt` (`BoneAni4_Path`, `TMPaths.h:54`)

Text, one line per bank: `index aniTypeCount numParts name` (`MeshManager.cpp:104`). 54 lines.

```
0 186 8 mesh\ch01      ← bank 0: 186 clips, 8 parts, skeleton mesh\ch01
1 149 8 mesh\ch02
2 55  8 mesh\or01
3 20  8 mesh\wb01
4 21  8 mesh\mi01
5 22  8 mesh\sq01
...
20 8 2 mesh\dr01
...
33 2 2 mesh\to01
...
60 1 1 mesh\sh01       ← skins with a single bone / single clip (props, scenery)
87 1 2 mesh\tr21
```

`BoneAni3.txt` is a shorter, older list (25 lines, no mount/prop rows). Nothing in the current source
reads it — `InitBoneAnimation` uses `BoneAni4_Path` only (`MeshManager.cpp:77`). `[OBSERVED]`

`numParts` = number of `.msh` sub-meshes for that skin type (TMSkinMesh loops `i < numParts`,
`TMSkinMesh.cpp:161`). For the player banks it is 8: body parts 1..6, plus the two hand slots 7 and 8.

### 6.3 `Mesh/ValidIndex.bin` (`ValidIndex_Path`) and `stValidAniList[100][186]`

* Size **74400 = 100 × 186 × 4** `int32`. Loaded in one `fread` of `sizeof(m_stValidAniList)`
  (`MeshManager.cpp:87-94`). `[OBSERVED]`
* `stValidAniList[bank][k].nI` = the cut number `nI` such that the file is `<bank>%04d.ani` with
  `%04d = nI+1`. `[OBSERVED]`
* Rows are only filled for `k < numAniTypeCount` (checked: nonzero counts per row are exactly
  `186,149,55,20,21,22,7,7,12,6,8,6,…` matching `BoneAni4.txt` column 2). `[OBSERVED]` —
  `ani-validindex-bonemap.txt`.

Example rows: bank 0 → `100,101,…,285`; bank 4 → `100..107` then `200,201,202,203` (= the second
weapon row).

### 6.4 `stBoneAni` — the per-bank runtime record (with exact byte offsets)

`MeshManager.h:21-32`, `MeshManager::m_BoneAnimationList[100]` (declared `MAX_BONE_ANIMATION_LIST`).
Pointers are 4 bytes (32-bit build).

| off | type | size | field | real meaning |
|---|---|---|---|---|
| 0 | `char[128]` | 128 | `szAniName` | bank name, e.g. `mesh\ch01` |
| 128 | `uint32` | 4 | `numBone` | bones in the `.bon` = `filesize/8` |
| 132 | `uint32` | 4 | `numParts` | number of `.msh` parts |
| 136 | `uint32*` | 4 | `pBone` | `malloc`ed `.bon` contents |
| 140 | `uint32` | 4 | `numAniTypeCount` | number of clips (rows in `ValidIndex.bin`) |
| 144 | `uint32[186]` | 744 | `numAniCut` | **frame count of each loaded clip** |
| 888 | `uint32` | 4 | `numAniFrame` | **rows per keyframe = bones** (name is wrong) |
| 892 | `D3DXMATRIX*` | 4 | `matAnimation` | `numAniFrame × totalFrames` matrices, `[frame][bone]` |
| 896 | `D3DXQUATERNION*` | 4 | `matQuaternion` | same shape, **only for banks 0 and 1** |
| **900** | | | `sizeof(stBoneAni)` | `100` records = 90000 bytes |

`[OBSERVED]` field list; the offsets are confirmed by the absolute addresses recorded in the header
comments — `numBone` at `0x0ED9560` implies `szAniName` at `0x0ED94E0`, and then `numAniFrame` at
`0x0ED94E0+888 = 0x0ED9858`, `matAnimation` at `0x0ED985C`, `matQuaternion` at `0x0ED9860`, exactly
the values written in those comments. `numAniCut` spans `0x0ED9570..0x0ED9858` = 744 B = 186 ints. `[OBSERVED]`

### 6.5 `g_MobAniTable[60]`, `g_MobAniTableEx[4][2]`, and `AniSound4.txt`

```c
struct stAniSoundTable  { uint32 dwSpeed[56]; uint32 dwAniTable[56]; uint32 dwSoundTable[56]; };  // 672 B
struct stAniSoundTable2 { uint32 dwSpeed[56]; uint32 dwAniTable[56]; };                             // 448 B
stAniSoundTable  g_MobAniTable[60];        // indexed by *skin type*  (m_nSkinMeshType)
stAniSoundTable2 g_MobAniTableEx[4][2];    // [head class][skin type 0|1]
```

`[OBSERVED]` `MeshManager.h:95-109`, `MeshManager.cpp:10-11`.

These are **not compile-time tables**: they are filled by `ObjectManager::InitAniSoundTable()`
(`ObjectManager.cpp:860-911`) from `AniSound4.txt` at the client root. `[OBSERVED]`

`AniSound4.txt` format (`ani-anisound4-sections.txt`):

```
[Knight] 0                       ← "type 0" and "type 1" (the two player banks)
STAND01   0 20   0 20   0 20   0 20   0
          ^a0 ^s0 ^a1 ^s1 ^a2 ^s2 ^a3 ^s3 ^sound
WALK      2 13   2 13   2 13   2 13   0
...
[orc] 2                          ← monsters use the short form
STAND01   0 20   0
```

* column 1 = **cut ordinal** within that bank (`dwAniTable[motion]`)
* column 2 = **`dwSpeed`** = milliseconds per *quarter* frame (see §8.1 — the code calls it
  `m_dwFPS`, which is doubly wrong)
* last = sound id (`dwSoundTable[motion]`)

Measured speeds for `[Knight]`: STAND 20, WALK **13**, RUN 20, ATTACK 15, DIE 15, PUNISH/YAWN/MERCHL
30, LEVELUP 18, HOLY 25. `[OBSERVED]`

Only **38 sections** exist for `MAX_ANI_TYPE = 60`; the parser keeps scanning, `fscanf` fails at EOF,
and **table entries 38..59 stay all zero** (they are zero-initialised globals). A zero `dwSpeed`
makes `FrameMove` fall back to 30 (§8.1) and a zero `dwAniTable` selects cut 0. `[OBSERVED]` code +
file; `[INFERRED]` the effective behaviour.

Motion index map (identical for all sections, from the 56 line names): `0 STAND01, 1 STAND02,
2 WALK, 3 RUN, 4 ATTACK1, 5 ATTACK2, 6 ATTACK3, 7 SKILL01, 8 SKILL02, 9 SKILL03, 10 STRIKE, 11 DIE,
12 DEAD, 13 SEAT, 14 LEVELUP, 15 PUNISH, 16 CACKLE, 17 YAWN, 18 MERCHL, 19 RELAX, 20 NEVER,
21 COMEON, 22 SALUTE, 23 HOLY, 24 SEATING, 25 STAND03, 26 PUNING, 27 PUNEND,`
then **mounted variants 28..55** (`MSTND01, MSTND02, MWALK, MRUN, MATT1..3, MSKIL01..03, MSTRIKE,
MDIE, MDEAD, MSEAT, MLVLUP, MPUNISH, …`). This is why the code adds 28 when mounted:
`g_MobAniTable[type].dwAniTable[(int)eMotion + 28 * m_cMount]` (`TMHuman.cpp:6209`). `[OBSERVED]`

### 6.6 `m_sAnimationArray[6][60][56]` — and a warning

```c
static short m_sAnimationArray[MAX_ANI_CLASS][MAX_ANI_TYPE][MAX_ANI_MOTION];
//                        [6]             [60]            [56]
```

Written during `InitBoneAnimation` as `m_sAnimationArray[bank][weaponRow][motion] = cutOrdinal`
(`MeshManager.cpp:192`), plus a pile of hard-coded fix-ups (rows 4..9/25..29 propagating a cut,
bank 0 mirroring rows 11/12/14 into rows ≥12, special nI values 107/108/111/137/138 copying an
entire motion row, …) `[OBSERVED]` `MeshManager.cpp:194-321`.

Consumed as `m_sAnimationArray[m_nSkinMeshType][m_nWeaponTypeIndex][dwAniTable[motion]]`
(e.g. `TMHuman.cpp:5902`, `:9348`). `[OBSERVED]`

⚠ Two hazards, flag them when porting:

* For banks 3 (`wb01`) and 5 (`sq01`) the loader writes to
  `m_sAnimationArray[2][nI/100 + 59][…]` and `m_sAnimationArray[4][nI/100 + 59][…]`
  (`MeshManager.cpp:278`, `:291`). With `nI/100 == 1` that is index **60** in a dimension of 60 →
  **out-of-bounds**, writing into the next bank. Either the original array was larger or this is a
  decompiler artefact. `[OBSERVED]` code; `[UNKNOWN]` original layout.
* Using `m_nSkinMeshType` (0..59 for monsters) as the first index of a `[6]` dimension is only valid
  for the two player banks (0,1). Monsters normally bypass this table and use
  `g_MobAniTable[type].dwAniTable[]` directly. `[OBSERVED]` usage sites.

### 6.7 `g_dwHandIndex[101][2]`

`TMGlobal.h:389-406`. For skin types 0..18 (and 45/46 which are handled specially elsewhere) it
gives the two **hand bone ids**:

```
type : {right, left}
0    : {19,25}   1 : {18,24}   2 : {15,21}   3 : {15,21}   4 : {12,18}
5    : {22,28}   6 : {23,29}   7 : {20,26}   8 : {24,30}   9 : {23,31}
10   : {32,17}  11 : {22,35}  12..18 : {34,44}   ≥19 : {0,0}
```

`[OBSERVED]`. Used for weapon attachment (§11) and for the weapon-trail effect.

### 6.8 `g_nMaxSizeOffset[100]`

Set at load time to `bytesOfAllLoadedMatricesForThatBank` (`MeshManager.cpp:336`). **Nothing reads
it** in this codebase (`grep` finds only the definition, the write and the declaration). It was
presumably the size of a later-uploaded buffer in the original engine. `[OBSERVED]` — safe to ignore.

---

## 7. Runtime structures and loading

### 7.1 Loader pseudocode (`MeshManager::InitBoneAnimation`)

```
open BoneAni4.txt ("%d %d %d %s")
fread ValidIndex.bin -> stValidAniList[100][186]          // 74400 bytes, one shot

for count in 0..99:
    parse  index, aniTypeCount, numParts, name  (stop at EOF)
    rec = m_BoneAnimationList[index]
    rec.numAniTypeCount = aniTypeCount
    rec.numParts        = numParts
    rec.szAniName       = name

    # pass 1: skeleton + total frame count
    rec.pBone  = malloc(filelength(name.bon)); read it
    rec.numBone = filelength/8
    totalFrames = 0
    for k in 0..aniTypeCount-1:
        f = open("<name><ValidIndex[index][k].nI+1:04d>.ani")
        read frames (dword0) ; read rec.numAniFrame (dword1)
        totalFrames += frames
        close

    rec.matAnimation  = malloc(rec.numAniFrame * totalFrames * 64)
    if index == 0 or index == 1:
        rec.matQuaternion = malloc(rec.numAniFrame * totalFrames * 16)

    # pass 2: read every clip into matAnimation
    offset = 0
    for k in 0..aniTypeCount-1:
        f = open(same name)
        read frames (dword0), rows (dword1)
        rec.numAniCut[offsetInList] = frames
        fread(matAnimation + offset, rows*frames*64)
        if index == 0 or 1:  for j in 0..rows*frames-1:
            D3DXQuaternionRotationMatrix(&matQuaternion[j+offset], &matAnimation[j+offset])
        offset += rows*frames
        ... fill m_sAnimationArray (see 6.6) ...
        if ++k >= aniTypeCount: g_nMaxSizeOffset[index] = offset*64; break
```

`[OBSERVED]` `MeshManager.cpp:72-344`.

Notes for a re-implementation:

* Memory per bank is `rows × totalFrames × 64` bytes. For `or01` (35 rows × 194 frames over its 14
  shipped clips) that is 434 560 bytes; for `ch01` with 186 clips it is several MB. A port should
  keep the clips **separate** (one buffer per clip) instead of concatenating — nothing except the
  `[frame][bone]` index math depends on the concatenation.
* `numAniFrame` (rows) and `numAniCut[k]` (frames) are **re-read on every iteration** and stored in
  the same two fields, so after loading they describe the *last* clip read. Only `numAniCut[]`,
  `matAnimation`, `numBone` and the accumulated `matQuaternion` are actually used later.
  `[OBSERVED]`
* The quaternion array is built for banks 0/1 only and is used **exclusively** by the cross-fade
  path (§8.3).

### 7.2 `TMSkinMesh::RestoreDeviceObjects` (skeleton + parts)

```
m_pRoot = new CFrame(0); m_pframeToAnimate[0] = m_pRoot
build the CFrame tree from rec.pBone            (see §4)
for i in 0..rec.numParts-1:                     # i is 0-based
    meshIdx = Mesh_i ; skinIdx = Skin_i         # LOOK_INFO is 16 shorts: Mesh0,Skin0,Mesh1,Skin1,…
    szName    = "<bank><i+1:02d><meshIdx + 20*m_bExpand + 1:02d>.msh"
    szTexture = "<bank><i+1:02d><(skinIdx & 0xFFF) + meshIdx + 20*m_bExpand + 1:02d>.wyt"
    (≈40 hard-coded string exceptions follow — MantleException, ch0102xx remaps, tr13..tr20 remaps,
     costumes by m_nCosType, God2 single-texture parts)
    mesh = new CMesh(this)
    mesh->m_nTextureIndex = TextureManager->GetModelTextureIndex(szTexture)
    if i == 1 and Look.Mesh1 == 40:  mesh->m_bHead  = 1
    if m_bMount:                     mesh->m_bMount = 1
    if i in {6,7} and skinType < 19:            # the two hands
        mesh->m_dwID = g_dwHandIndex[skinType][i-6]
        frame = m_pRoot->FindFrame(mesh->m_dwID)
        frame->m_pMesh = mesh;  mesh->InitEffect()      # NO .msh is loaded
    else if mesh->LoadMesh(szName) == 1:
        m_pRoot->FindFrame(mesh->m_dwID)->m_pMesh = mesh
m_pRoot->LinkBones(m_pRoot)      # binds .msh palette entries to live CFrame matrices
```

`[OBSERVED]` `TMSkinMesh.cpp:119-423`.

---

## 8. Sampling a pose at time t (`FrameMove`)

### 8.1 Clock

```c
void TMSkinMesh::FrameMove(unsigned int /*ignored*/)     // TMSkinMesh.cpp:428
{
    t = m_dwStartTime + g_pTimerManager->GetServerTime() - m_dwStartOffset;   // ms

    if (m_dwFPS == 0) m_dwFPS = 30;
    quarter = t / m_dwFPS;                      // integer division
    frames  = numAniCut[m_nAniIndex];           // frames of the current cut
    if (skinType == 49) frames -= 2;            // (sic)
    if (frames == 0) return;
    quarter %= 4 * frames;                      // loop
    frame    = quarter / 4;
    phase    = quarter % 4;                     // 0..3
}
```

`[OBSERVED]`

* **`m_dwFPS` is milliseconds per quarter frame, not FPS.** A clip frame is played for
  `4 × m_dwFPS` ms. With the Knight table: STAND `4×20 = 80 ms` (12.5 fps), WALK `4×13 = 52 ms`
  (≈19 fps), ATTACK `4×15 = 60 ms` (16.7 fps). `[OBSERVED]` code + `AniSound4.txt` values.
* The clock is **server time** (`timeGetTime()` + server offset, `TimerManager.cpp:56-80`) — it does
  not stop when the client stalls, and it wraps as a `DWORD` (`[OBSERVED]`; the wrap is harmless
  because of the modulo).
* `m_dwStartOffset` is set to `GetServerTime()` in `SetAnimation` (`TMSkinMesh.cpp:739`), so each
  cut restarts at phase 0.
* `m_dwStartTime` is **only initialised for skin types ≥ 60** ("scenery": trees, grass) where it
  adds a pseudo-random phase `500*((type>>1)%5 + rand()%5)` (`TMSkinMesh.cpp:47-48`). For characters
  it is uninitialised memory `[OBSERVED]` — harmless (a constant phase offset), but a port should
  simply use 0 there.
* Loop rule: `frame = (quarter mod 4*frames)/4`, and the sub-frame blend source wraps to the **first
  frame of the cut** on the last frame — i.e. clips loop seamlessly by design, with no pause, no
  "hold last frame", no ping-pong. `[OBSERVED]`
* There is **no `m_dwOffset` clamp against the last frame** other than the modulo; a clip change
  while `frames` shrinks can momentarily index a shorter cut. `[OBSERVED]` code, `[INFERRED]` risk.

### 8.2 Interpolation: element-wise matrix LERP with {1/4, 1/2, 3/4} weights

When smoothing is active (`frames > 1 and m_nSmooth != 0 and device FPS ≥ 10`), and *not* inside the
10-quarter cross-fade window:

```
A = matAnimation[ (base + frame)     * rows + bone ]
B = matAnimation[ (base + frame + 1) * rows + bone ]        // next frame
    if quarter >= 4*frames - 3:                             // last frame
        B = matAnimation[ base * rows + bone ]              //   → first frame of the cut

phase 0: pose = A
phase 1: pose = (3*A + B) / 4        // element-wise on all 16 floats
phase 2: pose = (A + B) / 2
phase 3: pose = (A + 3*B) / 4
```

`[OBSERVED]` `TMSkinMesh.cpp:471-536` (the three `for (i<16)` loops at `:507-533`).

**So the interpolation is a linear blend of raw matrix elements — not slerp, not nlerp, not
"nearest key".** The 3×3 part is therefore *not* re-orthonormalised; the result drifts slightly off
rotation, which is visually irrelevant for 25 % steps and is part of the original look. `[OBSERVED]`

When smoothing is disabled the frame matrix is copied verbatim (`TMSkinMesh.cpp:457-469`). `[OBSERVED]`

### 8.3 Cross-fade when the animation changes

`SetAnimation(n)` (`TMSkinMesh.cpp:727-753`) stores

```
m_nAniIndexLast = m_nAniIndex                       // old cut
m_dwTickLast    = rows * (m_dwOffset + m_nAniBaseIndex)   // old absolute matrix index of bone 0
m_dwStartOffset = GetServerTime()
m_nAniIndex     = n
m_nAniBaseIndex = Σ numAniCut[0..n-1]               // first frame of the new cut
```

Then, while `m_nAniIndexLast != 0 and quarter < 10` (i.e. the first 10 quarters = 2.5 frames),
`FrameMove` blends the *new* pose with the *frozen old* pose:

```
w = quarter / 10                       // 0..0.9
bank 0/1:  q = slerp(q_new(t), q_old(frozen), 1-w);  translation = lerp(new, old, 1-w)
otherwise: pose = w*A_new + (1-w)*A_old          // element-wise, all 16 floats
```

`[OBSERVED]` `TMSkinMesh.cpp:538-587` (`D3DXQuaternionSlerp` at `:550`).

`m_nAniIndexLast` is reset to 0 once `quarter >= 10`, so the cross-fade happens at most once per
switch. `[OBSERVED]` `:480-483`.

A port can reproduce this exactly with the two per-bank quaternion arrays, or (better) do the
cross-fade in quaternion space for **all** banks — the visual difference on non-human skeletons is
negligible. `[INFERRED]`

### 8.4 Pose-sampling pseudocode (normative)

```text
samplePose(skin, absTimeMs) -> localPose[boneCount]
    if skin.fps == 0: skin.fps = 30
    t       = skin.startTime + absTimeMs - skin.cutStartOffset
    quarter = floor(t / skin.fps)
    frames  = clip[skin.cut].frames
    if frames == 0: return previous pose
    quarter = quarter mod (4*frames)
    frame   = quarter / 4
    phase   = quarter mod 4

    for bone in 0 .. rows-1:
        if skin.frames[bone] == null: continue          # bone id not present in .bon
        A = clip[skin.cut].mat[frame*rows + bone]
        if smoothing and frames > 1:
            nxt = frame + 1
            if quarter >= 4*frames - 3: nxt = 0
            B = clip[skin.cut].mat[nxt*rows + bone]
            A = lerp16(A, B, phase/4)                   # 0, .25, .5, .75
        skin.frames[bone].local = A
```

---

## 9. Bone matrices: local → world

```c
void CFrame::UpdateFrames(D3DXMATRIX* parentCombined)   // CFrame.cpp:279
{
    m_matCombined = m_matRot * (*parentCombined);       // row-vector: local then parent
    ... (gameplay probes, see §14.4) ...
    if (m_pFirstChild and |m_matCombined._11| <= 1000)  // corruption guard
        m_pFirstChild->UpdateFrames(&m_matCombined);
    if (m_pSibling) m_pSibling->UpdateFrames(parentCombined);
}
```

called once per draw as `m_pRoot->UpdateFrames(&identity)` (`TMSkinMesh.cpp:677-707`). `[OBSERVED]`

The **root** bone's `m_matRot` is *overwritten* every frame by `TMSkinMesh::Render` with the object's
world matrix, so the skinned result is already in world space:

```c
// normal case (m_bBaseMat == 0), TMSkinMesh.cpp:620-675
R = RotationYawPitchRoll(m_vAngle.y - 90°, m_vAngle.x - 90°, m_vAngle.z)
S = Scaling(m_vScale)
T = Translation(m_vPosition)
root.matRot = R * S * T                      // row-vector: scale, then rotate, then translate

// "mounted rider / attached actor" case (1 <= m_bBaseMat <= 5), :606-618
root.matRot = ( m_matMantua * Translation(fLen2, fLen, 0) * Scaling(fScale) ) * m_BaseMatrix
```

`[OBSERVED]`. Note the **parameter order trap** in the mounted case:
`Render(fLen, fScale, fLen2)` → translation is `(fLen2, fLen, 0)` = `(param3, param1, 0)`
(`TMSkinMesh.cpp:614`). `TMHuman` passes
`Render(vecPosScale.x, vecPosScale.y, vecPosScale.z)` with the y component set to
`1/m_fMountScale`, i.e. **the rider is scaled by the inverse mount scale and offset in model X/Z**
(`TMHuman.cpp:1580-1668`). `[OBSERVED]`

Skin-mesh types 45..57 (ships, cannons, ballistae) get extra handling: an 0.85 uniform scale for
type 48, a forced 1.5 scale for 47, and a different yaw sign (`TMSkinMesh.cpp:626-648`). Types 48-52
also **remap rows of the combined matrix** to build `m_OutMatrix` (a mirror/axis-swap for boats),
`CFrame.cpp:822-881`. `[OBSERVED]`

---

## 10. The bone matrix palette and GPU skinning

### 10.1 Skinning is 100 % GPU (or D3D's software vertex-shader emulation)

`CMesh::RenderMesh` (`CMesh.cpp:598-774`) is the only path that draws skinned geometry:

```c
if (m_numFaceInflunce > 4 || m_numFaceInflunce < 1) return 0;
declIdx = (skinType == 61) ? m_numFaceInflunce + 3 : m_numFaceInflunce - 1;
SetVertexDeclaration(m_pVertexDeclaration[declIdx]);
SetVertexShader   (m_pVertexShader   [declIdx]);        // Shader\skinmesh{declIdx+1}.bin
...
DrawIndexedPrimitive(TRIANGLELIST, 0, 0, mesh->AttRange[0].VertexCount, 0, m_numFaces);
```

`[OBSERVED]`. There is **no CPU skinning code anywhere** — the "software path" is
`SetSoftwareVertexProcessing(1)` (`CMesh.cpp:601-602`), which makes the D3D runtime emulate the *same*
vertex shader. `[OBSERVED]`

`skinType == 61` selects the upper four shaders (`skinmesh5..8`) — behaviour difference `[UNKNOWN]`
(the bytecode is a separate build; possibly a fog/alpha variant for the "lf01" effect type).

### 10.2 Palette construction (exact)

```c
for (i = 0; i < m_numPalette; ++i) {                       // CMesh.cpp:625-631
    D3DXMATRIXA16 mat;
    D3DXMatrixMultiply(&mat, &m_pBoneMatrix[i], m_pBoneOffset[i]);   // Bind * BoneWorld
    D3DXMatrixMultiplyTranspose(&mat, &mat, &g_pDevice->m_matView);  // (Bind*BoneWorld*View)^T
    SetVertexShaderConstantF(3*i + 9, (const float*)&mat, 3);        // 3 float4 = 12 floats
}
```

`[OBSERVED]`

Semantics in plain terms:

```
palette_i = transpose( Bind_i · BoneWorld_i · View )          (all matrices row-vector)
```

which is *exactly* the column-vector matrix a GL/Vulkan shader wants:

```
palette_i^col = View^T · BoneWorld_i^T · Bind_i^T
v_view = palette_i^col · vec4(v_mesh, 1)          // 4×4, translation in the 4th column
```

**Only 3 of the 4 rows/columns are uploaded**, because the 4th is implicit:

* The three uploaded `float4` are rows 0..2 of `transpose(A)`, i.e. the *columns* 0..2 of `A` written
  as row-vectors. The shader therefore computes each output component with one `dp4` against the
  4-component input `(x,y,z,1)`.
* The 4th component would need column 3 of `A`, which for every affine matrix in the chain
  (`Bind`, `BoneWorld`, `View`) is exactly `(0,0,0,1)` — so `w` is implicitly 1 and the projection
  matrix (also uploaded **transposed**, into `c2..c5`, `RenderDevice.cpp:965-967`) produces the real
  clip-space `w` afterwards.

`[OBSERVED]` producer code and the 3-constants-per-bone upload; the `w = 1` argument is `[INFERRED]`
(it is the only consistent reading of a 3-constant upload of a transposed affine matrix, and it is
confirmed by the bytecode's `m4x3` usage — a 4×3 multiply has exactly 3 output components).

### 10.3 Vertex-shader constant map (complete)

| registers | contents | who writes it |
|---|---|---|
| `c0` | `{ 1.0, material.Power, serverTime%10000/10000, 765.01 }` — the `z` component is a 10-second sawtooth phase | `CMesh::SetMaterial`, `CMesh.cpp:993`, `:1034-1036` |
| `c1` | light direction `float4(dir.x, 0.5, dir.z, 0)` — from the world light **or** `normalize(-1,1,1)` | `RenderDevice.cpp:1689` (per frame), re-written by `CMesh.cpp:633-635` before each skinned draw |
| `c2..c5` | `transpose(Projection)` (4 float4) | `RenderDevice.cpp:965-967` (and again at `:1851` on device reset) |
| `c6` | `{1.0, fogEnd, 1/(fogEnd-fogStart), 0}` — linear vertex fog | `CMesh.cpp:1046-1049` |
| `c7` | ambient×0.25 + emissive, as a `D3DXCOLOR` (r,g,b,a) | `CMesh.cpp:1031-1033` |
| `c8` | the material's `Diffuse` `D3DCOLORVALUE` (r,g,b,a) — first 16 bytes of `D3DMATERIAL9` | `CMesh.cpp:1028-1030` |
| `c9 + 3*i` | palette entry `i`, 3 float4 = columns/rows 0..2 of `palette_i` | `CMesh.cpp:630` |
| `c92..c95` | `transpose(inverse(View))` — view matrix for the specular term | `CMesh.cpp:637-640` |

`[OBSERVED]` — every write is cited. `SetMaterial` is only called when `nBright == 0`
(`CMesh.cpp:54-55`), so the hover-highlight pass (which uses `nBright != 0`, `:737`) relies on the
constants of the previous normal pass.

Budget check: `3*i+9` must stay below `92`, so **at most 27 palette entries** fit; the data uses at
most **26** (`ani-verify-sizemath-msh.txt`). `[OBSERVED]`

### 10.4 Shader bytecode evidence (what the bytecode *does* tell us)

`Shader\skinmesh1.bin` (468 B) starts with `0xFFFE0101` (`vs_1_1`). Scanning the token stream for
declarations (`0x0000001F`) and for relatively-addressed constants gives a decisive picture
(`docs/rendering/evidence/hex/ani-shader-dcl-and-relative-addressing.txt`,
`ani-shader-relative-addressing.txt`):

| file | declarations (usage) | relative const reads at base `c9` |
|---|---|---|
| `skinmesh1.bin` | POSITION, BLENDINDICES, NORMAL, TEXCOORD | **2** |
| `skinmesh2.bin` | POSITION, BLENDWEIGHT, BLENDINDICES, NORMAL, TEXCOORD | **4** |
| `skinmesh3.bin` | POSITION, BLENDWEIGHT, BLENDINDICES, NORMAL, TEXCOORD | **6** |
| `skinmesh4.bin` | POSITION, BLENDWEIGHT, BLENDINDICES, NORMAL, TEXCOORD | **8** |
| `skinmesh5..8.bin` | identical to 1..4 | 2 / 4 / 6 / 8 |

`[OBSERVED]`

Three facts follow directly, and they confirm §10.2/§10.5 independently of the CPU code:

1. The palette is addressed **relative to `a0.x` with base register 9** — i.e. exactly
   `c[a0.x*3 + 9]`, three constants per bone (a 4×3 transform), as the CPU upload implies.
2. There are exactly **2 palette reads per bone influence** — one for the position (`m4x3`) and one
   for the normal (`m3x3`). So the shader is a plain linear-blend skinning loop.
3. The declaration sets match the vertex declarations byte for byte (no `BLENDWEIGHT` for 1 influence,
   `BLENDWEIGHT` present for 2..4). `[OBSERVED]`

The D3D9 instruction-length nibble is zero in these files (a `dcl` appears as `0x0000001F` instead of
`0x0200001F`), so a byte-exact linear disassembly is not reliable and whether D3D9's
`CreateVertexShader` accepts them is untested. `[OBSERVED]` bytes, `[UNKNOWN]` runtime acceptance.
**A port should not try to reuse these files** — write a new shader with the semantics of §10.2.

### 10.5 Blend maths

The bytecode proves the shape of the loop (§10.4): per influence, one `m4x3` on the position and one
`m3x3` on the normal, both against `c[a0.x*3 + 9]`, accumulated linearly. Filled in with the vertex
layout (§5.2) this is the complete model:

```glsl
// per influence k = 0..n-1
idx   = int(indices[k]);          // from the packed D3DCOLOR (4 bytes, one per influence)
w     = (k == n-1) ? 1.0 - sum(weights[0..n-2]) : weights[k];
pos  += w * (palette[idx] * vec4(vPosition, 1.0)).xyz;
nrm  += w * mat3(palette[idx]) * vNormal;
```

`[INFERRED]` for the weight bookkeeping (`n-1` stored weights ⇒ last = `1-Σ`), `[OBSERVED]` for
everything else (2 relative lookups per influence, base `c9`, stride 3, `a0.x` driven by the packed
index bytes). A port reproduces the *data* — which is what determines the output — and can use any
equivalent shader.

Informational: the vertex shader is a complete fixed-function replacement for skinned meshes:

* lighting: `lambert = max(0, dot(normal_palette_transformed, c1))`, colour =
  `texture × (c7 + c8 × lambert)` plus a specular term built from `c0` (`Power`, phase) and `c92..c95`
  (view matrix) — i.e. one directional light from `c1` with the per-mesh ambient/emissive/diffuse from
  the CPU-side material, no `D3DTS_LIGHT` involvement. `[INFERRED]` from the constant contents;
  the shader source is `[UNKNOWN]`.
* fog: linear fog from `c6`, computed from the view-space `z` the shader produces. `[INFERRED]`
* the 10-second `c0.z` sawtooth is presumably used for a subtle shimmer on legendary items.
  `[UNKNOWN]` what exactly it modulates.

A port that writes its own shader should therefore emit `alpha-tested texture × (ambient + diffuse·N·L)`
and, after conversion, a linear-fog term — anything else changes the look.

### 10.6 Per-draw state

* Cull mode is forced to **none** for skinned meshes (`CMesh.cpp:647-648`) — WYD characters are
  rendered double-sided. `[OBSERVED]`
* Fog is disabled per draw (`SetRenderState(D3DRS_FOGVERTEXMODE, NONE)`, `CMesh.cpp:600`) and line
  `525` sets `FOGVERTEXMODE = 0` again for the fogged path; the shader produces view-space positions,
  so vertex fog *is* possible — but the port's own fog works on the final depth, so treat fog as
  normal. `[OBSERVED]` state calls.
* Alpha test is enabled (`D3DRS_ALPHATESTENABLE=1`) except for textures whose `cAlpha == 'C'`
  (`CMesh.cpp:47-50`).
* One draw call per `.msh` part, `PrimitiveCount = indexCount/3`, `StartIndex = 0`,
  `MinIndex = 0`, `NumVertices = vertexCount` (`CMesh.cpp:650`). `[OBSERVED]`

---

## 11. Attachments: weapons, shields, heads, mounts, mantles

### 11.1 The three attachment mechanisms

| mechanism | used for | how the world matrix is produced |
|---|---|---|
| **skinning** | all body/armour `.msh` parts | palette (§10.2) |
| **rigid `.msa` on a bone** | weapons, shields, quivers, held items | fixed-function world matrix = hand-bone `m_matCombined` (+ small T/R offsets), then `TMMesh::Render(cMulti, 0)` which draws with that transform as-is (§11.2) |
| **nested `TMSkinMesh`** | mounts, mantles, wings, ships | rider's root matrix = `(...) * mountBone->m_matCombined` (§11.4) |

### 11.2 Weapon / shield on a hand bone

`CFrame::Render` (`CFrame.cpp:68-245`) — for the frame whose id equals a hand index:

```c
matrixComb = m_matCombined;                            // hand bone, world space
if right hand: matTrans = Translation(-m_fLenDetail, +fShield+0.01, 0)
               mat      = RotationYawPitchRoll(180°, 0, 0)     // or (0,0,180°) if m_cRotate[0]
else (left):   matTrans = Translation(-m_fLenDetail(*±), -0.01, 0)
               mat      = RotationYawPitchRoll(0, 0, 180°)     // type-specific yaw/pitch for 1 and 6
matrixComb = (matTrans * mat) * m_matCombined;         // offsets are applied *before* the bone
SetTransform(D3DTS_WORLDMATRIX(0), &matrixComb);
m_pMesh->Render(0);                                    // CMesh → common .msa
```

`[OBSERVED]`. Constants: `m_fLenDetail = 0.07f` for skin type 1, else `0.1f`
(`TMSkinMesh.cpp:68-71`); shields with `1700 < Look.Mesh7 <= 1800` get `fShield = -0.05f`
(`CFrame.cpp:85-89`).

Important detail: the overload actually called here is `TMMesh::Render(char cMulti, int nTexOffset)`
(`TMMesh.cpp:52`), which **does not touch any transform** — it simply draws with whatever world
matrix is current, i.e. the hand-bone matrix set on the line above. `[OBSERVED]`
(`g_pDevice->m_matWorld`, which the placement overloads multiply into their matrices, is set to the
identity once at device creation and never changed: `RenderDevice.cpp:1577-1578`, no other
assignment exists in the codebase.) `[OBSERVED]`

The `.msa` itself is fetched by index: `g_pMeshManager->GetCommonMesh(nPartIndex, 0, 20_min)` where
`nPartIndex = m_pParentSkin->m_Look.Mesh6` (right) / `Mesh7` (left) (`CMesh.cpp:558-586`), and only
drawn when `13 ≤ idx < 1000` or `2700 < idx ≤ 3000` (`CMesh.cpp:573`). `[OBSERVED]`

Note the hand `CMesh` never loads a `.msh` (§7.2), so a weapon is *always* a rigid `.msa`. The
"two-hand"/"one-hand" pose differences come from the animation row, not from different geometry.

### 11.3 Head / mount / shield flags on `CMesh`

| flag | set when | effect |
|---|---|---|
| `m_bHead` | part 1 and `Look.Mesh1 == 40`, or any part when a costume is worn (`TMSkinMesh.cpp:336-345`) | switches texture-stage 0/1 to a "skin-colour × texture" blend for legendary head items (`CMesh.cpp:386-412`) |
| `m_bMount` | the whole skin mesh is a mount (`TMSkinMesh.cpp:338-339`) | selects effect-texture base 440 instead of 233 (`CMesh.cpp:490-497`) |
| `m_bSheild` | set at draw time when a legend-type 116..125 item has no texture (`CMesh.cpp:365-366`) | forces the "glow" blend mode |
| `m_god2cos` | `Look` bytes are `_`, `a`, `N`, `L` (`TMSkinMesh.cpp:164-165`) | God2 costume handling |

`[OBSERVED]`. **None of these affect the skeleton** — they are texture/blend selectors only.

### 11.4 Mounts (riding)

`TMHuman::Render` (`TMHuman.cpp:1570-1674`):

```c
m_pMount->Render(0, 1, 0);                       // the mount animates on its own skeleton
m_pSkinMesh->m_BaseMatrix = m_pMount->m_OutMatrix;   // the mount's *saddle* bone matrix
m_pSkinMesh->Render(vecPosScale.x, vecPosScale.y, vecPosScale.z);  // rider, m_bBaseMat >= 1
```

* `m_pMount->m_OutMatrix` is filled inside `CFrame::UpdateFrames` when the mount's skeleton reaches a
  hard-coded saddle bone id (per skin type; e.g. type 31 → frame 4, type 25 → frame 3, type 29 → frame 5,
  `CFrame.cpp:659-665`, `:729-731`, `:754-761`, `:764-765`, `:824-826`, `:844-846`, `:864-866`,
  `:884-886`). `[OBSERVED]`
* `m_bBaseMat` (1..5) is set by `TMSkinMesh::SetVecMantua(type, skinType)`
  (`TMSkinMesh.cpp:837-894`), which also builds the ~90°/180° correction matrix `m_matMantua` and a
  per-mount vertical offset (`fMantuaUp`, 0.1..0.5). `[OBSERVED]`
* The rider is mounted **per mount skin type** with hand-tuned offsets (`TMHuman.cpp:1585-1666`) —
  these numbers are needed for a pixel-exact port.

### 11.5 Weapon trail (`TMEffectSWSwing`) — animation data reused directly

`TMSkinMesh::SetSwingMatrix()` (`TMSkinMesh.cpp:779-835`) pre-computes, for the **current cut**, up
to 48 matrices of the hand bone in world space by concatenating the raw keyframe rows:

```
for i in 0..min(frames,48)-1:
    mat = clip[cut].mat[i*rows + handBoneID]
    for j in 0..parentChainLen-1:
        mat = mat * clip[cut].mat[i*rows + ancestorID[j]]
    swing->m_matRot[i] = mat
swing->m_nNumTicks = frames
```

`[OBSERVED]`. The parent chain is collected once at attach time by walking
`CFrame::m_dwParentID` up to 48 levels (`CMesh.cpp:955-969`). Note this uses **keyframes**, not the
interpolated pose, so the trail is quantised to clip frames. `[OBSERVED]`

### 11.6 Effect-skin meshes (`TMEffectSkinMesh`)

Copies the owner's animation state (`SetAnimation(m_sAnimationArray[...])`, then
`m_dwFPS = owner->m_dwFPS`, e.g. `TMHuman.cpp:9348-9400`) so that a "ghost"/afterimage plays the
identical clip in sync. `[OBSERVED]`

---

## 12. `.msa` — rigid/effect models and their "animation"

### 12.1 Format

| offset | type | size | meaning |
|---|---|---|---|
| 0x00 | `uint32` | 4 | FVF (D3D9 fixed-function FVF) |
| 0x04 | `uint32` | 4 | vertex stride |
| 0x08 | `uint32` | 4 | attribute count `n` (1..30 observed) |
| 0x0C | `D3DXATTRIBUTERANGE[n]` | `20n` | `{AttribId, FaceStart, FaceCount, VertexStart, VertexCount}` — **FaceCount is a *triangle* count here** |
| 0x0C+20n | `char[n][11]` | `11n` | texture file names, fixed 11 bytes, NUL-padded (`"aef004.tga\0"`) |
| … | `uint32` + data | `4 + idxBytes` | index block: `idxBytes = totalTris * 3 * 2` |
| … | `uint32` + data | `4 + vtxBytes` | vertex block: `vtxBytes = totalVerts * stride` |

`[OBSERVED]` `TMMesh::LoadMsa` `TMMesh.cpp:481-728`, byte-exact verification for **all 1698**
`.msa` files (`0 mismatches`, `docs/rendering/evidence/hex/ani-verify-sizemath-msa.txt`).

Worked examples:

| file | size | FVF | stride | att | arithmetic |
|---|---|---|---|---|---|
| `Effect\ankh01.msa` | 3795 | 322 | 24 | 1 | `12 + 20 + 11 + 4+288 + 4+3456 = 3795` ✔ |
| `mesh\aef001.msa` | 41985 | 274 | 32 | 3 | `12 + 60 + 33 + 4+4272 + 4+(1175*32=37600) = 41985` ✔ |

`aef001.msa` attribute table (offsets `0x0C..0x48`), from
`docs/rendering/evidence/hex/ani-msa-aef001-3attrib.txt`:

```
attrib0: id=0 faceStart=0     faceCount=0x1A8(424) vertStart=0     vertCount=0x325(805)
attrib1: id=1 faceStart=0x1A8 faceCount=0xC8(200)  vertStart=0x325 vertCount=0xC0(192)
attrib2: id=2 faceStart=0x270 faceCount=0x58(88)   vertStart=0x3E5 vertCount=0xB2(178)
textures @0x48: "aef004.tga\0" "aef001.tga\0" "aef002.tga\0"
index bytes   @0x69: 0x10B0 = 4272 = 712 tris × 3 × 2
```

### 12.2 Vertex layouts (all 1698 files)

| FVF | count | layout | stride used by the loader after patching |
|---|---|---|---|
| 274 `0x112` (XYZ+NORMAL+TEX1) | 1638 | pos3, normal3, uv | 32 **(+8 → 40)** |
| 322 `0x142` (XYZ+DIFFUSE+TEX1) | 59 | pos3, `D3DCOLOR` (diffuse/alpha), uv | 24 (unchanged) |
| 18 `0x012` (XYZ+NORMAL) | 1 (`grid.msa`) | pos3, normal3 | 24 → 32 |

`[OBSERVED]` histogram in `ani-verify-sizemath-msa.txt`.

**Lighting split.** `D3DRS_LIGHTING` is on by default (`RenderDevice.cpp:1714`) and re-enabled after
every effect draw, so `.msa` objects are lit by the fixed-function T&L with the current
`D3DMATERIAL9` and the world lights. Therefore `[INFERRED]` (standard D3D9 T&L behaviour):

* **FVF 274** (1638 files, has `NORMAL`): lit — `ambient + Σ diffuse·N·L` (+ specular), then
  modulated by the texture.
* **FVF 322** (59 files, has `DIFFUSE` but **no normal**): D3D9 *ignores lighting* for vertex formats
  without normals, so these objects render as `material × vertexColour × texture` — flat/unlit.
  This is why they carry a baked colour instead of a normal.
* `grid.msa` (FVF 18) has a normal but no texture coordinates → lit, untextured.

A port must therefore keep two `.msa` material paths (lit vs. vertex-colour-only), or bake the
difference into the shader selection.

`TMMesh::LoadMsa` **patches the vertex buffer at load time**: unless the FVF is exactly 322 it adds
`D3DFVF_TEX1` (`m_dwFVF += 256`), allocates `stride+8` per vertex, copies the file data into the first
`stride` bytes and leaves the last 8 bytes zero; then for `FVF 530` it duplicates `tu1,tv1` into
`tu2,tv2` (`TMMesh.cpp:642-703`). A port should simply read the file's own layout and ignore the
patch — the extra UV set only exists so that the *second* texture stage has something to sample.
`[OBSERVED]`

### 12.3 Texture resolution

`TMMesh::LoadMsa` reads the 11-byte name, strips the directory, **keeps everything up to the first
`.`, and appends `.wys`** — the `.tga` in the file is legacy. Prefix is `mesh\` normally and
`Effect\` when the `.msa` itself lives in an `Effect` directory (`TMMesh.cpp:539-598`). If the lookup
fails, the `.msa`'s own base name is tried, and if that fails too the mesh is rejected. `[OBSERVED]`

Concretely: `aef001.msa` names `aef001.tga` → the file on disk is `mesh\aef001.wys` (present).
There is **no 1:1 `.msa`/`.wys` name pairing**: 849 of the 1661 `mesh\*.msa` files have no
same-named `.wys`; the real pairing is *through the names embedded in the `.msa`*. `[OBSERVED]`
(measured; the client contains no `.tga` at all).

### 12.4 Does a `.msa` animate?

**No skeleton, no keyframes, no morphs.** `TMMesh` has no animation clock and no per-frame state
except texture coordinates. What looks like animation is produced by:

1. **UV scrolling** of the second texture stage, driven by server time
   (`TMMesh::Render(char cMulti, ...)`, `TMMesh.cpp:52-157`):
   `progress = (time mod 4000)/4000` (or `/8000` for `cMulti == 100`), then per mode:
   `1` both UVs `+p`, `2` `u2 = u1+p`, `3` `v2 = v1+p`, `4` `u2 = u1-p`, `5` `v2 = v1-p`,
   `6`/`101` two-phase scroll (`v1 = v2 - p` after the first frame, with `u1 = u2 + p` for 101),
   `7` `u1 = u2 - 3p`, `100` `u1 = u2 - 2p`. `[OBSERVED]`
   The patch is applied **by locking the vertex buffer and rewriting it on the GPU/CPU every draw**
   (`m_pVB->Lock`), which is also why the vertex buffer is `D3DPOOL_MANAGED`. `[OBSERVED]`
2. **Rigid transform** supplied by the caller each frame (position/rotation/scale), see §13.

`m_nTextureIndex[0..31]`, `m_AttRange[0..31]` (`TMMesh.h:41-42`) — up to 32 attributes; the data max
is 30 (`ani-verify-sizemath-msa.txt`). Each attribute is drawn in its own
`DrawIndexedPrimitive` with its own texture (`TMMesh::RenderDraw`, `TMMesh.cpp:215-232`). `[OBSERVED]`

---

## 13. Non-skinned animation paths

### 13.1 Rigid objects (`.msa`) — placement, not animation

`TMMesh` offers three placement overloads (`TMMesh.cpp:159-202`):

```c
Render(fX,fY,fZ, fAngle,fAngle2,fAngle3, cMulti, nTexOffset):
    matScale    = Scaling(m_fScaleH, m_fScaleV, m_fScaleH)
    matPosition = Translation(fX,fY,fZ)
    mat         = RotationYawPitchRoll(fAngle, fAngle2 - 90°, fAngle3)
    mat = matWorld * mat * matScale * matPosition        // world → local rot → scale → translation
    SetTransform(WORLDMATRIX(0), mat)

Render(fX,fY,fZ, quat, cMulti, nTexOffset):               // same, but orientation from a quaternion
    mat = RotationQuaternion(quat) * matWorld * matScale * matPosition
```

`[OBSERVED]`. The `-90°` on the pitch is the same Z-up→Y-up conversion as for characters; the
quaternion overload is used by effects that store an orientation quaternion. `[OBSERVED]`
Signature quirk: in the first overload the multiplication starts from `matWorld * rot`, in the second
from `rot * matWorld` — an inconsistency in the original code; the second is what the effects use for
camera-facing billboards. `[OBSERVED]`

### 13.2 "Scenery" skin meshes

Trees, leaves, grass, fish, ships, butterflies are full `TMSkinMesh` objects with a 1-bone skeleton
(skin types ≥ 60) that get a **random phase** so they do not sway in lock-step:

```c
if (nBoneAniIndex >= MAX_ANI_TYPE)                       // >= 60
    m_dwStartTime = 500 * (((nBoneAniIndex >> 1) % 5) + rand() % 5);
```

`[OBSERVED]` `TMSkinMesh.cpp:47-48`. They still go through the same `FrameMove`/palette path.

### 13.3 Per-object animation speed hacks

`TMHuman::FrameMove` divides/multiplies `m_dwFPS` (remember: ms per quarter frame — **larger value =
slower**) for gameplay reasons (`TMHuman.cpp:6023-6106`):

* `m_cFreeze == 1` → `× 1.15` (15 % slower)
* combo attacks (`m_nMotionCount > 1`) → `/ 1.2`; double attack → `/ 1.7` (or `/ 2.0` for
  `m_nWeaponTypeL == 101`) → i.e. *faster*
* `LEVELUP` for skin type 2 & weapon row 4 → `× 1.6`
* mounted: the rider's fps is multiplied by `m_fMountScale` (`TMHuman.cpp:6001-6002`)

`[OBSERVED]`. A port must reproduce these to keep attack timings identical to the original.

---

## 14. Everything else that affects animation rendering

### 14.1 Bounding volumes are computed once, at load

`CMesh::LoadMesh` (`CMesh.cpp:865-895`) and `TMMesh::LoadMsa` (`TMMesh.cpp:676-722`) walk the vertex
positions of the **bind pose** and store `m_fMinX..m_fMaxZ`, `m_fRadius = max |min/max|` and
`m_vecCenter`. **Nothing recomputes them per frame** — the skinned pose can exceed these bounds.
`[OBSERVED]`

Known quirks to copy or fix deliberately:

* `TMMesh`'s constructor initialises the box *inverted*: `m_fMax* = 1.175494f` (≈FLT_MIN) and
  `m_fMin* = 3.402823f` (large positive) (`TMMesh.cpp:24-29`). `[OBSERVED]`
* Both loops skip vertices: the seed values are read at `pVertex[nAddedFloatNum + nFloatCount]`
  (element 0 of the **second** vertex) and the loop runs `i < nNumVertex - 1`, so vertex 0 and the
  last vertex never contribute. `[OBSERVED]` (`TMMesh.cpp:676-703`, `CMesh.cpp:865-879`). Note that
  `nFloatCount + nAddedFloatNum` *is* the correct stride in floats (24 B layout: 6+0; 32 B layout
  patched to 40 B: 6+2), so the walk itself is sound.
* Radius is the **Chebyshev** bound `max(|min|,|max|)` over axes, not the true distance to the centre.
  `[OBSERVED]`
* The `FVF == 530` branch also copies `tu1,tv1` into `tu2,tv2` inside that loop, i.e. only for the
  vertices that the loop visits — the first and last vertex keep `tu2 = tv2 = 0` until the first
  `TMMesh::Render` rewrites the whole buffer anyway. `[OBSERVED]`

### 14.2 Visibility / culling

* Skinned meshes: `CULLMODE = NONE` (§10.6), no per-part frustum test; parts are drawn by the CFrame
  traversal as long as the owning `TMSkinMesh` says `m_bMeshGenerated != 0`
  (`TMSkinMesh.cpp:600-601`) — that flag is only set by `FrameMove`, so **a mesh that never had its
  animation sampled is never drawn**. `[OBSERVED]`
* Static/common meshes use a distance test: `if (mesh->m_fRadius >= |objPos - camPos|) visible`
  (`TMObject.cpp:351`, `TMHouse.cpp:702`, `TMEffectMesh.cpp:324`) — i.e. a sphere-of-radius test
  against the *square* root distance (dimensionally wrong but that is what ships). `[OBSERVED]`
* `TMObject::IsVisible` additionally rejects objects behind the camera by dot product with
  `m_vecCamDir`, then calls `IsInView()` (`TMObject.cpp:357-366`). `[OBSERVED]`
* Fog cut-off for common meshes: `if (|pos - cam| > m_fFogEnd + radius) skip` (`TMObject.cpp:472`).
  `[OBSERVED]`

### 14.3 Animation-driven gameplay/render events

`TMHuman` reads `m_pSkinMesh->m_dwOffset` (the current **frame index**) as an event trigger:

| frame window | event | source |
|---|---|---|
| `< 2` | footstep sound (`dwSoundTable[10]`) | `TMHuman.cpp:6733-6750` |
| `5..6` | death sound (`dwSoundTable[11]`) | `TMHuman.cpp:6766-6783` |
| `6..7` | attack-connect sound (`dwSoundTable[motion]`) | `TMHuman.cpp:6803-6833` |
| `13..14` | (mounted) hoof sound | `TMHuman.cpp:6844` |
| `8..10` | another sound layer | `TMHuman.cpp:6853` |
| `15..16` | (player) additional sound | `TMHuman.cpp:7117` |
| every 5th frame (skin type 20 only) | head/helm visibility check (`m_dwOffset % 5`) | `TMHuman.cpp:7288` |

`[OBSERVED]`. These thresholds are in **clip frames** (not ms), so they scale with `dwSpeed` — this is
the mechanism a port must keep if sound must stay in sync with the animation.

### 14.4 Skeleton probes used for effects and gameplay

`CFrame::UpdateFrames` contains a large hard-coded switch on the skin type that, while walking the
skeleton, writes bone world positions into the owner's `m_vecTempPos[0..10]` (particle emitters, hand
effects, horse-hoof fire, "dizzy" stars) plus `m_OutMatrix` (the saddle/prop attach matrix) and the
mirror matrices for boats. `[OBSERVED]` `CFrame.cpp:284-898` — ~600 lines, **purely game-side side
effects**. They do not change skinning; a port can implement them lazily as "give me world matrix of
bone N".

Convenient naming for a port: `boneWorld(boneId)` after `UpdateFrames` *is* `frame->m_matCombined`;
every effect in that switch is just `boneWorld(id) * offsetVector`. `[INFERRED]`

### 14.5 Texture animation clock

`TMHuman`/`CMesh` also scroll effect textures with `GetServerTime() % 10000 / 10000`
(`CMesh.cpp:147-148`, `:217-218`, `:990`) and pulse the emissive material of skin type 44 with a
`sin()` of a 3000 ms period (`TMSkinMesh.cpp:680-703`). `[OBSERVED]`

---

## 15. Decompiled-name warning list

Do **not** trust these names; the right-hand column is what the code actually does.

| decompiled name | reality | evidence |
|---|---|---|
| `stBoneAni::numAniFrame` | **number of bones / keyframe rows per frame** | `MeshManager.cpp:134` (read as `.ani` dword1), used as row stride in `FrameMove` |
| `stBoneAni::numAniCut[186]` | **frame count of each clip** | `MeshManager.cpp:170` (`buffer` = `.ani` dword0) |
| `TMSkinMesh::m_dwFPS` | **milliseconds per quarter frame** (larger = slower) | `FrameMove` divides time by it, then `/4` |
| `TMSkinMesh::FrameMove(dwServerTime)` | the parameter is **ignored** (overwritten on line 1) | `TMSkinMesh.cpp:430` |
| `CMesh::m_numFaceInflunce` | number of bone influences per vertex (1..4) | `CMesh.cpp:608`, declaration index |
| `CMesh::m_pBoneOffset[40]` | array of **pointers to live bone world matrices** (not offsets) | `CFrame.cpp:267` |
| `CMesh::m_pBoneMatrix` | the **bind (inverse-bind)** matrices from the `.msh` | `CMesh.cpp:628` |
| `CMesh::m_numPalette` | number of bone matrices for this sub-mesh | `CMesh.cpp:625` |
| `TMMesh::m_bDummy` | "loading failed" marker; if set the mesh is skipped | `CMesh.cpp:817`, `:918` |
| `CMesh::m_pBoneOffset[i]->_41.._43` | the **live world position of bone `i`** (`m_pBoneOffset` points at the frame's `m_matCombined`), used as a distance proxy for the hover-highlight scale — not a "bone offset" | `CMesh.cpp:676-689` |
| `TMSkinMesh::m_bBaseMat` | "this actor rides/attaches to another skeleton" mode (1..5) **and** the mantua type | `TMSkinMesh.cpp:606`, `:837-894` |
| `stCommonMeshList::m_dwNames` | bone-id list of a `.msh` (DWORD per palette entry) | `CMesh.cpp:840` |
| `TMSkinMesh::m_nBoneAniIndex` | **skin mesh type** (index into `m_BoneAnimationList` / `g_MobAniTable`) — not a bone index | `TMSkinMesh.cpp:17` |
| `MeshManager::m_nFullCount` | total frames of the *last* bank parsed (global, not per-bank) | `MeshManager.cpp:141` |

---

## 16. Reimplementation notes (OpenGL / Vulkan)

### 16.1 The matrix-convention trap (read this twice)

The original pipeline is row-vector:

```
v_clip = v_mesh · Bind⁻¹ · BoneWorld · View · Projection
```

and the shader receives `paletteRow_i = (Bind⁻¹ · BoneWorld · View)` **transposed**, three
`float4`s at a time.

For a column-vector GLSL/HLSL port you want

```
gl_Position = uProj_col * ( uView_col * ( uBoneWorld_col * ( uBind_col * vec4(vPos,1) ) ) )
```

with

```
uBind_col      = transpose(Bind⁻¹)          = transpose(M_file)
uBoneWorld_col = transpose(BoneWorld)       = transpose(m_matCombined)
uView_col      = transpose(View)
uProj_col      = transpose(Projection)      (the original also transposes the projection, c2..c5)
```

and the combined palette uploaded per bone is

```
palette_col_i = transpose( M_file_i · m_matCombined_i · View )      // == uView_col·uBoneWorld_col·uBind_col
```

Practical recipe: **build everything in double-checkable row-vector form exactly as this document
describes, then transpose once, right before upload**. Two transposes (one on the CPU, one in the
shader) reproduce the original bit-for-bit behaviour; doing it "by intuition" will produce a
character that is mirrored, inside-out, or rotated 180°.

Three more traps in the same area:

1. `D3DXMatrixMultiply(&out, &a, &b)` = `a*b` in **row-vector** order. If you keep D3DX-style code in
   C++ and call `glUniformMatrix4fv(loc, n, GL_TRUE, m)` you have *already* transposed — do not
   transpose again.
2. Only 3 of the 4 columns exist per bone (`c[3i+9..3i+11]`); the 4th is implicitly `(0,0,0,1)` —
   which is only valid because every matrix in the chain is affine. If you ever insert a
   non-affine matrix (a projection) into the palette, you must upload 4 columns.
3. Winding: the original renders skinned geometry with `CULLMODE = NONE`. If your port culls, you
   must pick the winding that matches the model's Z-up→Y-up conversion, otherwise every character
   becomes inside-out. Drawing double-sided (as the original does) is the safe default.

### 16.2 Where to put the palette

| approach | fit |
|---|---|
| **UBO (`std140`)** | 26 bones × 48 B = 1248 B — trivially fits the 16 KB minimum. Best choice; one `mat4x3` array indexed by `gl_VertexID`-free per-vertex index. Use `mat4x3` (`vec3` columns) and let the 4th column be implicit, mirroring the original. |
| **SSBO** | Overkill (max 26 entries) but useful if you also store per-vertex bone matrices for many characters and want one binding for the whole scene. Remember `std430` alignment: `mat4` = 64 B, `mat4x3` = 48 B. |
| **Push constants** | 128 B guaranteed (Vulkan) → only 2 `mat4x3`; not enough for 26 bones. Use push constants for `uView/uProj/uLightDir` and a UBO for the palette. |
| **Vertex attributes** | The data already carries 4 packed index bytes + up to 3 weight floats per vertex; keep that layout, it is optimal. |

Recommended GPU-side layout per bone (matching the original's 12 floats):

```glsl
layout(std140) uniform Palette { mat4x3 bone[27]; };   // 27 * 48 = 1296 B
// bone[i] columns are vec4s in std140; use mat4x3 so the compiler packs 3 vec4.
```

### 16.3 Sampling and upload order (per frame, per actor)

```
1. cut        <- SetAnimation-equivalent  (see §8.3: remember old cut/frame for the cross-fade)
2. pose[bone] <- samplePose(actor, serverTimeMs)                     (§8.4)
3. for each bone in the .bon tree (root-first, depth-first):
       combined[bone] = pose[bone] · combined[parent]                (row-vector)
   root.pose      = R·S·T  (or the mount-attach matrix)              (§9)
4. upload palette_col[i] = transpose( M_file[i] · combined[boneId_i] · View )   (§10.2)
5. draw each .msh part with its own index/vertex buffer, double-sided, one texture
6. draw the hand-bone .msa(s) with model matrix = T_R · combined[handBone]      (§11.2)
```

Step 3 must run **before** step 4 for every actor, and steps 4-6 are per-part. In a modern renderer
you would instead skin into a compute/vertex stage and batch by texture; nothing in the data prevents
that.

### 16.4 Exact numbers a port needs (copy-paste list)

| constant | value | where |
|---|---|---|
| matrix size on disk | 64 B, row-major, translation in row 3 | §1.1 |
| keyframe index math | `mat[(base + frame) * rows + bone]` | §8.1 |
| sub-frame weights | 0, 1/4, 1/2, 3/4 (element-wise lerp of 16 floats) | §8.2 |
| cross-fade window | 10 quarters (2.5 frames), weights `q/10` | §8.3 |
| frame duration | `4 * dwSpeed` ms | §8.1 |
| default speed | 30 ms/quarter (when `dwSpeed`/`m_dwFPS` is 0) | §8.1 |
| palette formula | `transpose(Bind · BoneWorld · View)` | §10.2 |
| constants per bone | 3 `float4`, starting at `c9` | §10.3 |
| max palette | 26 in the data, 27 by shader budget | §10.3 |
| influences | 1..4, `n-1` weights stored, 4 indices packed in a `D3DCOLOR` | §5.2 |
| vertex strides | 36 / 40 / 44 / 48 B for 1/2/3/4 influences | §5.2 |
| `.msh` layout | `32 + 64n + 4n + stride*verts + 2*idx` | §5.1 |
| `.msa` layout | `12 + 20a + 11a + 4 + idxBytes + 4 + vtxBytes` | §12.1 |
| model→world | `R(yaw-90°, pitch-90°, roll) · S · T` | §9 |
| hand bone ids | `g_dwHandIndex[type][2]` (type 0 → {19,25}) | §6.7 |
| max bones per skeleton | 70 (data), `m_pframeToAnimate[100]` | §3.2 |

### 16.5 Things that are *not* needed

* `g_nMaxSizeOffset[]` — write-only.
* `matQuaternion[]` — only used for the 2.5-frame cross-fade on banks 0/1; a port can slerp on the fly
  from the matrix data or skip the cross-fade entirely.
* `BoneAni3.txt` — never read.
* `stAniSoundTableEx` → sound ids: only needed for audio, not rendering.
* The `0xCD`-filled garbage keyframes (bones not referenced by any `.msh` palette).
* The software-vertex-processing branch: replace with a CPU reference skinning path *for debugging
  only*.

---

*End of document. Evidence files: `docs/rendering/evidence/hex/ani-*.txt` (17 files).*
