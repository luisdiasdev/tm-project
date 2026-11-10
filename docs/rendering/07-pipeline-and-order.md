# 07 — Frame Pipeline and Draw Order (WYD 748 client port)

Purpose: the exact, ordered list of draw passes a WYD 748 frame issues, the render states of each pass, the culling/sorting rules, and the invariants a reimplementation must preserve for pixel parity.

Last updated: 2026-11-10

## Confidence summary

Tag occurrences in this document (a tag on a table row or paragraph covers every claim in it):

| Tag | Count |
|---|---|
| `[OBSERVED]` | 154 |
| `[INFERRED]` | 12 |
| `[UNKNOWN]` | 2 |

Biggest unknowns:

1. **No scene-level render function exists in the decompiled source.** `TMScene`, `TMFieldScene`, `TMSelectCharScene`, `TMSelectServerScene`, `TMDemoScene` all inherit `TreeNode::Render()` (a no-op returning 1, `TreeNode.cpp:99-102`); grep for `RenderFrame` yields nothing. Every pass below therefore comes from the generic tree walk plus each leaf's `Render()`. Whether the shipped binary had extra scene passes outside that walk cannot be proven from this source alone (see §2.4).
2. Runtime depth-stencil format (16 vs 24-bit) — affects the 0.966…70 Z range's precision.
3. What the original DX8 `JBlur` post-process did (class is an empty stub).
4. Whether `m_pOriginTexture` / `m_pBloomTexture` were ever sampled by a scene render that is missing from this source.
5. Exact per-frame ordering *between* two children of the same container when objects respawn (list is insertion-dependent, i.e. spawn-order dependent).

## Table of contents

