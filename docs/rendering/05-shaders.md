# 05 — Shader usage (WYD 748 client / TMProject D3D9 port)

Purpose: complete specification of every shader asset, how it is loaded, what it computes, which
constants it reads and which meshes bind it — enough to re-implement the same shading in OpenGL or Vulkan
without reading the C++.

Last updated: 2026-11-10

---

## Confidence summary

| Tag | Count | Where it dominates |
|---|---|---|
| `[OBSERVED]` | ~78 | file sizes/hexdumps, version tokens, every instruction (disassembled), all source lines quoted |
| `[INFERRED]` | ~14 | "which shader belongs to which effect", shader author intent, DX8-vs-DX9 modifier numbering |
| `[UNKNOWN]` | 5 | see below |

Biggest unknowns:

1. **`[UNKNOWN]`** Whether the *original* `client_748\WYD.exe` ever creates a pixel shader. The exe's
   import table is obfuscated (section names blanked, D3D8 entry points resolved dynamically — the literal
   `"d3d8.dll"` sits next to `"read error"` at file offset `0x1A68FC`), so import-name inspection is
   inconclusive. Resolved by: running the 748 client with API Monitor / apitrace on `d3d8.dll` and watching
   for `CreatePixelShader`. The absence of any `pseffect*` string in the exe (`[OBSERVED]`) already makes
   "no pixel shaders in 748" the strong default.
2. **`[UNKNOWN]`** Exact DX8 source-modifier numbering for `ps_1_1` (`code 4`). The bytecode contains
   `code 4` exactly where the only available source text says `_bx2` (`[OBSERVED]` cross-check), but the
   published DX9 enum assigns 4 to `D3DSPSM_SIGN`. Consequence is cosmetic only (these shaders are never
   bound). Resolved by: `D3DXDisassembleShader` on `pseffect5.bin` with the real D3DX9.
3. **`[UNKNOWN]`** What `m_dwBufferUsage` was consumed by in the 755 source this port descends from — in
   this port it is written and never read (`[OBSERVED]`, only `RenderDevice.cpp:44` and `:927`).
   Resolved by: diffing against the 755 port branch.
4. **`[UNKNOWN]`** Whether `c95` (fog constants for `skinmesh1..4`) is uploaded anywhere in the 755/760
   client. It is not uploaded in this port (`[OBSERVED]`), which makes `skinmesh1..4` fog read whatever is
   left in `c95` from the device default (all zeros). Resolved by: disassembling the 755 client's
   `CMesh::SetMaterial` equivalent.
5. **`[UNKNOWN]`** Exact per-file compiler for `Shader\skinmesh*.bin` — those files carry **no** debug blob,
   so the assembler version cannot be read out of them (the `vseffect*`/`pseffect*` do carry one).

---

## Table of contents

