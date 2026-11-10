# 10 — Reimplementation guide (OpenGL / Vulkan)

Purpose: turn chapters 01–09 into an executable plan for rebuilding this renderer on
OpenGL 4.5 or Vulkan 1.1 — what to port 1:1, what to replace, in what order, and how to
prove you got it right.

Last updated: 2026-11-10

This chapter adds no new reverse-engineering. Every claim here is a restatement of a
tagged claim in 01–09; the tag on each line is inherited from the chapter it cites.

---

## Confidence summary

| Tag | Count | Note |
|---|---|---|
| `[OBSERVED]` | inherit | Every statement here cites a chapter where it is tagged. |
| `[INFERRED]` | inherit | Strategy/ordering advice is `[INFERRED]` by nature and marked as such. |
| `[UNKNOWN]` | 11 | Consolidated in [§7](#7-open-questions-ranked-by-risk-to-a-reimplementation). |

---

## Table of contents

1. [Strategy: what to keep, what to replace](#1-strategy-what-to-keep-what-to-replace)
2. [Target architecture](#2-target-architecture)
3. [Math conversion — the highest-risk section](#3-math-conversion--the-highest-risk-section)
4. [Asset pipeline](#4-asset-pipeline)
5. [Pipeline objects, one per pass](#5-pipeline-objects-one-per-pass)
6. [Shader plan](#6-shader-plan)
7. [Open questions ranked by risk](#7-open-questions-ranked-by-risk-to-a-reimplementation)
8. [Work plan with gates](#8-work-plan-with-gates)
9. [Parity test harness](#9-parity-test-harness)
10. [Responsibility map: which chapter owns which file](#10-responsibility-map-which-chapter-owns-which-file)

---

## 1. Strategy: what to keep, what to replace

The single most important decision: **the file formats and the frame's visual
semantics must be reproduced exactly; the API layer must be thrown away.**

`[INFERRED]` — justified by how the code is layered. Almost every visual constant lives
in *data* or in *state blocks*, not in API behaviour, so a clean API rewrite that replays
the same state blocks and the same draw order will match.

### Keep 1:1 (do not "improve")

| Thing | Why | Chapter |
|---|---|---|
| The pass order | It is the only thing defining occlusion and transparency composition. | [07 §2](07-pipeline-and-order.md#2-the-ordered-pass-list) |
| `ONE / INVSRCALPHA` blending for 3-D | Premultiplied-alpha convention baked into every texture. | [06 §1.2](06-render-states-and-materials.md#12-the-main-3-d-block--setrenderstateblock1-renderdevicecpp1632-1759) |
| Alpha-test reference `0xDD` for 3-D, `8` for UI | Foliage cutouts and icon edges depend on the exact threshold. | [06 §1](06-render-states-and-materials.md#1-the-two-state-entry-points) |
| Per-vertex linear fog, `|z|` distance | Fragment-distance fog looks different at grazing angles. | [06 §4.4](06-render-states-and-materials.md#44-the-two-competing-fog-formulas-must-both-be-reproduced) |
| `FOV = 45°`, near `0.966`, far `70`/`90` | Camera feel and Z-precision. | [01 §3.3](01-overview.md#33-units-and-scale) |
| Left-handed projection, `z ∈ [0,1]` | Every matrix, every bone palette, the fog formula. | [01 §3.2](01-overview.md#32-handedness-and-projection) |
| The `-90°` pitch bias on objects | Model space is Z-up. | [01 §3.4](01-overview.md#34-matrix-convention-row-vector-world--rst-with-identity-parent) |
| Terrain tile size `2.0`, height step `0.1`, page `64×64` | World scale. | [01 §3.3](01-overview.md#33-units-and-scale) |
| Element-wise matrix LERP for poses | Slerp produces visibly different in-betweens. | [04 §7](04-animations.md) |
| UI at raw device pixels, `800×600` layout base | Panel positions and font sizes are authored against it. | [09 §2](09-ui-and-text.md) |
| Font atlas `A4R4G4B4`, `alpha = blue>>4` | The text look is a direct consequence. | [09 §6](09-ui-and-text.md) |

### Replace outright

| Retired thing | Replacement | Reason |
|---|---|---|
| D3D9 fixed-function lighting (`D3DRS_LIGHTING`) | One forward shader with the two directional lights passed as uniforms. | [06 §3](06-render-states-and-materials.md#3-lighting) — only **two** lights and no specular; trivial to reproduce. |
| D3D9 texture-stage state machine | Written-out shader math per pass. | [06 §1.2](06-render-states-and-materials.md) rows 26–40 are the entire stage configuration. |
| `vs_1_1`/`ps_1_1` bytecode in `Shader\*.bin` | A modern skinning vertex shader (UBO/SSBO palette) + trivial pixel shader. | [05](05-shaders.md) — the shipped shaders are 8 nearly-identical vs_1_1 programs; nothing is gained by emulating vs_1_1. |
| `D3DXSprite` / `DrawPrimitiveUP` for UI | One persistent dynamic vertex buffer + one batched draw per texture. | [09 §4](09-ui-and-text.md). **Caveat:** this changes nothing visually only if you reproduce the un-flagged `ID3DXSprite::Begin()` state — see [§7](#7-open-questions-ranked-by-risk-to-a-reimplementation) Q3. |
| GDI font rendering | Pre-baked glyph atlas, or keep FreeType/GDI on a loader thread. | [09 §6](09-ui-and-text.md) — the current code re-uploads a 512×64 texture on every `SetText`, which is a per-frame stall you must not copy. |
| `m_dwRenderStateList` etc. caches | Pipeline objects / pipeline cache. | [06 §2](06-render-states-and-materials.md#2-state-caching-arrays). |
| The entire render-target/post-process chain | Nothing (delete it). | [07 §6](07-pipeline-and-order.md#6-multi-pass-rendering-and-render-targets) — no sampling site exists. |

### Delete, do not port

`[OBSERVED]`/`[INFERRED]` from [05](05-shaders.md) and [06 §11.8](06-render-states-and-materials.md):

- `vseffect1..4.bin`, `pseffect1..6.bin` — created at init, **never bound**.
- Root-level `shader1..8.bin` — the 748 exe's variant of the same family; unused by the port.
- `JBlur` — empty stub.
- `m_pOriginTexture` / `m_pAccumTexture` / `m_pBloom*` — write-only.
- `m_pDXFont` / `m_pDXFontLarge` — vestigial.
- `RenderRectProgress2` — dead.
- Three of the `RenderRectTex2*` variants — dead and internally broken.
- `m_god2*` fields — never read by the shader path.
- `m_nShadowTextureSize`, `m_bAlwaysOnTop`, `nZ` — write-only.

---

## 2. Target architecture

`[INFERRED]` — a shape that fits the findings. It is deliberately close to the original
decomposition so that chapter references map onto modules.

```
Platform layer
  window + swapchain (1 colour target, 1 depth target)
  ── D3D9: BackBufferCount=1, DISCARD, EnableAutoDepthStencil, D16+DISCARD,
     no vsync when windowed, LOCKABLE_BACKBUFFER
     [06 §8.1]

Frame
  begin:  clear(colour = sky colour, depth = 1.0)   [07 §1]
  pass 1  dropped items / gates        (pipeline P1)
  pass 2  sky                          (pipeline P2)
  pass 3  characters (skinned)         (pipeline P3)
  pass 4  terrain                      (pipeline P4, per-tile draws)
  pass 5  static objects / water / vegetation (P1 or P5 with stage 1)
  pass 6  blob shadows / decals        (pipeline P6, z-write off)
  pass 7  effects / particles          (pipeline P7, additive variants)
  pass 8  3-D-anchored text            (pipeline P8, font atlas)
  pass 9  2-D UI                       (pipeline P9, XYZRHW-equivalent)
  pass 10 FPS overlay (debug)          (P9)
  end:    present

Subsystems
  TextureStore      ← chapter 02   (decode, own GPU textures, LRU)
  MeshStore         ← chapter 03   (vertex/index buffers, per-mesh texture)
  AnimStore         ← chapter 04   (keyframes, pose sampling, palettes)
  WorldData         ← chapter 08   (.trn/.dat/tiles/sky/water)
  UiRenderer        ← chapter 09   (quads, font atlas, z-bucket order)
  StateMachine      ← chapter 06   (the four state blocks, as PSOs)
```

**One structural change is mandatory.** The original has no scene-level `Render()`
([01 §1](01-overview.md), [07 §2.4](07-pipeline-and-order.md)) — draw order falls out of a
prepended linked list. Do **not** copy that. Instead, make the pass list explicit and
data-driven, but keep the *result* identical: the ordering rules in
[07 §2](07-pipeline-and-order.md) are the specification, the tree walk is just how the
original happened to implement them.

---

## 3. Math conversion — the highest-risk section

`[OBSERVED]` the original is D3D9: row-vector, left-handed, `z ∈ [0,1]`, and matrices are
uploaded **transposed** into shader constants. `[INFERRED]` the conversion below.

### 3.1 Conventions to adopt

| Aspect | Original | Your renderer |
|---|---|---|
| Vector convention | row `v' = v·W·V·P` | column `v' = P·V·W·v` |
| Matrix memory layout | row-major, uploaded transposed for shaders | column-major (GL native) or explicitly transposed for Vulkan |
| Handedness | left-handed | **keep left-handed**, use `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)` or Vulkan's native `[0,1]` depth |
| Depth range | `[0,1]` | identical — Vulkan needs no change; **GL must not use the default `[-1,1]`** |
| Front face | `D3DRS_CULLMODE = CCW` (cull CCW, keep CW) | `frontFace = CLOCKWISE`, `cullFace = BACK` |
| Texture V axis | D3D `v` down from top-left, and `.wyt` TGAs are bottom-left origin | see below |

**The transpose rule:** take the D3D matrix as the engine builds it, transpose once, and
use it as a column-major `mat4`. Do not also flip Z. Do not also negate the view. The
`-90°` pitch bias must stay in the model matrix, not be folded into the projection.
`[OBSERVED] 01 §3.5`

### 3.2 The one trap that will cost a day

Skinned characters never touch `D3DTS_WORLD`. The bone palette is
`transpose(bone · offset · view)` and is uploaded **3 float4 per bone starting at `c9`**,
with the projection already inside the palette and the projection matrix separately at
`c2..c5`. `[OBSERVED] 04 §10, 05` If you build a conventional `model · skin · VP`
pipeline you must remove exactly one view multiplication, or characters will be
double-transformed. Verify by loading `ch010103` in the T-pose and comparing against a
static `.msa` object at the same world position.

### 3.3 The `-90°` bias and Z-up models

`[OBSERVED] 01 §3.4, 04 §10`: object model space is **Z-up**; world space is **Y-up**.
The bias is applied as `D3DXMatrixRotationYawPitchRoll(yaw, pitch - 90°, roll)` and then
composed `W = R · S · T`. Ground vertices are built directly as `(tileX, height, tileY)`
and scaled by `(2.0, 0.1, 2.0)` — terrain needs **no** bias. `[OBSERVED] 01 §3.1`

### 3.4 Fog

`[OBSERVED] 06 §4.4`: two formulas coexist.

- **Fixed-function path** (most geometry): per-vertex, `|z|` in view space,
  `f = clamp((|z| - start) / (end - start), 0, 1)`, colour = `m_dwClearColor`,
  with `fogStart = m_fFogStart + sightLength - 8`, `fogEnd = m_fFogEnd + sightLength - 15`.
- **Vertex-shader path** (skinned meshes): `c6 = (1.0, fogEnd, 1/(fogEnd-fogStart), 0)`.
  Note `skinmesh1..4` read `c95` instead of `c6` and `c95` is never uploaded — so in the
  *port*, 1–4-bone meshes effectively have broken fog. Decide deliberately whether to
  reproduce the bug or use `c6` for all eight; **document which you chose**. `[OBSERVED] 05`

`[INFERRED]` Reproduce the fixed-function formula in the fragment shader using the
interpolated `|view.z|`, not `length(viewPos)` — `RANGEFOGENABLE = 0` is explicit.

### 3.5 UI has no matrix

`[OBSERVED] 09 §2, 01 §3.5`: UI vertices are `D3DFVF_XYZRHW` — pre-transformed, in device
pixels, `z` around `0.01`. Draw them with an identity/orthographic replacement and a
pixel-to-NDC transform of `x_ndc = 2x/W - 1`, `y_ndc = 1 - 2y/H`. The `-0.5` texel
convention is already baked into the source's UV arithmetic — see [09 §3](09-ui-and-text.md).

---

## 4. Asset pipeline

You have two viable strategies. Both are legitimate; pick per asset type.

### 4.1 Recommended: offline conversion, runtime loads a modern format

`[INFERRED]` — cheapest to get right, because the tricky decode work happens once and is
then frozen, and the runtime never has to care about `DXT3` ordering quirks.

| Source | Convert to | Notes |
|---|---|---|
| `.wys` (`"WS10 "` + DDS) | KTX2 / raw DDS | Strip the 5-byte prefix, patch `"DDS"` into the magic, keep the `"2z73"`→`DXT1` / `"7x0s"`→`DXT3` mapping. `[02]` |
| `.wyt` (`"WT10"` + TGA 2.0) | PNG/KTX2 | **Compose the colour key yourself**: `0xFF000000` → alpha 0. Do not rely on a converter honouring the origin bit. `[02]` |
| `.msh` | your own buffer layout | Skinned; keep the packed palette-index DWORD and the `n-1` stored weights (last = `1 - Σ`). `[03]` |
| `.msa` | your own buffer layout | Static multi-submesh; note `FaceCount` is **triangles**, not indices. `[03]` |
| `.ani` | half-float or float keyframe matrices | `{frames, bones}` + `frames·bones` 64-byte matrices; row 3 is the translation. `[04]` |
| `.bon` | 8-byte `{parent, id}` pairs, or a plain array | Literally nothing else in the file. `[03]` |
| `FieldNNNN.trn` | heightmap + per-tile table | 8-byte header + 4096 × 12-byte tiles. `[08]` |
| `FieldNNNN.dat` | placement list | 28 B/record, +8 B (`fScaleH/V`) for objType `501..506 ∪ 511..599`. `[08]` |
| `object.bin` | decrypted 2048×16×16 mask | Pad-cipher with the 170-byte key; checksum is over the **encrypted** bytes. `[03]` |

### 4.2 Alternative: runtime decode

`[INFERRED]` — choose this only if you must ship without a conversion step (e.g. you are
keeping the original asset distribution). Then implement [02](02-textures.md) verbatim,
including the 5-byte `.wys` prefix, and treat the `"2z73"`/`"7x0s"` tags as
single-byte tests (`buf[0x54] == '2'`) exactly as the client does.

### 4.3 Texture index spaces — do not flatten them

`[OBSERVED] 02`: four independent index spaces with fixed 264-byte record arrays:

| Space | Count | List file |
|---|---|---|
| UI | 512 | `UI\UITextureList.bin` |
| Effect | 512 | `Effect\EffectTextureList.bin` |
| Model | 2048 | `Mesh\MeshTextureList.bin` |
| Env | 512 | `Env\EnvTextureList3.bin` |

Plus 512 UI **texture sets** (`UI\UITextureSetList.txt`, `ControlTextureCoord`) and 72
dynamic slots. Material references in `.msa` files are **names**, resolved through
`MeshTextureList.bin`, and the lookup is **extension-agnostic** — a `.msa` that says
`sword01.tga` resolves to `sword01.wys`. `[OBSERVED] 03`

### 4.4 Mip and sampler policy

`[OBSERVED] 02, 06 §1.2`: `MIN = LINEAR`, `MAG = LINEAR`, `MIP = LINEAR` for stage 0 in
3-D; **`MIPFILTER = NONE` for stage 1** (this is why the terrain detail layer shimmers and
must keep shimmering). UI block 0 uses `MIP = POINT`, block 2 (font) uses
`MIN/MAG = POINT`. Mip counts differ per class (UI always 1).

---

## 5. Pipeline objects, one per pass

`[OBSERVED]` state values from [06 §1](06-render-states-and-materials.md) and
[07 §7.1](07-pipeline-and-order.md#71-the-four-canned-state-blocks-renderdevicesetrenderstateblock);
the PSO grouping is `[INFERRED]`.

| PSO | Passes | Depth test | Depth write | Blend | Alpha test | Cull | Notes |
|---|---|---|---|---|---|---|---|
| **P1 world opaque** | 1, 5 (most), 3-adjacent statics | on, `LESS_EQUAL` | on | `ONE / INV_SRC_ALPHA` | ref `0xDD`, `GREATER_EQUAL` | back, front-face CW | The "main 3-D block" (`SetRenderStateBlock(1)`). |
| **P2 sky** | 2 | on | on | `ADD_SIGNED` / `BLEND_DIFFUSE_ALPHA` variants | as P1 | as P1 | Lighting **off**, fog **off**; 2 texture stages; camera-locked. `[08]` |
| **P3 skinned** | 3 | on | on | as P1 | as P1 | **NONE — two-sided** | `CMesh.cpp:647-648` sets `CULLMODE=NONE` unconditionally after the per-mesh `CW` branch, so characters/hair are never back-face culled. Reproduce that or silhouettes will thin out. `[OBSERVED]` |
| **P4 terrain** | 4 | on | on | as P1 | as P1 | **none observed** | 2 `MODULATE` stages; stage 1 mip off; 1 draw per tile. `[08]` |
| **P5 water** | 5 | on | on | 3 variants (see 08) | as P1 | as P1 | Procedural grid; UV scroll periods 12 s / 18 s. `[08]` |
| **P6 decal** | 6 | on | **off** | `SRC_ALPHA / INV_SRC_ALPHA` | ref `0xDD` | none | Blob shadows; must not z-write or they occlude later particles. `[07 §2.7]` |
| **P7 additive effect** | 7 | **off** (most) | **off** | `ONE / ONE` | ref `0xDD` | none | Fire/spark/glow. `[06 §5.3]` |
| **P7b alpha effect** | 7 | off | off | `SRC_ALPHA / INV_SRC_ALPHA` | ref `0xDD` | none | Smoke/dust. |
| **P8 world text** | 8 | off | off | `SRC_ALPHA / INV_SRC_ALPHA` | ref `8` | none | `TMFont3`; block 2 then block 1 quirk. `[06 §1.5]` |
| **P9 UI** | 9, 10 | **off** | **off** | `SRC_ALPHA / INV_SRC_ALPHA` | ref `8` | back, CW | Block 3. `MULTISAMPLE = 0`. Fog off, lighting off. `[06 §1.4]` |
| **P9b UI transparent fill** | 9 | off | off | blend on per-quad | — | — | `RenderRectNoTex(..., bTrans=1)` toggles blending per quad — with an explicit PSO you need two variants. `[06 §1.3]` |

Notes that will otherwise be missed:

- `SetRenderStateBlock(0)` (used by `RenderGeomRectImage`) enables **depth test** for UI
  quads at `z ≈ 0.01` and disables fog/lighting but **leaves blending off**. Only block 3
  is used for the general UI pass. `[06 §1.3, §1.4]`
- Block 0's stage-0 colour op reads as `SELECTARG1(TEXTURE)`, which would make `dwColor`
  tinting impossible — yet every call site tints. Flagged as a probable transcription
  artifact; if your UI comes out untinted, this is why. `[UNKNOWN] 09 §2`
- There is no stencil anywhere, and the depth format is `D16`. `[06 §6]`

---

## 6. Shader plan

`[OBSERVED] 05`: eight live `vs_1_1` programs, `skinmesh1..8`, differing only in the
number of bone influences (1..4) and a fog-register bug shared by 1..4. Nothing else is
bound. So the whole 3-D vertex-shading surface is one program:

```
vs_main(aPos, aNormal, aUV, aBlendIndex /* packed D3DCOLOR */, aWeight /* n-1 floats */):
    slot  = (aBlendIndex.b * 765.01) / 3          # c0.w, blue byte, 3-strided
    w_n   = 1 - sum(w_i)                          # implicit last weight
    pos   = Σ w_i * (palette[slot_i] * aPos)      # palette already view-projected
    nrm   = Σ w_i * (paletteRot[slot_i] * aNormal)
    oPos  = pos
    oD0   = min(c7 + lit(nrm, c1) * c8, c0.x)     # ambient + diffuse, clamped
    oUV   = aUV + c0.z * scroll                   # scroll: time01, 0..1 sawtooth
    oFog  = linear(|view.z|, c6)
```

`[OBSERVED] 05` constant map: `c0 = (1.0, power, time01, 765.01)`, `c1` = light dir,
`c2..c5` = transposed projection, `c6` = fog, `c7` = ambient,
`c8` = diffuse, `c9 + 3i` = palette, `c92..c95` = `transpose(inverse(view))`.

`[INFERRED]` recommendations:

- **Do not emulate vs_1_1.** Write the above as GLSL/HLSL with a `mat4` palette in a
  UBO (`MAX_BONES` — the largest observed is 35, so 64 is safe) or an SSBO. `[04]`
- Keep `oD0 = min(ambient + N·L·diffuse, 1.0)` exactly — the clamp at `c0.x = 1.0` is a
  visible saturation on bright materials.
- **Light vectors do not have one value.** `[OBSERVED]` `RenderDevice.cpp:1657-1658` overrides
  both `D3DLIGHT9` directions every frame to `(1,-1,-1)` (unnormalised) and `(0,0,1)`;
  the `normalize(-10,10,-6)` / `normalize(10,-14,6)` pair set in the constructor exists only
  before the first frame. `[OBSERVED]`
- **Skinned geometry gets a third vector.** `RenderDevice.cpp:1689` uploads a derived
  `c1` for the fixed-function path, but `CMesh::Render` overwrites `c1` per skinned mesh
  with `normalize(-1,1,1)` (`CMesh.cpp:633-635`). Keep the world and character light
  vectors separate or characters will be lit differently from the terrain.
  `[OBSERVED] 05, 08 §5.4`
- **No specular.** `D3DRS_SPECULARENABLE = 0` and no `oD1` write. `[06 §1.2]`
- Effects (`TMEffect*`) use stage-state programs described in
  [06 §5.3](06-render-states-and-materials.md) — most are `MODULATE` + additive blend,
  which is a `texture * vertexColor` fragment shader.
- Sky: two stages, `ADD_SIGNED` with `BLEND_DIFFUSE_ALPHA` — implement as a two-sampler
  shader; the exact on-screen result of that combo is one of the open questions. `[UNKNOWN] 08`

---

## 7. Open questions ranked by risk to a reimplementation

Ordered by "how wrong will it look if you guess". Each names the experiment that closes it.

| # | Question | Risk | How to close it | Chapter |
|---|---|---|---|---|
| 1 | `cdata.bin` / `.trn` checksum gates: does the shipped exe enforce them? Shipped data cannot satisfy them. | **High** — if enforced, the original never rendered these fields either. | Breakpoint `MessageBoxA("DataFile Error.")` in `WYD.exe`, or disassemble both `fopen("cdata.bin")` sites. | [08](08-scene-and-world-assets.md) |
| 2 | Does the 748 exe ever create a **pixel** shader? Its import table is obfuscated; no `pseffect*` string exists. | **High** — changes whether effects need per-fragment work at all. | apitrace / API Monitor on `d3d8.dll`, watch `CreatePixelShader`. | [05](05-shaders.md) |
| 3 | `ID3DXSprite::Begin()` with **no flags**: which blend/alpha state is actually in force for `RenderRect`? | **High** — every UI quad path. | Force block-2 states before `Begin()` and diff screenshots; or run `LogRenderState` immediately after `Begin()`. | [09 §4](09-ui-and-text.md) |
| 4 | Stage-0 colour op in state block 0: `SELECTARG1(TEXTURE)` vs `MODULATE`. | **High** — UI tinting. | Temporarily change to `MODULATE` and diff; or log the state after `SetRenderStateBlock(0)`. | [09 §2](09-ui-and-text.md) |
| 5 | `D3DXSprite::Draw` rotation-pivot semantics (source-relative vs destination-relative). | **Medium** — rotated UI (minimap diamond, compass). | Single rotated quad A/B test at two resolutions. | [09](09-ui-and-text.md) |
| 6 | Does D3DX flip bottom-up TGAs on load? All `.wyt` are bottom-left origin. | **Medium** — all UI atlases upside down if wrong. | Load one `.wyt` both ways and compare. | [02](02-textures.md) |
| 7 | `skinmesh1..4` fog reads `c95`, which is clobbered by the `c92` upload. Reproduce the bug or fix it? | **Medium** — fog on 1–4-bone meshes. | Render the same character with both and A/B against a 748 screenshot. | [05](05-shaders.md) |
| 8 | Runtime depth-stencil format actually selected (`D16` vs `D24S8`). | **Low** — Z fighting on distant geometry. | Log `m_d3dpp.AutoDepthStencilFormat` at runtime. | [06 §6.1](06-render-states-and-materials.md) |
| 9 | Sky's `ADD_SIGNED`/`BLEND_DIFFUSE_ALPHA` stage-2 result as seen on screen. | **Low** — sky tint. | Compare a sunset-state screenshot against a hand-computed blend. | [08](08-scene-and-world-assets.md) |
| 10 | `object.bin` mask value → ground-height maths (needs signed `char` and `×0.1`). | **Low** — picking/height only, not the render path. | Already partially solved in [03](03-meshes.md); confirm against a known field. | [03](03-meshes.md) |
| 11 | Whether `m_nWidthShift`/`m_nHeightShift` ever letterboxed (always 0 here). | **Low** — treat as "never". | Search the 748 exe for writes; or try a 16:9 mode and compare. | [01 §4.3](01-overview.md) |

---

## 8. Work plan with gates

`[INFERRED]` — sequencing chosen so each gate produces a comparable screenshot.

### Phase 0 — Harness (no rendering)

- Extract the shipped textures to a modern format; **verify by count and by one known
  image** (e.g. `UI\cursor.wyt`). Gate: 221/221 `.wyt` and 2336/2336 `.wys` decode
  without error. `[02]`
- Build a 748 reference corpus: launch `client_748\WYD.exe`, capture login, server
  select, character select, Agate field, one combat frame.

### Phase 1 — Clear colour + UI (biggest visual surface per unit effort)

- Clear to the sky colour, then draw `LoginScene2.bin`'s panels as tinted quads.
- Implement the font atlas path ([09 §6](09-ui-and-text.md)) with the `A4R4G4B4`
  `blue>>4` alpha rule.
- Gate: login screen is pixel-comparable up to the font rasteriser.

### Phase 2 — Terrain + sky

- `.trn` → height grid → 4-vertex strips, 2 units/tile, 0.1 units/height step, one draw
  per tile, stage-1 detail with mips off, per-tile 4-corner screen test, 2-unit→4-unit
  decimation beyond 16–17 tiles. `[08]`
- Sky: camera-locked 2-layer mesh `mesh\sky001.msa` + `mesh\sky01..04.wys`, 2 s crossfade,
  `(t/600000) % 12` day/night step. Clear colour = sky colour. `[08]`
- Gate: standing in Agate at a fixed camera, colours match; horizon has no seam.

### Phase 3 — Static objects and unskinned animation

- `.msa` loader + `MeshList.txt` index space + `MeshTextureList.bin` name resolution.
- `FieldNNNN.dat` placement (28 B records, the `fScaleH/V` extension).
- Rigid `.msa` animation path.
- Gate: houses, gates, trees, dropped items all in the right place with the right scale.

### Phase 4 — Skinned characters

- `.msh` + `.bon` + `.ani`; pose sampler with the `4 × dwSpeed` ms frame clock and
  element-wise matrix LERP; palette as a UBO.
- Reproduce the `min(ambient + N·L·diffuse, 1.0)` clamp and the Z-up→Y-up `-90°` bias.
- Gate: character select shows the same idle pose as the original at t=0, and the same
  walk cycle at t=1 s.

### Phase 5 — Effects, decals, water, weather

- `TMEffect*` family with the additive/alpha PSO split, blob shadows, snow/rain/leaf/dust
  (screen-space and world-space variants), water UV scroll 12 s / 18 s.
- Gate: a combat frame matches on effect brightness and ordering.

### Phase 6 — Parity sweep

- Run [§9](#9-parity-test-harness) across the 11 tiers below; fix remaining deltas.

---

## 9. Parity test harness

`[INFERRED]` — the point is to make each chapter independently falsifiable.

| Tier | Test | Proves |
|---|---|---|
| T1 | Decode all 221 `.wyt` + 2336 `.wys`; hash the RGBA output | [02](02-textures.md) |
| T2 | Size identity on all 657 `.msh`, 1661 `.msa`, 718 `.ani` | [03](03-meshes.md), [04](04-animations.md) |
| T3 | Render one `.msa` with no scene, orthographic, at 1:1; diff against a reference render | [03](03-meshes.md) |
| T4 | Render `ch010103` in the rest pose at `t=0`; diff bone positions against the `.ani` frame 0 matrix dump | [04](04-animations.md) |
| T5 | Clear colour == sky colour for all 16 weather states | [07 §1](07-pipeline-and-order.md) |
| T6 | Draw the login UI at 640×480, 800×600, 1024×768, 1280×1024; panel rects must scale by `W/800`, `H/600` independently | [09](09-ui-and-text.md) |
| T7 | Terrain at a fixed camera: tile count drawn and per-tile decimation threshold | [08](08-scene-and-world-assets.md) |
| T8 | Alpha-test ref: a foliage `.wys` at ref `0xDD` vs `0x80` must differ visibly | [06 §1.2](06-render-states-and-materials.md) |
| T9 | Blend check: a 50 %-alpha object must look *additive-ish*, not `SRCALPHA` | [06 §1.2](06-render-states-and-materials.md) |
| T10 | Fog: object at `z = fogStart` unchanged, at `z = fogEnd` == clear colour, per-vertex not per-fragment | [06 §4.4](06-render-states-and-materials.md) |
| T11 | UI z-order: cursor on top, tooltips above panels, 30 depth buckets | [09 §9](09-ui-and-text.md) |

A capture script already exists in this repo:
`docs/tools/capture_and_click.ps1` (see [`../port-748/build-and-test.md`](../port-748/build-and-test.md)).

---

## 10. Responsibility map: which chapter owns which file

Use this when a question is "where do I look".

| Source file | Chapter |
|---|---|
| `NewApp.cpp` | [01](01-overview.md), [07 §1](07-pipeline-and-order.md) |
| `D3DDevice.cpp/.h`, `D3DEnumeration.cpp`, `D3DSettings.h` | [01](01-overview.md), [06 §8](06-render-states-and-materials.md) |
| `RenderDevice.cpp/.h` | [06](06-render-states-and-materials.md), [07](07-pipeline-and-order.md), [09](09-ui-and-text.md) |
| `TextureManager.cpp/.h` | [02](02-textures.md) |
| `CMesh.cpp`, `TMMesh.cpp`, `MeshManager.cpp` | [03](03-meshes.md) |
| `TMSkinMesh.cpp` | [03](03-meshes.md), [04](04-animations.md) |
| `ObjectManager.cpp`, `TreeNode.cpp` | [01 §7](01-overview.md), [07 §2](07-pipeline-and-order.md) |
| `TMScene.cpp/.h`, `TMFieldScene.cpp` | [01](01-overview.md), [07](07-pipeline-and-order.md) |
| `TMGround.cpp` | [08](08-scene-and-world-assets.md) |
| `TMSky.cpp`, `TMSea.cpp`, `TMHouse.cpp`, `TMGate.cpp`, `TMTree.cpp`, `TMLeaf.cpp` | [08](08-scene-and-world-assets.md) |
| `TMCamera.cpp` | [01 §5](01-overview.md) |
| `TMFont2.cpp`, `TMFont3.cpp`, `SControl*.cpp`, `GeomObject.cpp` | [09](09-ui-and-text.md) |
| `TMEffect*.cpp`, `TMDust.cpp`, `TMRain.cpp`, `TMSnow.cpp`, `TMLight.cpp`, `TMShade.cpp` | [06 §5.3](06-render-states-and-materials.md), [07 §2.8](07-pipeline-and-order.md) |
| `TMHuman.cpp`, `TMObject.cpp`, `TMItem.cpp` | [03](03-meshes.md), [04](04-animations.md), [07 §2](07-pipeline-and-order.md) |
| `TMPaths.h` | every chapter (path constants) |
| `Shader\*.bin` (assets) | [05](05-shaders.md) |

---

## Summary: the shortest correct path

1. Decode the two texture containers correctly (5-byte `.wys` prefix, colour-keyed
   `.wyt`) — this alone unblocks every other phase.
2. Reproduce the four state blocks as explicit PSOs, including `ONE/INVSRCALPHA`.
3. Re-emit the pass list in the order of [07 §2](07-pipeline-and-order.md); do not copy
   the linked-list draw order.
4. Transpose matrices exactly once; keep left-handed and `[0,1]` depth; keep the `-90°`
   bias.
5. Write one modern skinning shader from [05](05-shaders.md)'s constant map; do not
   emulate `vs_1_1`.
6. Draw UI as pixel-space quads against 800×600 with per-element scaling.
7. Close the five High-risk open questions in [§7](#7-open-questions-ranked-by-risk-to-a-reimplementation)
   before you spend time on effects.