1. [Frame boundaries: `Lock` / `Unlock`](#1-frame-boundaries-lock--unlock)
2. [The ordered pass list](#2-the-ordered-pass-list)
   - 2.1 [Pass 0 — frame begin](#pass-0--frame-begin-setviewport--lock1)
   - 2.2 [Pass 1 — dropped items / gates](#pass-1--dropped-items-gates-and-cannons-m_pitemcontainer)
   - 2.3 [Pass 2 — sky](#pass-2--skydome-m_psky)
   - 2.4 [Pass 3 — characters, NPCs, monsters](#pass-3--characters-npcs-monsters-m_phumancontainer)
   - 2.5 [Pass 4 — terrain](#pass-4--terrain-tmground)
   - 2.6 [Pass 5 — static world objects, water, vegetation](#pass-5--static-world-objects-water-vegetation-m_pobjectcontainerlist)
   - 2.7 [Pass 6 — blob shadows and ground decals](#pass-6--shadowsdecals-m_pshadecontainer)
   - 2.8 [Pass 7 — effects and particles](#pass-7--effects-and-particles-m_peffectcontainer)
   - 2.9 [Pass 8 — floating damage text](#pass-8--floating-world-anchored-text-m_pextracontainer)
   - 2.10 [Pass 9 — 2-D UI](#pass-9--2-d-ui-rendercontrol)
   - 2.11 [Pass 10 — FPS overlay / frame end](#pass-10--fps-overlay--frame-end)
   - 2.12 [What is missing here vs. the original](#212-what-is-missing-here-vs-the-original)
3. [Sorting and culling](#3-sorting-and-culling)
4. [Where per-frame matrices are set](#4-where-per-frame-matrices-are-set)
5. [Light and shadow handling](#5-light-and-shadow-handling)
6. [Multi-pass rendering and render targets](#6-multi-pass-rendering-and-render-targets)
7. [State invariants that must be preserved](#7-state-invariants-that-must-be-preserved)
8. [HUD/UI over 3-D, and 3-D-projected text](#8-hudui-over-3-d-and-3-d-projected-text)
9. [Reimplementation notes (OpenGL/Vulkan)](#9-reimplementation-notes-openglvulkan)

---

## 1. Frame boundaries: `Lock` / `Unlock`

### 1.1 `RenderDevice::Lock(int bClear)` — `RenderDevice.cpp:360-408`

| Step | Code | Effect |
|---|---|---|
| 1 | `TestCooperativeLevel` when `m_bDeviceLost` | on `D3DERR_DEVICELOST` return 0 (skip frame); on `D3DERR_DEVICENOTRESET` → `Reset3DEnvironment()` and return. `[OBSERVED] :362-383` |
| 2 | choose `m_dwActualClearColor` | if scene has a sky: dungeon 0/3/4 → `m_dwClearColor`; else by `m_pSky->m_nState`: 0 → `0xFF335599`, 1 → `0xFF333333`, 2 → `0xFF441100`, 3 → `0xFF222222`. `[OBSERVED] :387-399` |
| 3 | `Clear(0, nullptr, D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, m_dwActualClearColor, 1.0f, 0)` | **colour + depth are cleared every frame** (no stencil flag). `[OBSERVED] :401` |
| 4 | `BeginScene()` | the only `BeginScene` in the codebase. `[OBSERVED] :405` |

Notes:
- `m_dwClearColor` is initialised to `0` (black) in the `RenderDevice` ctor (`:47`) and then continuously overwritten by `TMSky::FrameMove`/`SetWeatherState` from the weather table (`TMSky.cpp:481, 606`). So **the clear colour is the sky colour**, which is what makes the fog and the horizon blend. `[OBSERVED]`
- There is no `D3DCLEAR_STENCIL`; the stencil buffer (if any) is unused. `[OBSERVED]`

### 1.2 `RenderDevice::Unlock(int bEnd)` — `RenderDevice.cpp:410-450`

| Step | Code | Effect |
|---|---|---|
| 1 | if `bEnd==1 && m_pFont` | measure FPS over a ≥2 s window into `m_fFPS`; set `m_bShowEffects=1`; if `g_bDebugMsg==1` draw `"FPS: … | Objects: … | EN: … ET: …"` at `(10, screenH-15)`. `[OBSERVED] :412-438` |
| 2 | `EndScene()` | `[OBSERVED] :440` |
| 3 | if `bEnd==1` → `Present(nullptr,nullptr,nullptr,nullptr)` | on `D3DERR_DEVICELOST` set `m_bDeviceLost=1`. **No vsync flag argument** — the interval comes from `m_d3dpp.PresentationInterval` (0 when windowed, `D3DEnumeration`-chosen when fullscreen). `[OBSERVED] :443-447`, `D3DDevice.cpp:284, 294` |

### 1.3 Present parameters and `m_bFull`

- `BackBufferCount = 1`, `SwapEffect = D3DSWAPEFFECT_DISCARD`, `EnableAutoDepthStencil = 1`, `Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER` (+ `DISCARD_DEPTHSTENCIL` in windowed). `[OBSERVED] D3DDevice.cpp:258-296`
- Fullscreen vs windowed is `m_bFull` (`RenderDevice.cpp:51`), set from `config.txt [WINDOW]` or the `/w` command line (`NewApp.cpp:229-230, 1410-1415`); `Initialize()` calls `ToggleFullscreen()` when full (`:155-156`). `[OBSERVED]`
- Gamma: `SetGamma()` rewrites the whole 256-entry ramp from `m_nBright` (0…100, 50 = untouched) and, when windowed, also calls `SetDeviceGammaRamp`. This is a **global brightness** that a port must reproduce with a post-process or a per-frame colour scale. `[OBSERVED] RenderDevice.cpp:719-748`, `NewApp.cpp:238-242`

---

## 2. The ordered pass list

The order below is the order in which the code issues draw calls for the **field scene** (`ESCENE_FIELD`). It is produced by `ObjectManager::RenderObject()` walking the scene tree depth-first (`ObjectManager.cpp:628-660`) and because `TreeNode::AddChild` **prepends** (`TreeNode.cpp:120-129`), children are visited in reverse insertion order.

> **Read this first:** the sequence is *not* a hand-written list — it is the tree. The mapping "tree child → pass" is exact and verified:
> `TMScene.cpp:74,78,82,86,90` (extra, effect, shade, groundObject, human) then `TMFieldScene.cpp:1933-1934` (sky, item).
> Reverse order ⇒ item → sky → human → groundObject → shade → effect → extra. `[OBSERVED]`

### Pass 0 — frame begin: `SetViewPort` + `Lock(1)`

`NewApp.cpp:716-730`

1. `TestCooperativeLevel()` → bail if `D3DERR_DEVICELOST`. `[OBSERVED] :718-720`
2. `SetViewPort(0, 0, m_dwScreenWidth, m_dwScreenHeight)` → `D3DVIEWPORT9{0,0,W,H,MinZ=0,MaxZ=1}` + `SetProjectionMatrix()`. `[OBSERVED] :722`, `RenderDevice.cpp:343-358`
3. `Lock(1)` → Clear(target|depth) with `m_dwActualClearColor`, depth=1.0, `BeginScene()`. `[OBSERVED] :724`
4. `SetViewVector(camera->m_cameraPos, camera->GetCameraLookatPos())` → `D3DXMatrixLookAtLH`, `SetTransform(D3DTS_VIEW)`. `[OBSERVED] :726-729`, `RenderDevice.cpp:1828-1837`
5. `SetRenderStateBlock(1)` → the "3-D world" state block (full table in §7.1). `[OBSERVED] :730`

State at the start of every 3-D pass (block 1): ZENABLE 1, ZWRITE 1, ZFUNC LESSEQUAL, ALPHABLEND **on** with `ONE / INVSRCALPHA`, ALPHATEST on (`GREATEREQUAL` ref `0xDD`), LIGHTING on, FOGENABLE = `m_bFog`, CULLMODE = CCW-cull, COLOROP = MODULATE(TEX × DIFFUSE), MIPFILTER LINEAR. `[OBSERVED] RenderDevice.cpp:1632-1759`

> ⚠ The `ONE / INVSRCALPHA` default blend is **premultiplied alpha**. Any textured surface that does not explicitly override the blend will look too bright unless the texture's RGB is premultiplied (WYD `.wys` textures are authored that way). `[OBSERVED]` for the state; `[INFERRED]` for the premultiplied-texture explanation.

---

### Pass 1 — dropped items, gates and cannons (`m_pItemContainer`)

Issued by: `TMItem::Render()` (`TMItem.cpp:466`), `TMGate::Render()` (`TMGate.cpp:141`), `TMCannon` (derives `TMItem`), each a direct child of `m_pItemContainer` (`TMFieldScene.cpp:15237, 15272`).

Order inside the container: reverse spawn order (newest item first). `[OBSERVED]`

Per-item sequence (`TMItem::Render`, `TMItem.cpp:466-585+`):
1. `IsVisible()` (distance + screen-rect test, §3.2). `[OBSERVED] :474`
2. `SetMaterial(&m_Materials)` — material computed at InitObject from the ground colour. `[OBSERVED] :488`
3. axis-bias/hover-height remap (big `if/else` chain mapping item index → `fAxisAngle`, `fHeight`). `[OBSERVED] :493-569`
4. `pMesh->Render(x, height, z, angle, 0,0,0, 0)` → `TMMesh::Render` → world = `R·S·T`, `DrawIndexedPrimitive` per attribute range. `[OBSERVED] :292`, `TMMesh.cpp:159-232`
5. Legends/sanc glow: second texture stage (1) with `GetEffectTexture(nBaseIndex + m_sMultiType)`. `[OBSERVED] CMesh.cpp:110-139` (same pattern), `TMMesh.cpp:343-364`

State: inherits block 1; `TMObject`-style items set `ZWRITEENABLE 1`, disable blend when not alpha, and set `ALPHAREF 0xAA` (32-bit) when the model texture's alpha class is not `'N'`. `[OBSERVED] TMObject.cpp:257-281`

Depth: **tested and written**. Blend: off for opaque, `SRCALPHA/INVSRCALPHA` when the mesh texture is flagged alpha. Sort: none. `[OBSERVED]`

---

### Pass 2 — skydome (`m_pSky`)

Issued by: `TMSky::Render()` (`TMSky.cpp:125-224`).

1. Bail if `g_bHideBackground`; bail (return 0) when `m_bDungeon` ∈ {1,2,5} or the scene is `ESCENE_SELCHAR`. `[OBSERVED] :127-131, 216-221`
2. `SetMaterial` with diffuse 0.69 grey + emissive 0.3. `[OBSERVED] :134-153`
3. World matrix: `RotationYawPitchRoll(m_fAngle, -90°, 0) · Scale(0.5, 0.25, 0.5) · Translate(cam.x, m_fHeight+1, cam.z)` — **the dome is camera-locked** (translation = camera position, `m_fHeight = -5`). `[OBSERVED] :161-168`
4. **LIGHTING 0, FOGENABLE 0.** `[OBSERVED] :169-170`
5. Texture stage setup (two-layer dome):
   - stage 0: `COLOROP = ADDSIGNED` (TEX + DIFFUSE − 0.5)
   - stage 1: texture = cloud layer `GetEffectTexture(m_nTextureIndex)`, `TEXCOORDINDEX 0`; during a weather transition `COLOROP = BLENDDIFFUSEALPHA` (cross-fade by vertex alpha), otherwise `SELECTARG1`(=off). `[OBSERVED] :172-187`
6. `DESTBLEND = INVSRCALPHA` (SRCBLEND stays `ONE` from block 1 ⇒ additive-ish over clear colour). `[OBSERVED] :189`
7. `pMesh->Render(0,0)` with `m_dwObjType = 1` (`GetCommonMesh(1, ...)`). `[OBSERVED] :190-194`
8. Restore: stage0 `COLOROP = MODULATE`, stage1 `TEXCOORDINDEX 1`, `COLOROP SELECTARG1`, `FOGENABLE = m_bFog`, `LIGHTING 1`. `[OBSERVED] :196-200`
9. Night sky only (`m_nState` ∈ {3,10,13} and texture 67): 20 star billboards + 2 moon billboards are drawn *here*, inside the sky pass. `[OBSERVED] :202-212`

Depth: inherits block 1 (ZWRITE **on**, ZFUNC LESSEQUAL) — the dome writes far depth. Blend: `ONE / INVSRCALPHA`. Sort: n/a (single mesh). Fog: **disabled** (so the dome never fogs; the horizon is hidden by the fog colour == clear colour trick). `[OBSERVED]`

---

### Pass 3 — characters, NPCs, monsters (`m_pHumanContainer`)

Issued by: `TMHuman::Render()` (`TMHuman.cpp:1435-1884`) for every human node, in reverse spawn order (`TMFieldScene.cpp:18484` for mobs, `:1939` for the player). `[OBSERVED]`

Per-character sequence:

| # | Call | What it draws | State highlights |
|---|---|---|---|
| 3.1 | `IsVisible()` + hide/shadow guards | screen-rect + distance test (§3.2) | `[OBSERVED] :1444` |
| 3.2 | `SetRenderState(ALPHAREF, 0xAA/0xFF)` | alpha-test reference per bpp | `[OBSERVED] :1504-1507` |
| 3.3 | hide / "shadow-walk" branch | `ALPHABLEND on`, `DESTBLEND = DESTCOLOR(9)` for ghost NPCs | `[OBSERVED] :1515-1520` |
| 3.4 | normal branch | `ALPHABLEND off`, stage0 `COLOROP = MODULATE`, `DESTBLEND = INVSRCALPHA` | `[OBSERVED] :1537-1543` |
| 3.5 | class-specific overrides | e.g. class 32/face 2 → `ALPHAFUNC = ALWAYS`; class 59 or shadow-form → alpha test off, `DESTBLEND = ONE` (additive) | `[OBSERVED] :1545-1561` |
| 3.6 | `m_pSkinMesh->Render(0,1,0)` **or** `m_pMount->Render(...)` + mounted human | hardware-skinned body; the mount is drawn first and its bone matrix becomes the human's base matrix | `[OBSERVED] :1570-1678` |
| 3.7 | `m_pMantua->Render(fLen, fScale, fLenUp)` | cape/cloak as a second skin mesh with its own base matrix | `[OBSERVED] :1691-1775` |
| 3.8 | `RenderEffect()` | per-class attached effects (eye-fire, golem rock, bone dragon, minotauros, …), dispatched at `:8198+` | `[OBSERVED] :1776` |
| 3.9 | `LabelPosition()` / `LabelPosition2()` | **does not draw** — it projects the head point and re-positions the 2-D label controls (see §8) | `[OBSERVED] :1777-1778, 7304-7623` |

Skinned-mesh inner loop (`CMesh::RenderMesh`, `CMesh.cpp:598-760`):
- `SetStreamSource` + `SetIndices` + `SetVertexDeclaration/SetVertexShader` for the N-influence variant. `[OBSERVED] :604-623`
- bones: `SetVertexShaderConstantF(3*i + 9, transpose(boneWorld * boneOffset * view), 3)` for every palette bone. `[OBSERVED] :625-631`
- `c92 = transpose(inverse(view))` (camera-space lighting), `c1 = normalize(-1,1,1)`. `[OBSERVED] :633-640`
- `CULLMODE = NONE` for skinned meshes (both set-calls are `NONE`, then restored to CCW). `[OBSERVED] :642-656`
- mouse-over/hover highlight: a **second draw of the same geometry** with `SHADEMODE = FLAT`, `COLOROP = SELECTARG2(TFACTOR)`, `SRCALPHA/INVSRCALPHA`, `FOGENABLE 0`, and bone matrices scaled about the pick point. `[OBSERVED] :665-754`

Depth: tested + written (block 1). Blend: off for opaque bodies (premultiplied `ONE/INVSRCALPHA` still active but `ALPHABLENDENABLE=0`); additive (`DESTBLEND ONE`) for shadow forms; `SRCALPHA` for ghost. Sort: **none** — characters are drawn in spawn order, overlapping bodies rely on the Z buffer. `[OBSERVED]`

---

### Pass 4 — terrain (`TMGround`)

Issued by: `TMGround::Render()` (`TMGround.cpp:2685-3269`). `m_pGroundList[0]` is a child of `m_pGroundObjectContainer` and, because it was added **after** the object container (`TMFieldScene.cpp:1921` vs `:1488`), it is visited **first** ⇒ **terrain is drawn before static world objects**. `[OBSERVED]`

1. Bails: `g_bHideBackground`, `!m_bVisible`, camera pitch > 0.4 rad (looking up), `ESCENE_SELCHAR`. `[OBSERVED] :2687-2698`
2. `SetTransform(D3DTS_WORLD, Scale(2.0, 0.1, 2.0) · Translate(offset.x, 0, offset.y))` — computed from the global parent `m_matWorld` (identity). `[OBSERVED] :2711-2716`
3. `RANGEFOGENABLE 0`, `FOGVERTEXMODE = LINEAR`, `COLORVERTEX 1`, `DESTBLEND = INVSRCALPHA`, **`ALPHATESTENABLE 0`**. `[OBSERVED] :2726-2740`
4. `SetMaterial(&m_materials)` (per-page material). `[OBSERVED] :2750`
5. Clip window: `nClipIndex` = 15 (18 for demo, `sightLength*1.5+8` when |pitch|>1.0), then a radial window of radius `nLen = sightLength + 1 (≈13..17) + 2` around the camera tile, reshaped by camera yaw. `[OBSERVED] :2752-2820`
6. Per-tile loop over that window (`:2822-3251`):
   - **HLOD by decimation**: beyond 16/17 tiles from the camera the loop *skips every other tile* and uses `nTickX/nTickY = 2` so a tile is drawn with double size — i.e. tile resolution halves at distance. `[OBSERVED] :2832-2875`
   - Vertex data: positions `(tileX, height, tileY)` (heights from `m_TileMapData[].cHeight`), normals from `m_TileNormalVector`, diffuse from `m_TileMapData[].dwColor`, UVs from `TileCoordList[byTileCoord]` (stage 0) and `BackTileCoordList[byBackTileCoord]` (stage 1). `[OBSERVED] :3125-3193`
   - Texture binding: stage 0 = `GetEnvTexture(byTileIndex + 10)`; stage 1 = `GetEnvTexture(byBackTileIndex + 256)` when not in a dungeon. `[OBSERVED] :3211-3214`
   - Special tiles: animated lava/water (`38/39` → stage1 `COLOROP = MODULATE2X` + scrolling UV + texture 344; `62..65` → `MODULATE2X` + texture `nIndex+286`), and lava tiles (`170/171`) **spawn fire/glow billboards into `m_pEffectContainer` during the render pass** (deferred to the next frame's effect pass). `[OBSERVED] :2986-3122`
   - **Per-tile culling**: all four corners are transformed by `m_matView` then `m_matProj`; the tile is drawn only if any corner has `0 ≤ z < 1` **and** projects inside screen bounds expanded by `50*ratio`. `[OBSERVED] :3218-3247`
   - Draw: `SetFVF(594)` (`XYZ|NORMAL|DIFFUSE|TEX2`) + `DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_vertex, 44)`. **One draw call per tile** — up to ~(2·17)² ≈ 1156 tiles/frame. `[OBSERVED] :3216-3248`
7. Restore: `COLORVERTEX 0`, `ALPHABLENDENABLE 1`, `ALPHATESTENABLE 1`, stage1/stage2 `COLOROP = SELECTARG1`, fill mode back to solid. `[OBSERVED] :3259-3266`

Depth: tested + written. Blend: **`ONE/INVSRCALPHA` still active from block 1** (the code never sets SRCBLEND here — only DESTBLEND), so terrain is drawn premultiplied. Alpha test: off. Fog: per-vertex linear. Sort: none. `[OBSERVED]`

---

### Pass 5 — static world objects, water, vegetation (`m_pObjectContainerList`)

Issued by each object's own `Render()`; they are children of `TMObjectContainer` (`TMObjectContainer.cpp:26-42, 952`), which is itself the second child of `m_pGroundObjectContainer`. **Draw order inside the container is reverse load order** (later entries in the `.dat` are drawn first). `[OBSERVED]`

Concrete classes encountered (from the loader dispatch, `TMObjectContainer.cpp:279-960`):

| Object | Render entry | Notes |
|---|---|---|
| `TMHouse` | `TMHouse.cpp:92-160` | two-mesh house (base + open/closed door mesh); when the camera/player is within 6 units it enables `ALPHABLEND` and fades (`DESTBLEND = INVSRCALPHA`, or `DESTCOLOR` for the door) — the classic "wall transparency" behaviour. Otherwise delegates to `TMObject::Render()`. `[OBSERVED]` |
| `TMTree` | `TMTree.cpp:31-80` | skinned mesh; swaps mesh by war-clan state; `ALPHAREF 0xAA` |
| `TMSea` (water/lava) | `TMSea.cpp:25-175` | see below |
| `TMButterFly`, `TMFish`, `TMLeaf` | `TMButterFly.cpp:108`, `TMFish.cpp:52`, `TMLeaf.cpp:38` | animated skin meshes / billboards, animated per frame |
| `TMDust` | `TMDust.cpp:21` (`TMDust::Render`) | dust/steam prop built on `TMObject` (`TMDust.h:5`) |
| `TMDrop` | `TMDrop.cpp:46` | waterfall billboards |
| `TMBike` | `TMBike.cpp` (`TMObject` subclass) | parked bike prop |
| `TMShip` | `TMShip.cpp:36-60` | `TMObject::Render()` for the hull mesh **plus** a second `TMSkinMesh` pass for the animated boat |
| `TMEffectMesh` (object-attached glow meshes, created for many `.dat` object types) | `TMEffectMesh.cpp:35` | additive mesh with per-vertex colour write, `ZWRITE 0`; creation sites `TMObjectContainer.cpp:574-851` |
| `TMShade` (ground ambient occlusion patches, lamp pools) | `TMShade.cpp:208` | `ZWRITE 0`, blend, ground-projected grid |
| `TMObject` (generic prop) | `TMObject.cpp:85-311` | the base path for most `.dat` objects |
| `TMLight` | `TMLight.cpp:65-86` | **not a draw** — installs a D3D point light and tints 5 ground cells (§5) |

`TMObjectContainer::m_pGroundEffectContainer` is added **first** in the container ctor (`TMObjectContainer.cpp:41`) and therefore drawn **last** within pass 5: it holds the small ground billboards (lamp glows `TMEffectBillBoard`, leaves, butterflies, fish, `TMEffectMesh` decorations) with keys `AddChildWithKey(..., Key)` so they are grid-culled by `isVisualKey()`. `[OBSERVED]`

`TMObject::Render()` state for the generic path (`TMObject.cpp:253-310`):
- `ZWRITEENABLE 1`; `ALPHABLEND off` when not alpha. `[OBSERVED] :257-259`
- if the model texture's alpha class ≠ `'N'` (or objType 156…185): `ALPHAREF 0xAA`, `ALPHATESTENABLE 1`, `ALPHABLEND on`, `SRCALPHA/INVSRCALPHA`, stage0 `COLOROP MODULATE`, stage1 `SELECTARG1`. `[OBSERVED] :261-276`
- `SetMaterial(&materials)` where the material = ground colour × light blend: in dungeons `Diffuse = 0.5*groundColor + 0.5*m_colorLight`, `Emissive = 0.3*m_colorBackLight + 0.4*groundColor`; outdoors a fixed 0.69 grey with emissive 0.3. `[OBSERVED] :104-145`
- FOG: `FOGENABLE = m_bFog`. `[OBSERVED] :147`
- special objTypes 1934/1976/1977 (magic portals): stage1 `COLOROP = ADDSMOOTH`, texture = current sky texture, `TEXTURETRANSFORMFLAGS = COUNT2`, `TEXCOORDINDEX 0x30000` (camera-space projection). `[OBSERVED] :284-290`

**Water (`TMSea`, `TMSea.cpp:25-175`)** — three variants:

| Case | Blend | Textures | ZWrite | Cull |
|---|---|---|---|---|
| normal sea/river | `SRCBLEND = SRCCOLOR`, `DESTBLEND = DESTALPHA` | stage0 model tex 2, stage1 model tex 3, stage1 `COLOROP = MODULATE2X` | inherited (on) | NONE |
| dungeon water (bDungeon 1/2/5) | `SRCALPHA / INVSRCALPHA` | stage0 tex 8, stage1 tex 9 | **0** | NONE |
| "fog"/lava at world cell (28..29, 22..23) | `INVSRCCOLOR / SRCCOLOR` | stage0 effect tex 406, stage1 effect tex 406, `COLOROP = MODULATE` | inherited | CCW-cull |

All water variants: `FOGENABLE 0`, a grid mesh whose Y is animated per-vertex in `FrameMove` (`TMSea.cpp:299-343`) and whose UVs scroll by `serverTime % 12000 / 18000`. `[OBSERVED]`
Water is culled with a **hard 36-unit radius** around the focused object in addition to the normal test (`TMSea.cpp:347-366`). `[OBSERVED]`

---

### Pass 6 — shadows/decals (`m_pShadeContainer`)

Issued by: `TMShade::Render()` → `RenderUnder()` (`TMShade.cpp:208-283`).

- Contents: one `TMShade` per character (`TMHuman.cpp:1313-1380`, parented at `:1380`), plus craters, footprints, lamp pools and skill ground decals (`TMHuman.cpp:2040, 2513`, `TMFieldScene.cpp:21109, 21324, 21436`, every `TMSkill*` ctor). `[OBSERVED]`
- Geometry: a `(nGridNum+1)²` grid in world space, each vertex snapped to the terrain height +0.05 and texture-rotated by `m_fAngle`. Built once in `SetPosition()` (`TMShade.cpp:80-206`). `[OBSERVED]`
- State (`TMShade.cpp:224-281`):
  - `SetTransform(D3DTS_WORLD, identity)`
  - `ALPHABLEND on`, `SRCBLEND SRCALPHA`, `DESTBLEND INVSRCALPHA` (default) **or** `DESTBLEND ONE` + stage0 `ALPHAOP = SELECTARG1` when `m_efAlphaType == EF_BRIGHT`
  - **`ZWRITEENABLE 0`**, `ALPHATESTENABLE 0`, `ALPHAFUNC = ALWAYS`, `LIGHTING 0`
  - FOG on (linear, vertex mode) for the alpha variant, off for the bright variant
  - `SetFVF(322)` + `DrawIndexedPrimitiveUP(TRIANGLELIST, …, FMT_INDEX16, …, 24)`
  - restores `LIGHTING 1`, `ALPHAOP SELECTARG2`, `SRCBLEND ONE`, `ALPHAFUNC GREATEREQUAL`, `ALPHATEST 1`, `ZWRITE 1`
- Depth: **tested, not written** — these are decals, they never occlude.
- Sort: none (they are ground-aligned; overlapping decals blend in tree order). All character shadows are drawn *after all characters* and *after terrain*, which is required: they read the height field. `[OBSERVED]`

---

### Pass 7 — effects and particles (`m_pEffectContainer`)

Children are visited newest-first (prepend), and the fixed members were inserted at scene init, so the in-pass order is:

```
[newest runtime effect] ... [targetBill][target2][target1][snow2][snow][rain][sun]
```
`[OBSERVED] TMFieldScene.cpp:1927-1971` + prepend semantics.

| Sub-pass | Node | Issue site | State summary |
|---|---|---|---|
| 7.a | runtime effects, skills, dust, fire, level-up, fireworks (all `TMEffect*`) | `TMEffectBillBoard::Render` `:86`, `TMEffectBillBoard2::Render` `:48`, `…3`, `…4`, `TMEffectMesh::Render` `:35`, `TMEffectSkinMesh::Render` `:172`, `TMEffectParticle::Render` `:111`, `TMEffectDust::Render` `TMEffectDust.cpp:220`, `TMSkill*::Render` | additive plates / alpha plates, ZWRITE 0, no cull |
| 7.b | `m_pTargetBill` (current-target marker) | `TMEffectBillBoard2` | additive |
| 7.c | `m_pTarget1`, `m_pTarget2` (selection rings, effect meshes 316/317) | `TMEffectMesh` | `EF_BRIGHT` additive |
| 7.d | `m_pSnow2`, `m_pSnow` | `TMSnow::Render` `:47` | `SRCALPHA/ONE`, ZWRITE 0, per-flake quad, attribute-cell skip |
| 7.e | `m_pRain` | `TMRain::Render` `:46` | `SRCALPHA/ONE`, ZWRITE 0, 50 camera-facing quads |
| 7.f | `m_pSun` (lens flare) | `TMSun::Render` `:91` | **screen-space** XYZRHW quads, `SRCCOLOR/DESTALPHA`, ZWRITE 0, z=0.8−0.04·i |

Canonical billboard state (`TMEffectBillBoard::Render`, `TMEffectBillBoard.cpp:132-171`):
```
SetTransform(D3DTS_WORLD, m_matEffect)      // billboard matrix built in FrameMove
ALPHABLEND on; SRCBLEND SRCALPHA
EF_BRIGHT  : DESTBLEND ONE,       ALPHAOP SELECTARG1
otherwise  : DESTBLEND INVSRCALPHA, ALPHAOP MODULATE
FOGENABLE 0; LIGHTING 0; ALPHAFUNC ALWAYS; ALPHATESTENABLE 0; ZWRITEENABLE 0; CULLMODE NONE
SetFVF(322); SetTexture(0, GetEffectTexture(m_nCycleIndex + m_nTextureIndex, 5000))
DrawPrimitiveUP(TRIANGLEFAN, 2, m_vertex, 24)
// restore: CULLMODE CCW, LIGHTING 1, SRCBLEND ONE, ALPHAFUNC GREATEREQUAL,
//          ALPHATESTENABLE 1, ZWRITEENABLE 1, DESTBLEND INVSRCALPHA, ALPHAOP SELECTARG2
```
`[OBSERVED]`

Distance cull: `if (fLen > 45.0f) return 1;` measured on the XZ plane from the camera (`TMEffectBillBoard.cpp:108-119`). `[OBSERVED]`
`TMEffect::IsVisible()` currently always ends in `m_bVisible = 1` with a `// TODO` (it evaluates the camera rectangle but then ignores it) — **effects are effectively unculled in this port**. `[OBSERVED] TMEffect.cpp:44-62`

---

### Pass 8 — floating world-anchored text (`m_pExtraContainer`)

Issued by: `TMFont3::Render()` (`TMFont3.cpp:202-300`).

- Contents: damage/heal numbers, "+EXP", level-up strings, spawn text (`TMFieldScene.cpp:14387, 14401`). `[OBSERVED]`
- Two modes: `m_nType == 0` → plain `TMFont2::Render` (text quads); otherwise → digit sprites from a UI texture set, per-glyph quads with a fan-out animation. `[OBSERVED] :206-300`
- State: `SetRenderStateBlock(2)` for the text variant (ZENABLE 0!), `ZENABLE 0` + `FOGENABLE 0` for the sprite variant; XYZRHW quads. `[OBSERVED] :208-219`
- This is the **last 3-D-scene pass**, but it is really 2-D: the positions were projected to pixels when the effect spawned. `[OBSERVED]`

---

### Pass 9 — 2-D UI (`RenderControl`)

`NewApp.cpp:732-733` → `SetRenderStateBlock(3)` then `ObjectManager::RenderControl()` (`ObjectManager.cpp:591-626`).

Sequence:

1. `SetMatrixForUI()` — pseudo-projection FOV 0.1 rad, aspect `(bb.Width/bb.Height)*0.94`, near 10, far 100; view at `(0,0,50)` → origin. Needed only for `RENDER_3DOBJ`. `[OBSERVED] ObjectManager.cpp:595`, `RenderDevice.cpp:1856-1872`
2. `SetTexture(0..2, nullptr)`. `[OBSERVED] :596-598`
3. `SetRenderState(D3DRS_ZENABLE, 0)` **once** before the loop. `[OBSERVED] :602`
4. For `layer = 0 … 29` (`MAX_DRAW_CONTROL = 30`, `SControlContainer.h:7`): walk the `GeomControl` list of that layer and for each: `FOGENABLE 0` then `RenderGeomControl(control)`; the list is consumed (head/tail reset to null). `[OBSERVED] ObjectManager.cpp:603-623`
5. `SetRenderState(D3DRS_ZENABLE, 1)`. `[OBSERVED] :624`

`RenderGeomControl` (`RenderDevice.cpp:3253-3427`) per control:
- set `ALPHAREF` (0xDD 32-bit / 0xD 16-bit, or the 0xFF000000 VGAID=1 variant)
- `SetRenderStateBlock(0)` — **note this re-enables ZENABLE** (block 0 sets `ZENABLE 1`), so from the second control onwards Z-testing is on again. The hand-built UI quads keep `z = 0.01` (written once in `RenderDevice::Initialize`, `:208-283`, and never overwritten by `RenderRectTex`/`RenderRectC`/`RenderRectNoTex`), and `ZFUNC = LESSEQUAL`, so equal-z quads still pass and draw order wins; `ID3DXSprite` quads (`RENDER_IMAGE`) carry their own z from D3DX. `[OBSERVED] :3273`, `RenderDevice.cpp:1608, 2057-2064`
- dispatch:
  - `RENDER_TEXT` / `RENDER_SHADOW` → `TMFont2::Render(x, y, type)` which itself calls `SetRenderStateBlock(2)`. `[OBSERVED] :3277-3289`, `TMFont2.cpp:302`
  - `RENDER_IMAGE` → `RenderRect` = `ID3DXSprite::Begin/Draw/End` (one sprite batch per quad). `[OBSERVED] :3290-3294`, `RenderDevice.cpp:1893-1911`
  - `RENDER_IMAGE_TILE` / `RENDER_IMAGE_STRETCH` / `RENDER_TEXT_FOCUS` → `RenderGeomRectImage` → `RenderRectTex` (FVF 324 fan) with optional clip/legend/sanc overlays. `[OBSERVED] :3294`, `:2833-3252`
  - `RENDER_3DOBJ` → **switches back to block 1**, `ZFUNC = ALWAYS`, FOG off, blend off, alpha test off, draws a `TMMesh` via `RenderForUI` (rotated so it lands under the control's pixel), optional background billboard, then restores `FOGENABLE = m_bFog` and `ZFUNC = LESSEQUAL`. `[OBSERVED] :3320-3368`
- transparent quads: `SRCBLEND SRCALPHA`, `DESTBLEND INVSRCALPHA`, `ALPHAFUNC ALWAYS`, `ALPHATEST off`, `ZWRITEENABLE 0`. `[OBSERVED] RenderDevice.cpp:2141-2150`

The UI draw list is built **during `FrameMove`**, not during rendering: `SControlContainer::FrameMove` calls `FrameMove2(m_pDrawControl, …)` on every visible control, which computes the control's absolute rect and `AddRenderControlItem(pDrawList, geom, inParentLayer)` where `inParentLayer` is the **tree depth**. The cursor is always appended at layer 29. `[OBSERVED] SControlContainer.cpp:259-311`, `SControl.cpp:365-382, 843-960`

⇒ UI z-order = widget tree depth, not z-coordinate. Two siblings at the same depth are drawn in *prepend* order (last added first). `[OBSERVED]`

Global UI fade: `g_nUIFadeAlpha` multiplies every UI/text colour (`RenderDevice.cpp:2835`, `TMFont2.cpp:293-295`), used for the login/demo fades. `[OBSERVED]`

---

### Pass 10 — FPS overlay / frame end

Inside `Unlock(1)`, still between `BeginScene`/`EndScene`: if `g_bDebugMsg == 1`, `m_pFont->Render(10, screenHeight-15, 0)` (which itself uses block 2). `[OBSERVED] RenderDevice.cpp:432-438`. Then `EndScene()` + `Present()`. `[OBSERVED] :440-447`

---

### 2.12 What is missing here vs. the original

The following appear **nowhere** in the port's frame and are the most likely candidates for missing original passes. Each is tagged with what the source shape suggests:

| Candidate pass | Evidence | Status |
|---|---|---|
| Scene-level pre/post render (sky before ground, water reflection, weather overlay) | no `TMScene::Render`/`TMFieldScene::RenderFrame` exists; scenes inherit the no-op `TreeNode::Render` | `[UNKNOWN]` — resolve by REA on `WYD.exe` (find the function that calls the tree walk and see what surrounds it) |
| Water reflection (`m_matWaterView`) | the matrix member exists (`RenderDevice.h:89`) but is never written or read (grep: only the declaration) | `[INFERRED]` the original had a mirrored-pass for reflections; dead here |
| Motion blur / bloom | `JBlur` skeleton (`Begin`, `BeginMotionBlur`, `MakeBlur`, `CopyTexture`, `SetVSConstant`) and `g_nUseBlur`/`g_nReflection` globals exist and are forced to 0 on low-end PS (`RenderDevice.cpp:541-545`) | `[INFERRED]` post-process existed in DX8 original; disabled here |
| Off-screen origin/accum/bloom targets | `TextureManager::InitRenderTargetTexture` returns 1 immediately, the whole body is `#if 0` (`TextureManager.cpp:1446-1453, 1455-1569`) | `[OBSERVED]` disabled |
| Minimap render-to-texture | minimap reads `GetUITextureSet(11)` and only *scrolls UVs* (`TMGround.cpp:3283-3292`) | `[INFERRED]` the dynamic texture is filled at load (`SetMiniMapData`), not per frame |
| Screenshot / AV I-playback overlays | `CaptureScreen`, `TMVideoWnd` | out of scope, not part of the 3-D frame |

Also worth knowing: `TMSelectServerScene::ResetDemoPlayer()` calls `FrameMove(0)` **and `Render()`** on each of the 50 demo humans *during scene setup* (`TMSelectServerScene.cpp:1190-1198`), i.e. outside any `BeginScene/EndScene`. In D3D9 those draw calls are invalid (the debug runtime flags them, the release runtime usually drops them); they are harmless here but must **not** be copied into a port. `[OBSERVED]`

---

## 3. Sorting and culling

### 3.1 Transparency ordering

- **There is no distance sort anywhere.** `grep std::sort|qsort` finds only a display-mode sort in `D3DEnumeration.cpp:351`. `[OBSERVED]`
- All ordering is **tree order**: reverse insertion within a container, containers visited in the fixed order of §2. Newly spawned effects are prepended, so a fresh explosion is drawn *before* older ones. `[OBSERVED]`
- Consequences to preserve: (a) overlapping additive particles accumulate in spawn order — visually identical for commutative `ADD`/`ONE` blending; (b) alpha-blended billboards (non-`EF_BRIGHT`) are *not* sorted, so they can pop if you change spawn order; (c) semi-transparent world objects (house fade, ghost NPCs) rely on being drawn late rather than on sorting. `[INFERRED]` from the blend modes + absence of sort.

### 3.2 Frustum / visibility culling (what actually exists)

| Mechanism | Where | Rule |
|---|---|---|
| Grid-cell key cull | `TMObject::isVisualKey()` `TMObject.cpp:671-684`, keys from `TMCamera::GetCameraPos` `TMCamera.cpp:251-259` | object skipped unless `(x>>5)+((z>>5)<<16)` equals one of the camera's 9 cells (a 3×3 of 32×32-unit cells) |
| Distance + radius reject | `TMObject::IsVisible` `TMObject.cpp:347-355` | `pMesh->m_fRadius >= |pos-cam|` ⇒ visible immediately; else if dot(pos−cam, camDir) ≤ 0 ⇒ *invisible*; else `IsInView()` |
| Screen-rect test on 9 box corners | `TMObject::IsInView` `TMObject.cpp:459-605` | distance > `m_fFogEnd + radius` ⇒ cull; otherwise `D3DXVec3Project` 9 corners and `IntersectRect` with the viewport |
| Per-tile screen test | `TMGround::Render` `TMGround.cpp:3218-3247` | project 4 corners; draw only if a corner is inside `[-50r, W+50r] × [-50r, H+50r]` and `0 ≤ z < 1` |
| Tile-window radius | `TMGround.cpp:2752-2820` | ~13–20-tile radius around the camera tile, reshaped by yaw |
| Distance cull for billboards | `TMEffectBillBoard.cpp:118-119` | `> 45` units ⇒ skip |
| Distance cull for foliage | `TMLeaf.cpp:69-70` | `m_vecPosition.DistanceFrom2(cam) > 900` — a 2-D distance metric whose exact semantics (squared vs. linear) are not verified here |
| Water radius | `TMSea.cpp:355-359` | `< 36` units from the focused object |
| Light frustum | `TMLight::IsVisible` `TMLight.cpp:88-162` | diamond-shaped camera-region test in world XZ |

- **No `D3DXPlane` / no frustum-plane extraction anywhere** (`grep D3DXPlane|Frustum` → 0 hits). `[OBSERVED]`
- Culling is therefore *screen-space*, not plane-based, and it reads `m_matView/m_matProj/m_viewport` which are only valid after `SetViewPort` + `SetViewVector` — so all culling happens **inside** the render pass, not in `FrameMove`. `[OBSERVED]`

### 3.3 LOD

- Terrain: 2:1 tile decimation beyond ~16 tiles from the camera (see Pass 4 step 6). `[OBSERVED] TMGround.cpp:2832-2875`
- Characters/objects: **no LOD**. `[INFERRED]` (no LOD selection code in `TMMesh`/`TMSkinMesh`; meshes come from a single index)
- Mip-mapping: `D3DDevice::m_nMipMap = 30` by default, reduced to 20/10/0 when `GetAvailableTextureMem() ≤ 64/56/48` (the comparison is against bytes-of-free-mem semantics that look wrong, but that is what ships). `[OBSERVED] NewApp.cpp:236, 507-525`
- Sampler: stage 0 `MIPFILTER = LINEAR` in block 1, `POINT` in block 0/2; MIN/MAG always LINEAR. `[OBSERVED] RenderDevice.cpp:1744-1746, 1784-1786, 1626-1628`

### 3.4 Object-count limits and draw-order variables

- `MAX_OBJECT_LIST = 4096` static objects per container (`TMObjectContainer.h:8`), `MAX_LIGHT_CONTAINER = 6` lights per container (`:9`), `MAX_DRAW_CONTROL = 30` UI layers (`SControlContainer.h:7`), `MAX_UI_TEXTURE_SET_LIST = 600` (`TextureManager.h:3`).
- There is **no** `m_nBuildOrder` / explicit sort key field in this codebase. The only ordering keys are: tree position (draw order) and `m_dwKey` (grid cell, culling only). `[OBSERVED]`
- Effect "priority" is expressed by *container choice*: `m_pExtraContainer` (top-most), `m_pEffectContainer`, `m_pShadeContainer` (below everything else). `[OBSERVED]`

---

## 4. Where per-frame matrices are set

| Matrix | Set where | Frequency |
|---|---|---|
| `m_matProj` | `SetProjectionMatrix()` called from `SetViewPort()` (`RenderDevice.cpp:357`), which the frame calls at `NewApp.cpp:722`; also `SetMatrixForUI()` overwrites `D3DTS_PROJECTION` for the UI pass (`:1861`) | once per frame + once per UI pass |
| `m_matView` | `SetViewVector()` (`RenderDevice.cpp:1834`, `SetTransform(D3DTS_VIEW)`) at `NewApp.cpp:729`; `SetMatrixForUI()` overwrites it for UI (`:1867`) | once per frame + UI pass |
| `m_matWorld` (global parent) | set to identity **once** in `InitializeRenderingState` (`RenderDevice.cpp:1577-1578`); never changed afterwards | never |
| `D3DTS_WORLD` per object | every object computes `W = m_matWorld · R · S · T` and calls `SetTransform(D3DTS_WORLD)` (or `D3DTS_WORLDMATRIX(0)`, same field) — `TMMesh.cpp:170-175`, `TMObject` via `TMMesh`, `TMGround.cpp:2716`, `TMSky.cpp:168`, `TMSea.cpp:164`, `TMRain.cpp:119`, `TMSnow.cpp:143`, `TMShade.cpp:226` | per object / per tile-group |
| Skinned characters | **no** world matrix; bones pre-multiplied by view: `c9 + 3i` (`CMesh.cpp:625-631`) | per mesh part |
| Shader constants | `c1` = direction light, `c2` = transposed projection (`RenderDevice.cpp:1689, 1851`), `c9+` = bones, `c92` = transposed inverse view (`CMesh.cpp:640`) | per frame / per mesh |

- **Nothing resets `D3DTS_WORLD` to identity between passes.** Each pass that needs identity does it itself (`TMShade.cpp:224-226`); everything else overwrites it. A port must not assume identity at pass start. `[OBSERVED]`
- Multiplication order (row-vector): `W = M_parent · R · S · T`, then the fixed-function pipeline applies `v · W · V · P`. `[OBSERVED] TMMesh.cpp:167-175`
- The frame's matrices are **never** restored after the UI pass; the next frame's `SetViewPort`/`SetViewVector` re-establishes them. `[OBSERVED]`

---

## 5. Light and shadow handling

### 5.1 Global lights

- Two directional lights exist in `RenderDevice::m_light[2]` (`D3DLIGHT_DIRECTIONAL`), with diffuse/specular from `m_colorLight` and `m_colorBackLight`. `[OBSERVED] RenderDevice.cpp:89-111, 1632-1658`
- They are refreshed **once per frame** in `SetRenderStateBlock(1)` (called at `NewApp.cpp:730`), not per object: `m_light[0].Direction = (1,-1,-1)`, `m_light[1] = (0,0,1)`, `LightEnable(0..1, 1)`, plus `D3DRS_AMBIENT = 0x33FFFFFF`, and the direction is pushed to VS constant `c1` (flipped in dungeons). `[OBSERVED] :1657-1710`
- `m_bShowBoss` acts as a **one-shot blackout flag**: if set, light 0's diffuse/specular are zeroed for exactly one frame and the flag is cleared. ⚠ the decompiled name is misleading — it is a "kill the key light for one frame" hack, not a boss flag. `[OBSERVED] :1639-1653`
- `RenderDevice::SetLight()` (`:452-462`) exists but is **not called anywhere** in the frame (grep: only definition). `[OBSERVED]`

### 5.2 Point lights

- `TMLight` (`D3DLIGHT_POINT`, `Range = 5.0`, attenuation 0.001/0.001/0.001) is installed into **D3D light slots starting at index 2** (`m_dwBaseLightIndex = 2`, `TMLight.cpp:6, 13`), at most 6 per `TMObjectContainer` (`MAX_LIGHT_CONTAINER`). `[OBSERVED]`
- `TMLight::FrameMove` uploads/updates the light and enables it — this happens in the **FrameMove** phase, not in the render pass (`TMLight.cpp:65-86`, driven from `TMObjectContainer::FrameMove` `:1010-1015`). `[OBSERVED]`
- `TMLight::InitPosition` also **bakes** the light into the terrain by tinting 5 ground cells (`GroundSetColor`, `TMLight.cpp:57-62`) — a per-light vertex-colour bake, i.e. the ground lighting you see is partly static. `[OBSERVED]`
- `TMSky::FrameMove` enables/disables *all* container lights depending on the weather state (`TMSky.cpp:516-541`). `[OBSERVED]`

### 5.3 Shadows

- **There is no shadow map and no stencil shadow volume.** No `D3DUSAGE_DEPTHSTENCIL`, no `D3DRS_STENCILENABLE` other than `0`, no depth-texture creation anywhere. `[OBSERVED]` (grep)
- All shadows are **blob/projected decals**: `TMShade` grids placed on the terrain (Pass 6), one per character (`TMHuman.cpp:1313-1380`), plus craters/footprints. `[OBSERVED]`
- `m_nShadowTextureSize` (64/128) is chosen from the pixel-shader version (`RenderDevice.cpp:531-545`) but never used to create anything — a leftover of a removed shadow-map path. `[INFERRED]`
- Characters can also *be* shadows (ghost/stealth): `m_cShadow == 1` renders them with `DESTBLEND = DESTCOLOR` (`TMHuman.cpp:1515-1520`). `[OBSERVED]`

---

## 6. Multi-pass rendering and render targets

### 6.1 Current state: single-pass, back buffer only

- Exactly one colour target: the swap-chain back buffer. No `SetRenderTarget`, no `StretchRect`, no `ColorFill` outside dead code. `[OBSERVED]` (grep)
- `TextureManager::InitRenderTargetTexture()` returns `1` before its `#if 0` body, with an explicit comment that 748 uses no off-screen post-processing and that the 755/760 code created `D3DUSAGE_RENDERTARGET` textures in `D3DPOOL_MANAGED` (invalid in D3D9). `[OBSERVED] TextureManager.cpp:1446-1453`

### 6.2 What the DX8 shape implies (documented, not implemented)

The surviving declarations describe the original post-process chain:

```
m_pOriginTexture / m_pOriginSurface   // full-res X8R8G8B8 copy of the frame
m_pOriginRTT                          // optional multisampled RT (m_nAntiAliasLevel)
m_pAccumTexture / m_pAccumSurface     // accumulation buffer (motion blur / trail)
m_pRenderTexture / m_pRenderSurface   // scratch
m_pBloomTexture[2]  (w/4, h/4)        // downsample chain A
m_pBloomTexture1[2] (w/2, h/2)        // upsample chain B
```
`[OBSERVED] TextureManager.h:118-129`, `TextureManager.cpp:1455-1569`

`JBlur`'s surviving method set — `Begin()`, `End()`, `BeginMotionBlur()`, `EndMotionBlur()`, `MakeBlur()`, `SwitchSetTexture()`, `CopyTexture(nScale, pSrc, pDestSurface, fDepth)`, `SetVSConstant(fScale, fWidth, fHeight, fWidthPixelOffset, fHeightPixelOffset, fDepth)`, `SetLoginColor()`, `SetDefaultColor()` — is a textbook 2-pass separable blur with a vertex-shader-driven 4-tap offset, plus a "login colour" tint. All bodies are empty. `[OBSERVED] JBlur.cpp:12-66`; the *interpretation* is `[INFERRED]`.

`g_nReflection` and `g_nUseBlur` are global toggles that are force-cleared on PS < 1.1 hardware (`RenderDevice.cpp:541-545`), and `m_matWaterView` (a mirrored view matrix) exists unused → the original very likely had a **water reflection pass** re-rendering nearby geometry with the view mirrored about the water plane. `[INFERRED]`

⚠ For parity: the shipped 748 client with default `config.txt` runs the **single-pass** path unless `[BRIGHT]`/blur options enabled it; matching the current port is the safe target. `[INFERRED]`

---

## 7. State invariants that must be preserved

### 7.1 The four canned state blocks (`RenderDevice::SetRenderStateBlock`)

Values are D3D9 enums; "block N" = `SetRenderStateBlock(N)`.

| State | Block 0 (2-D/UI) `:1606-1631` | Block 1 (3-D world) `:1632-1759` | Block 2 (text) `:1760-1791` | Block 3 (UI global) `:1792-1823` |
|---|---|---|---|---|
| ZENABLE | 1 | 1 | **0** | **0** |
| ZFUNC | default | LESSEQUAL(4) | default | default |
| ZWRITEENABLE | default | 1 | default | default |
| ALPHABLENDENABLE | 0 | 1 | 1 | 0 |
| SRCBLEND | SRCALPHA(5) | **ONE(2)** | SRCALPHA(5) | SRCALPHA(5) |
| DESTBLEND | DESTALPHA(7) | INVSRCALPHA(6) | INVSRCALPHA(6) | INVSRCALPHA(6) |
| ALPHATESTENABLE / FUNC / REF | default | 1 / GREATEREQUAL(7) / 0xDD (32bpp) | 1 / GREATEREQUAL / 8 | 1 / GREATEREQUAL / 8 |
| LIGHTING | 0 | 1 | (unchanged) | (unchanged) |
| SPECULARENABLE | 1 | 0 | — | — |
| FOGENABLE | 0 | `m_bFog` | 0 | 0 |
| CULLMODE | CCW(3) | CCW(3) | CCW(3) | CCW(3) |
| SHADEMODE | GOURAUD(2) | GOURAUD(2) | — | — |
| COLORVERTEX | 1 | 1 | — | — |
| NORMALIZENORMALS | — | 1 | — | — |
| AMBIENT | 0x000AAAAA | 0x33FFFFFF | — | — |
| stage0 COLOROP | SELECTARG1(TEX) | MODULATE(TEX×DIFFUSE) | MODULATE(TEX×DIFFUSE) | MODULATE |
| stage0 ALPHAOP | SELECTARG1(TEX) | SELECTARG1(TEX) | MODULATE(TEX×DIFFUSE) | MODULATE |
| sampler | MIN/MAG LINEAR, MIP POINT | MIN/MAG LINEAR, MIP LINEAR | MIN/MAG/MIP POINT | MIN/MAG/MIP LINEAR |
| FVF | 322 (`XYZ|DIFFUSE|TEX1`) | 322 | 324 (per-quad) | 322 |
| FOGVERTEXMODE / FOGCOLOR | — | LINEAR(3) / `m_dwClearColor` | — | — |
| FOGSTART / FOGEND | — | `m_fFogStart + sight − 8` / `m_fFogEnd + sight − 15` | — | — |

`[OBSERVED] RenderDevice.cpp:1604-1826`

### 7.2 Per-pass invariants you must not lose

1. **Z-write disabled** for every particle/billboard/decal/lens-flare/rain/snow/UI quad (`ZWRITEENABLE 0`) — otherwise the additive effects punch holes in the depth buffer and later geometry disappears. `[OBSERVED]` (billboard `:150`, shade `:262`, rain `:72`, snow `:77`, sun `:135`, UI `:2149`)
2. **Additive blending for fire/glow**: `EF_BRIGHT` ⇒ `SRCBLEND SRCALPHA` + `DESTBLEND ONE` (`DESTBLEND 2`). Everything else ⇒ `INVSRCALPHA`. `[OBSERVED]`
3. **Premultiplied default** (`ONE / INVSRCALPHA`) for world geometry (block 1) — must not be replaced by `SRCALPHA/INVSRCALPHA` or all alpha-tested foliage/trees will double-darken. `[OBSERVED]` (state) / `[INFERRED]` (art is premultiplied)
4. **Alpha test reference depends on colour depth**: 32 bpp → `0xDD`, 16 bpp → `0xD`, VGAID 1 → `0xFF000000 / 0xF000`, and per-model `0xAA` for characters/items. Alpha-ref changes are visual, not cosmetic. `[OBSERVED]`
5. **Culling is disabled for skinned characters and billboards** (`CULLMODE 1 = NONE`), and restored to `3 = CCW` right after. Some weapon/shield sub-meshes deliberately flip to `CW`. `[OBSERVED] CMesh.cpp:642-656`
6. **Fog is disabled** for sky, billboards, lens flare, UI, and the two special water/dungeon branches; enabled for terrain (per-vertex linear) and opaque objects. `[OBSERVED]`
7. **Two texture stages** are used everywhere: stage 0 = base/albedo, stage 1 = detail/glow/mask/cross-fade. Establishing stage 1 `COLOROP` correctly for each case is the subtlest part of the port (see table in §7.3).
8. **`D3DRS_DIFFUSEMATERIALSOURCE/SPECULARMATERIALSOURCE = MATERIAL(1)`** in block 1 ⇒ materials (not vertex colours) drive lighting for `TMMesh`; but terrain/effects set `COLORVERTEX 1` so their vertex colours modulate. `[OBSERVED] :1734-1737`
9. **`D3DRS_FOGVERTEXMODE = LINEAR`** everywhere (never table fog). `[OBSERVED]`

### 7.3 Stage-1 (second texture) colour-op cheat sheet

| Case | stage1 COLOROP | Meaning |
|---|---|---|
| default / restore | `SELECTARG1` (2/1) → effectively off | `[OBSERVED]` block 1 `:1748` |
| terrain back-texture | `MODULATE` (4) | `TMGround.cpp:2745` |
| terrain lava/water tiles | `MODULATE2X` (5) | `TMGround.cpp:3097, 3110` |
| dungeon terrain | `SELECTARG1`(disable stage) | `TMGround.cpp:2743` |
| sky weather cross-fade | `BLENDDIFFUSEALPHA` (12) | `TMSky.cpp:182` |
| sky steady | `SELECTARG1` | `TMSky.cpp:184` |
| magic portals (obj 1934/1976/1977) | `ADDSMOOTH` (11) + `TEXTURETRANSFORMFLAGS = 2` (COUNT2) + `TEXCOORDINDEX 0x30000` (camera-space) | `TMObject.cpp:286-289` |
| water (normal) | `MODULATE2X` | `TMSea.cpp:112` |
| item/character legend glow | `MODULATEALPHA_ADDCOLOR` (18) / `MODULATE4X` (6) / `ADD` (7) / `ADDSMOOTH` (11) depending on `m_sMultiType`/`sLegend` | `CMesh.cpp:128-139`, `TMMesh.cpp:345-364` |
| ground dust | `MODULATE` with effect tex 6 | `TMEffectDust.cpp:284-285` |

---

## 8. HUD/UI over 3-D, and 3-D-projected text

### 8.1 Ordering guarantee

- The 3-D scene is fully finished before the UI pass because both are sequential within the **same** `BeginScene/EndScene` block and `RenderObject()` returns only after the whole tree walk (`NewApp.cpp:731-733`). There is no interleaving, no second `BeginScene`. `[OBSERVED]`
- The UI pass switches projection/view (`SetMatrixForUI`) and never restores them — the next frame re-establishes both. `[OBSERVED]`

### 8.2 Floating name labels, HP bars, guild marks, chat bubbles

These are **2-D controls positioned by a 3-D projection**, not 3-D geometry:

1. Each `TMHuman` creates `m_pNameLabel` (`SText`), `m_pNickNameLabel`, `m_pProgressBar` (`SProgressBar`), `m_pMountHPBar`, `m_pChatMsg`, `m_pKillLabel`, `m_stGuildMark.pGuildMark`, `m_pTitleProgressBar`, auto-trade plates — and registers them with the scene's control container: `g_pCurrentScene->m_pControlContainer->AddItem(...)` (`TMHuman.cpp:340-357`). `[OBSERVED]`
2. Each frame, `TMHuman::LabelPosition()` (called at the **end** of `TMHuman::Render`, `:1777-1778`) does:
   `vecPos = (human.x, pickSize.y*scale + height + 0.2, human.z)`;
   `project = Vec3TransformCoord(vecPos, m_matView)` then `· m_matProj` (manual, not `D3DXVec3Project`);
   `if (0 ≤ z < 1)` compute `px = (x+1)/2*(W-widthShift)`, `py = (-y+1)/2*(H-heightShift-1)` with separate mount/pedestrian offsets, then `SetRealPos()` on the labels. Offscreen/invalid ⇒ `SetVisible(0)`. `[OBSERVED] TMHuman.cpp:7413-7623`
3. The labels are therefore drawn in **Pass 9**, in the UI layer of their control's tree depth — i.e. *above* all 3-D and potentially interleaved with other HUD panels by depth. `[OBSERVED]`
4. Same mechanism for damage numbers? No — damage numbers are `TMFont3` in `m_pExtraContainer` (Pass 8), because they animate independently of a control. `[OBSERVED] TMFieldScene.cpp:14387`

### 8.3 3-D objects inside the UI

`S3DObj` / `GeomControl` with `RENDER_3DOBJ` (character portrait, item icons, boss orbs): handled **inside** the UI pass, which temporarily re-enters the 3-D state block and the UI projection (`RenderDevice.cpp:3320-3368`). This is the only place the two worlds mix, and it is strictly local. `[OBSERVED]`

### 8.4 Modal / cursor layering

- `m_pDrawControl[]` layer index = widget tree depth; the **cursor is forced to layer 29** (`SControlContainer.cpp:307-308`), i.e. top-most. `[OBSERVED]`
- Modal message boxes are just controls with `m_bModal = 1` and are *not* given a higher layer — they are drawn at their own depth (usually later because they are added deep in the tree). `[OBSERVED] TMScene.cpp:196-206`, `SControlContainer.cpp:41-51`

---

## 9. Reimplementation notes (OpenGL/Vulkan)

1. **Encode the pass list literally** (§2) as an explicit ordered pipeline. Do not rely on a general scene graph unless you reproduce `AddChild`-prepend + DFS — the visual result depends on it (especially for additive stacking and un-sorted alpha).
2. **Frame structure**: one render pass, one depth buffer cleared to 1.0, colour cleared to the *sky* colour, `Z ∈ [0,1]`, `CULL_CCW` default. `LoadOp = CLEAR`, `StoreOp = STORE`, no MSAA (the D3D path sets `MultiSampleType` from the enumerated list but nothing in the frame depends on it).
3. **Four pipeline states** (blocks 0…3) as immutable objects; every draw is one of them plus a small delta. Reproduce `ONE / INVSRCALPHA` as the default world blend and `SRC_ALPHA / ONE` for `EF_BRIGHT`.
4. **Batch aggressively but keep order.** The terrain path is one `DrawPrimitiveUP` per tile with a CPU-side 4-corner projection test — in a modern port, build one vertex buffer per frame from the tiles that survive the test, and keep the same tile-window + 2:1 decimation rules so silhouettes and texture density match.
5. **Fog is per-vertex linear with a colour identical to the clear colour**, and the far plane (70) is *behind* the fog end (~39–44 in the field) — that is what hides the terrain edge. If you use shader fog, use the same start/end formula `(m_fFogStart + sightLength − 8) … (m_fFogEnd + sightLength − 15)`.
6. **Sky must be camera-locked and fog-free**, drawn *after* items and *before* characters in this client. If you "fix" the order to sky-first you will change nothing visually *except* additive items behind the horizon — verify before changing.
7. **UI in pixel space.** Emit `XYZRHW`-equivalent quads (position in pixels, `z = 0.01`, `rhw = 1`) rather than routing through an ortho matrix; then `g_nUIFadeAlpha`, the `800×600 × ratio` scaling and all the hard-coded offsets line up. Use a separate ortho pass only if you also port every `BASE_ScreenResize`/ratio expression.
8. **Text**: the client rasterises text into a 512×512 `DIB` font atlas on the CPU (`RenderDevice.cpp:564-599`) and draws per-string quads (`TMFont2::Render`). In a port, replace with a proper glyph atlas + one draw per string, but keep the *pixel* X/Y math (`m_nFontSize` chosen by width, +1 px shadow offset for non-`RENDER_TEXT` variants).
9. **Don't forget the four "one-shot" flags** that change visuals for exactly one frame: `m_bShowBoss` (light blackout), `m_bPunchEffect` (character emissive), `g_bHideEffect`/`g_bHideBackground` (debug), `m_nState` of the sky (weather transition cross-fade). They are per-frame state, not persistent settings.
10. **Verification harness**: `g_objectnumber`, `g_effectnumber`, `g_totaleffect` are counted in `ObjectManager::FrameMove`/`EffectFrameMove` and printed by the FPS overlay (`RenderDevice.cpp:428`). Use them to compare "how many nodes are being considered" against the original client on the same map/position — a cheap way to validate your culling port.