1. [TL;DR — what actually renders](#1-tldr--what-actually-renders)
2. [Shader asset inventory (with version tokens)](#2-shader-asset-inventory-with-version-tokens)
3. [Loading code path](#3-loading-code-path)
4. [Does DX8-era bytecode load on D3D9?](#4-does-dx8-era-bytecode-load-on-d3d9)
5. [Root-level `shader1..8.bin` — used or leftovers?](#5-root-level-shader18bin--used-or-leftovers)
6. [Vertex declarations](#6-vertex-declarations)
7. [Shader constant registers](#7-shader-constant-registers)
8. [Skinned-mesh vertex shaders `skinmesh1..8`](#8-skinned-mesh-vertex-shaders-skinmesh18)
9. [Effect shaders `vseffect1..4` / `pseffect1..6`](#9-effect-shaders-vseffect14--pseffect16)
10. [Which mesh/effect uses which shader index](#10-which-mesheffect-uses-which-shader-index)
11. [Fixed-function fallback](#11-fixed-function-fallback)
12. [Reimplementation notes (OpenGL / Vulkan)](#12-reimplementation-notes-opengl--vulkan)

Evidence directory: `docs/rendering/evidence/hex/`
* `shader-<name>.txt` — 16-byte-per-line hexdumps of every `.bin` (both `Shader\` and root level)
* `disasm/<name>.txt` — instruction listings produced by `tools/disasm_dx8.py`
* `tools/disasm_dx8.py` — the disassembler used (token layout documented in its header)

---

## 1. TL;DR — what actually renders

* The **only** shaders that affect a frame in this port are the **eight `vs_1_1` skinned-mesh vertex
  shaders** `Shader\skinmesh1..8.bin`. `[OBSERVED]` `CMesh.cpp:612-622` is the only place a shader is bound
  (`SetVertexShader`/`SetVertexDeclaration`), plus `SetVertexShader(0)` to unbind at `CMesh.cpp:759`.
* `Shader\vseffect1..4.bin` (4 × `vs_1_1`) and `Shader\pseffect1..6.bin` (6 × `ps_1_1`) are **created but
  never bound**. `[OBSERVED]`: `SetPixelShader` does not appear anywhere in `Projects\TMProject\`;
  `m_pVSEffect[]`/`m_pPSEffect[]` appear only in creation, release and null-check code
  (`RenderDevice.cpp:121-127, 328-334, 644-648, 1497-1560`). They are dead weight — **do not port them.**
* The root-level `client_748\shader1..8.bin` are **not** referenced by this port (`[OBSERVED]`
  `TMPaths.h:28-30` only defines `Shader\…` paths). `[INFERRED]` they are the files the **original** 748 exe
  loads: the exe's only shader path format string is `shader%d.bin` (`[OBSERVED]`), i.e. the port and the
  original disagree on which skinned-mesh bytecode is used. See §5.
* Everything else (world geometry, sky, UI, particles, billboards) is **fixed-function D3D9**: multi-stage
  texture blending, `D3DMATERIAL9`, two `D3DLIGHT9`s. See `06-render-states-and-materials.md`.
* The pixel-shader-dependent features of later clients (blur/reflection/`m_nShadowTextureSize` render-to-
  texture shadows) are **not implemented** in this port: `m_nShadowTextureSize` is written and never read,
  `g_nUseBlur`/`g_nReflection` are forced to 0 when `ps_1_1` is missing and never re-enabled.
  `[OBSERVED]` `RenderDevice.cpp:527-548`.

Consequence for the porting strategy: **the shader port is 8 small `vs_1_1` programs and nothing else.**
Both asset sets (root `shader*.bin` and `Shader\skinmesh*.bin`) are `vs_1_1`, ~30 instructions each, and can
be transliterated into GLSL/HLSL/SPIR-V by hand in half a day (§8 gives the reconstructed source).

---

## 2. Shader asset inventory (with version tokens)

All values below are `[OBSERVED]` from the raw files; hexdumps in `evidence/hex/shader-*.txt`.

DWORD 0 of every file is the D3D shader version token (`little-endian`):

* `0xFFFE0101` = `vs_1_1`  (bytes `01 01 FE FF`)
* `0xFFFF0101` = `ps_1_1`  (bytes `01 01 FF FF`)

### 2.1 `client_748\Shader\` (the set the port loads)

| File | Bytes | DWORDs | Version token | Debug blob / comment | Program dwords |
|---|---|---|---|---|---|
| `skinmesh1.bin` | 468 | 117 | `0xFFFE0101` vs_1_1 | none | 116 |
| `skinmesh2.bin` | 616 | 154 | `0xFFFE0101` vs_1_1 | none | 153 |
| `skinmesh3.bin` | 700 | 175 | `0xFFFE0101` vs_1_1 | none | 174 |
| `skinmesh4.bin` | 784 | 196 | `0xFFFE0101` vs_1_1 | none | 195 |
| `skinmesh5.bin` | 464 | 116 | `0xFFFE0101` vs_1_1 | none | 115 |
| `skinmesh6.bin` | 612 | 153 | `0xFFFE0101` vs_1_1 | none | 152 |
| `skinmesh7.bin` | 696 | 174 | `0xFFFE0101` vs_1_1 | none | 173 |
| `skinmesh8.bin` | 780 | 195 | `0xFFFE0101` vs_1_1 | none | 194 |
| `vseffect1.bin` | 644 | 161 | `0xFFFE0101` vs_1_1 | `COMMENT(72 dwords)` = `DBUG…` + `D:\Client-ver\run\Shader\vseffect1.vsh` + `D3DX9 Shader Assembler` | 87 |
| `vseffect2.bin` | 748 | 187 | `0xFFFE0101` vs_1_1 | `COMMENT(84 dwords)`, same shape, `…\vseffect2.vsh` | 101 |
| `vseffect3.bin` | 252 | 63 | `0xFFFE0101` vs_1_1 | `COMMENT(40 dwords)`, `…\vseffect3.vsh` | 21 |
| `vseffect4.bin` | 236 | 59 | `0xFFFE0101` vs_1_1 | `COMMENT(38 dwords)`, `…\vseffect4.vsh` | 19 |
| `pseffect1.bin` | 112 | 28 | `0xFFFF0101` ps_1_1 | none | 27 |
| `pseffect2.bin` | 140 | 35 | `0xFFFF0101` ps_1_1 | none | 34 |
| `pseffect3.bin` | 256 | 64 | `0xFFFF0101` ps_1_1 | none | 63 |
| `pseffect4.bin` | 184 | 46 | `0xFFFF0101` ps_1_1 | none | 45 |
| `pseffect5.bin` | 112 | 28 | `0xFFFF0101` ps_1_1 | none | 27 |
| `pseffect6.bin` | 116 | 29 | `0xFFFF0101` ps_1_1 | none | 28 |

The "Program dwords" column counts the tokens *after* the 1-DWORD version header and after any `COMMENT`
blocks — i.e. how many instruction/parameter DWORDs the GPU actually executes. A `COMMENT` token is
`0xFFFE | (nDwords<<16)` followed by `nDwords` DWORDs of data, so it is skipped entirely by the
disassembler.

### 2.2 Root level `client_748\shader*.bin` (the set the *original* 748 exe loads)

| File | Bytes | Version token | Debug blob | Notes |
|---|---|---|---|---|
| `shader1.bin` | 796 | `0xFFFE0101` vs_1_1 | `COMMENT(9)` = `D3DX8 Shader Assembler Version 0.91` + per-instruction `COMMENT(2)` = `FILEskinmesh1.vsh` / `LINE<n>` | no `dcl` at all |
| `shader2.bin` | 1052 | `0xFFFE0101` | same | no `dcl` |
| `shader3.bin` | 1196 | `0xFFFE0101` | same | no `dcl` |
| `shader4.bin` | 1340 | `0xFFFE0101` | same | no `dcl` |
| `shader5.bin` | 792 | `0xFFFE0101` | same | no `dcl` |
| `shader6.bin` | 1048 | `0xFFFE0101` | same | no `dcl` |
| `shader7.bin` | 1192 | `0xFFFE0101` | same | no `dcl` |
| `shader8.bin` | 1336 | `0xFFFE0101` | same | no `dcl` |

`[OBSERVED]` both sets are `vs_1_1`; **no** file anywhere in the 748 asset tree is `vs_2_0`, `ps_2_0` or
higher, and no file is a D3D9-only version.

### 2.3 Two facts worth repeating

1. `[OBSERVED]` `client_748\Shader\skinmesh{1..8}.bin` and `pseffect{1..6}.bin` are **byte-identical**
   (SHA-256) to the same-named files in `E:\wyd\test-setup\wydkr2023debug\Shader\`. That second tree also
   ships the **`.vsh`/`.psh` source text**, which is used throughout this document as human-readable
   corroboration of the disassembly.
2. `[OBSERVED]` `client_748\Shader\vseffect{1..4}.bin` differ from the KR tree only inside the `COMMENT`
   debug blob (20 bytes longer path): after stripping all `COMMENT` tokens the instruction streams are
   identical. So the KR `.vsh` sources describe the 748 `vseffect*` binaries too.

---

## 3. Loading code path

### 3.1 Paths

`[OBSERVED]` `Projects\TMProject\TMPaths.h:28-30`:

```cpp
constexpr const char* ShaderSkinMesh_Path      = "Shader\\skinmesh%d.bin";
constexpr const char* ShaderVertexShader_Path  = "Shader\\vseffect%d.bin";
constexpr const char* ShaderPixelShader_Path   = "Shader\\pseffect%d.bin";
```

Relative to the process working directory, which is `E:\wyd\test-setup\client_748`.

### 3.2 `RenderDevice::InitVertexShader()` — `RenderDevice.cpp:768-1528`

Sequence `[OBSERVED]`:

1. `:920` `m_bUseSW = 16;` (initial value = `D3DUSAGE_SOFTWAREPROCESSING`)
2. `:924-930` `if (d3dCaps.VertexShaderVersion >= 0xFFFE0101) { m_bUseSW = 0; m_dwBufferUsage = 8; LOG("VertexShader HW Accel Enabled"); }`
   else `LOG("VertexShader SW Mode ")`.
3. `:936` `if (m_pVertexShader[0] == nullptr)` → the whole block is a one-shot initialiser.
4. `:938-963` for `i = 0..7`:
   * `:942` `sprintf(szBinFile, ShaderSkinMesh_Path, i+1)` → `Shader\skinmesh1.bin` … `skinmesh8.bin`
   * `:944-953` `_open(..., _O_BINARY)` / `_filelength` / `D3DXCreateBuffer(len)` / `_read` — **raw bytes
     into a D3DX buffer, no assembly, no transcoding, no validation.** If the file is missing the function
     `return 0` and `Initialize()` aborts.
   * `:956` `CreateVertexDeclaration(VertexDecl[i % 4], &m_pVertexDeclaration[i])`
   * `:959` `CreateVertexShader((const DWORD*)pCode->GetBufferPointer(), &m_pVertexShader[i])` — the file
     bytes are handed to D3D9 **verbatim**. Any failure → `return 0`.
5. `:965-967` after the loop, `m_matProj` is transposed and uploaded to `c2` (`SetVertexShaderConstantF(2, …, 4)`).
6. `:970-1495` builds 19 more vertex declarations (`DeclWater`, `DeclEquip`, … `DeclRefectWater`) into
   `VertexDeclEff[19]`.
7. `:1497-1525` `if (m_pVSEffect[0] == nullptr)` → for `j = 0..3` load `Shader\vseffect{1..4}.bin` the same
   way (`CreateVertexDeclaration(VertexDeclEff[j], &m_pVDEffect[j])`, `CreateVertexShader(...)`).

So the answer to "`D3DXAssembleShader` from text or pre-compiled bytecode?" is: **pre-compiled bytecode,
passed straight to `CreateVertexShader`**. `D3DXAssembleShader` / `D3DXCompileShader` appear nowhere in the
port (`[OBSERVED]`). Note that `m_pVSEffect[]` is only *created* here; nothing ever makes one current.

### 3.3 `RenderDevice::InitPixelShader()` — `RenderDevice.cpp:1530-1573`

`[OBSERVED]`:

```cpp
if (d3dCaps.PixelShaderVersion >= 0xFFFF0101) {      // :1534
    if (m_pPSEffect[0] == nullptr) {
        for (i = 0; i < 6; ++i) {                    // :1538
            sprintf(szBinFile, ShaderPixelShader_Path, i + 1);
            ... _open/_filelength/D3DXCreateBuffer/_read ...
            if (FAILED(m_pd3dDevice->CreatePixelShader((const DWORD*)pCode->GetBufferPointer(),
                                                       &m_pPSEffect[i])))
                return 0;                            // :1556-1557
        }
    }
    return 1;
} else {
    m_bSupportPS11 = 0;
    m_nShadowTextureSize = 64;
    return 1;
}
```

Again: **raw bytecode → `CreatePixelShader`**, and again the result is never bound. Note `RestoreDeviceObjects()`
deliberately skips it: `[OBSERVED]` `RenderDevice.cpp:616-617`

```cpp
InitVertexShader();
// Not used
//InitPixelShader();
```

so after a device reset the pixel shaders are gone and, because nothing references them, nothing breaks.

### 3.4 Capability gates and dead capability flags

| Symbol | Set at | Value | Actually used? |
|---|---|---|---|
| `m_bSupportPS11` | `RestoreDeviceObjects:531-545`, `InitPixelShader:1567` | `PixelShaderVersion >= 0xFFFF0101` | Only to pick `m_nShadowTextureSize` 128 vs 64 and to zero `g_nReflection`/`g_nUseBlur`. `[OBSERVED]` |
| `m_bSupportPS12` | `:547` | `PixelShaderVersion >= 0xFFFF0102` | **never read** `[OBSERVED]` (grep: only this assignment) |
| `m_bSupportVS20` | `:548` | `VertexShaderVersion >= 0xFFFE0200` | **never read** `[OBSERVED]` |
| `m_bUseSW` | `InitVertexShader:920,926` | `16` when no HW VS, else `0` | Read at `CMesh.cpp:601,756` → wraps the draw in `SetSoftwareVertexProcessing(1)/(0)`; also truth-tested as a boolean |
| `m_dwBufferUsage` | ctor `:44` = `24`, `:927` = `8` | `24` = `D3DUSAGE_WRITEONLY(8) | D3DUSAGE_SOFTWAREPROCESSING(16)`, `8` = WRITEONLY | **never read** `[OBSERVED]` |
| `m_nShadowTextureSize` | `:534`/`:539`/`:1568` = 128 or 64 | – | **never read** `[OBSERVED]` — no render-to-texture shadow exists in this port |

There is no `#if 0`-disabled shader path in the port; the only disabled call is the commented
`InitPixelShader()` at `RenderDevice.cpp:617` (`[OBSERVED]`).

### 3.5 Release / re-create on device loss

`[OBSERVED]`
* `InvalidateDeviceObjects()` `:628-672` releases `m_pVertexShader[0..7]`, `m_pVertexDeclaration[0..7]`,
  `m_pVSEffect[0..3]`, `m_pPSEffect[0..5]`. (`m_pVDEffect[]` is **not** released — a leak, harmless.)
* `RestoreDeviceObjects()` `:527-626` re-runs `InitializeRenderingState(); InitVertexShader();` — pixel
  shaders are not recreated.
* `Finalize()` `:289-341` releases everything.

---

## 4. Does DX8-era bytecode load on D3D9?

**Yes — and this is confirmed at runtime, not assumed.** `[OBSERVED]`

* `client_748\WYD.log` (the log of this port's own run, 2026-11-10) contains
  `11/10 10:04:35 - VertexShader HW Accel Enabled` and **no** `Error VertexShader`, **no**
  `Error Pixel Shader`, and the session proceeded into `>> New Field Scene`. Because
  `InitVertexShader()`/`InitPixelShader()` `return 0` on the *first* failed `Create*Shader` and
  `RenderDevice::Initialize()` then `return 0`s, the absence of those strings proves all 8 + 4 + 6 shaders
  were accepted by D3D9 `CreateVertexShader` / `CreatePixelShader`.
* The version tokens are the DX8-era ones (`0xFFFE0101`, `0xFFFF0101`) but those encodings are shared with
  D3D9: `vs_1_1`/`ps_1_1` bytecode is the same binary format in D3D8 and D3D9, and the `0x0000FFFE`
  `COMMENT` token is valid in both. The one genuinely version-specific detail found is the **`ps_1_1`
  source modifier numbering** (§9.2), which does not fail creation, only interpretation.
* The `vseffect*` files were produced by **D3DX9** (their debug blob literally contains
  `"D3DX9 Shader Assembler"`), the root `shader*` files by **D3DX8 0.91**. Both are accepted.

So: **no re-assembly or transcoding happens, none is needed, and the files are not "dead" because of a
format problem.** `Shader\skinmesh*` are live; `vseffect*`/`pseffect*` are dead only because nothing binds
them; the root `shader*` are dead *in this port* only because `TMPaths.h` points elsewhere.

---

## 5. Root-level `shader1..8.bin` — used or leftovers?

`[OBSERVED]` evidence:

* The **only** shader path format string in `client_748\WYD.exe` is `shader%d.bin` (file offset `0x1B499C`,
  ASCII), immediately followed in the same string table by the error message
  `"Read VertexShader %d Error.. "`. There is **no** `skinmesh%d.bin`, **no** `vseffect*`, **no**
  `pseffect*` string anywhere in the exe (0 occurrences each; 1 occurrence of `shader%d.bin`).
* `[INFERRED]` **Therefore the original 748 binary loads `shader1.bin … shader8.bin` from the client root**
  and has no pixel-shader / effect-shader file loading at all. (Strength: exactly one shader-path format
  string exists and it has no directory component; the error message that follows it in the string table
  matches the "read the file, then create the shader" pattern; the root directory *does* contain exactly
  eight `shader*.bin` files.)
* This port instead loads `Shader\skinmesh*.bin` (`TMPaths.h:28`). Nothing in `Projects\TMProject\`
  references `shader%d.bin` (`[OBSERVED]` grep) — the root files are inert in the port.
* `[INFERRED]` the port kept the 755/760 loader and asset layout; the `Shader\` subdirectory contents are
  leftovers of that newer client dropped into the 748 asset dump. This is consistent with (a) the
  `vseffect*` debug paths reading `D:\Client-ver\run\Shader\…` (a later build machine) and (b) the
  byte-identical KR-tree copies.

Relationship between the two sets `[OBSERVED]` (compare `evidence/hex/disasm`):

* Root `shader1.bin` is the same *family* as `Shader\skinmesh5.bin` — same skeleton sequence
  (`mul r1, v2.zyxw, c0.wwww` … `m3x3 r2, r5, c92` … fog from `c6`) — but is **not** the identical program:
  it has **no `dcl` instructions**, writes `m4x3 r4` with a full `xyzw` destination mask where `skinmesh5`
  uses `xyz`, uses `mul r0, r1.y, c8 / add r0, r0, c7` where `skinmesh5` uses `mov r0, c7 / add r0, r0, c8`,
  and `min oFog.x` vs `min oFog.xyzw`. It is an **older revision** of the same shading model.
* Root files carry per-instruction `LINE`/`FILE` comments (D3DX8 0.91 debug build) which is why they are
  ~330-550 bytes larger than the `Shader\` versions for the same amount of code.

**Porting recommendation:** pick one set and be consistent. Either is a valid `vs_1_1` program; the
root set is what the 748 client actually ran, the `Shader\` set is what the port currently runs and is the
one with a matching, well-understood constant layout for `skinmesh5..8` only (§7.4 explains the `c95`
trap in `skinmesh1..4`).

---

## 6. Vertex declarations

All declarations are built inside `InitVertexShader()`; every one is
`Stream 0` attributes followed by the `Stream 255 / Type 17 (D3DDECLTYPE_UNUSED)` terminator.
`[OBSERVED]` `RenderDevice.cpp:775-1474`.

Type codes: `0`=FLOAT1, `1`=FLOAT2, `2`=FLOAT3, `4`=D3DCOLOR.
Usage codes (`D3DDECLUSAGE`): `0`=POSITION, `1`=BLENDWEIGHT, `2`=BLENDINDICES, `3`=NORMAL, `5`=TEXCOORD,
`6`=TANGENT, `10`=D3DDECLUSAGE_COLOR? (see note) — as *values*, `Usage = 10` appears in several
declarations.

> ⚠ **Flag: decompiled/source naming.** `Usage = 10` is written in the source but `D3DDECLUSAGE` only
> defines 0..10 with 10 = `D3DDECLUSAGE_TESSFACTOR`. In practice `10` here is used for "second texture
> coordinate set promoted to COLOR" in effect meshes; the literal numeric value is what matters and is
> reproduced below verbatim. Treat the symbolic name as unreliable.

### 6.1 `m_pVertexDeclaration[0..7]` — used with `skinmesh*` (the live path)

Built at `RenderDevice.cpp:956` with `VertexDecl[i % 4]`, i.e. indices 4..7 alias 0..3.

| Index | Source | Elements (offset, type, usage, usageIndex) | Stride | Bound with |
|---|---|---|---|---|
| 0 | `VertexDecl1` `:775-804` | (0, FLOAT3, POSITION), (12, D3DCOLOR, BLENDINDICES), (16, FLOAT3, NORMAL), (28, FLOAT2, TEXCOORD0) | 36 | `skinmesh1` |
| 1 | `VertexDecl2` `:805-840` | (0, FLOAT3, POSITION), (12, FLOAT1, BLENDWEIGHT), (16, D3DCOLOR, BLENDINDICES), (20, FLOAT3, NORMAL), (32, FLOAT2, TEXCOORD0) | 40 | `skinmesh2` |
| 2 | `VertexDecl3` `:841-876` | (0, FLOAT3, POSITION), (12, FLOAT2, BLENDWEIGHT), (20, D3DCOLOR, BLENDINDICES), (24, FLOAT3, NORMAL), (36, FLOAT2, TEXCOORD0) | 44 | `skinmesh3` |
| 3 | `VertexDecl4` `:877-912` | (0, FLOAT3, POSITION), (12, FLOAT3, BLENDWEIGHT), (24, D3DCOLOR, BLENDINDICES), (28, FLOAT3, NORMAL), (40, FLOAT2, TEXCOORD0) | 48 | `skinmesh4` |
| 4 | = `VertexDecl1` | as index 0 | 36 | `skinmesh5` |
| 5 | = `VertexDecl2` | as index 1 | 40 | `skinmesh6` |
| 6 | = `VertexDecl3` | as index 2 | 44 | `skinmesh7` |
| 7 | = `VertexDecl4` | as index 3 | 48 | `skinmesh8` |

The four **blend indices are packed into one `D3DCOLOR`** (4 bytes) at the offset shown. D3D expands a
`D3DCOLOR` attribute to `float4` as `(r,g,b,a)` = `(bits16-23, bits8-15, bits0-7, bits24-31)/255`, so the
packed DWORD is laid out as **`bone0` in the blue byte (LSB), `bone1` green, `bone2` red, `bone3` alpha**.

The shaders re-expand it with `mul r1, v2.zyxw, c0.wwww` where `c0.w = 765.01001` (`[OBSERVED]`
`CMesh.cpp:993`), and `765 = 3 × 255`, so

```
a0.x = v2.z * c0.w = (blueByte / 255) * 765.01 ≈ 3 × blueByte
```

i.e. **`a0.x` is not the bone index, it is the bone's constant-register address** (3 constants per bone,
palette base `c9`). Verified against real data `[OBSERVED]`:

| mesh file | header | first vertex BLENDINDICES | decoded |
|---|---|---|---|
| `mesh\ag010101.msh` | `infl=3, palettes=10, stride=44` | `0x00000005` | bone0 = 5 → `a0.x = 15` → `c24` = `c9 + 3×5` ✔ |
| `mesh\ag010104.msh` | `infl=1, palettes=10, stride=36` | `0x00000009` | bone0 = 9 (the **last** of 10 bones) → `a0.x = 27` → `c36` = `c9 + 3×9` ✔ |
| `mesh\bd010101.msh` | `infl=4, palettes=16, stride=48` | `0x00020300` | bone0=0, bone1=3, bone2=2, bone3=0 |

This is *not* the `UBYTE4` declarator — see the shader-source comment
`// Compensate for lack of UBYTE4 on Geforce3`. (The `.vsh` comment block claims `c0 = {1, power, 0, 1020.01}`;
that is the 4-constants-per-bone variant used by later clients. **For these eight 3-row shaders `765.01` is
the correct value** and the port's `765.01001` is right.)

### 6.2 `m_pVDEffect[0..3]` — created with `vseffect1..4` (never used)

| Index | Source | Elements | Stride |
|---|---|---|---|
| 0 | `DeclWater` `:970-994` | (0, FLOAT3, POSITION), (12, D3DCOLOR, usage 10 idx 0), (16, FLOAT2, TEXCOORD0) | 24 |
| 1 | `DeclEquip` `:996-1020` | (0, FLOAT3, POSITION), (12, FLOAT3, NORMAL), (24, FLOAT2, TEXCOORD0) | 32 |
| 2 | `DeclEquipShadow` `:1022-1034` | (0, FLOAT3, POSITION) | 12 |
| 3 | `DeclBlur` `:1036-1054` | (0, FLOAT3, POSITION), (12, FLOAT2, TEXCOORD0) | 20 |

The remaining 15 declarations (`DeclMesh`, `DeclSwing`, `DeclEfMesh`, `DeclBumpEquip`, `DeclEnvMesh`,
`DeclBumpMesh`, `DeclUV2Mesh`, `DeclWaterFallMesh`, `DeclLODMesh`, `DeclNormalMesh`, `DeclSpecNormalMesh`,
`DeclSpecWaterFall2Mesh`, `DeclSpecWater`, `DeclChannel2Alpha`, `DeclRefectWater`, `RenderDevice.cpp:1056-1474`)
are **built and then discarded** — `VertexDeclEff[]` is only used for `j = 0..3`
(`[OBSERVED]` `:1517`). They are vestigial.

---

## 7. Shader constant registers

Upload sites (complete list, `[OBSERVED]` grep `VertexShaderConstant` over `Projects\TMProject\`):

| Site | Register | Value |
|---|---|---|
| `RenderDevice.cpp:967` (end of `InitVertexShader`) | `c2..c5` | `transpose(m_matProj)` |
| `RenderDevice.cpp:1851` (`SetProjectionMatrix`) | `c2..c5` | `transpose(m_matProj)` |
| `RenderDevice.cpp:1689` (`SetRenderStateBlock(1)`) | `c1` | light direction (see below) |
| `CMesh.cpp:630` (bone loop) | `c(3i+9)..c(3i+11)` | `transpose(boneMatrix[i] * boneOffset[i] * m_matView)`, i = 0..m_numPalette-1 |
| `CMesh.cpp:635` | `c1` | `normalize(-1, 1, 1)` → `(-0.57735, 0.57735, 0.57735, 0)` |
| `CMesh.cpp:640` | `c92..c95` | `transpose(inverse(m_matView))` |
| `CMesh.cpp:696` (mouse-over overlay) | `c(3l+9)` | same bone matrices re-scaled/translated (highlight) |
| `CMesh.cpp:1028` (`SetMaterial`) | `c8` | material **Diffuse** (first 16 bytes of `D3DMATERIAL9`) |
| `CMesh.cpp:1031` (`SetMaterial`) | `c7` | `Ambient*0.25 + Emissive` (D3DXCOLOR ops) |
| `CMesh.cpp:1034` (`SetMaterial`) | `c0` | `(1.0, materials.Power, fProgress, 765.01f)` |
| `CMesh.cpp:1047` (`SetMaterial`) | `c6` | `(1.0, fogEnd, 1/(fogEnd-fogStart), 0.0)` |

> ⚠ **`c92..c95` collision.** `CMesh.cpp:640` writes **4** constants at `c92`, which clobbers `c95`. The
> shader source declares `c95 = FogStart, FogEnd, Distance, 0`. Nothing uploads `c95` specifically, so
> `c95` always holds `transpose(inverse(view))` row 3 = `(0,0,0,1)`.

### 7.1 Register map (consolidated)

| Reg | Format | Meaning | Producer |
|---|---|---|---|
| `c0` | `(1.0, power, time01, 765.01)` | `c0.x` = clamp ceiling for `oD0`; `c0.y` = material `Power` (used by `skinmesh1..4` as the `oT1` UV offset); `c0.z` = time in 0..1, `(serverTime%10000)/10000` (used by `skinmesh5..8` as the `oT1` UV offset); `c0.w` = 765.01 = `3 × 255.01`, converts a normalised blend-index byte into a **3-strided bone constant address** (§6.1) | `CMesh::SetMaterial` |
| `c1` | `float3/4` | light **direction** | `SetRenderStateBlock(1)` then overwritten per mesh with `normalize(-1,1,1)` |
| `c2..c5` | 4×4 | `transpose(projection)` — **already view-projected** (the bone matrices include the view matrix) | `InitVertexShader` / `SetProjectionMatrix` |
| `c6` | `(1.0, fogEnd, 1/(fogEnd-fogStart), 0)` | fog params for `skinmesh5..8`. **`skinmesh1..4` treat `c6` as the camera position** — see §7.4 | `CMesh::SetMaterial` |
| `c7` | RGBA | ambient term = `material.Ambient × (0.25,0.25,0.25,1)` `+ material.Emissive` | `CMesh::SetMaterial` |
| `c8` | RGBA | material diffuse | `CMesh::SetMaterial` |
| `c9 .. c9+3·(numPalette-1)` | 3×4 each | world-view-bone palette: 3 rows of `transpose(bone × offset × view)` per bone | `CMesh::Render` |
| `c88` | RGBA | diffuse for `vseffect3` | **never uploaded** (vseffect never bound) |
| `c89` | xyz | eye position for `vseffect2` | **never uploaded** |
| `c92..c95` | 4×4 | `transpose(inverse(view))` (world←view for normals) | `CMesh.cpp:640` |
| `c95` | `(1, fogEnd, 1/(fogEnd-fogStart), 0)` **intended** | fog for `skinmesh1..4` | **never uploaded** (clobbered by `c92`) |
| `c30` | 4×3 | extra transform for `vseffect2` | never uploaded |
| `c20` | float | UV scroll for `vseffect1` | never uploaded |

`[OBSERVED]` nothing uploads `c30`, `c88`, `c89`, `c20`, or a dedicated `c95`; all of them read as the
device default (all-zero) — irrelevant for `vseffect*` (never bound), **visible** for `c95` in `skinmesh1..4`.

### 7.2 Consequences for the port (flag these)

1. **`skinmesh1..4` fog is wrong in the port.** Those programs compute
   `dist = |c6 - r4|` (i.e. they expect `c6` = camera position) and then
   `oFog = clamp((c95.y - dist) * c95.z, c95.w, c0.x)`. The port uploads *fog parameters* into `c6` and
   nothing into `c95`, so `oFog ≈ 0` for every character → characters are fogged as if at maximum distance
   (i.e. fully blended into the fog colour) whenever `D3DRS_FOGENABLE` is on… except that the VS path
   ignores `D3DRS_FOGENABLE` — fog is applied because the pixel stage blends with `D3DRS_FOGCOLOR` using the
   interpolated `oFog`. **Fix**: either upload `c95 = (1.0, fogEnd, 1/(fogEnd-fogStart), 0)` **and**
   `c6 = camera position (view space, xyz)`, or switch `skinmesh1..4` to the root `shader*.bin` programs
   (which use `c6` as fog parameters, matching the port's `SetMaterial`).
2. **`oT1` UV animation differs by variant**: `skinmesh1..4` add `c0.y` (= material `Power`, usually 0) and
   `skinmesh5..8` add `c0.z` (= the 0..1 time). If the enchant/glow overlay must animate, use the 5..8
   variant or fix `c0.y`.
3. `c1` is re-uploaded per mesh with a constant, so `SetRenderStateBlock(1)`'s camera-relative light
   direction (`RenderDevice.cpp:1674-1687`) is **always overwritten** for skinned meshes. The fixed-function
   lights (`m_light[0..1]`) are what the camera-relative computation actually affects.

### 7.3 The `SetMaterial` math, exactly

`[OBSERVED]` `CMesh.cpp:982-1050`, called from `CMesh::Render` when its `nBright` argument is 0:

```
materials        = m_pParentSkin->m_materials                     // D3DMATERIAL9
ambEmm           = materials.Ambient × (0.25, 0.25, 0.25, 1.0)    // D3DXColorModulate
if (legendType in [116..125] and multiType > 0) {                  // "blessed" items
    Emi          = materials.Emissive;  Emi.rgb = max(Emi.rgb, 0.55); Emi.a = 0.89
    materials.Diffuse.a = 0.89;  materials.Diffuse.rgb = min(rgb, 0.1)
    ambEmm      += Emi
} else {
    ambEmm      += materials.Emissive
}
vConst           = (1.0, materials.Power, (serverTime % 10000)/10000.0, 765.01001)
fogEnd           = m_fFogEnd * (boneAniIndex == 61 ? 1.4 : 1.0)
fogDist          = max(fogEnd - m_fFogStart, 0.01)
c8 = materials.Diffuse      (rgba)
c7 = ambEmm                 (rgba)
c0 = vConst                 (xyzw)
c6 = (1.0, fogEnd, 1/fogDist, 0.0)
```

`D3DXColorModulate` and `D3DXColorAdd` are component-wise; the results are **not** clamped to 1.

### 7.4 `CMesh::Render(int nBright)` — what the flag really means

`[OBSERVED]` `CMesh.cpp:36-55`:

```cpp
int CMesh::Render(int nBright) {
    ...
    if (!nBright) SetMaterial(cAlpha);     // :54-55
```

`nBright` is **not** a brightness value. It is a "this is an extra pass over a mesh whose constants are
already uploaded" flag: when non-zero the whole `SetMaterial` block (c0/c6/c7/c8 + `SetMaterial`) is skipped.
Callers:

* `CFrame.cpp:235` `m_pMesh->Render(0)` — normal path, constants uploaded.
* `CFrame.cpp:206` and `:224` `m_pMesh->Render(1)` — the **"god / blessing glow"** double pass: the same mesh
  is drawn twice more with `D3DRS_WORLDMATRIX(0)` scaled by `owner->m_fScale + 0.05` and a material colour
  that ramps (`CFrame.cpp:185-231`), then `SRCBLEND=ONE / DESTBLEND=INVSRCALPHA` if
  `m_cDefaultAlpha == 1`.
* `CMesh::Render` also calls `RenderMesh(cAlpha)` which contains a `m_pMesh->Render(bMulti, 0)` for
  weapon-part overrides (`CMesh.cpp:573-589`), where `bMulti = (m_sMultiType > 0)`; that `TMMesh::Render`
  overload is the fixed-function mesh path, not `CMesh::Render`.

**Brightness in the user-visible sense** is `RenderDevice::m_nBright` (static, 0..100, default 50, set from
`Config.bright` at `NewApp.cpp:220-242`) and it is applied **only** through the gamma ramp (§9 of the
other document) and through UI text colour offsets (`TMFont2.cpp:37-39`, `SControl.cpp:788-792`,
`SControl.cpp:1683-1687`, formula: if `m_nBright > 58`, subtract `m_nBright - 40` from each of R,G,B).
`[OBSERVED]` — there is **no** per-object brightness scalar anywhere in the shader path.

---

## 8. Skinned-mesh vertex shaders `skinmesh1..8`

`vs_1_1`, all `[OBSERVED]` from the bytecode (`evidence/hex/disasm/skinmesh*.txt`). Structure is identical
across all eight; the differences are (a) how many `dcl` attributes, (b) how many bone influences are
blended, (c) which constant holds the fog parameters, (d) which constant feeds the `oT1` UV offset.

### 8.1 Common prologue

```hlsl
// c0 = (1.0, power, time01, 765.01)   c1 = lightdir
// c2..c5 = transpose(proj)            c9.. = 3 rows per bone of transpose(bone*offset*view)
mul  r1, v2.zyxw, c0.wwww     ; r1 = (3*bone0, 3*bone1, 3*bone2, 3*bone3) — constant addresses
mov  a0.x, r1.x               ; address of bone #0 = 3 × blue byte of the packed indices
m4x3 r4.xyz, v0, c[a0.x + 9]  ; position into view space
m3x3 r5.xyz, v3, c[a0.x + 9]  ; normal  into view space
mov  r4.w, c0.xxxx            ; w = 1.0
m4x4 oPos, r4, c2             ; clip position (c2 = *projection*)
dp3  r5.w, r5, r5
rsq  r5.w, r5.w
mul  r5, r5, r5.wwww          ; normalize view-space normal
```

### 8.2 Per-variant differences

| Variant | `dcl` list | Bone influences | Lighting | Fog | `oT1` offset |
|---|---|---|---|---|---|
| `skinmesh1` | position v0, blendindices v2, normal v3, texcoord0 v4 (4 dcls) | 1 | view-space normal · `c1` | `c95` + `c6` as camera pos | `c0.y` |
| `skinmesh2` | + blendweight v1 (5) | 2 (`v1.x`, `1-v1.x`; `dp3 r0.w, v1.xyzz, c0.xzzz` makes the "last weight") | world space: `m3x3 r2, r5, c92`, renormalise, · `c1` | `c6` | `c0.z` |
| `skinmesh3` | 5 | 3 (`v1.x`, `v1.y`, last) | world space | `c6` | `c0.z` |
| `skinmesh4` | 5 | 4 (`v1.x`,`.y`,`.z`, last) | world space | `c6` | `c0.z` |
| `skinmesh5` | 5 | 1 | world space | `c6` | `c0.z` |
| `skinmesh6` | 5 | 2 | world space | `c6` | `c0.z` |
| `skinmesh7` | 5 | 3 | world space | `c6` | `c0.z` |
| `skinmesh8` | 5 | 4 | world space | `c6` | `c0.z` |

So: **`skinmesh1..4` = "1..4 influences, view-space lighting, `c95` fog, `Power`-offset UV"**;
**`skinmesh5..8` = "1..4 influences, world-space lighting (via `c92`), `c6` fog, time-offset UV"**.

### 8.3 Reconstructed source — `skinmesh1` (full)

```hlsl
vs_1_1
dcl_position     v0
dcl_blendindices v2      // D3DCOLOR, 4 packed bone indices
dcl_normal       v3
dcl_texcoord0    v4

mul  r1, v2.zyxw, c0.wwww
mov  a0.x, r1.xxxx
m4x3 r4.xyz, v0, c[a0.x+9]
m3x3 r5.xyz, v3, c[a0.x+9]
mov  r4.w, c0.xxxx
m4x4 oPos, r4, c2

dp3  r5.w, r5, r5
rsq  r5.w, r5.wwww
mul  r5, r5, r5.wwww

dp3  r1.x, r5, c1          // N·L  (view space)
lit  r1, r1                // r1.y = max(N·L,0)
mul  r0, r1.yyyy, c8       // × material diffuse
add  r0, r0, c7            // + ambient/emissive
min  oD0, r0, c0.xxxx      // clamp to 1.0
mov  oD1, c0.zzzz          // specular = time01  (effectively unused: SPECULARENABLE=0)

// fog  (NOTE: expects c6 = camera position, c95 = (1, fogEnd, 1/(end-start), 0))
add  r6, c6, r4_neg         // camPos - viewPos
dp3  r6.w, r6.xyzz, r6.xyzz
rsq  r6.x, r6.wwww
mul  r6.x, r6.xxxx, r6.wwww  // r6.x = distance to camera
add  r6.y, c95.yyyy, r6.xxxx_neg  // fogEnd - dist
mul  r6.z, r6.yyyy, c95.zzzz      // * 1/(end-start)
max  r6.z, r6.zzzz, c95.wwww      // >= 0
min  r6.z, r6.zzzz, c0.xxxx       // <= 1
mov  oFog, r6.zzzz

mov  oT0, v4
add  r4.x, r5.xxxx, c0.yyyy       // UV2 = normal.xy + material Power
add  r4.y, r4.yyyy, c0.yyyy
mov  oT1, r4
```

### 8.4 Reconstructed source — `skinmesh5` (full; `6/7/8` differ only in the influence loop)

```hlsl
vs_1_1
dcl_position     v0
dcl_blendweight  v1
dcl_blendindices v2
dcl_normal       v3
dcl_texcoord0    v4

mul  r1, v2.zyxw, c0.wwww
mov  a0.x, r1.xxxx
m4x3 r4.xyz, v0, c[a0.x+9]
m3x3 r5.xyz, v3, c[a0.x+9]
mov  r4.w, c0.xxxx
m4x4 oPos, r4, c2

dp3  r5.w, r5, r5
rsq  r5.w, r5.wwww
mul  r5, r5, r5.wwww

m3x3 r2.xyz, r5, c92        // view-space normal -> world space
dp3  r2.w, r2, r2
rsq  r2.w, r2.wwww
mul  r2, r2, r2.wwww

dp3  r1.x, r2, c1           // N·L in world space
lit  r1, r1
mov  r0, c7                 // ambient
add  r0, r0, c8             // + diffuse      (note the order swap vs skinmesh1)
min  oD0, r0, c0.xxxx
mov  oD1, c0.yyyy           // specular = material Power

// fog
add  r0.x, r4.zzzz_neg, c6.yyyy    // fogEnd - viewZ
mul  r0.x, r0.xxxx, c6.zzzz        // / (end-start)
max  r0.x, r0.xxxx, c6.wwww        // >= 0
min  oFog, r0.xxxx, c6.xxxx        // <= 1

add  r4.x, r5.xxxx, c0.zzzz        // UV2 = normal.xy + time01
add  r4.y, r4.yyyy, c0.zzzz
mov  oT0, v4
mov  oT1, r4
```

### 8.5 Influence loop (variants 2/3/4 and 6/7/8)

`skinmesh4` / `skinmesh8` body replaces the single `m4x3/m3x3` pair with:

```hlsl
// skinmesh4 (4 influences, view-space lighting variant)
mul  r4.xyz, r4.xyzz, v1.xxxx       // first bone contribution × w0
mul  r5.xyz, r5.xyzz, v1.xxxx
mov  a0.x, r1.yyyy                  // bone #1
m4x3 r2.xyz, v0, c[a0.x+9]
m3x3 r3.xyz, v3, c[a0.x+9]
mad  r4.xyz, r2.xyzz, v1.yyyy, r4
mad  r5.xyz, r3.xyzz, v1.yyyy, r5
mov  a0.x, r1.zzzz                  // bone #2
...
mad  r5.xyz, r3.xyzz, v1.zzzz, r5
mov  a0.x, r1.wwww                  // bone #3 (last)
m4x3 r2.xyz, v0, c[a0.x+9]
m3x3 r3.xyz, v3, c[a0.x+9]
mad  r4.xyz, r2.xyzz, r0.wwww, r4   // r0.w = last weight
mad  r5.xyz, r3.xyzz, r0.wwww, r5
m3x3 r2.xyz, r5, c92                // (only in 4/6/7/8 = world-space variants)
...
```

The "last weight" is synthesised from the sum of the others:

| Variant | instruction producing the last weight |
|---|---|
| `skinmesh2` / `skinmesh6` | `dp3 r0.w, v1.xyzz, c0.xzzz` … `add r0.w, r0.wwww_neg, c0.xxxx` |
| `skinmesh3` / `skinmesh7` | `dp3 r0.w, v1.xyzz, c0.xxzz` … |
| `skinmesh4` / `skinmesh8` | `dp3 r0.w, v1.xyzz, c0.xxxx` … |

i.e. `dp3(v1, (1,0,0,1)-ish mask)` sums the used weights (because `v1` is `xyzz`-swizzled, the dot with
`c0.xxxx` sums x+y+z) and then `1 - sum` yields the last. `[OBSERVED]` — matches the classic D3D sample
"skinned mesh with 4 weights" trick.

### 8.6 Bone palette range

`[OBSERVED]` the palette starts at `c9` and each bone consumes 3 constants (`CMesh.cpp:630`), so
`numPalette ≤ (96 - 9)/3 = 29` bones is the hard ceiling for `vs_1_1` (96 float4 constants). Nothing in the
code checks it; the mesh file's `numPalette` is trusted (`CMesh.cpp:827`). For a GL/Vulkan port use
uniform arrays / SSBOs and this limit disappears — but **must not** silently exceed the original 29-bone
behaviour when comparing screenshots.

---

## 9. Effect shaders `vseffect1..4` / `pseffect1..6`

> **These are never bound by this port and — by string analysis — never loaded by the original 748 exe.**
> Documented here for completeness; a faithful port can skip them.

### 9.1 `vseffect1..4` (vs_1_1)

`vseffect1` (87 dwords) — projected/reflected water-style transform, "DBUG" path `…\vseffect1.vsh`,
disassembly `evidence/hex/disasm/vseffect1.txt`:

```hlsl
dcl_position v0
dcl_color    v5          // usage 10 in this decoder = COLOR
dcl_texcoord v7
mov  oD1, v5
mov  oD0, v5
mov  r1.xyz, v0.xyzz
mov  r1.w, c13.yyyy
mul  r0, r1.yyyy, c3
mad  r0, r1.xxxx, c2, r0
mad  r0, r1.zzzz, c4, r0
mad  oPos, r1.wwww, c5, r0          // full projection
mul  r0, r1.yyyy, c9
mad  r0, r1.xxxx, c8, r0
mad  r0, r1.zzzz, c10, r0
mad  r1, r1.wwww, c11, r0
rcp  r0.x, r1.wwww                  // projective texture coordinate
mul  r1.x, r1.xxxx, r0.xxxx
mov  oT1.xzw, r1
mul  r0.xzw, r1.yyyy, r0.xxxx
mov  oT1.y, r0.xxzw
add  oT0, v7, c20                   // UV scroll
```

`vseffect2` (101 dwords) — skinned/equip with lighting + specular + fog-less:
`dcl position v0 / normal v3 / texcoord v4`, bone palette at `c9`, `m4x3 r2.xyz, v4, c30` (extra matrix),
`dp3 r1.x, r5, c1` / `lit` / `mul r0, r1.y, c8` / `add r0, r0, c7`, then a *specular-ish*
view-vector term: `add r6.xyz, c89.xyzz, r4.xyzz_neg` (c89 = eye position), normalise, `dp3 r7.x, r5, r6`,
`mul r8.xyz, r7.x, c88.xyzz`, `add r0.xyz, r0, r8`; `mov oD1, c0.zzzz`; `add r6, c6, r4` (dead);
`mov oT0, v4`, `mov oT1, r5` (normal as UV → environment map).
`vseffect3` (21 dwords) — minimal: `dcl position v0`, single-bone `m4x3` from `c9`, project, `mov oD0, c88`.
`vseffect4` (19 dwords) — `dcl position v0 / texcoord v3`, `m4x4 oPos, v0, c4`, `add oT0, v3, c0`,
`add oT1, v3, c1` (two independently scrolled UV sets; typical water/reflection).

### 9.2 `pseffect1..6` (ps_1_1)

All are `ps_1_1` with a maximum of 4 texture registers `t0..t3`. `[OBSERVED]` disassembly in
`evidence/hex/disasm/pseffect*.txt`; cross-checked line-by-line against the `.psh` sources.

| File | Purpose `[INFERRED]` | Instructions (from bytecode) |
|---|---|---|
| `pseffect1` | cube-map/environment ("texm3x3" family) + specular-ish blend | `tex t0; texm3x3pad t1,t0; texm3x3pad t2,t0; texm3x3vspec t3,t0; mul r0,t3,v0; dp3 r1,t0_bx2,v1; mul r0,r0,r1; mov r0.w,v0.w` |
| `pseffect2` | 4-tap average (soft blur / glow) | `def c0,0.25,0.25,0.25,0.25; tex t0..t3; mul r0,t0,c0; mad r0,t1,c0,r0; mad r0,t2,c0,r0; mad r0,t3,c0,r0` |
| `pseffect3` | 3×3 matrix (env) + palette lookup | `def c4,{-1/3,0,1/3,2/3}; def c5,{0,1/3,2/3,1}; def c6,0.75×4; tex t0; texm3x3pad t1,t0_bx2; texm3x3pad t2,t0_bx2; texm3x3tex t3,t0_bx2; sub_sat r0,c3.w,c4; mul_sat r0,r0,c6; sub_sat r1,c3.w,c5; mul_sat r1,r1,c6; sub_sat r1,r0,r1; dp3 r0,t3,r1; mad r0,t3.w,r1.w,r0; mul r0,r0,c2` |
| `pseffect4` | 4-tap "toon/outline" combiner with `cnd` | `tex t0..t3; mul r0,t0,c1; dp3 r0,t0,t0; sub r0,r0,c5; cnd r0,r0.w,r0,c3; mul r0,t0,r0; mad r0,t1,c0,r0; mad r0,t2,c1,r0; mad r0,t3,c2,r0` |
| `pseffect5` | normal-map dot3 + specular | `tex t0; tex t1; dp3_sat r0,t1_bx2,v0_bx2; mad_sat r0,r0,c3,c4; add r0,r0,c6; mul r0,t0,r0; mad r0,t0.w,v1,r0` |
| `pseffect6` | 2-tap glow | `def c0,0.75×4; tex t0; tex t1; mul r0,t0,c0; dp3 r0,r0,r0; mul r0,t0,r0; mad r0,t1,c0,r0` |

Notes:
* `_bx2` on `t0/t1/v0/v1` is encoded as source-modifier code **4** on the parameter token. Under the DX9
  `D3DSPSM_*` enum 4 means `SIGN`; under the DX8-era `ps_1_x` enum it means `BX2`. Which interpretation D3D9
  applies to a `ps_1_1` program is `[UNKNOWN]` (§Confidence #2). Irrelevant while nothing binds them.
* `pseffect3`'s source text (`pseffect3.psh`) says `mul_x4_sat r0, r0, c6`; the bytecode has plain
  `mul_sat` with dest-saturate only (`0x800f0000` → mask xyzw, destmod saturate). The `.psh` was edited
  after the `.bin` was built. **The bytecode is authoritative.**
* `def` (opcode `0x51`) is used by `pseffect2/3/6` to define `c0`, `c4`, `c5`, `c6` **inside the shader**,
  so those constants need no upload — worth copying verbatim into any re-implementation.

---

## 10. Which mesh/effect uses which shader index

`[OBSERVED]` — the *only* shader binding site is `CMesh::RenderMesh` → `CMesh.cpp:604-624`:

```cpp
if (g_pDevice->m_pd3dDevice->SetStreamSource(0, m_pMesh->m_pVB, 0, m_pMesh->m_sizeVertex) < 0) return 0;
if (g_pDevice->m_pd3dDevice->SetIndices(m_pMesh->m_pIB) < 0) return 0;
if (m_numFaceInflunce > 4 || m_numFaceInflunce < 1) return 0;      // :608

if (m_pParentSkin->m_nBoneAniIndex == 61) {                        // :610
    SetVertexDeclaration(m_pVertexDeclaration[m_numFaceInflunce + 3]);   // :612  -> decl 4..7
    SetVertexShader     (m_pVertexShader     [m_numFaceInflunce + 3]);   // :614  -> shader 5..8
} else {
    SetVertexDeclaration(m_pVertexDeclaration[m_numFaceInflunce - 1]);   // :619  -> decl 0..3
    SetVertexShader     (m_pVertexShader     [m_numFaceInflunce - 1]);   // :621  -> shader 1..4
}
```

| `m_numFaceInflunce` | normal pose → shader / decl | `m_nBoneAniIndex == 61` → shader / decl |
|---|---|---|
| 1 | `skinmesh1` / decl 0 (stride 36) | `skinmesh5` / decl 4 = decl 0 (36) |
| 2 | `skinmesh2` / decl 1 (40) | `skinmesh6` / decl 5 = decl 1 (40) |
| 3 | `skinmesh3` / decl 2 (44) | `skinmesh7` / decl 6 = decl 2 (44) |
| 4 | `skinmesh4` / decl 3 (48) | `skinmesh8` / decl 7 = decl 3 (48) |

`m_numFaceInflunce` comes straight from the mesh file (`CMesh::LoadMesh`, `CMesh.cpp:826`) and
`m_numPalette` (bone count) likewise (`:827`); they are independent — the influence count selects the
*vertex format* and the shader variant, the palette count only sizes the constant upload.

After the draw the code unbinds: `SetVertexShader(0)` (`CMesh.cpp:759`) and disables stages 0..2
(`:653-655`), then restores `D3DCULL_CCW` (`:656`).

`vseffect*`/`pseffect*` → **no consumer**. `[OBSERVED]` (exhaustive grep, §1).

`TMEffect*` classes (`TMEffectBillBoard`, `TMEffectBillBoard2/3/4`, `TMEffectMesh`, `TMEffectSkinMesh`,
`TMEffectSWSwing`, `TMEffectSpark`, …) do **not** use vertex or pixel shaders at all: they are
`DrawPrimitiveUP` + `D3DFVF_XYZ|DIFFUSE|TEX1` (322) with the fixed-function pipeline. See
`06-render-states-and-materials.md` §5 for the per-effect blend state.

---

## 11. Fixed-function fallback

There is **no separate "fallback" code path** — the fixed-function path is the *primary* path for every
non-skinned object, and it is also what runs for skinned meshes if the device cannot create `vs_1_1`.

### 11.1 How the choice is made

`[OBSERVED]` `InitVertexShader()`:

```cpp
m_bUseSW = 16;                                                  // :920  D3DUSAGE_SOFTWAREPROCESSING
if (d3dCaps.VertexShaderVersion >= 0xFFFE0101) {                 // :924
    m_bUseSW = 0;
    m_dwBufferUsage = 8;
} else if (m_pVertexShader[0] == nullptr)
    LOG_WRITELOG("VertexShader SW Mode \r\n");                   // :933
if (m_pVertexShader[0] == nullptr) { for (i=0..7) { ...CreateVertexShader... } }   // :936
```

* If the device reports `vs_1_1`, the eight shaders are created and `m_bUseSW = 0`; skinned meshes draw
  through the VS.
* If the device reports **no** vertex-shader support, the loop still runs and `CreateVertexShader` on the
  `vs_1_1` bytecode fails (`E_FAIL` is a `FAILED` hr) → `return 0` → `RenderDevice::Initialize()` fails →
  the app exits. **There is no software-VS fallback that actually renders.** The
  `SetSoftwareVertexProcessing(1)` wrapper at `CMesh.cpp:601-602` / `:756-757` (driven by `m_bUseSW`) exists
  but is only reachable in a state where the shaders already failed to create. `[OBSERVED]` + `[INFERRED]`
  (the inference is only that the code path is unreachable in practice).
* The fixed-function state used for all other geometry is `SetRenderStateBlock(1)` (see the other document,
  §1.2). Its exact, relevant-to-lighting setup is:

```hlsl
D3DRS_LIGHTING          = 1
D3DRS_SPECULARENABLE    = 0
D3DRS_NORMALIZENORMALS  = 1
D3DRS_COLORVERTEX       = 1
D3DRS_AMBIENT           = 0x33FFFFFF
D3DRS_DIFFUSEMATERIALSOURCE   = 1 (MATERIAL)   // set twice, values 1 then 1
D3DRS_SPECULARMATERIALSOURCE  = 2 then 1 (COLOR1 then MATERIAL — last write wins = 1)
D3DRS_SHADEMODE         = 2 (GOURAUD)
D3DRS_CULLMODE          = 3 (CCW)
D3DLIGHT0/1             = see 06-render-states-and-materials.md §3
```

### 11.2 Software vertex processing (if ever needed)

`D3DCREATE_SOFTWARE_VERTEXPROCESSING` vs `MIXED` vs `HARDWARE` is decided by the D3D app framework
(`D3DDevice.cpp:585-595`) from `m_d3dSettings.GetVertexProcessingType()`; `ConfirmDevice` rejects
`D3DCREATE_PUREDEVICE` (`RenderDevice.cpp:522-525`: `return (dwBehavior & 0x40) ? E_FAIL : 0;` where
`0x40` = `D3DCREATE_PUREDEVICE`) because the code needs mixed-mode `SetSoftwareVertexProcessing`.
`[OBSERVED]`

---

## 12. Reimplementation notes (OpenGL / Vulkan)

### 12.1 Decide which bytecode set you are porting

| Choice | Consequence |
|---|---|
| **A. Port `Shader\skinmesh1..8` (what this port runs today)** | You must also fix/duplicate the constant layout: `c95` fog + `c6`=camera-position for 1..4; `c6` fog for 5..8. Reproduce `c92 = transpose(inverse(view))` for the world-space variants. |
| **B. Port root `shader1..8` (what the 748 client ran)** | Slightly simpler: no `dcl` (attribute bindings come from the vertex declaration anyway), one fog convention (`c6`) for all eight, `m4x3 r4` (xyzw) instead of `xyz`. This is the behaviour a pixel-comparison against the real 748 client will match. |

Either way you need only the §8.3 / §8.4 code, ×4 influence variants.

### 12.2 Do not port these

* `vseffect1..4`, `pseffect1..6` — never bound (§1, §9). Deleting them changes nothing on screen.
  **Verify first** with a one-line instrumentation build (`LogRenderState`-style, or log inside `InitPixelShader`)
  that your build never binds them either.
* `m_pVDEffect[]` and 15 of the 19 `Decl*` arrays — created, never used (§6.2).

### 12.3 OpenGL (3.3 core) translation

* **Vertex attributes** — from the declaration table in §6.1, e.g. for `skinmesh2`:
  `layout(location=0) in vec3 aPos; location=1 in float aWeight; location=2 in vec4 aIndices;`
  (the `D3DCOLOR` indices arrive as 4 floats 0..1; if you declare them as `GL_UNSIGNED_BYTE` **normalised**
  the shader's `× c0.w` reproduces the 3-strided constant address exactly, or declare them un-normalised
  integers and index the palette with `aIndices.x` directly — cleaner, and it removes the `765.01` magic).
* **Uniform buffers** — D3D constants map 1:1 to `std140` blocks. Do **not** transpose: D3D9 stores row
  vectors, so `c9..` already are `transpose(bone*offset*view)`; in GLSL use `mat3(uPalette[i])` and
  `uPos * mat` accordingly (row-vector math → `vec4 p = vec4(pos,1.0) * mat3x4;`).
* **`oD0`/`oD1`/`oFog`/`oT0`/`oT1`** → `out vec4 vColor : 0; out vec4 vSpec : 1; out float vFog : 2;
  out vec2 vUV0 : 3; out vec2 vUV1 : 4;`
* **Fixed-function replacement** (the big one): every `D3DTSS_*` combination in this engine is
  `MODULATE`/`ADD`/`ADDSMOOTH`/`MODULATEALPHA_ADDCOLOR`/`DOTPRODUCT3` over up to 8 stages with
  `D3DTA_DIFFUSE`/`CURRENT`/`TEXTURE`/`TFACTOR`. Implement **one** fragment shader with
  `uniform int uColorOp[8], uColorArg1[8], uColorArg2[8]` and a `switch` — that reproduces the whole
  fixed-function set generically (see `06-render-states-and-materials.md` §1 for the value tables).
* **Fog**: D3D9 `D3DFOG_LINEAR` with `D3DRS_FOGSTART/END` in *device space*:
  `f = clamp((fogEnd - dist)/(fogEnd - fogStart), 0, 1)`, `color = mix(fogColor, color, f)` for the FF path.
  The VS path supplies `oFog` directly (see §8.3/§8.4) — if you use the 5..8 variants,
  `f = clamp((fogEnd - viewZ) / (fogEnd - fogStart), 0, 1)` with a **different** fog range than the FF path
  (the VS gets `c6` from `SetMaterial`, the FF path gets `SetRenderStateBlock(1)`'s start/end — see the other
  document §4.3). Reproduce both or images will not match.
* **Blending**: everything is fixed-function blend factors, described in the other document §5.

### 12.4 Vulkan translation

* 8 pipeline objects for `skinmesh*` (or 4 if you drop the `boneAniIndex==61` duplicate variants and instead
  pick lighting convention per-material), one dynamic UBO for `{c0,c1,c2..c5,c6,c7,c8,c92..c95}` plus a
  second UBO/SSBO for the bone palette (`vec4 palette[29*3]`).
* Attribute bindings: `VkVertexInputAttributeDescription` matching §6.1 exactly, including the
  `D3DCOLOR` (`VK_FORMAT_B8G8R8A8_UNORM` for the packed blend indices so the `.zyxw` swizzle order matches).
* Clip-space: D3D `vs_1_1` writes clip space with **z ∈ [0,1]** and no Y flip; Vulkan also uses z ∈ [0,1],
  so only the **Y axis / front-face winding** need flipping (D3D is Y-down NDC, Vulkan Y-down as well;
  D3D's default cull was set to `D3DCULL_CCW` which corresponds to Vulkan `VK_FRONT_FACE_CLOCKWISE` with a
  negative-height viewport). Reproduce the original's `CULLMODE` per object rather than a global swap.
* The `ps_1_1` `def` constants (§9.2) are compile-time literals; nothing to upload.

### 12.5 Regression checks

1. Log/inspect which shaders are bound per frame; expect only `skinmesh1..8`.
2. Compare a character's silhouette under heavy fog with the 748 client. If your port uses `skinmesh1..4`
   *and* `c95` is zero, the character will be fully fogged — that is a bug in this port, not in the assets
   (§7.2).
3. Confirm the bone index comes from the **blue byte** of the packed `D3DCOLOR` (`v2.z`) for influence 0,
   green for 1, red for 2, alpha for 3, and that `a0.x = 3 × index` (palette base `c9`) — see §6.1's
   real-mesh table.
