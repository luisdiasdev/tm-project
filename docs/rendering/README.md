# Rendering — complete specification (WYD 748 client)

This tree documents **every part of the rendering pipeline** of the WYD client as
shipped in `client_748` (English 748, 2007-04) and as implemented in the D3D9
port under `Projects/TMProject`.

The goal is narrow and strict: **a different team must be able to rebuild the
same renderer on OpenGL or Vulkan using only these documents** — without the
C++ source, without the original executable, and without guessing.

Last updated: 2026-11-10

---

## Reading order

| # | Chapter | What it answers |
|---|---|---|
| — | **`10-reimplementation-guide.md`** | **Start here if your job is to write the renderer.** The synthesis: target architecture, matrix conversion, asset pipeline, per-pass pipeline objects, work plan, and the exact list of unknowns to close first. |
| 01 | [`01-overview.md`](01-overview.md) | Class ownership, per-frame call chain, axes/units/handedness, resolution policy, camera, timing, scene-graph-as-draw-order. |
| 02 | [`02-textures.md`](02-textures.md) | `.wyt` / `.wys` / DDS / TGA decoding, the proprietary fourCC tags, palette + colour-key rules, the four texture lists, mip policy, caching/eviction, dynamic textures, guild marks. |
| 03 | [`03-meshes.md`](03-meshes.md) | `.msh` (skinned) and `.msa` (static) containers, `.bon` skeletons, vertex/index formats, FVFs, `MeshList.txt` / `MeshTextureList.bin` / `object.bin`. |
| 04 | [`04-animations.md`](04-animations.md) | `.ani` matrix keyframes, sampling and LERP rules, bone palettes, skinning maths, attachments, model-space conventions. |
| 05 | [`05-shaders.md`](05-shaders.md) | Every shader asset, bytecode versions, disassembly, constant-register map, which shader binds where, and what is dead code. |
| 06 | [`06-render-states-and-materials.md`](06-render-states-and-materials.md) | The four canned state blocks, lighting, fog, blending/alpha test, depth/stencil, culling, swap chain, gamma. |
| 07 | [`07-pipeline-and-order.md`](07-pipeline-and-order.md) | **The ordered pass list**, per-pass states, sorting, culling, matrix upload points, multi-pass/render targets. |
| 08 | [`08-scene-and-world-assets.md`](08-scene-and-world-assets.md) | Terrain `.trn`/`.dat`, `AttributeMap.dat`, tile-texture lookup, sky, water, decoration, world-object placement. |
| 09 | [`09-ui-and-text.md`](09-ui-and-text.md) | 2-D projection, every `RenderRect*` primitive, the control layer, the GDI font atlas, 3-D-projected overlays, z-order. |
| — | [`evidence/`](evidence/) | 143 raw artifacts: `hex/` (88 hexdumps + struct decodes), `hex/disasm/` (18 shader/bytecode listings), `analysis/` (12 whole-asset-set parse proofs), `tools/` + `hex/tools/` (14 read-only scripts that regenerate them). |

---

## Evidence tags

Every factual claim in every chapter carries one of three inline tags. Nothing is
stated without one.

| Tag | Meaning | Required evidence |
|---|---|---|
| `[OBSERVED]` | Read in the source, or measured byte-for-byte in the real asset files, or disassembled from the real binary. | A `file:line`, a file+offset, or a hexdump in `evidence/`. |
| `[INFERRED]` | Deduced from indirect evidence. | The reasoning is stated at the point of use. |
| `[UNKNOWN]` | Not determinable from the artifacts available here. | The exact experiment or artifact that would resolve it is stated. |

