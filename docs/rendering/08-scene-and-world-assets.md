# 08 — Scene & World Assets: formats and loading

Purpose: a byte-exact, framework-independent specification of every non-character, non-animation,
non-UI asset that the WYD 748 client renders (terrain, sky, water, decoration, world-object
placement, world metadata), so the identical scene can be rebuilt in OpenGL or Vulkan from this
document alone.

Last updated: 2026-11-10

Source tree: `Projects/TMProject` (decompiled 755/760 code being ported to 748 data).
Asset root: `E:\wyd\test-setup\client_748`. Original binary: `client_748\WYD.exe` (DX8).
Raw evidence: `docs/rendering/evidence/hex/world-*.txt`.

---

## Confidence summary

| tag | count | meaning |
|---|---|---|
| `[OBSERVED]` | 134 | read in `Projects/TMProject/*.cpp|*.h` with `file:line`, or measured byte-accurately in `client_748` |
| `[INFERRED]` | 16 | deduced; the evidence is stated inline |
| `[UNKNOWN]` | 5 | enumerated below, each with the experiment that would resolve it |

Biggest unknowns:

1. **`cdata.bin` checksum algorithm.** The decompiled formula cannot reproduce the hard-coded
   constant `5855606140`, and the shipped table does not contain the byte-sums of the shipped
   `.trn` files. `[UNKNOWN]`
2. **Exact `D3DTSS_*` blend result** of the sky's second stage (`D3DTOP 12`,
   BLENDDIFFUSEALPHA) as *seen on screen*; the enum values and operands are `[OBSERVED]`, the
   resulting colour is `[INFERRED]`.
3. **`object.bin` mask values → ground height** mapping. Owned by another chapter; this document
   only fixes the container (2048×16×16 + checksum) and its role. `[OBSERVED]` container,
   `[UNKNOWN]` mask maths here.
4. **Whether the original 748 `WYD.exe` enforces the `.trn`/`.dat` checksum gates.** The data in
   `cdata.bin` cannot satisfy them, so either the exe skips them or the data was regenerated.
   Resolves by: running `client_748\WYD.exe` with a breakpoint on `MessageBoxA("DataFile Error.")`,
   or by disassembling the two `fopen("cdata.bin")` call sites.
5. **What consumes the `night1..7.msa` meshes** (MeshList 2738-2770). No source path and no shipped
   `.dat` object type reaches them. Resolves by: logging `GetCommonMesh` indices at runtime, or by
   scanning `WYD.exe` for those numeric ids.

Resolved during this pass: the `.wys` inner format is now fully decoded (DDS + two obfuscated
fields, DXT1/DXT3 only — see §6.4 and `evidence/hex/world-wys-headers.txt`), and the two
directional light vectors were found in `RenderDevice.cpp:99-109`.

---

## Table of contents

1. [Global coordinate system and scale constants](#1-global-coordinate-system-and-scale-constants)
2. [Terrain height/tile map — `Env\FieldNNNN.trn`](#2-terrain-heighttile-map--envfieldnnnntrn)
3. [Terrain object list — `Env\FieldNNNN.dat`](#3-terrain-object-list--envfieldnnnndat)
4. [Attribute map — `Env\AttributeMap.dat`](#4-attribute-map--envattributemapdat)
5. [Terrain renderer](#5-terrain-renderer)
6. [Terrain textures — naming, lookup, container format](#6-terrain-textures--naming-lookup-container-format)
7. [Sky](#7-sky)
8. [Water / sea](#8-water--sea)
9. [Environmental decoration](#9-environmental-decoration)
10. [World object placement](#10-world-object-placement)
11. [Scene-definition files](#11-scene-definition-files)
12. [Effect texture plumbing](#12-effect-texture-plumbing)
13. [The `object.bin` / `cdata.bin` / `ItemList.bin` family](#13-the-objectbin--cdatabin--itemlistbin-family)
14. [Reimplementation notes (OpenGL/Vulkan)](#14-reimplementation-notes-openglvulkan)
15. [Decompiled names that are obviously wrong](#15-decompiled-names-that-are-obviously-wrong)

---

## 1. Global coordinate system and scale constants

| quantity | value | evidence |
|---|---|---|
| world X axis | `x` (metres, float) | `[OBSERVED]` TMGround.cpp:2711 |
| world Y axis | **up** (`height`) | `[OBSERVED]` same |
| world Z axis | `y` of the 2-D map data | `[OBSERVED]` TMGround.cpp:2712 `D3DXMatrixTranslation(&matPos, m_vecOffset.x, 0, m_vecOffset.y)` |
| 1 height unit (`cHeight`) | **0.10 world units** | `[OBSERVED]` TMGround.cpp:2711 scale `(2.0, 0.1, 2.0)`; TMGround.cpp:3224 `tempVec.y = position.y * 0.1f` |
| 1 tile (grid cell) | **2.0 × 2.0 world units** | `[OBSERVED]` TMGround.cpp:2711 (x/z scale 2.0); TMGround.cpp:3519 `/2.0f` |
| tiles per zone | **64 × 64** = 4096 | `[OBSERVED]` TMGround.h:60 `m_TileMapData[4096]`, index `x + (y<<6)` |
| zone (map square) | **128 × 128 world units** | `[OBSERVED]` TMScene.cpp:2021-2022 `offset + 128.0f`; TMScene.cpp:2205 `g_HeightPosX = nZoneX << 7` |
| walk/detail mask | **128 × 128** cells, 1 unit each | `[OBSERVED]` TMGround.h:61-62 `char m_pMaskData[128][128]` |
| global playfield | 32 × 32 zones (Field0101 … Field3131) | `[OBSERVED]` `Env\` file set; TMGround.h:44 `m_nCheckSum[64][32]` |

Zone offset in world units (`[OBSERVED]` TMGround.cpp:2344-2345):

```c
m_vecOffset.x = (m_vecOffsetIndex.x * 2.0f) * 64.0f;   // = zoneX * 128
m_vecOffset.y = (m_vecOffsetIndex.y * 2.0f) * 64.0f;   // = zoneY * 128
```

So the whole world is 4096 × 4096 units, and the attribute map is 1024 × 1024 cells of
4 × 4 units (section 4).

---

## 2. Terrain height/tile map — `Env\FieldNNNN.trn`

### 2.1 File identification

`[OBSERVED]` Every `.trn` in `client_748\Env` is **exactly 49160 bytes** (97 field files +
`Character.trn`, see `evidence/hex/world-trn-inventory.txt`).

Worked size math:

```
49160 = 8 (header) + 4096 (tiles) * 12 (bytes/tile)
4096  = 64 * 64
```

`[OBSERVED]` The loader reads exactly that shape — TMGround.cpp:2474-2493:

```c
int byNameLen = 0;
fread(&byNameLen, 1, 1, fp);          // 1 byte
if (byNameLen > 128) byNameLen = 128; // clamp (porting guard)
fread(m_MapName, 1, byNameLen, fp);   // byNameLen bytes
m_MapName[byNameLen] = '\0';
int bPosX = 0, bPosY = 0;
fread(&bPosX, 1, 1, fp);              // 1 byte
fread(&bPosY, 1, 1, fp);              // 1 byte
SetPos(bPosX, bPosY);
SetAttatchEnable(bPosX, bPosY);
fread(m_TileMapData, 12, 4096, fp);   // 12 * 4096 = 49152 bytes
```

Header size is therefore `1 + nameLen + 1 + 1`. All shipped files have `nameLen = 5`
(name `"Field"`), giving the observed 8.

### 2.2 Header

| field | type | byte offset | size | meaning |
|---|---|---|---|---|
| `nameLen` | `uint8` | 0 | 1 | length of the following name, **not** NUL-terminated on disk |
| `name` | `char[nameLen]` | 1 | 5 (in practice) | `"Field"` in 96 of 97 files, `"field"` in `Field2513.trn` `[OBSERVED]` |
| `posX` | `uint8` | 1+nameLen | 1 | zone X (0…31); equals the `XX` of the filename |
| `posY` | `uint8` | 2+nameLen | 1 | zone Y (0…31); equals the `YY` of the filename |

`[OBSERVED]` Verified for all 97 files that `Field%02d%02d.trn` ↔ `posX,posY` (script in
`evidence/hex/world-trn-inventory.txt`). The single exception is `Character.trn`, which is a
byte-for-byte-style copy of `Field1616.trn` (name `"Field"`, pos `16,16`) and is loaded by the
character-select / server-select scenes (TMSelectCharScene.cpp:118, TMSelectServerScene.cpp:280).

### 2.3 Tile record (12 bytes) — proof of layout

| field | type | offset in record | size | meaning |
|---|---|---|---|---|
| `cHeight` | `int8` | 0 | 1 | terrain height, **signed**, ×0.1 = world units |
| `byTileIndex` | `uint8` | 1 | 1 | base texture id; texture list index = `byTileIndex + 10` |
| `byTileCoord` | `uint8` | 2 | 1 | UV orientation 0…7 → `TileCoordList[8]` |
| `byBackTileIndex` | `uint8` | 3 | 1 | detail texture id; list index = `byBackTileIndex + 256` |
| `byBackTileCoord` | `uint8` | 4 | 1 | detail UV variant 0…31 → `BackTileCoordList[32]` |
| *padding* | 3 bytes | 5 | 3 | **uninitialised garbage** written by the map tool |
| `dwColor` | `uint32` (`D3DCOLOR`, 0x00RRGGBB) | 8 | 4 | baked vertex colour / "ground light" |

The 12-byte stride comes from MSVC's default struct alignment of
`struct FileTileInfo { char cHeight, byTileIndex, byTileCoord, byBackTileIndex, byBackTileCoord;
unsigned int dwColor; }` (TMGround.h:6-14): five `char`s (offsets 0-4), **three bytes of padding**
so `dwColor` lands on a 4-byte boundary at offset 8, `sizeof == 12`.

`[OBSERVED]` proof that bytes 5-7 are padding, not data:

```
Field0101.trn tile[0] @0x0008 : 00 64 02 00 03 80 FB 09 00 00 00 00
                               ^cH ^TI ^TC ^BTI ^BTC |pad|  ^--- dwColor ---
Field0630.trn tile[0]        : .. .. .. .. .. 85 FA 09
Field0813.trn tile[0]        : .. .. .. .. .. 01 41 02
```

Three different files have three different garbage triples; only 2 of 4096 tiles in `Field0101.trn`
even have a non-zero pad. A padding *value* would not vary like this. `[OBSERVED]`
(`evidence/hex/world-trn-Field0101-head.txt`).

`[OBSERVED]` Range proof over all 97 `.trn` (397 312 tile records,
`evidence/hex/world-trn-inventory.txt`):

| field | observed range | predicted limit | consistent? |
|---|---|---|---|
| `cHeight` | −127 … +127 (253 distinct values) | int8 → −128…127 | ✔ (world −12.7 … +12.7) |
| `byTileIndex` | 0 … 221 | must be ≤ 235 so that +10 ≤ 245 (list ends at 245) | ✔ |
| `byTileCoord` | 0 … 7 | `TileCoordList[8]` | ✔ exactly 8 |
| `byBackTileIndex` | 0 … 136 | must be ≤ 139 so that +256 ≤ 395 (list ends at 395) | ✔ |
| `byBackTileCoord` | 0 … 31 | `BackTileCoordList[32]` | ✔ exactly 32 |
| `dwColor` | non-zero in 310 145 / 397 312 tiles | — | ✔ |

The two `uint8` ranges that stop *exactly* at `sizeof(table)-1` are the strongest structural
evidence: `byTileCoord` never exceeds 7 and `byBackTileCoord` never exceeds 31.

### 2.4 UV orientation tables

`TileCoordList[8][4][2]` (TMGround.cpp:13-23) — eight full-tile UV orientations, 4 corners each,
in the vertex order used by the renderer (v0 = (x,y), v1 = (x, y+Δy), v2 = (x+Δx, y),
v3 = (x+Δx, y+Δy)):

```
[0] (0,0) (0,1) (1,0) (1,1)      <- identity
[1] (1,0) (0,0) (1,1) (0,1)      <- u flipped
[2] (1,1) (1,0) (0,1) (0,0)      <- 180°
[3] (0,1) (1,1) (0,0) (1,0)
[4] (0,0) (1,0) (0,1) (1,1)      <- diagonal flip (transpose)
[5] (0,1) (0,0) (1,1) (1,0)
[6] (1,1) (0,1) (1,0) (0,0)
[7] (1,0) (1,1) (0,0) (0,1)
```

`BackTileCoordList[32][4][2]` (TMGround.cpp:25-59) uses only the coordinate values
`0.0, 0.5, 1.0`, i.e. it addresses **one quadrant of a 2×2 atlas** per quad.
`[INFERRED]` 32 = 4 quadrants × 8 orientations, so each `MTile*.wys` holds 4 detail variants
arranged 2×2 and `byBackTileCoord` selects quadrant + mirror/rotation.

### 2.5 Post-load processing (`LoadTileMap`)

Order matters for a reimplementation; all steps `[OBSERVED]` at TMGround.cpp:2518-2675:

1. **Normals** — for interior tiles 1…62 × 1…62, `m_TileNormalVector[i] = GetNormalInGround(x,y)`
   (finite difference of the 3×3 neighbourhood, TMGround.cpp:3720+). Zone border normals are
   copied inwards (lines 2526-2536).
2. **Edge height extension** — row 0 copies row 1's heights; column 0 copies column 1
   (lines 2557-2561). Prevents seams between zones.
3. **Walk mask** — `m_pMaskData[128][128]` is a bilinear *upsample* of the 64×64 heights to
   1-unit resolution (lines 2563-2603):

```c
fCenter = (h[x,y] + h[x+1,y] + h[x,y+1] + h[x+1,y+1]) / 4;
mask[2y  ][2x  ] = (h[x,y]     + fCenter) / 2;   // char truncation
mask[2y  ][2x+1] = (h[x+1,y]   + fCenter) / 2;
mask[2y+1][2x  ] = (h[x,y+1]   + fCenter) / 2;
mask[2y+1][2x+1] = (h[x+1,y+1] + fCenter) / 2;
```

   Note it is assigned into a `char` (truncating, signed), and this is the *only* height used for
   ground snapping of objects, water, rain and snow.
4. **Zone-border flattening** — if a neighbour is absent (`!m_cUpEnable` etc.) the outer 15 mask
   rows/cols are forced to `127` (lines 2605-2675), which makes the zone edge a 12.7-unit-high
   wall so that the camera cannot see past the map edge. `[INFERRED]` purpose (code only sets
   the constant).

### 2.6 `SetAttatchEnable` / `Attach` (neighbour knowledge)

`[OBSERVED]` TMGround.cpp:3769-4456 hard-codes an exception table of zone coordinates
(`(1,1)`, `(16,15)`, `(16,16)`, `(17,16)`, `(18,16)`, … `(30,28)`) that sets the four
`m_cLeft/Right/Up/DownEnable` flags. This is *not* derivable from disk: it is baked into the code.
`Attach()` (TMGround.cpp:2385-2465) then copies the shared edge heights/normals between two
resident zones and links left/right/up/down pointers.

### 2.7 Loader pseudocode

```
load_trn(path):
    f = open(path)
    n = u8(f); name = f.read(n); px = u8(f); py = u8(f)
    assert f.tell() == 3 + n                          # 8 for the shipped data
    raw = f.read(4096 * 12); assert len(raw) == 49152
    for i in 0..4095:
        t = raw[i*12 : i*12+12]
        tile[i] = { h: i8(t[0]),
                    tex: LUT_ENV[10 + t[1]],
                    uv1: TILE_COORD[t[2] & 7],
                    dtex: LUT_ENV[256 + t[3]],
                    uv2: BACK_COORD[t[4] & 31],
                    color: u32le(t[8:12]) }
    offset = (px * 128.0, py * 128.0)
    build_normals(); extend_edges(); upsample_mask(); clamp_borders()
```

`[OBSERVED]` TMGround.cpp:2496-2516 additionally validates a per-zone checksum against
`m_nCheckSum[posY][posX]` read from `cdata.bin`; see section 13.3 — **that gate cannot be
satisfied by the shipped data** and should be treated as optional.

---

## 3. Terrain object list — `Env\FieldNNNN.dat`

> ⚠️ **Naming trap.** `FieldNNNN.dat` is *not* terrain tile data. It is the **static object
> placement list** for the zone, read by `TMObjectContainer::Load()`
> (TMScene.cpp:2213, TMObjectContainer.cpp:54). The name suggests "data for Field", which is why
> it is often mistaken for a second heightmap.

### 3.1 Record layout

`[OBSERVED]` TMObjectContainer.cpp:85-100 (note `sz1 = 28`, `sz2 = 8`):

```c
ObjectFileItem* item = (ObjectFileItem*)&buff[pos];
unsigned int dwObjType = item->dwObjType;
TMVector2    vecPosition = item->vecPosition;
float        fHeight     = item->fHeight;
float        fAngle      = item->fAngle;
int          nTextureSetIndex = item->nTextureSetIndex;
int          nMaskIndex  = item->nMaskIndex;
pos += sz1;                       // 28
```

The live struct is `ObjectFileItem` (Structures.h:416-426) which is 36 bytes as declared, but the
file stride is **28**. `[OBSERVED]` `TMObject::Save()` (TMObject.cpp:657-665) writes exactly the
six fields below — 4 + 8 + 4 + 4 + 4 + 4 = **28 bytes** — which is the authoritative layout.

| field | type | byte offset | size | meaning |
|---|---|---|---|---|
| `dwObjType` | `uint32` | 0 | 4 | object type id (see section 10) |
| `vecPosition.x` | `float` | 4 | 4 | local X inside the zone, 0…128 |
| `vecPosition.y` | `float` | 8 | 4 | local Z (map Y) inside the zone, 0…128 |
| `fHeight` | `float` | 12 | 4 | world Y of the object origin |
| `fAngle` | `float` | 16 | 4 | yaw, radians |
| `nTextureSetIndex` | `int32` | 20 | 4 | texture set / **grid width ÷2** for water |
| `nMaskIndex` | `int32` | 24 | 4 | `object.bin` mask index / **grid height ÷2** for water |

### 3.2 The trailing 8 bytes (effect-only)

For a specific set of object types the record is followed by **8 more bytes** which are two
floats, `fScaleH` and `fScaleV` (`[OBSERVED]` TMObjectContainer.cpp:410, 562, 662, 698, 747, 752,
757 — every such branch starts with `pos += sz2;`).

`[OBSERVED]` In `Field0630.dat` those bytes decode as sensible scale factors and not
integers/pointers:

```
objType=501 pos=(17.4,16.5) h=2.625 ang=1.0  -> extra = 1.8000002, 1.8000002
objType=506 pos=(19.9,19.7) h=0    ang=1.0    -> extra = 1.0, 1.0
objType=531 pos=(50.8,31.1) h=-2.475 ang=1.0  -> extra = 1.0, 9.299999
objType=504 pos=(113.0,34.5) h=1.1 ang=1.0    -> extra = 2.0000002, 2.0000002
```

The type set that carries the 8 extra bytes, from the branches in `Load()`, is:

```
501..506  ∪  511..599
```

(`>=501 && <506` + `==506`; `>=511 && <519`; `>=520 && <=530`; `==531`; `==532`;
`>532 && <600`.)

### 3.3 Proof of the 28 + 8(+8) rule

`[OBSERVED]` Parsing **all 98 files** (`Env\*.dat`, i.e. 97 field files + `Character.dat`) with
this rule consumes every file **exactly to EOF** — see `evidence/hex/world-dat-parse-proof.txt`
(`files parsed exactly: 98 / 98`). Examples:

```
Field0813.dat  len= 23600  records= 830  extra= 45   ->  830*28 + 45*8  = 23600
Field0630.dat  len= 83876  records=2899  extra=338   -> 2899*28 + 338*8 = 83876
Field1912.dat  len=  2772  records=  99  extra=  0   ->   99*28        =  2772
Character.dat  len= 27276  records= 969  extra= 18   ->  969*28 + 18*8  = 27276
```

The size variance (2772 … 83876 bytes) is therefore just "how many objects a zone has", not a
variable tile grid. `[INFERRED]` (direct arithmetic consequence).

### 3.4 Worked hex example

`Field0101.dat` first 192 bytes (`evidence/hex/world-dat-Field0101-head.txt`):

```
000000  8F 00 00 00 00 00 20 40 00 00 38 41 00 00 00 00
000010  00 00 00 00 00 00 00 00 00 00 00 00 8F 00 00 00
        |<-- rec0: type=143 -->|
```

Decoded:

```
rec[0] type=143  pos=(2.5, 11.5)  height=0  angle=0  texSet=0  mask=0
```

`[OBSERVED]` and `[INFERRED]` (coordinates ≤ 128, consistent with a 128-unit zone).

### 3.5 Loader pseudocode

```
load_dat(path, zone):                      # zone = (px, py)
    offx = zone.x * 128.0; offy = zone.y * 128.0
    pos = 0; checksum = 0
    while pos < len(file):
        o = rec(pos)
        checksum += o.type + int(o.x) + int(o.y) + int(o.h) + o.mask
        pos += 28
        if 501 <= o.type <= 506 or 511 <= o.type <= 599:
            o.scaleH = f32(pos); o.scaleV = f32(pos+4); pos += 8
        spawn_object(o, origin = (offx + o.x, o.h, offy + o.y))
```

`[OBSERVED]` Note the spawner adds the zone offset **at spawn time**
(`m_fOffsetX + vecPosition.x`, TMObjectContainer.cpp:133) and the *mask* registration uses the
local coordinate (`RegisterMask(pGround, vecPosition.x, vecPosition.y)` line 950).

---

## 4. Attribute map — `Env\AttributeMap.dat`

### 4.1 The 4-byte discrepancy, explained

| file | size | content |
|---|---|---|
| `Env\AttributeMap.dat` | **1048580** | 1 048 576-byte grid + **4-byte LE checksum** |
| `UI\AttributeMap.dat` | **1048576** | grid only, no checksum |
| `mesh\AttributeMap.dat` | 1048580 | same size as `Env` but **different bytes** (stale copy) |
| `UI\AttributeMap.dat` content | | differs from `Env` in 11 349 of 1 048 576 bytes |

`[OBSERVED]` Basedef.cpp:108-110 reads exactly this:

```c
fread(g_pAttribute, 1024, 1024, fp);   // 1024 * 1024 = 1048576
int tsum = 0;
fread(&tsum, 4, 1, fp);                // the trailing u32
```

`[OBSERVED]` Verified numerically: the trailer of `Env\AttributeMap.dat` is
`0x02768D74` (41 323 892) and `BASE_GetSum()` re-computes **exactly that value** when the grid is
treated as **signed `char`** (MSVC default). Using unsigned bytes does **not** match
(41 506 871). The winning variant is the literal decompiled form of `BASE_GetSum`
(Basedef.cpp:249-272), including the malformed `else` that binds to `if (mod == 5)`:

```c
int sum = 0;
for (i...) { int mod = i % 7;
    if (!mod)   sum += p[i] / 2;
    if (mod==1) sum += p[i] ^ 0xFF;
    if (mod==2) sum += 3 * p[i];
    if (mod==3) sum += 2 * p[i];
    if (mod==4) sum -= p[i] / 7;
    if (mod==5) sum -= p[i]; else sum += p[i] / 3;   // note the dangling else
}
```

(`p` is `char*` → **signed**.) `[OBSERVED]` so the "4-byte discrepancy" is a checksum, and the
reason `UI\` lacks it is that the UI copy is a map-editor working copy, not the runtime file.
`[INFERRED]` `[OBSERVED]` no code path opens `UI\AttributeMap.dat` or `mesh\AttributeMap.dat`.

### 4.2 Layout and semantics

| property | value | evidence |
|---|---|---|
| dimensions | 1024 × 1024 `int8` | `[OBSERVED]` TMGlobal.h:149 `char g_pAttribute[1024][1024]` |
| cell size | **4 × 4 world units** | `[OBSERVED]` Basedef.cpp:130 `g_pAttribute[(y>>2)&0x3FF][(x>>2)&0x3FF]` |
| world coverage | 4096 × 4096 units, wrapping with `& 0x3FF` | `[OBSERVED]` same |
| indexing | `[y][x]`, y = world Z | `[OBSERVED]` TMObject.cpp:377 `g_pAttribute[nY4][nX4]` |

Observed flag bits (`[OBSERVED]`, function names give the meaning):

| bit | mask | consumer | meaning |
|---|---|---|---|
| 0 | `0x01` | TMObject::IsInTown (TMObject.cpp:377) | "town / safe zone" |
| 1 | `0x02` | BASE_ApplyAttribute (Basedef.cpp:130) | **blocked** — ground height forced to 127 (unwalkable) |
| 3 | `0x08` | TMObject::IsInHouse (TMObject.cpp:448), TMRain.cpp:115, TMSnow.cpp:126 | "indoor/covered" — no rain/snow particles, no object spawn |
| 6 | `0x40` | TMObject::IsInPKZone (TMObject.cpp:407) | PK zone |

Beyond that, `Basedef.cpp:1684` uses `g_pAttribute[..] < 0` (signed negative = another class), and
`TMFieldScene` line-of-sight code compares raw bytes — those semantics are gameplay, not rendering.
`[INFERRED]`

### 4.3 Effect on the rendered terrain

`[OBSERVED]` TMScene.cpp:2223-2236, immediately after loading a zone:

```c
memcpy(m_HeightMapData[y], m_pGround->m_pMaskData[y], 128);   // 128x128
BASE_ApplyAttribute((char*)m_HeightMapData, 256);             // forces blocked cells to 127
memcpy(m_GateMapData, m_HeightMapData, sizeof(m_GateMapData)); // 256x256
```

Note the size argument mismatch (128×128 mask vs `size=256`) `[OBSERVED]` — a genuine bug in the
decompiled/ported code; the loop at Basedef.cpp:126-133 walks `size`×`size` = 256×256 over a
128-row buffer. Flagged, not resolved.

---

## 5. Terrain renderer

### 5.1 Per-frame, immediate mode (no VB/IB, no LOD meshes)

`[OBSERVED]` `TMGround::Render()` (TMGround.cpp:2685-3269) builds **one 4-vertex triangle strip
per tile every frame** and calls `DrawPrimitiveUP` (line 3248). There is no index buffer, no
static vertex buffer, no chunking: the terrain is fully CPU-emitted per frame.

* FVF `594` = `0x252` = `XYZ | NORMAL | DIFFUSE | TEX2`, stride **44** = `RDLNVERTEX2`
  (Structures.h:257-266).
* Voodoo path uses FVF `322` (`0x142` = `XYZ | DIFFUSE | TEX1`), stride 24, `RDLVERTEX`.
* Render state at entry: `CULLMODE = 3` (none), `ZENABLE=1`, `ZFUNC=4`, `ALPHABLENDENABLE=1`,
  `SRCBLEND=2` (ONE), `DESTBLEND=6` (INVSRCALPHA), `SHADEMODE=2` (flat), `COLORVERTEX=1`,
  `LIGHTING=1` — set by `RenderDevice::SetRenderStateBlock(1)` (RenderDevice.cpp:1700-1758).

### 5.2 Vertex construction

`[OBSERVED]` TMGround.cpp:3125-3146 (interior case; the `x==63` / `y==63` edge cases at
3148-3193 duplicate the last row/column so the quad degenerates instead of reading out of bounds):

```
v0 = ( nX,      h[nX,nY],          nY      )
v1 = ( nX,      h[nX,nY+tickY],    nY+tickY)
v2 = ( nX+tickX,h[nX+tickX,nY],    nY      )
v3 = ( nX+tickX,h[nX+tickX,nY+tickY], nY+tickY)
uv1[k] = TileCoordList[byTileCoord][k]
uv2[k] = BackTileCoordList[byBackTileCoord][k]
diffuse[k] = tile.color of that corner
normal[k]  = m_TileNormalVector[..]
```

World transform (TMGround.cpp:2711-2716):

```c
matScale = Scale(2.0, 0.1, 2.0) * deviceWorld * Translate(offset.x, 0, offset.y)
```

i.e. `world_pos = (tileX*2 + offsetX, cHeight*0.1, tileY*2 + offsetZ)`. The identity
`TileCoordList[0]` maps `u = tileX_frac`, `v = tileY_frac`, so **UV space is aligned to the map
grid axis**, and `byTileCoord` rotates/mirrors that mapping.

### 5.3 Culling and LOD ("nTick")

`[OBSERVED]` TMGround.cpp:2822-2875:

* Camera tile = `nCamPosX = (cam.x - offset.x)/2`, same for Z.
* If `|nX - camX| > 16` (or `> 17` when `camX` is odd): every **second** column is skipped and the
  remaining quads get `nTickX = 2`. Same rule for Y.
* Consequence: the quad pitch jumps from **2.0 to 4.0 world units** beyond ~32-34 units from the
  camera. `[INFERRED]` that this is the entire LOD system — there are no separate meshes.

Draw range `[OBSERVED]` (TMGround.cpp:2752-2820):

```c
nClipIndex = 15;                              // tiles; 18 in ESCENE_DEMO
if (fabs(verticalAngle) > 1.0) nClipIndex = sightLength*1.5 + 8;
nMinX = camTileX - nClipIndex/3; nMaxX = camTileX + nClipIndex;
```

plus a camera-forward-biased rect when the scene is a dungeon/demo/select-server scene
(`nLen = (int)(sightLength + 1)`, clamped to ≥ 6, `+2`, then `nXList/nYList` from
`(cos,sin)` of `m_fHorizonAngle`).

Per-quad screen-space rejection `[OBSERVED]` (TMGround.cpp:3220-3248): the 4 vertices are
projected by hand (`view * proj`) and the quad is drawn only if some vertex has `0 <= z < 1` and
its screen position is inside the viewport expanded by 50 pixels.

### 5.4 Lighting and fog

* `[OBSERVED]` Material: `Diffuse = Specular = (1,1,1,1)`, `Power = 0`, `Emissive = 0.3` gray
  (0.2 on Voodoo) — TMGround.cpp:2538-2555.
* `[OBSERVED]` Vertex colours are multiplied by hardware lighting (`COLORVERTEX=1`,
  `DIFFUSEMATERIALSOURCE=1`), so the final colour is
  `texture0 * texture1 * vertexDiffuse * (materialEmissive + globalAmbient + Σ_i DirLight_i · materialDiffuse)`.
* `[OBSERVED]` **Two** directional lights are configured and both are enabled
  (`RenderDevice::SetLight()` loops `i < 2` and calls `LightEnable(i, 1)`,
  RenderDevice.cpp:453-461):

  ```c
  m_light[0].Type = D3DLIGHT_DIRECTIONAL;
  m_light[0].Direction = normalize(-10, 10, -6);   // key light, from above/right/front
  m_light[1].Type = D3DLIGHT_DIRECTIONAL;
  m_light[1].Direction = normalize( 10,-14,  6);   // fill light, from below/left/back
  // ambient of both = 0; only Diffuse/Specular are set, from
  // m_light[0] = m_colorLight, m_light[1] = m_colorBackLight   (RenderDevice.cpp:1634-1637)
  ```

  **Correction — the constructor values above are not what reaches the device.**
  `SetRenderStateBlock(1)` overwrites both directions *every frame* before it
  uploads them (`RenderDevice.cpp:1657-1658`):

  ```c
  m_light[0].Direction = D3DXVECTOR3( 1.0f, -1.0f, -1.0f);   // NOT normalised
  m_light[1].Direction = D3DXVECTOR3( 0.0f,  0.0f,  1.0f);
  ```

  So the live fixed-function key light is `(1,-1,-1)` (length √3, i.e. 0.577×
  over-bright if D3D does not renormalise), and the fill light points straight
  down +Z. The `normalize(-10,10,-6)` / `normalize(10,-14,6)` pair only exists
  between the `RenderDevice` constructor and the first frame. `[OBSERVED]`
  `RenderDevice.cpp:99-110` vs `:1657-1658`.

  The *vertex-shader* light is derived from `m_light[0].Direction` separately and
  is **not** normalised either (`RenderDevice.cpp:1669-1689`):

  ```c
  // m_bDungeon != 0 && != 4:
  c1 = (camDir.x - 1.0, 0.5, camDir.z + 1.0 - 0.3, 0);
  // otherwise:
  c1 = (-1.0, 0.5, -1.0, 0);      // = -Direction.x, 0.5, Direction.z
  ```

  And `CMesh::Render` then **overwrites `c1` per skinned mesh** with
  `normalize(-1, 1, 1)` (`CMesh.cpp:633-635`). So: fixed-function geometry gets
  `(1,-1,-1)` / `(0,0,1)`; skinned geometry gets `normalize(-1,1,1)`. Three
  different light vectors coexist in one frame — a reimplementation must keep
  them separate or characters will be lit differently from the world.
  `[OBSERVED]`

  `m_colorLight` / `m_colorBackLight` are rewritten every frame by `TMSky::FrameMove` (section 7).
  Because both lights are *added*, the diffuse term is roughly
  `m_colorLight·(N·L0) + m_colorBackLight·(N·L1)` and can exceed 1.0 (e.g. 0.70 + 0.40 = 1.10 in
  daylight) — that over-bright is intentional and part of the look.
* `[OBSERVED]` Fog is **linear** (`D3DRS_FOGVERTEXMODE = 3`), colour = `m_dwClearColor`
  (RenderDevice.cpp:1730-1733), and the effective range is
  `start = m_fFogStart + sightLength - 8`, `end = m_fFogEnd + sightLength - 15`.
* `[OBSERVED]` Far clip `g_ClipFar` is 540 by default (TMGlobal.cpp:24), 70 for field /
  character-select scenes (TMScene.cpp:34, TMSelectCharScene.cpp:23, TMSky.cpp:254), and 90 inside
  the swamp rectangle (TMSky.cpp:412).

### 5.5 Special tile behaviours (hard-coded texture ids)

`[OBSERVED]` The renderer inspects `nIndex = byTileIndex + 10` (TMGround.cpp:2973-3122):

| `nIndex` | behaviour |
|---|---|
| `170, 171` | lava (`Env\Tile14140.wys`, `Tile14141.wys`): `tu1 += (serverTime % 10000)/10000` (UV scroll, 10 s period) + particle emitters (see below) |
| `38, 39` | animated water: stage-1 `COLOROP = 5` (MODULATE2X), stage-1 texture = **env** `GetEnvTexture(344)` = `Env\MTile04040.wys`, `tv2 = fY[k] + (serverTime%10000)/10000`; base texture becomes `nIndex + 92` |
| `62 … 65` | stage-1 `COLOROP = 5`, stage-1 texture = **env** `GetEnvTexture(nIndex + 286)` = env 348…351 = `Env\MTile04050..MTile04053.wys`, `uv2 = uv1`; base texture becomes `nIndex % 2 + 130` |
| anything else | stage-1 `COLOROP = 1` (DISABLE) — detail layer off |

Lava particles (`[OBSERVED]`, TMGround.cpp:3001-3084): when `rand()%200 < 2` spawn a small
`TMEffectBillBoard(0=smog)` and, on a 2 s budget with `(x even, y%3==0, rand%100<1)`, glow
billboards `56` (flare) / `60` (line) plus occasional `TMSkillFire`.

### 5.6 The "earthquake / world effect" vertex displacement

`[OBSERVED]` TMGround.cpp:3195-3209 — if `m_dwEffStart && serverTime < m_dwEffStart+2000`, every
terrain vertex gets

```c
Height = (4 * m_fEffHeight) * (serverTime - m_dwEffStart - 2000) / 2000;   // m_fEffHeight default 2.0
y += cos((effset.x - x)*(effset.y - z)/10 + serverTime/300) * Height;
```

This is a 2-second, one-shot wave used by world events. `[INFERRED]` gameplay-triggered, cost is
negligible to replicate.

### 5.7 Renderer state restore

`[OBSERVED]` TMGround.cpp:3259-3266: `COLORVERTEX=0`, `ALPHABLENDENABLE=1`, `ALPHATESTENABLE=1`,
stage1/2 `COLOROP=1`, `FILLMODE=3`.

---

## 6. Terrain textures — naming, lookup, container format

### 6.1 The list file is the only mapping

`Env\EnvTextureList3.bin` — `[OBSERVED]` size **135 168** = `512 × 264`, matching
`stTextureListInfo m_stEnvTextureList[512]` (TextureManager.h:111) with

`struct stTextureListInfo { char szFileName[255]; char cAlpha; u32 dwLastUsedTime; u32 dwShowTime; }`
→ 255 + 1 + 4 + 4 = **264** (TextureManager.h:39-45).

`[OBSERVED]` Content (`evidence/hex/world-envtexturelist3-map.txt`):

| index range | content |
|---|---|
| 0 … 9 | empty (`cAlpha = 'N'`, `0xCDCDCDCD` fill) |
| **10 … 245** | `Env\TileNNNNN.wys` — 236 entries, the **base** tile textures |
| 246 … 255 | empty |
| **256 … 395** | `Env\MTileNNNNN.wys` — **140** entries, the **detail/2×2-atlas** textures |
| 396 … 511 | empty |

Lookup is a plain array index with a **hard-coded bias** `[OBSERVED]`:

```
base   texture = env_list[ byTileIndex     + 10  ]      // TMGround.cpp:2879, 2973
detail texture = env_list[ byBackTileIndex + 256 ]      // TMGround.cpp:3214
```

### 6.2 Name encoding (`TileNNNNN`)

`[OBSERVED]` from the list: `Tile00000, Tile00001, Tile00002, Tile00003, Tile00010, …`.
`[INFERRED]` the 5 digits are `G | BB | V` where `V` (0-3) is the variant and `G`,`BB` the tile
group — the 4 variants per group map onto `TileCoordList[0..3]` (the 4 rotations). `MTileNNNNN`
uses the same scheme.

### 6.3 ⚠️ Case and completeness traps (measured)

`[OBSERVED]` (full measurement in this repo's run log; reproducible from
`evidence/hex/world-asset-inventory.txt` + the list dump):

1. **Case inconsistency is real but harmless on Windows-only.** The list spells every path with a
   capital `T` (`Env\Tile….wys`, `Env\MTile….wys`). On disk **12 files** are lower-case:
   `tile00014.wys`, `tile00024.wys`, `tile00034.wys`, `tile00064.wys`, `tile00074.wys`,
   `tile01044.wys`, `tile01054.wys`, `tile01064.wys`, `tile01084.wys`, `tile01104.wys`,
   `tile04074.wys`, `tile08094.wys`.
   **Those 12 files are not referenced by `EnvTextureList3.bin` at all** (they are the `…4`
   variant, unused). So on a case-sensitive filesystem (Linux/Android) the correct strategy is:
   *resolve the name from the list*, and if you insist on synthesising names, do a
   case-insensitive directory index.
2. **The list is a superset.** 52 of the 236 `Tile` names and 31 of the 140 `MTile` names have no
   file on disk. Conversely **all 146 distinct `byTileIndex` values and all 90 distinct
   `byBackTileIndex` values actually used by the 97 `.trn` files do resolve to an existing file**
   (`[OBSERVED]`, measured). Practical rule: *missing file → skip the tile's stage, do not crash
   and do not substitute another texture* (the original silently gets a null texture).
3. **There is no fallback search order.** `GetEnvTexture → LoadEnvTexture` does exactly
   `_open(m_stEnvTextureList[nIndex].szFileName)` (TextureManager.cpp:1015) and returns 0 on
   failure; `GetEnvTexture` then returns `nullptr`. `[OBSERVED]` No `Tile`→`tile` retry, no
   extension switching.
4. **`GetEnvTexture` bounds check is off-by-one** `[OBSERVED]` TextureManager.cpp:1157
   `if (nIndex < 0 || nIndex > MAX_ENV_TEXTURE) return nullptr;` with `MAX_ENV_TEXTURE = 512` →
   index 512 is accepted and reads one past the array.

### 6.4 `.wys` container format (fully decoded)

`[OBSERVED]` Every `.wys` is a **DDS file with two fields obfuscated**. Decoding rule, taken
verbatim from `LoadEnvTexture` (TextureManager.cpp:1050-1066) and confirmed by hand on 10 files:

```
1. skip 1 byte (0x57 'W')
2. read the rest into a buffer; overwrite buffer[0..2] with "DDS"
   -> now buffer looks like: "DDS " | dwSize(124) | dwFlags | dwHeight | dwWidth | ...
3. buffer[84] is ddspf.dwFourCC's first byte:
       '2'  -> the real format is DXT1   (on-disk fourCC is the literal string "2z73")
       else -> the real format is DXT3   (on-disk fourCC is the literal string "7x0s")
   then memcpy(&buffer[84], "DXT1"/"DXT3", 4)   // normalise for D3DX
```

On-disk bytes of the first 5:

```
57 53 31 30 20   = 'W' + "S10 "   (the "S10 " replaces the DDS magic "DDS ")
```

Two real headers (`Env\Tile00000.wys` = DXT1, `mesh\water3.wys` = DXT3) differ **only** in the
fourCC bytes at file offset 85-88 — proof that the rest of the header is a stock `DDS_HEADER`:

```
Tile00000:  ... 00 | 20 00 00 00 | 04 00 00 00 | 32 7A 37 33 | 00 00 00 00 | ... 08 10 40 00
                    ddspf.dwSize  ddspf.dwFlags  "2z73"        dwRGBBitCount   dwCaps=0x401008
water3  :   ... 00 | 20 00 00 00 | 04 00 00 00 | 37 78 30 73 | 00 00 00 00 | ... 08 10 40 00
                                                  "7x0s"
```

`[OBSERVED]` Worked size proof (pixel-data size == exact mip chain of the declared format, with
`dwMipMapCount` read from header offset 29):

| file | fourCC | format | w×h | mips | pixel bytes | mip-chain sum |
|---|---|---|---|---|---|---|
| `Env\Tile00000.wys` | `2z73` | DXT1 | 128×128 | 4 | 10 880 | 8192+2048+512+128 = **10 880** |
| `Env\MTile00010.wys` | `2z73` | DXT1 | 128×128 | 4 | 10 880 | same |
| `mesh\water1.wys` | `2z73` | DXT1 | 64×64 | 4 | 2 720 | 2048+512+128+32 = **2 720** |
| `mesh\water3.wys` | `7x0s` | DXT3 | 128×128 | 4 | 21 760 | 16384+4096+1024+256 = **21 760** |
| `mesh\sky01.wys` | `2z73` | DXT1 | **1024×256** | 0 | 131 072 | 131072 = **131 072** |
| `Effect\moon.wys` | `7x0s` | DXT3 | 256×256 | 4 | 87 040 | 65536+16384+4096+1024 = **87 040** |
| `mesh\aef001.wys` | `2z73` | DXT1 | 128×256 | 4 | 21 760 | 16384+4096+1024+256 = **21 760** |

So terrain tiles are **128×128 DXT1 with 4 mip levels (128→16)** `[OBSERVED]`, and the sky dome
textures are **1024×256 DXT1, no mipmaps** `[OBSERVED]`. There is no third format: the fourCC is
always one of the two scrambled strings.

Alpha handling is driven by the list entry's `cAlpha` byte, not by the DDS header
`[OBSERVED]` (TextureManager.cpp:1035-1099): `'N'` = opaque (`X8R8G8B8`, color-key 0xFF000000),
`'A'/'C'` = `A8R8G8B8`, `'a'` = `A1R5G5B5`. **Every `Env\Tile*`/`MTile*` entry is `'N'`**
`[OBSERVED]`, i.e. terrain is fully opaque.

If the name does **not** end in `.wys` (e.g. a hypothetical `.wyt`), the loader instead strips
4 bytes and appends an 18-byte TGA footer (TextureManager.cpp:1027-1034,
`m_szTGAHeader = "TRUEVISION-XFILE"`). `[OBSERVED]` no `.wyt` file exists in this client
(0 files in `mesh\`), so that path is dead for 748.

---

## 7. Sky

> ⚠️ **Asset-location trap.** The sky is **not** in `Env\`. All sky/water/night art lives in
> `mesh\`, and the sky is a **mesh**, not a billboard.

### 7.1 What the sky actually is

`[OBSERVED]` `TMSky` (TMSky.cpp:41-224) holds only `m_dwObjType = 1` and calls

```c
TMMesh* pMesh = g_pMeshManager->GetCommonMesh(1, /*bEffect=*/1, 3_min);   // TMSky.cpp:190
```

`[OBSERVED]` `GetCommonMesh(nIndex)` indexes `Mesh\MeshList.txt` by *line index* (MeshManager.cpp:62,
`fscanf(fp, "%d %s", &nIndex, szMeshName)` → `m_stCommonMesh[nIndex]`), and line 1 of that file is

```
1 mesh\\sky001.msa
```

So the sky dome = **`mesh\sky001.msa`** (4323 bytes). `[OBSERVED]` header, dword by dword:

```
0x00 42 01 00 00   FVF        = 322  (XYZ | DIFFUSE | TEX1)
0x04 18 00 00 00   sizeVertex = 24
0x08 01 00 00 00   attCount   = 1
0x0C ..            attRange[0] (5 dwords = 20 bytes):
       0   AttribId=0 | FaceStart=0 | FaceCount=180 | VertexStart=0 | VertexCount=133
0x20 A4 29 5C 73 6B 79 30 34 00 CD CD   11-byte texture field: junk(2) + "\sky04"
```

i.e. one attribute range, **180 triangles / 133 vertices**, and the mesh's own texture name
`sky04` (the loader takes everything after the last `\`, strips the extension, prefixes `mesh\`,
then looks it up in `MeshTextureList.bin` → `mesh\sky04.wys`). `[OBSERVED]`
`sizeof(D3DXATTRIBUTERANGE) == 20`, which is why the texture field starts at 0x20.

The `bEffect = 1` flag switches texture resolution to the **effect** namespace
(`TMMesh::SetTexture`, TMMesh.cpp:204-213), which is why the sky's runtime texture indices are
*effect* indices (67…70) and not model indices.

### 7.2 Sky textures and the weather state machine

`[OBSERVED]` `Effect\EffectTextureList.bin` entries 67-70 (see
`evidence/hex/world-effecttexturelist-map.txt`):

```
67 = mesh\sky01.wys   68 = mesh\sky02.wys   69 = mesh\sky03.wys   70 = mesh\sky04.wys
```

(131 201 bytes each; the largest textures in the game.)

`[OBSERVED]` `TMSky::SetWeatherState(nState)` (TMSky.cpp:594-617):

```c
if (!(nState/10))        pMesh->m_nTextureIndex[0] = nState + 67;        // instant
else if (nState/10==1) { pMesh->m_nTextureIndex[0] = nState%10 + 67;     // crossfade target
                         m_nTextureIndex          = nState%10 + 67; }
m_nState = nState; m_dwStartTime = now; m_dwChangeTime = 2000;
```

So **states 0-3 = instant**, **states 10-13 = 2000 ms crossfade to the same 4 textures**
(`%10`). Per state `[OBSERVED]` (TMGround/TMSky constants):

| state%10 | sky texture | `m_dwR/G/B` → clear colour | `m_LightVal` (r,g,b,a) |
|---|---|---|---|
| 0 | `mesh\sky01.wys` | 28, 120, 189 (day blue) | 0.70, 0.70, 0.70, 0.1 |
| 1 | `mesh\sky02.wys` | 51, 54, 42 (dusk/rain) | 0.30, 0.30, 0.30, 1.0 |
| 2 | `mesh\sky03.wys` | 98, 47, 4 (orange) | 0.50, 0.40, 0.20, 1.0 |
| 3 | `mesh\sky04.wys` | 19, 46, 51 (night teal) | 0.26, 0.34, 0.34, 1.0 |

`[OBSERVED]` The **day/night cycle is a texture + light + clear-colour swap, not a rotation**
(TMFieldScene.cpp:7628-7722):

```c
int nTime = (serverTime / 600000) % 12;      // 600 000 ms = 10 min per step; 12 steps = 2 h/cycle
if (m_nWTime) nTime = m_nWTime;              // GM/debug override -- the ONLY way to reach 12,13,14
nTime 0..7  -> SetWeatherState(10)   // sky01, day      (+ rain off / snow off)
nTime 8..9  -> SetWeatherState(12)   // sky03
nTime 10..11-> SetWeatherState(13)   // sky04
nTime 12    -> SetWeatherState(11)   // sky02 + rain visible
nTime 13    -> SetWeatherState(11) + snow   (g_nWeather overwritten to 13)
nTime 14    -> SetWeatherState(11) + snow + snow2 (g_nWeather = 14)
```

`[OBSERVED]` Because of the `% 12` the clock-driven cycle only produces `nTime ∈ 0…11`; the
rain/snow branches (`12`, `13`, `14`) are reachable **only** through the `m_nWTime` override. Weather
rain/snow in normal play therefore comes from `g_nWeather` (server), not from the clock.

Plus: inside the "fog swamp" rectangle (zone x 27…30, y 21…24) daytime is forced to state 11 with
snow (TMFieldScene.cpp:7645-7651).

`[OBSERVED]` Server weather (`g_nWeather`, set by `TMFieldScene::SetWeather`,
TMFieldScene.cpp:15381-15462): `0` = clear (auto cycle), `1` = rain (`state 11`), `2` = snow
(`state 11`), `3` = heavy snow (`state 11` + `m_pSnow2`), plus dungeon overrides.

### 7.3 Geometry, transform, depth

`[OBSERVED]` TMSky.cpp:155-171:

```c
D3DXMatrixTranslation(&matPosition, cam.x, m_fHeight + 1.0f, cam.z);   // m_fHeight = -5.0
D3DXMatrixRotationYawPitchRoll(&mat, m_fAngle /*180°*/, -90°, 0);
D3DXMatrixScaling(&matScale, m_fScale /*0.5*/, m_fScale*0.5, m_fScale);
mat = world * rot * scale * translate;
```

* **Camera-locked**: the translation is the camera position every frame → infinite-distance dome.
* **No time rotation**: `m_fAngle` is a constant 180° (90° in the DEMO scene); nothing animates it.
* `m_fScale` is 0.5 normally, 0.89 inside the swamp rectangle (TMSky.cpp:411, 424).
* `FOGENABLE = 0` while drawing (TMSky.cpp:170), `LIGHTING = 0`, material emissive 0.3 /
  diffuse 0.69 gray (TMSky.cpp:137-153). Depth test stays on but the dome is emitted before the
  terrain, and `DESTBLEND = 6` (`ONE_MINUS_SRC_ALPHA`).
* `[OBSERVED]` Two texture stages: stage 0 `COLOROP = 8` (**ADDSIGNED**),
  stage 1 `TEXCOORDINDEX = 0` (same UV as stage 0) and `COLOROP = 0xC` (**BLENDDIFFUSEALPHA**) when
  `m_nState/10` (i.e. during a crossfade) else `1` (DISABLE); stage-1 texture =
  `GetEffectTexture(m_nTextureIndex)`. `[INFERRED]` the second layer is the "clouds/aurora"
  overlay, cross-faded by the *vertex alpha* that `FrameMove` writes into the dome's vertex buffer
  (`dwAlpha = 0x888888 | ((progress*255)<<24)`, TMSky.cpp:456, 546-547).

### 7.4 Night sky: stars, moon, milky way

`[OBSERVED]` `TMSky` constructor (TMSky.cpp:60-119):

* 20 star billboards, `m_nTextureIndex = 56` (`Effect\flare.wys`), scale 0.1, positioned on a
  sphere shell of radius ~6 by
  `y = 0.5 - sqrt(x²+z²)/50*20`, colours cycled over `0xFFFFFFFF, 0xFF8888AA, 0xFFFF9955,
  0xFFFFFFAA, 0xFFAAFFAA`, base alpha 0.03, half with `m_nFade = 0`, half `2`.
* 2 moon billboards: texture **214** = `Effect\moon.wys` (scale 5, `0xFF..` white) and texture
  **2** = `Effect\bright.wys` (scale 12, colour `0x00554433` = a halo), position
  `(-1, 0.5, 0.3) * 15` relative to the camera.
* Rendered only in states 3, 13, 10 and only if the sky texture is 67 (`TMSky.cpp:202-214`).
* `m_nTextureIndex == 68` (sky02) hides the sun; `pMesh->m_nTextureIndex[0] == 68` too
  (TMSky.cpp:237-239).
* `n_pMeshMilkyway` is declared (TMSky.h:23) but **never assigned** except `= 0` `[OBSERVED]`, so
  `Effect\milkyway.wys`/`.msa` are unused by this code path.

`[OBSERVED]` The `night1..7.msa` meshes exist and are reachable only as MeshList indices
2738, 2739, 2740, 2750, 2768, 2769, 2770 (`MeshList.txt` lines 1576-1606) and their textures as
model-texture indices 1150-1152, 1224, 1253-1255 (`MeshTextureList.bin`). No source file references
those names, and no `.dat` object type in the shipped fields maps to 2738-2770
`[OBSERVED]` (type histogram of all field `.dat` files never reaches 2000 except a handful:
2035, 2048…). `[UNKNOWN]` what consumes them — resolves by logging `GetCommonMesh` indices at
runtime, or by scanning `WYD.exe` for the numeric ids.

### 7.5 Sun / lens flare — `TMSun`

`[OBSERVED]` `TMSun` (TMSun.cpp) is **screen-space**, not a world object:

* 12 quads, effect textures `206, 212, 212, 211, 210, 211, 212, 213, 210, 209, 206, 208`
  (= `Effect\flare002, flare008, flare008, flare007, flare006, flare007, flare008, flare009,
  flare006, flare005, flare002, flare004.wys`).
* Projected with `D3DXVec3Project(camera + (-1, 0.7*m_fDefSize, 0.3))`, hidden if `z > 1`.
* FVF `324` = `XYZRHW | DIFFUSE | TEX1`, stride 28 (`RDTLVERTEX`), `TRIANGLEFAN`, 2 tris.
* Each successive quad is pushed *towards the camera* by `z = 0.8 - i*0.04` (fake flare depth).
* Hidden in dungeons, when the sky texture is 68/69/70, and while `m_nState` ∈ 1..9.

### 7.6 Fog tables

`[OBSERVED]` `TMSky::FogList[16][2]` (TMSky.cpp:21-39) — (start, end) in world units:

```
[0] 14,34   [1] 25,34   [2] 55,70  [3] 25,34
[4] 14,23   [5] 14,22   [6] 16,34  [7] 16,34
[8] 39,70   [9] 19,30   [10] 19,30 [11] 19,70
[12] 29,40  [13..15] 0,0 (unused)
```

Selected by dungeon id / scene type (TMSky.cpp:257-444). Field uses `[7]` normally and `[8]`
inside the swamp rectangle; `+5` is added to both ends afterwards (TMSky.cpp:443-444) and then
`RenderDevice` adds `sightLength - 8 / -15` (RenderDevice.cpp:1727-1728) — so the *effective* fog is
`(16+5+sight-8, 34+5+sight-15) = (sight+13, sight+24)`.

---

## 8. Water / sea

### 8.1 It is procedural geometry, driven by `.dat` records of type 2

`[OBSERVED]` `TMObjectContainer::Load` (TMObjectContainer.cpp:111-140): `dwObjType == 2` creates a
`TMSea`, and **reinterprets the two "texture" fields as the grid size**:

```c
sea->m_nGridNumX = m_nMaskIndex / 2;         // TMSea::InitObject, TMSea.cpp:179
sea->m_nGridNumY = m_nTextureSetIndex / 2;   // TMSea.cpp:180
```

`[OBSERVED]` Real example — `Env\Character.dat` record 0: `type=2 pos=(80.5,73.5) h=-2.3
texSet=113 mask=13` → grid = 6 × 56 cells of 2 units = 12 × 112 world units of water.

`[OBSERVED]` `TMSea::InitObject` (TMSea.cpp:177-257) builds a static VB/IB once:

* vertices `(x - gx/2)*2.0`, `y = 0`, `(y - gy/2)*2.0`
* UV0 `= (x/2, y/2)`, UV1 `= (x/12, y/12)`
* FVF `578` = `0x242` = `XYZ | DIFFUSE | TEX2`, stride 32 (`RDLVERTEX2`)
* index buffer `D3DFMT_INDEX16`, 6 indices per cell, standard quad split

`[OBSERVED]` The "sea level" is `m_fHeight` from the `.dat` record; the surface oscillates ±0.05
(`+ (sin(...)*0.05) - 0.05`), with a **12 s** and an **18 s** time base (TMSea.cpp:304-305):

```c
nIndex0 = serverTime % 12000;   nIndex1 = serverTime % 18000;
```

### 8.2 Animated UVs and waves (exact speeds)

`[OBSERVED]` TMSea.cpp:307-341, three variants:

**Normal water** (`bFog == false`, overland): stage-0 UVs stay at the `InitObject` values
`(x/2, y/2)`, stage-1 U stays `x/12`, and only stage-1 V scrolls:

```
uv2_v = y/3.0 + (t % 12000)/12000
uv2_u = x/12.0                                    // static
uv1   = (x/2.0, y/2.0)                            // static
y     = sin(x*PI/2 + (t%12000)/6000 * 2PI) * 0.05 - 0.1    // -0.05 offset in dungeon 2
```

**Normal water, dungeon variant** (dungeon id ∉ {0,3,4}): stage-0 UVs scroll and use a different
tiling:

```
uv1_u = x/1.8
uv1_v = y/1.2 + (t % 12000)/12000
```

**Fog/swamp water** (zone x∈{28,29} ∧ y∈{22,23}):

```
uv1 = (x/9.5, y/9.5 + (t%18000)/18000)
uv2 = (x/3.8 + (t%18000)/18000, y/3.8)
y   = sin(x*PI/4.5 + (t%12000)/6000 * 2PI) * 0.9 - 0.1
```

Note the wave is a **1-D sine along X only** (no Z term), and the scroll wraps every 12 s / 18 s.
`[OBSERVED]`

### 8.3 Textures and blending

`[OBSERVED]` TMSea.cpp:62-155, three material/blend configurations:

| case | stage0 texture | stage1 texture | blend (D3D9 `D3DBLEND` values) |
|---|---|---|---|
| normal (land) | model **2** = `mesh\water1.wys` | model **3** = `mesh\water2.wys` | `SRCBLEND=3` (**SRCCOLOR**), `DESTBLEND=7` (**DESTALPHA**), stage0 `MODULATE`(4), stage1 `MODULATE2X`(5) |
| dungeon (id ∉ {0,3,4}) | model **8** = `mesh\water3.wys` | model **9** = `mesh\water4.wys` | `SRCBLEND=5` (**SRCALPHA**), `DESTBLEND=6` (**INVSRCALPHA**), `ALPHAOP=2` (SELECTARG1), `CULLMODE=1` |
| swamp rectangle | mesh `m_bEffect=1`, effect **406** = `Effect\fog2.wys` | effect 406 | `SRCBLEND=4` (**INVSRCCOLOR**), `DESTBLEND=3` (**SRCCOLOR**), both stages `MODULATE`(4), `CULLMODE=3` |

(`D3DBLEND`: 1=ZERO, 2=ONE, 3=SRCCOLOR, 4=INVSRCCOLOR, 5=SRCALPHA, 6=INVSRCALPHA, 7=DESTALPHA.
The engine-wide default elsewhere is `SRCBLEND=2` / `DESTBLEND=6`, i.e. straight alpha.)

`[OBSERVED]` Also available and *not* used by `TMSea`: model 1 = `mesh\water5.wys`, 10 =
`mesh\water6.wys`, 242 = `mesh\sea002.wys`, 1163 = `mesh\owater5.wys` (see
`evidence/hex/world-meshtexturelist-map.txt`). `[UNKNOWN]` whether 755/760 code uses them; the
748 `TMSea` does not. `m_matWaterView` mentioned in the task brief **does not exist anywhere in
the source** `[OBSERVED]` (grep over all `*.cpp|*.h` returns nothing) — the reflective-water path
is not implemented in this decompilation.

### 8.4 Gameplay queries

`[OBSERVED]` TMSea.cpp:274-284 / TMGround.cpp:4458-4502:

```c
sea->GetHeight(x, z) = inRect ? sin(((x-left)*PI/2) + (t%12000)/6000*2PI)*0.1 - 0.1 : -100
sea->IsInWater(pos, h) = (sea->m_fHeight > h) ? 1 : 0
water height = sea->GetHeight(...) + sea->m_fHeight
```

`[OBSERVED]` `TMGround` holds at most **10** `TMSea` (`m_pSeaList[10]`, TMGround.h:74) and
`m_nSeaIndex` is incremented without a bounds check (TMObjectContainer.cpp:137) — an 11th sea
record in one `.dat` would write out of bounds. `[OBSERVED]` The maximum in the shipped data is
**exactly 10** (`Env\Field1013.dat`), i.e. the cap is reached in shipping content.

`[OBSERVED]` While the camera is inside water, `TMSky::FrameMove` overrides the fog to
`1.0 … 10.0` and the clear colour to `0xFF335599` / `0xFF333333` / `0xFF441100` / `0xFF222222`
per state (TMSky.cpp:501-514).

---

## 9. Environmental decoration

All of these are spawned from `.dat` object types by `TMObjectContainer::Load`; none of them has a
spawn file of its own. Densities below are *what the code does per placed marker*.

| class | `.dat` objTypes | asset source | form | density / period | evidence |
|---|---|---|---|---|---|
| `TMTree` | 331…342, 351…378 | skin-mesh type `(t-331)/2+63` (→ 63…68 = `tr01..tr06`) or `(t-351)/2+71` (→ 71…84 = `tr07..tr20`); spring mode → 87 (`tr21`) | **skinned 3-D mesh** | 1 per marker | TMTree.cpp:157-200 `[OBSERVED]` |
| `TMLeaf` | 311…322 | skin-mesh type **61** = `mesh\lf01` | skinned mesh, alpha-faded | 1 per marker | TMLeaf.cpp:10-18, 168-171 `[OBSERVED]` |
| `TMFloat` | 3, 5 | type 3 → skin-mesh **62** = `mesh\fl01`; type 5 → billboard texture **90** = `Effect\frog01.wys` | mesh or billboard | 1 per marker (+1 water billboard tex **10**) | TMFloat.cpp:30-72 `[OBSERVED]` |
| `TMButterFly` | 343, 4, 6, 7 | skin-mesh **69** = `mesh\bf01` (type 7 → **24** = `mesh\bd01`) | skinned mesh | **5 per marker** | TMObjectContainer.cpp:141-236 `[OBSERVED]` |
| `TMFish` | 344, 12 | skin-mesh **70** = `mesh\fs01` | skinned mesh | **5 per marker** | TMObjectContainer.cpp:237-276, TMFish.cpp:18 `[OBSERVED]` |
| `TMDust` | 8, 9, 13, 531 | `TMEffectDust` / `TMEffectParticle` | particle | every **10 000 ms** (`2 500` for type 2) | TMDust.cpp:20-26 `[OBSERVED]` |
| `TMSnow` | weather, not `.dat` | effect **2** = `Effect\bright.wys` | 200 camera-facing quads | 1 system per scene, 200 particles | TMSnow.cpp:66-83, 88 `[OBSERVED]` |
| `TMRain` | weather | effect **9** = `Effect\rain01.wys` | 50 quads + impact billboards tex 10 | 1 system, 50 particles | TMRain.cpp:19-31, 109 `[OBSERVED]` |
| `TMFireEffect` | via skills | caller texture + 1 | trail of billboards | 20 sprites per lifetime | TMFireEffect.cpp:44-60 `[OBSERVED]` |
| `TMShade` | 501…503, 506 | procedural grid, effect texture id passed in (7 = `lightmap`, 118 = `dlight`) | ground-projected mesh | 1–2 per marker | TMShade.cpp:6-30; TMObjectContainer.cpp:547, 600 `[OBSERVED]` |
| `TMShip` | 487…489 | `TMMesh` via objType index | mesh | 1 | TMObjectContainer.cpp:323 `[OBSERVED]` |

`[OBSERVED]` The decoration **mesh** names come from a second, different list:
`mesh\BoneAni4.txt`, which is `nCount aniTypeCount parts <name prefix>` and is indexed by
*nCount*, not by line number (MeshManager.cpp:104):

```
line 39 -> list[61] 1 1 mesh\lf01      <- TMLeaf
line 40 -> list[62] 1 1 mesh\fl01      <- TMFloat
line 41 -> list[63] 1 1 mesh\tr01      \
...                                     > TMTree 331..342
line 46 -> list[68] 1 1 mesh\tr06      /
line 47 -> list[69] 1 1 mesh\bf01      <- TMButterFly
line 48 -> list[70] 1 1 mesh\fs01      <- TMFish
line 49 -> list[71] ... mesh\tr07      \
...                                     > TMTree 351..378
line 62 -> list[84] 1 1 mesh\tr20      /
line 65 -> list[87] 1 2 mesh\tr21      <- spring tree (g_nSpringTree)
```

`[OBSERVED]` `TMSkinMesh` then builds `<prefix>%02d%02d.msh` / `.ani` / `.wyt`
(TMSkinMesh.cpp:167-186). For `tr01` that is `tr010101.msh`, `tr010101.ani`, `tr010101.wys`
(60 752 / 19 208 / 32 897 bytes) — note the code asks for `.wyt` but **no `.wyt` exists**
(0 files in `mesh\`); the extension is normalised to `.wys` inside
`GetModelTextureIndex` (TextureManager.cpp:705-708) by overwriting the last 3 characters with the
list entry's last 3 characters. `[OBSERVED]`

---

## 10. World object placement

### 10.1 The decision tree

`[OBSERVED]` `TMObjectContainer::Load` (TMObjectContainer.cpp:88-953) dispatches purely on
`dwObjType`. Summary of the type → class mapping (the full switch is in the file; this is the
subset that affects rendering):

| objType | created | notes |
|---|---|---|
| 2 | `TMSea` | grid dims from `nMaskIndex/2`, `nTextureSetIndex/2` |
| 3 | `TMFloat` | lily/float, skin-mesh 62 |
| 5 | `TMFloat` | billboard 90 |
| 4, 6, 7, 343 | 5 × `TMButterFly` | |
| 12, 344 | 5 × `TMFish` | |
| 8, 9, 13, 531 | `TMDust` | |
| 10 | `TMDrop` | water drip effect |
| 311…322 | `TMLeaf` | also marks `m_pVAttrData` (2×2 cells) |
| 331…342, 351…378 | `TMTree` | |
| 487…489 | `TMShip` | |
| 251…254, 195, 273, 274, 292, 474, 490, 607, 610, 614, 697, 699, 1520, 1526, 1535, 1665, 1695, 1696, 1993, 2005, 1711, 1739, 1750, 1855 | `TMHouse` (with `cHouseType` 1…11) | house type 11 → `GetCommonMesh(objType)` directly |
| 501…506, 511…518, 520…532, 533…599 | effect (billboards / `TMEffectMesh` / `TMLight`) | these carry the extra 8 bytes |
| 121 | skipped (`continue`) | |
| 1528, 1540…1543, 1597 | `TMEffectMesh` with a remapped effect id | |
| 1549…1551 | `TMBike` | |
| 1846, 1980, 2035 | hand-built effect clusters | |
| anything else | **`TMObject`** | generic: mesh = MeshList index **= objType** |

`[OBSERVED]` The generic fallback is `TMObject::Render` → `GetCommonMesh(m_dwObjType, 0, 3_min)`
(TMObject.cpp:242, 253, 340, 462, 688) — this is the **".msa-per-object convention"**: for
unclassified types the *object type number is literally the `MeshList.txt` index*, and
`MeshList.txt` maps index → `.msa` path (`mesh\…` or `effect\…`). Since
`MAX_COMMON_MESH = 3048` (MeshManager.h:39) and `MeshList.txt` has 1703 lines with sparse indices
up to ~2900, most "decoration" object ids are common-mesh ids.

### 10.1b `.msa` header (what a placement id resolves to)

`[OBSERVED]` `TMMesh::LoadMsa` (TMMesh.cpp:494-599):

| field | type | byte offset | size | meaning |
|---|---|---|---|---|
| `m_dwFVF` | `uint32` | 0 | 4 | D3D9 FVF (322 = `XYZ\|DIFFUSE\|TEX1`, 530 = `XYZ\|NORMAL\|TEX2`, 578 = `XYZ\|DIFFUSE\|TEX2`) |
| `m_sizeVertex` | `uint32` | 4 | 4 | vertex stride |
| `m_dwAttCount` | `uint32` | 8 | 4 | number of attribute ranges / sub-materials |
| `attRange[]` | 5 × `uint32` | 12 | 20 ea. | `AttribId, FaceStart, FaceCount, VertexStart, VertexCount` (`D3DXATTRIBUTERANGE`) |
| `texName[i]` | `char[11]` | 12 + 20·attCount + 11·i | 11 | e.g. `"\sky04"`; junk before the `\` is ignored |
| *index buffer* | `uint32 byteCount` + `uint16[byteCount/2]` | 12+20·attCount+11·attCount | — | present only if `byteCount != 0` (TMMesh.cpp:603-619) |
| *vertex buffer* | `uint32 byteCount` + raw verts, stride `sizeVertex` | — | — | `[OBSERVED]` two `fread`s (TMMesh.cpp:627-667); for FVF ≠ 322 the loader expands the stride by +8 and synthesizes a second UV set (TMMesh.cpp:644-669) — mesh-chapter detail |

Texture resolution for each attribute (TMMesh.cpp:513-598):
`name-after-last-backslash` → strip extension → if the msa path starts with `effect` use
`Effect\%s.wys` + effect list, else `mesh\%s.wys` + model list; if the lookup fails, retry with the
*msa file's own stem*; if that fails too, `LoadMsa` returns 0 and the mesh is not loaded at all.

### 10.2 Spatial bucketing and LOD of objects

`[OBSERVED]` TMObjectContainer.cpp:101-103:

```c
Key = (int)(x+offx) >> 5  |  ((int)(y+offy) >> 5) << 16;   // 32-unit buckets
AddChildWithKey(obj, Key);
```

`[OBSERVED]` `TMTree`/`TMLeaf` gate rendering on `m_dwKey` being one of `TreeNode::m_VisualKey1..4`
(e.g. TMTree.cpp:32-33). Those keys are recomputed **every frame from the camera**
(TMCamera.cpp:209-259):

```c
tempX1..3 = camera X (>>5 = 32-unit cells);  tempY1..3 = camera Y (>>5)
// the second/third index is forced to differ from the first by ±1 when they coincide
m_VisualKey1 = tempX1 + (tempY1 << 16);   // 1 = (x1,y1)
m_VisualKey2 = tempX1 + (tempY2 << 16);   // 2 = (x1,y2)
m_VisualKey3 = tempX2 + (tempY1 << 16);   // 3 = (x2,y1)
m_VisualKey4 = tempX2 + (tempY2 << 16);   // 4 = (x2,y2)
m_VisualKey5..9 = the remaining (x1..x3, y1..y3) combinations
```

so object culling is a **3×3 block of 32×32-unit cells** around the camera (9 keys defined,
but trees/leaves only test the first 4). `[INFERRED]` the "4" restriction is why trees pop in
slightly earlier than other objects.

### 10.3 Shadow / ground mask registration

`[OBSERVED]` TMObject.cpp:607-655: for each object, a 16×16 slice of
`MeshManager::m_aObjectMask[nMaskIndex]` is rotated by `m_fAngle` and blended into
`m_pMaskData[y][128*nBaseY + i + nBaseX]` as

```c
nTemp = (int)(m_fHeight / 0.1f) + 3 * maskValue;
if (m_fHeight > 0) ++nTemp; if (nTemp > 127) nTemp = 127;
if (nTemp > existing) existing = nTemp;
```

This is the *only* effect generic objects have on the rendered world: they raise the ground mask,
which the terrain does **not** draw (the mask is used for object snapping / water / rain), so a
generic `TMObject` with no mesh (e.g. types 1528, 532, 2035) is visually invisible except for the
effect it spawns. `[OBSERVED]` for the code, `[INFERRED]` for the "invisible" consequence.

### 10.4 Which `.bin`/`.txt` drives placement

| file | role |
|---|---|
| `Env\FieldNNNN.dat` | **the** placement list (this section) |
| `object.bin` | 2048 × 16×16 shadow/blocking masks + checksum (section 13.1) |
| `Mesh\MeshList.txt` | common-mesh index → `.msa` |
| `Mesh\MeshTextureList.bin` | model-texture index → `.wys` |
| `Effect\EffectTextureList.bin` | effect-texture index → `.wys` |

`[OBSERVED]` There is **no** server-side `Client\Field*` file, no `Field*.bin` placement file, and
no per-object `.msa` naming convention beyond the MeshList index (grep over the whole source for
`Field` + `bin` returns only the UI scene files, section 11).

---

## 11. Scene-definition files

| file | size | loader | rendering relevance |
|---|---|---|---|
| `Env\Character.trn` | 49160 | TMSelectCharScene.cpp:118 | **yes** — same format as a field, `pos=(16,16)` copied from Field1616 |
| `Env\Character.dat` | 27276 | TMSelectCharScene.cpp:119 | **yes** — 969 object records (18 with extra 8 bytes), mostly type 312 (leaves) |
| `UI\Field1031.dat` | 50760 | **none** `[OBSERVED]` | no — byte-identical to `Env\Field1031.dat` (SHA256 prefix `8917BB3ABF7535ED` for both) |
| `UI\Field2822.dat` | 18732 | **none** | no — byte-identical to `Env\Field2822.dat` (`E0A193C535F8E08E`) |
| `mesh\Field1813.dat` / `2722` / `2821` | — | **none** | no — stale copies, identical to the `Env` ones; `mesh\Field2822.dat` (17724) is an **older revision** of the `Env` file (18732) |
| `UI\TimeTable.bin` | 12800 | `TMDemoScene::ReadTimeTable()` is an **empty stub** (TMDemoScene.cpp:47-49); the live table is `static short g_sTimeTable[7] = {120,240,360,3360,3656,5936,7375}` hard-coded in TMGlobal.h:261 | no (in this build) |
| `UI\RC.bin` | 106496 | `ObjectManager::InitResourceList` (ObjectManager.cpp:850) | no — resource-string table for gameplay UI |
| `UI\mNNZZ.wyt` | mostly 49 182 (95 files), some larger (24 606 ×9, up to 393 246); 108 files | `TMGround::SetMiniMapData` (TMGround.cpp:3746-3767) | **yes** — 128×128 minimap tiles composited 3×3 into dynamic texture 4 at `(x<<7, (2-y)<<7)`; missing → UI texture 13 |
| `minimap.dat` | 20 bytes, text: `"2100\t2100\t1\t0\t0\tNOOB"` | `SetMinimapPos()` (TMFieldScene.cpp:589-591) reads up to 256 lines of `x y colour cx cy label` | **minimap only** — gives portal/town markers a colour per entry (1=white, 2=green, 3=blue, 4=magenta) |
| `Env\EnvTextureList3.bin` | 135168 | TextureManager.cpp:980 | **yes** (section 6) |
| `Effect\EffectTextureList.bin` | 135168 | TextureManager.cpp:505 | **yes** (section 12) |
| `Mesh\MeshTextureList.bin` | 540672 = 2048×264 | TextureManager.cpp:715 | **yes** |

`[OBSERVED]` `UI\UITextureList.bin` and `mesh\UITextureList.bin` are byte-identical
(`DC67D0CA714122FB`); `UI\MeshList.txt` and `mesh\MeshList.txt` are **different files** — the
runtime uses `Mesh\MeshList.txt` (TMPaths.h:52).

---

## 12. Effect texture plumbing

### 12.1 Does `Effect\EffectTextureList.bin` exist in 748? **Yes.**

`[OBSERVED]` `client_748\Effect\EffectTextureList.bin`, 135 168 bytes = `512 × 264`, same record
layout as the env list. **418** indices are populated (highest used index 438) with holes at
80-88, 140-149, 192 and 259. Full dump: `evidence/hex/world-effecttexturelist-map.txt`.

Highlights needed elsewhere in this document:

```
 0 smog      2 bright(*)   9 rain01   10 rain02   11..18 Fire01..08
56 flare    60 line       90 frog01  118 dlight  119 ddust  152 wdust
166..178 sky000..sky012  205..213 flare001..flare009  214 moon  215 milkyway
274 fog     406 fog2     407..409 win035/040/041  410 ArrowShade  411 arrow
422 snowsand  423 telehole 424 telespear
 67..70 mesh\sky01..04.wys  (sky dome, see 7.1)
300 = "EFFect\re0012.wys"  <- capital-EFFect typo in the list [OBSERVED]
```

`[OBSERVED]` `Effect\` contains 413 `.wys`, **37** `.msa` and the single list `.bin` (see
`evidence/hex/world-asset-inventory.txt`). The `.msa` files there are used by `TMEffectMesh`
(`Effect\*.msa` → effect-texture namespace), the `.wys` are the billboard/particle art.

### 12.2 How `TMEffect*` resolves a texture

```
TMEffectBillBoard(nTextureIndex, ...)  -> m_nTextureIndex = nTextureIndex   (TMEffectBillBoard.cpp:28)
                                       -> g_pTextureManager->GetEffectTexture(m_nTextureIndex, t)
                                       -> m_stEffectTextureList[n].szFileName   (effect list only)
```

`[OBSERVED]` `TextureManager::LoadEffectTexture` is the same code shape as the env loader:
1-byte-skip `.wys`→DDS unwrap, `cAlpha` mapping, `D3DXCreateTextureFromFileInMemoryEx`.

### 12.3 ⚠️ The "silently renders nothing" trap

`[OBSERVED]` All four billboard classes bail out before issuing geometry if the *name* is empty:

```c
if (!strlen(g_pTextureManager->m_stEffectTextureList[m_nTextureIndex].szFileName))
    return 0;                                   // TMEffectBillBoard.cpp:94
```

Same check in `TMEffectBillBoard2.cpp:58`, `TMEffectBillBoard3.cpp:59`,
`TMEffectBillBoard4.cpp:57`.

Consequences for a port:

* A hole in the effect list (80-88, 140-149, 192, 259) means "draw nothing", not "draw white".
* If `Effect\EffectTextureList.bin` is missing entirely, `InitEffectTextureList` fails
  (TextureManager.cpp:505-520) leaving the whole array zeroed → **every** effect disappears with
  no log line beyond one `LOG_WRITELOG`. Replicate the same behaviour (skip) or you will get a
  screen full of white quads.
* A *valid* index whose file is absent is different: `GetEffectTexture` returns `nullptr` and the
  geometry **is** submitted with no texture bound. `[INFERRED]` (code path, not visually verified).

---

## 13. The `object.bin` / `cdata.bin` / `ItemList.bin` family

### 13.1 `object.bin` — rendering-relevant (masks). Container only.

| property | value |
|---|---|
| size | **524 292** = 2048 × 16 × 16 + 4 |
| layout | `int8 m_aObjectMask[2048][16][16]`, then `int32 checksum` |
| encoding | per-byte: `m = m - key[j % 172] - j` (key = `ObjectMaskEncKeys[172]`, MeshManager.h:78) |
| checksum | `sum += mask % 4 + 2*MaskY + 5*MaskNum` over the **raw (still encoded)** bytes, compared with the trailing int32 |

`[OBSERVED]` Verified: computed = trailer = **−1 604 083 155**. Mask maths (how a byte becomes a
height) is owned by another chapter; cross-reference it and do not duplicate.

### 13.2 `cdata.bin` — anti-tamper table, not rendering

| property | value |
|---|---|
| size | **8192** = 64 × 32 × int32 (exactly `TMGround::m_nCheckSum[64][32]`) |
| read by | `TMGround::TMGround` (TMGround.cpp:2289-2293) |
| used as | `m_nCheckSum[zoneY][zoneX]` for `.trn` (TMGround.cpp:2505) and `m_nCheckSum[zoneY+32][zoneX]` for `.dat` (TMObjectContainer.cpp:958) |

`[OBSERVED]` **The shipped file cannot satisfy either gate**: only 141 of 2048 cells are non-zero,
and none equals the byte-sum of the corresponding `.trn` (e.g. `Field0101.trn` sums to 349 295
signed / 1 955 183 unsigned, table cell `[1][1] = 0`). The hard-coded magic
`nCDataCheckSum == 5855606140` also cannot be produced by the decompiled formula (computed
−189 869 228 032).

**Porting recommendation:** treat both checksum gates as disabled. Enabling them locks the client
out of every shipped zone. `[INFERRED]` (the data proves it).

### 13.3 The rest of the family

| file | size | rendering? | role |
|---|---|---|---|
| `ItemList.bin` | 910 004 (= `N*sizeof(STRUCT_ITEMLIST) + 4`), XOR 0x5A obfuscated, checksum must equal `0x1343B16` unless `_DEBUG` — Basedef.cpp:136-197 | **indirectly**: `g_pItemList[i].nIndexMesh` is the common-mesh id used by `TMGate`/`TMItem` (TMGate.cpp:31) | item → mesh mapping |
| `Itemname.bin` | 196 044 | no | UI strings |
| `SkillData.bin` | 9 988 (+4 checksum) | no | skill table |
| `InitItem.bin` | 768 | no | starter items |
| `extraitem.bin` | 0 (empty in this client) | no | optional item patch |
| `SkillData.bin`/`ItemList.bin` copies in `mesh\` | 9 572 / 520 004 | no | **different, stale** content than the root copies `[OBSERVED]` |
| `curse.bin` | 8 192 | no (text filter) | profanity table |
| `sn.bin`, `sn2.bin`, `serverlist.bin` | 143 / 990 / 7040 | no | server list |
| `shader1..8.bin`, `shader\vseffect*.bin`, `skinmesh*.bin` | 700-1400 | **yes** (D3D9 shaders only) | shader bytecode; irrelevant for GL/VK (rewrite, see §14) |

---

## 14. Reimplementation notes (OpenGL/Vulkan)

1. **World units = metres; Y up; 1 height byte = 0.1 unit; 1 tile = 2 units; 1 zone = 128 units.**
   Bake `world = (x*2 + zoneX*128, h*0.1, z*2 + zoneY*128)` into your vertex data; there is no
   per-frame matrix on the terrain other than the camera view.
2. **Build static terrain chunks at load time.** The original emits 4-vertex strips per tile per
   frame with `DrawPrimitiveUP`; that is an artifact of 2003-era immediate mode, not semantics.
   One vertex buffer per zone (65×65 grid, or 64×64 quads with 512 quads per chunk) + one index
   buffer reproduces it exactly. Keep the *decimation* rule (2-unit → 4-unit pitch beyond
   |Δ| > 16…17 tiles from the camera) only if you want bit-identical visuals; a modern LOD is
   visually equivalent.
3. **Two texture stages, `MODULATE`.** stage0 = `env_list[byTileIndex+10]`, stage1 =
   `env_list[byBackTileIndex+256]` with UVs from `BackTileCoordList[byBackTileCoord]`
   (half-tile atlas). Both are **BC1/DXT1 128×128 with 4 mips**; terrain is opaque
   (`cAlpha == 'N'` for every entry), so no alpha test is needed.
4. **Vertex colour is `0x00RRGGBB`** (alpha byte always 0 in the data — treat as 1.0). Multiply
   `texture0 * texture1 * color.rgb * light`. Lighting is *vertex-lit by hardware*: replicate
   `material.Diffuse·(Σ dirLight_i) + material.Emissive` with `Ambient = 0x33FFFFFF`
   (RenderDevice.cpp:1710) and the two directional lights of §5.4
   (`normalize(-10,10,-6)` key + `normalize(10,-14,6)` fill, no ambient term of their own).
   `D3DRS_DIFFUSEMATERIALSOURCE = 1` (material) so vertex colour and material diffuse multiply.
5. **Fog:** linear, colour = clear colour, `(sight+13 … sight+24)` world units (section 7.6), plus
   the underwater override `(1 … 10)`. In Vulkan implement as a fragment push-constant range.
6. **The sky is a 180-triangle dome mesh** (`mesh\sky001.msa`) drawn camera-locked at scale
   `(0.5, 0.25, 0.5)` with two additive/alpha-blended layers. Do **not** implement it as a
   skydome shader with a procedural gradient — the art is `mesh\sky01..04.wys`,
   **1024×256 DXT1, single mip** (§6.4).
7. **Water is procedural** and needs no assets beyond `mesh\water1..4.wys`: geometry parameters
   (grid W/H, level) come from the `.dat` record, UV scroll speeds are exactly
   `t/12000` and `t/18000` per second-of-server-time.
8. **Missing-asset policy must match the original's silent failures** (§6.3, §12.3): the shipped
   data relies on them.
9. **Textures are DDS in a 1-byte wrapper.** To convert offline:
   (a) drop the first byte (`'W'`, `0x57`); (b) overwrite the new bytes `0..2` (`"S10"`) with
   `"DDS"`; (c) at new offset 84, overwrite the 4-byte fourCC with `"DXT1"` if its first byte was
   `'2'` (`"2z73"`), else `"DXT3"` (`"7x0s"`). The result is a stock DDS that any tool can read
   (§6.4).
10. **Skip both `cdata.bin` checksums** (§13.2).
11. **Do not port the `.wyt` extension literals** (TMSkinMesh.cpp:179-192): normalise to `.wys` and
    look the name up in `MeshTextureList.bin`.
12. **Hard-coded world coordinates are load-bearing.** Zone-specific rendering (swamp fog at zones
    x 27…30 / y 21…24, dungeon ids from `TMGround::SetPos`, the `(29,22)`/`(18,30)` special cases)
    is baked into the code, not into data. Extract the tables from
    `TMGround.cpp:2339-2383` and `TMSky.cpp:257-444` verbatim.

---

## 15. Decompiled names that are obviously wrong

| name | problem | correct interpretation |
|---|---|---|
| `TMGround::m_TileMapData` | it is the *whole* `.trn` tile array, not just a map | `tiles[4096]` |
| `TMObjectContainer::Load` `sz1`/`sz2` | meaningless | `kObjectRecordSize = 28`, `kEffectExtraSize = 8` |
| `TMGround::m_nCheckSum[64][32]` | indexed `[y][x]` for tiles, `[y+32][x]` for objects | two 32×32 tables in one array |
| `m_pMaskData` / `m_pVAttrData` | not pointers (fixed arrays) | `walkMask[128][128]`, `vegetationMask[128][128]` |
| `TMSky::n_pMeshMilkyway` | hungarian `n_` on a pointer, and it is never used | dead member |
| `m_fEffset` / `m_vecEffset` | two spellings of the same field in `TMGround.h:77` | `m_vecEffset` |
| `TMObjectContainer.cpp:51` comment "why objects aren't deleted too?" | indicates a known leak | not a format issue |
| `FileTileInfo` field `dwColor` | it is a `D3DCOLOR` (`0x00RRGGBB`) | vertex colour |
| `TMGround::GetTileType` condition `nIndex>=14 && nIndex<=17 && nIndex>=38 …` | impossible chain (decompiler flattened `\|\|` into `&&`) | separate ranges, see §5.5 |
| `TMSea::m_nMaskIndex` / `m_nTextureSetIndex` reuse | same fields as `TMObject`, different meaning for type 2 | `gridH`, `gridW` |

---

## Appendix A — evidence files

| file | content |
|---|---|
| `evidence/hex/world-trn-Field0101-head.txt` | `.trn` header + 6 decoded tile records + hexdump |
| `evidence/hex/world-trn-inventory.txt` | all 98 `.trn`: header fields, per-file ranges, global range proof |
| `evidence/hex/world-dat-Field0101-head.txt` | `.dat` hexdump + 8 decoded records incl. the 8-byte extras |
| `evidence/hex/world-dat-parse-proof.txt` | 98/98 files parse exactly to EOF with the 28+8 rule |
| `evidence/hex/world-attributemap.txt` | sizes, trailer checksum, byte histogram, Env-vs-UI diff count |
| `evidence/hex/world-envtexturelist3-map.txt` | index → filename for `Env\EnvTextureList3.bin` |
| `evidence/hex/world-effecttexturelist-map.txt` | index → filename for `Effect\EffectTextureList.bin` |
| `evidence/hex/world-meshtexturelist-map.txt` | index → filename for `Mesh\MeshTextureList.bin` |
| `evidence/hex/world-cdata-object-sky-msa.txt` | `cdata.bin`, `object.bin` (+verified checksum), `sky001.msa` headers |
| `evidence/hex/world-wys-headers.txt` | `.wys` → DDS decoding proof: three raw headers + a size-verification table (all EXACT) |
| `evidence/hex/world-asset-inventory.txt` | size + SHA256 prefix of every relevant asset, plus both `MeshList.txt` heads |