Aggregate tag counts across chapters 01–09: **≈ 950 `[OBSERVED]`, ≈ 107 `[INFERRED]`, 43 `[UNKNOWN]`**
(chapter 10 inherits its tags from the others and adds none).
The open questions are also collected in one place in
[`10-reimplementation-guide.md` §7](10-reimplementation-guide.md#7-open-questions-ranked-by-risk-to-a-reimplementation).

Two conventions used throughout:

- **"Port"** = `Projects/TMProject` (this D3D9 codebase). **"748"** = the shipped
  binary `client_748/WYD.exe` (DX8). Where they disagree, both are stated and the
  748 behaviour is flagged, because the port is what is being built toward the
   748 data, not the other way round.
- **"Unit"** = one world unit. One terrain tile is 2 units; one height step is
  0.1 units. See [`01-overview.md` §3.3](01-overview.md#33-units-and-scale).

---

## The sixteen facts that change a port the most

These are the findings that, if missed, produce a renderer that looks wrong for
non-obvious reasons. Each links to its chapter.

1. **The scene graph *is* the render order.** There is no `TMScene::Render`;
   every draw happens through `ObjectManager::RenderObject()` walking the
   `TreeNode` children, and `AddChild` **prepends**, so insertion order is
   reversed. → [07 §2](07-pipeline-and-order.md#2-the-ordered-pass-list)
2. **The `.msh` / `.msa` extensions are inverted** relative to their usual
   meaning: `.msh` = *skinned* character mesh, `.msa` = *static* multi-submesh
   object. → [03](03-meshes.md)
3. **`.wys` is not a DDS file** — it is `"WS10"` + **one space byte** + a DDS
   body. A naive "copy 4 bytes then treat as DDS" loader (which the port does)
   produces `DDS\0` and fails. → [02 §2](02-textures.md)
4. **Two proprietary DDS fourCCs**: `"2z73"` = DXT1, `"7x0s"` = DXT3. DXT5 is
   never used. Only the first tag byte is tested by the client. → [02](02-textures.md)
5. **`.wyt` is `"WT10"` + an uncompressed TGA 2.0**, all files bottom-left
   origin, and **transparency comes from a colour key** (`0xFF000000` → fully
   transparent) because many UI atlases have no alpha channel at all. → [02](02-textures.md)
6. **Blending is `ONE / INVSRCALPHA` (premultiplied), not
   `SRCALPHA / INVSRCALPHA`.** Switching to the "normal" pair visibly changes
   every translucent object. → [06 §1.2](06-render-states-and-materials.md)
7. **The UI is drawn with `D3DFVF_XYZRHW` in raw device pixels.** There is no
   virtual-canvas transform and no letterboxing; layout constants are authored
   against an **800×600** reference and scaled per element by
   `m_fWidthRatio = W/800`. → [09](09-ui-and-text.md), [01 §4](01-overview.md)
8. **Fog is per-vertex linear fog** (`FOGVERTEXMODE = LINEAR`, `RANGEFOGENABLE = 0`)
   driven from the *view-space |z|*, and its colour is the clear colour, which
   is itself the sky colour. → [06 §4](06-render-states-and-materials.md)
9. **The skinned path uses no `D3DTS_WORLD` at all.** Bone matrices are
   pre-multiplied by the view matrix, transposed, and uploaded 3 registers per
   bone from `c9`. → [04 §10](04-animations.md), [05](05-shaders.md)
10. **Only `Shader\skinmesh1..8.bin` are ever bound.** `vseffect*`/`pseffect*`
    are created and never used (`SetPixelShader` does not exist in the project).
    → [05](05-shaders.md)
11. **Model space is Z-up**, and every object gets a `-90°` pitch bias at draw
    time. Y is up in *world* space. → [01 §3](01-overview.md), [04 §10](04-animations.md)
12. **Terrain is re-emitted as 4-vertex strips every frame**, one draw call per
    tile, with a 2×2 detail texture on stage 1 that has **mipmapping disabled**.
    → [08](08-scene-and-world-assets.md)
13. **The font is re-uploaded on every `SetText`**: GDI renders into a 512×512
    top-down DIB, alpha is `blue >> 4`, and the result lands in a 512×64
    `A4R4G4B4` texture. → [09](09-ui-and-text.md)
14. **Alpha test is on globally** with `ALPHAREF = 0xDD` (221/255) in the 3-D
    block, but only `8` in the UI blocks — tree/foliage cutouts depend on it.
    → [06 §1](06-render-states-and-materials.md)
15. **Animated poses are interpolated by element-wise matrix LERP**, not slerp;
    slerp exists only in the 10-quarter cross-fade. Frame time is
    `4 × dwSpeed` ms per frame. → [04](04-animations.md)
16. **There is no stencil and no shadow map.** "Shadows" are blob decals in
    `TMShade`, and `m_nShadowTextureSize` is write-only. → [06 §6](06-render-states-and-materials.md), [07 §5](07-pipeline-and-order.md)

---

## Corrections this work made to earlier notes

Several pre-existing statements in this repository were measured and found to be
wrong. If you read the older docs first, read this list second.

| Earlier claim | Where | Corrected fact |
|---|---|---|
| `MAX_UI_TEXTURE_SET_LIST` is 600. | `TextureManager.h:3`, AGENTS.md | The 748 client's bound is **512** (`memset 0x1000`, check `< 0x200`). The shipped `.txt` only reaches index 261, so nothing breaks today — but the port's 600 is not the client's contract, and its own bound check is off by one. [OBSERVED, 02] |
| The 748 client is DX8, so port the data only. | `port-748/renderer.md` | True for the API, but the 748 exe's *only* shader path is root-level `shader%d.bin`; the port uses `Shader\skinmesh%d.bin`. Both load; they are **different programs**. [05] |
| The virtual UI canvas is 1024×768. | assumed in UI work | **800×600** (`m_fWidthRatio = W/800`, `BASE_ScreenResize = v·W/800`). [OBSERVED, `RenderDevice.cpp:64-65`, `Basedef.cpp:33-35`] |
| `.msh` = static mesh, `.msa` = animated. | common assumption | Inverted. [OBSERVED, `CMesh.cpp:794`, `TMMesh.cpp:481`] |
| Render-target textures (`m_pOriginTexture`, bloom) are part of the frame. | `port-748/renderer.md` | No sampling site exists anywhere in the port; the whole post-process chain is dead. [07 §6] |
| `FieldNNNN.dat` is the terrain heightmap. | common assumption | `.dat` is the **object-placement list**; the heightmap is in `.trn`. [OBSERVED, 08] |

---

## Bugs in the current port, found while documenting

These were discovered by reading the code against the real 748 data. They are
listed here because they are latent rendering defects, not just documentation
trivia. None were fixed as part of writing these documents — the source tree was
left untouched.

| # | Location | Defect | Effect |
|---|---|---|---|
| 1 | `TextureManager.cpp:480` | `GetUITextureSet` rejects `nIndex > MAX_UI_TEXTURE_SET_LIST` but accepts `nIndex == MAX`, indexing one past the 600-element array | Off-by-one read of `m_UITextureSetList`. [02] |
| 2 | `TextureManager.h:3` | `MAX_UI_TEXTURE_SET_LIST = 600` vs the client's 512 | Diverges from the client contract. Harmless for the shipped data (the largest `SetIndex` in `UITextureSetList.txt` is **261**), so this is a latent, not an active, defect. [02] |
| 3 | `CMesh.cpp:640` | 4-wide constant upload at `c92` clobbers `c95`, which `skinmesh1..4` read for fog | Garbage fog on 1–4-bone meshes; the 5..8 variants read `c6` and are fine. [05 §…] |
| 4 | `Shader\skinmesh1..4` source | `oT1` offset uses `c0.y` (material power) where 5..8 use `c0.z` (time) | UV scroll on the first four shaders scrolls with the wrong value. [05] |
| 5 | `TMFont2::SetText` | Any string longer than 84 bytes split into 3+ chunks, at a width other than 640/800/1024/1280, writes **75 rows into a 64-row texture** | Heap-adjacent texture corruption / visual glitch on long tooltips. [09] |
| 6 | `SControlContainer` | UI z-layer counter is never decremented for one branch | Controls deeper than layer 29 are silently dropped. [09] |
| 7 | `CMesh::Render(int)` | Parameter is a "skip material upload" flag, not brightness | Calling code that passes brightness has no effect. [06 §3.5] |
| 8 | `CMesh::SetMaterial(char)` | Ignores its `cAlpha` argument | Per-face alpha classification never reaches the state. [03] |
| 9 | `MeshManager` | `m_sAnimationArray[…][60][…]` writes out of bounds | Silent memory corruption when skin type ≥ 60. [04] |
| 10 | `TMFont3.cpp:208-210` | Calls state block 2 then immediately block 1 | Block-2 settings (POINT sampling) are discarded before the draw. [06 §1.5] |

---

## What is *not* in this tree

- **Gameplay, networking, packets.** Only rendering.
- **Audio.** `AniSound4.txt` and the animation→sound tables are mentioned in
  [04](04-animations.md) only because they are stored alongside animation data.
- **The `.pane`/`.wyp` UI widget system.** That is a *different, newer* UI
  system documented separately in [`../wyd-pane/`](../wyd-pane/README.md). The
  748 client uses the older `.bin` scene + `SControl` model documented here.

## How to reproduce the measurements

All evidence is regenerable. Every parse proof in `evidence/` was produced by a
read-only PowerShell or Python script over the shipped assets; the scripts are
committed next to their outputs under `evidence/tools/` and
`evidence/hex/tools/`. Point them at your own copy of `client_748` and they will
reproduce every size-identity check quoted in the chapters (e.g. "657/657 `.msh`
files satisfy `32 + 64n + 4n + stride·verts + 2·indices == filesize`").

## Verification status of this tree

These chapters were produced by seven parallel analyses, so the most
consequential claims were re-checked line-by-line against the source after the
fact. That pass **corrected three published claims**:

| Was claimed | Reality | Where it was fixed |
|---|---|---|
| The port's `.wys` loader skips 4 bytes and therefore never loads `.wys`. | It skips **1** byte and is correct; the `- 4` two lines earlier belongs to the `.wyt`/TGA branch. All four loaders are correct. | [02 §3.1](02-textures.md) |
| The port's `600`-entry texture-set array reads 88 garbage sets. | The shipped `UITextureSetList.txt` never exceeds index **261**; the real defect is an off-by-one bound check at `TextureManager.cpp:480`. | [02](02-textures.md), bug list above |
| The world is lit by `normalize(-10,10,-6)` and `normalize(10,-14,6)`. | Those are constructor values; `SetRenderStateBlock(1)` overwrites them **every frame** with `(1,-1,-1)` and `(0,0,1)`, and skinned meshes get a third vector, `normalize(-1,1,1)`. | [08 §5.4](08-scene-and-world-assets.md), [06 §3](06-render-states-and-materials.md) |

Claims that were independently confirmed against the source during that pass:
the absence of any `SetPixelShader` call, the single skinning-shader bind site
(`CMesh.cpp:612-622`), `CMesh::Render`'s parameter being a skip-material flag
(`:54-55`), `CULLMODE = NONE` for skinned draws (`:647-648`), `ONE / INVSRCALPHA`
with per-vertex `|z|` linear fog and stage-1 mipmapping disabled
(`RenderDevice.cpp:1721-1757`), and the 800×600 layout base
(`RenderDevice.cpp:64-65`).

Chapters 03, 04, 05, 09 and the `.trn`/`.dat` findings in 08 rest on exhaustive
whole-asset-set arithmetic proofs rather than sampling, and those proofs are
committed under `evidence/` — they are the safest claims in the tree. The
`[UNKNOWN]` items are labelled at their sites and consolidated in
[`10-reimplementation-guide.md` §7](10-reimplementation-guide.md#7-open-questions-ranked-by-risk-to-a-reimplementation).
