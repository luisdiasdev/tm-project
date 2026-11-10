# 09 — 2D / UI / Text Rendering (WYD 748 client, TMProject D3D9 codebase)

Complete specification of the 2D drawing side of the client — the UI projection, every
quad/sprite primitive, the control layer that feeds it, text (GDI→atlas), 3D-projected
overlays, guild marks, dynamic textures, blending and draw ordering — so that the same
pixels can be produced with OpenGL or Vulkan from this document alone.

Last updated: 2026-11-10

Scope note: **texture *file* formats (.wys/.wyt/TGA/DDS decoding) are out of scope** —
this chapter owns the *draw* side, text, sprites and the 2D coordinate system. Where a
texture's bit layout matters for drawing it is stated as a constant only.

---

## Confidence summary

| Tag | Count in this document |
|---|---|
| `[OBSERVED]` (read in source / measured in real asset files) | 84 |
| `[INFERRED]` (deduced, evidence stated) | 10 |
| `[UNKNOWN]` (experiment named) | 7 |

Counts are of the explicit inline tags; most tags are attached to a paragraph or table
row that covers several individual facts (e.g. §9.2's state table carries one tag for
~40 constants), so the number of individual verifiable claims is several times the tag
count. Every file/line reference in this document was read directly from the tree at
`Projects/TMProject/` and every asset figure was measured with a byte-level reader;
hexdumps are in `evidence/hex/`.

Biggest unknowns, in order of risk to a reimplementation:

1. **`ID3DXSprite` internal state with `Begin()` flags = 0.** `RenderRect`/`RenderRectRot`
   call `m_pSprite->Begin()` with *no* flags, so `D3DXSPRITE_ALPHABLEND` is not requested
   and the effective blend/alpha-test state depends on the D3DX9 runtime and whatever
   state `SetRenderStateBlock()` left behind. §9.4 gives the experiment that settles it.
2. **Stage-0 colour op in state block 0.** Read literally (`SELECTARG1(TEXTURE)`),
   `dwColor` cannot tint solid fills or stretched images, which contradicts every call
   site. Almost certainly a transcription artifact for `MODULATE`. §9.2 callout.
3. **`ID3DXSprite::Draw` rotation-pivot semantics** (7-argument overload):
   `pRotationCenter` is documented as source-rect-relative, but `RenderRect` passes an
   absolute texture coordinate while `RenderRectRot` passes a destination-space centre.
   Only `RenderRectRot` is affected (rotation ≠ 0). §3.3–3.4, §2.6.
4. **748 scene-`.bin` record length vs `BinPanel`** (44 bytes): the record at offset 0 is
   not followed by a valid control type, and a 44-byte stride over `FieldScene2.bin` fails
   for 528 of 804 candidates, while the minimap record at 2784 *is* followed by a valid
   type. The panel record layout is therefore not settled. §4.1.
5. Whether the 748 original ever called `RenderRectTex2` / `RenderRectTex2C` /
   `RenderRectTex2M` / `RenderRectProgress2` (all four are **dead** in this source and
   three of them are internally broken — §3.10, §3.11).

---

## Table of contents

1. [Mental model: two passes, one of which ignores the matrices](#1-mental-model)
2. [The 2D coordinate system, ratios and matrices](#2-coordinate-system)
3. [The `RenderDevice` 2D primitive catalogue](#3-primitives)
4. [The control / geometry layer](#4-control-layer)
5. [Text rendering](#5-text)
6. [3D-projected 2D overlays](#6-overlays)
7. [Guild marks](#7-guild-marks)
8. [Dynamic textures (`GenerateTexture`)](#8-dynamic-textures)
9. [Alpha, colour, blending, depth, clipping](#9-alpha)
10. [Ordering and overlap](#10-ordering)
11. [Reimplementation notes (OpenGL / Vulkan)](#11-reimplementation)
12. [Appendix A — symbol index](#appendix-a)

---

<a name="1-mental-model"></a>
## 1. Mental model: two passes, one of which ignores the matrices

A frame is exactly two submissions `[OBSERVED]` `NewApp.cpp:716-742`:

```
RenderScene():
  device->SetViewPort(0,0,W,H)                 // also calls SetProjectionMatrix()
  device->Lock(1)                              // Clear(ZBUFFER|TARGET), BeginScene
    SetViewVector(camera)                      // m_matView
    SetRenderStateBlock(1)                     // "3D" state block
    ObjectManager::RenderObject()              // scene graph incl. TMFont3, ground, models
    SetRenderStateBlock(3)                     // "UI" state block
    ObjectManager::RenderControl()             // *** all 2D UI ***
  device->Unlock(1)                            // EndScene + Present
```

`ObjectManager::RenderControl` `[OBSERVED]` `ObjectManager.cpp:591-626` does:

```
SetMatrixForUI();                 // projection + view for 3D-in-UI only
SetTexture(0..2, nullptr);
SetRenderState(ZENABLE, 0);
for layer in 0..29:               // MAX_DRAW_CONTROL = 30
    for ctl in m_pDrawControl[layer] (FIFO):
        SetRenderState(FOGENABLE, 0);
        RenderGeomControl(ctl);
    clear layer list
SetRenderState(ZENABLE, 1);
```

**The single most important fact of this chapter:** every 2D primitive draws with
`SetFVF(324)` = `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1` (0x144) or
`SetFVF(580)` = `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2` (0x244)
`[OBSERVED]` `RenderDevice.cpp:1936,1965,2019,2156,2282,2362,2442,2533,2576,3486`.
`XYZRHW` vertices are **pre-transformed**: the world/view/projection matrices are *not*
applied. Therefore:

* UI geometry is expressed in **device pixels**, Y down, origin top-left.
* `SetMatrixForUI()` affects **only** `RENDER_3DOBJ` controls (3D model previews such as
  the character sheet or a 3D item icon). Everything else in the UI pass ignores it.
* There is **no letterboxing, no virtual-canvas transform, no DPI scaling** anywhere in
  the draw path. All "scaling to resolution" is done per element, in C++, by multiplying
  authored constants with `m_fWidthRatio`/`m_fHeightRatio` or `BASE_ScreenResize()`.

---

<a name="2-coordinate-system"></a>
## 2. The 2D coordinate system, ratios and matrices

### 2.1 Static configuration `[OBSERVED]` `RenderDevice.cpp:18-31`

| Symbol | Decl | Init | Changed by |
|---|---|---|---|
| `m_dwCurrScreenX/Y` | `DWORD` | 1024 / 768 | display mode |
| `m_nFontSize` | `int` static | 12 | `NewApp::InitInstance` (`NewApp.cpp:274-294`) |
| `m_nLargeFontSize` | `int` static | 40 | ctor × `m_fWidthRatio` (`RenderDevice.cpp:69`) |
| `m_nFontTextureSize` | `int` static | 512 | forced to 512 (`NewApp.cpp:295`) |
| `m_nFontTextureSizeY` | `int` static | 64 | never changed (used as `m_nFontTextureSize/8` implicitly) |
| `m_fWidthRatio` | `float` static | 1.0 | `W/800.0f` in ctor (`:64`) |
| `m_fHeightRatio` | `float` static | 1.0 | `H/600.0f` in ctor (`:65`) |
| `m_fFOVY` | `float` static | 0.25 (× π rad = 45°) | — |
| `m_nBright` | `int` static | 50 | options; >58 darkens all text (§5.2) |
| `g_nUIFadeAlpha` | `int` | 255 | server-select `[DEMO]` fade (`TMSelectServerScene.cpp:146`) |

`m_nFontSize` per width `[OBSERVED]` `NewApp.cpp:275-292`:

| back-buffer width | 640 | 800 | 1024 | 1280 | anything else |
|---|---|---|---|---|---|
| `m_nFontSize` | 10 | 12 | 14 | 20 | 24 |

### 2.2 The "virtual canvas": 800×600, but only as a *scaling source*

There is no canvas transform. The design grid is 800×600 and two conversion helpers exist:

```c
m_fWidthRatio  = (float)W / 800.0f;      // RenderDevice.cpp:64
m_fHeightRatio = (float)H / 600.0f;      // RenderDevice.cpp:65
BASE_ScreenResize(v) = (float)W * (v / 800.0f);   // Basedef.cpp:33-36  == v * m_fWidthRatio
```

`[OBSERVED]` `SControl.cpp:183-191` shows the intended identity explicitly:

```c
SetAutoSize():   m_nPosX = m_nPosX/800.0f * W;  m_nPosY = m_nPosY/600.0f * H;
SetCenterSize(): m_nPosX += (W-800)>>1;         m_nPosY += (H-600)>>1;
```

**Three different conventions coexist**, and a reimplementation must preserve each:

1. **Raw device pixels** — everything loaded from the `*.bin` scene files
   (`BinPanel.nStartX` → `SControl::m_nPosX`, `TMScene.cpp:341-348`) and most runtime
   positions (`m_nPosX = nX` on mouse move). Not rescaled for resolution.
2. **`BASE_ScreenResize()` / `* m_fWidthRatio`** — runtime-created panels, text offsets,
   name-label centring, damage-number offsets. Scales X by `W/800`, Y by `H/600`, i.e.
   **non-uniformly** on non-4:3 aspect ratios.
3. **Fixed pixel constants** — e.g. `m_nHeight - 16.0f*m_fHeightRatio)/2 + 2` for text
   vertical centring (`SControl.cpp:950-951`), `104` in align type 4, `+1` pixel text
   offset, `-12.0f` in text centring.

The nominal 4:3 assumption is baked into `TMMesh::RenderForUI` `[OBSERVED]`
(`TMMesh.cpp:285-290`): it maps screen pixels into a world box using
`6.3` (or `6.26` when `viewport.Width/viewport.Height < 1.26`) horizontally and `4.96`
vertically — constants that only line up with the projection matrix for 4:3.

### 2.3 `SetProjectionMatrix()` — the 3D projection `[OBSERVED]` `RenderDevice.cpp:1839-1854`

```c
D3DXMatrixPerspectiveFovLH(&m_matProj,
      m_fFOVY * D3DXToRadian(180),                       // 0.25 * PI = 0.785398 rad = 45 deg
      (float)((W - m_nWidthShift) / (H - m_nHeightShift)),// aspect, shift = pillarbox bars
      g_ClipNear * 1.4f,                                  // 0.69*1.4 = 0.966
      g_ClipFar);                                         // 70 in field, 90/540 elsewhere
SetTransform(D3DTS_PROJECTION, &m_matProj);
transpose(m_matProj) -> SetVertexShaderConstantF(2, ..., 4);   // c2..c5 for the VS path
```

`g_ClipNear = 0.69f`, `g_ClipFar = 540.0f` (`TMGlobal.cpp:23-24`); the scenes override
`g_ClipFar` to 70 (`TMScene.cpp:34`, `TMSelectCharScene.cpp:23`, `TMSky.cpp:254`) or 90
(`TMSky.cpp:412`). `SetViewPort()` calls `SetProjectionMatrix()` after every viewport
change (`:357`), so the aspect always follows the viewport, not the window.

Symbolically (D3DX row-vector convention, `v' = v·M`):

```
         │ h/aspect   0       0            0      │      h = 1/tan(fovy/2)
M_proj = │   0        h       0            0      │      w = h/aspect
         │   0        0   zf/(zf-zn)        1      │
         │   0        0  -zn·zf/(zf-zn)     0      │
```

Numeric, 45° FOV, `zn = 0.966`, `zf = 70`:

```
1024x768 (aspect 1.333333)          1600x900 (aspect 1.777778)
 1.810660  0         0        0      1.357995  0         0        0
 0         2.414214  0        0      0         2.414214  0        0
 0         0         1.013993 1      0         0         1.013993 1
 0         0        -0.979517 0      0         0        -0.979517 0
```

### 2.4 `SetMatrixForUI()` — the UI projection and view `[OBSERVED]` `RenderDevice.cpp:1856-1872`

```c
D3DXMatrixPerspectiveFovLH(&matUIProjection,
      0.1f,                                        // fovy = 0.1 rad (5.73 deg), NOT deg->rad
      (float)(m_d3dsdBackBuffer.Width / (float)m_d3dsdBackBuffer.Height) * 0.94f,
      10.0f, 100.0f);
SetTransform(D3DTS_PROJECTION, &matUIProjection);

D3DXMatrixLookAtLH(&matView, eye(0,0,50), at(0,0,0), up(0,1,0));
SetTransform(D3DTS_VIEW, &matView);
```

Note both matrices are **locals** — `m_matProj`/`m_matView` keep the 3D values.
`m_matWorld` is identity (`RenderDevice.cpp:1577-1578`).

Symbolically:

```
h = 1/tan(0.05) = 19.983331          w = h / ((W/H)*0.94)
         │ w  0  0     0 │
M_uiproj │ 0  h  0     0 │            zn=10, zf=100 ->  zf/(zf-zn) = 1.111111
         │ 0  0  1.1111 1 │                              -zn·zf/(zf-zn) = -11.111111
         │ 0  0 -11.1111 0 │

         │ -1  0  0   0 │              (D3DX LookAtLH with up=+Y and view dir -Z flips X)
M_uiview │  0  1  0   0 │
         │  0  0 -1  -50 │
         │  0  0  0   1 │
```

Numeric:

```
1024x768 (aspect = (1024/768)*0.94 = 1.253333)   1600x900 (aspect = (1600/900)*0.94 = 1.671111)
 15.944147  0          0        0                11.958110  0          0        0
 0         19.983331   0        0                0         19.983331   0        0
 0         0          1.111111  1                0         0          1.111111  1
 0         0        -11.111111  0                0         0        -11.111111  0
```

Note `800x600` and `1024x768` produce **identical** UI projections because the `*0.94`
fudge makes the aspect depend only on the aspect ratio, and both are 4:3.

The `0.94` fudge is exactly cancelled by `TMMesh::RenderForUI`'s `fWidthRatio = 6.3`
(§6.4): `2·tan(0.05)·50 / (6.3/2) = 5.0042/3.15 → 1.588…`; measured against the
`0.94`-scaled aspect the usable screen maps to `x ∈ [-3.15, +3.15]` (±0.45 % error) and
`y ∈ [-2.48, +2.48]` (0.8 % error).

**The X flip is not a bug in the output**: `S3DObj` meshes are placed by a world matrix
built from screen pixels (`TMMesh.cpp:289-309`):

```
world = m_matWorld(identity) · RotYawPitchRoll(yaw + fAngle, pitch − π/2, 0) · Scale · Translate
Translate = ( −fX − c.x·0.52·fScale ,  −fY − c.z·0.52·fScale ,  c.y·0.52·fScale )
```

The translate uses `−fX`, and the view then negates X again, so the model lands at the
intended screen X. Do **not** copy the flip into a GL implementation: build the same world
matrix and use an orthographic projection `x ∈ [−3.15, +3.15]`, `y ∈ [−2.48, +2.48]`,
`z ∈ [10, 100]`-equivalent with Y pointing *down* care. `[UNKNOWN]` whether the model body
itself ends up mirrored (a mirrored view of a mesh is a mirrored image unless the mesh is
authored for it) — settle by comparing a character-preview screenshot with the original.

### 2.5 Y axis, Z and RHW

* UI quads: `position = (x_px, y_px, z)`, `z = 0.01f` for every vertex of every 2D
  primitive — set once at init and never written again `[OBSERVED]` `RenderDevice.cpp:208-282`;
  `RenderGeomControlBG` is the single exception, using a local array with `z = 0.1f`
  (`:3461-3477`).
* `rhw = 1.0f` always (`:210-213` etc.), never modified — so z/w is literal.
* Y grows **downwards** in pixel space (D3D viewport convention for XYZRHW).
* Because `ZENABLE = 0` for the whole UI pass (`ObjectManager.cpp:602`), the `0.01/0.1`
  Z values have **no visible effect**. Draw order alone decides overlap (§10).
* The UI view matrix maps world `z` to eye `z−50`; a UI-in-3D object at `z≈0` sits
  49.99 units from the eye against a near plane of 10 and far plane of 100.

### 2.6 Pixel-centre conventions and the `+0.5` rule

Three distinct conventions are used, all `[OBSERVED]`:

| Context | Convention | Code |
|---|---|---|
| Font (`RenderRectC`) | left/top edge = `x/W`, right/bottom = `(x+size+0.5)/W` — **asymmetric half-texel inset on the far edges only** | `RenderDevice.cpp:1917-1930` |
| General tex (`RenderRectTex`) | `(x + 0.5)/W` on the near edge, `(x+size)/W − 0.5/W` on the far edge — **symmetric half-texel inset** | `:2112-2119` |
| Damage/2C/2/M | **no** inset: `x/W` … `(x+size+0.5)/W` | `:2219-2246`, `:2318-2325`, `:2398-2405`, `:2477-2496` |

Under the point sampling that the UI state block selects (`MAGFITER = POINT`, §9.2), a
texel `i` is selected when `floor(u·W) == i`, i.e. texel centres are at `(i+0.5)/W`. The
recommended GL/Vulkan equivalent is `u = (x + 0.5)/W`, `v = (y + 0.5)/H` at both edges
of the *pixel range*, which reproduces the font path exactly and differs from the others
by at most one texel on the far edge.

There is also a global **1-pixel vertical offset for all text**: `nPosY = nPosY + 1`
`[OBSERVED]` `TMFont2.cpp:289`, plus a `nUp = -1` correction when `m_dwScreenWidth == 640`
(`RenderDevice.cpp:3281-3283, 3300-3302`).

### 2.7 `SetViewPort`, `m_nWidthShift`, `m_nHeightShift`, letterboxing

`[OBSERVED]` `RenderDevice.cpp:343-358`:

```c
m_viewport = { nStartX, nStartY, nWidth, nHeight, MinZ=0, MaxZ=1 };
SetViewport(&m_viewport);
SetProjectionMatrix();          // aspect uses (W - m_nWidthShift)/(H - m_nHeightShift)
```

`m_nWidthShift`/`m_nHeightShift` are always **0** in this port: initialised to 0
(`:52-53`) and explicitly re-zeroed by every scene (`TMFieldScene.cpp:552,628,12851…18922`,
`TMSelectCharScene.cpp:114`, `TMSelectServerScene.cpp:261`) `[OBSERVED]`. They are
consumed only by the aspect ratio, `GetPickRayVector` (`:1877-1878`) and every
`3D→2D` projection (`Basedef.cpp:3057-3061`, `TMHuman.cpp:7446-7456`), where they
subtract from the *screen* extent.

`[INFERRED]` They are the vestige of a pillarbox/anamorphic scheme (render 4:3 content
into part of a wider back buffer). Since they are always 0, **a reimplementation that
renders 1:1 to the back buffer is equivalent**; but if you ever add black bars, these
subtractions are the places that must learn about them.

There is **no** letterboxing, no aspect-ratio letterbox clear, and no scaling of the UI
to the back buffer. A `.bin` panel authored at x=636 stays at x=636 whether you are at
800×600 or 1920×1080 `[OBSERVED]` (`TMScene.cpp:341-348` + `SControl.cpp:365-382`).

### 2.8 Worked numeric examples

**(A) Code-created panel at logical (100,200), size (300,150), screen 1600×900.**
`m_fWidthRatio = 2.0`, `m_fHeightRatio = 1.5`.

* Created with `BASE_ScreenResize` (`SControl.cpp:183-191` style):
  `x = 100·2.0 = 200`, `y = 200·1.5 = 300`, `w = 300·2.0 = 600`, `h = 150·1.5 = 225`
  → device rect **(200,300)–(800,525)**. Text inside it is vertically centred at
  `y + (225 − 16·1.5)/2 + 2 = 300 + 100.5 + 2 = 402.5`, plus the `+1` text offset → 403.5.
* Loaded from a `.bin` (raw pixels): device rect **(100,200)–(400,350)** at every
  resolution — it does *not* move or grow.

**(B) The real minimap panel (`FieldScene2.bin` offset 2784).**
`[OBSERVED]` id 289, texture set `-11` → set 11 `[N_minimap]`, `x=636 y=0 w=137 h=170`,
colour `0x88FFFFFF`, fill = 4 (`RENDER_IMAGE_STRETCH`). Set 11 = `4,0,0,384,384,0,0`
(UI texture #4, source rect 384×384). `TMFieldScene.cpp:1993` sets
`fAngle = -0.78539819f` (−45°). `RenderGeomRectImage` takes the `< −2` branch and calls
`RenderRectTex(0,0, 384,384, 636,0, 137,170, tex, 0x88FFFFFF, bTrans=1, −0.78539819, 1.0)`.

Result at 1024×768: the quad is rotated about its destination centre (704.5, 85) with
half-extents (68.5, 85); corners are

```
src(-68.5,-85) -> (595.96,  73.33)
src(+68.5,-85) -> (692.83, -23.54)
src(+68.5,+85) -> (813.04,  96.67)
src(-68.5,+85) -> (716.17, 193.54)
```

i.e. a 45° diamond, its top vertex 24 px above the screen edge and its right vertex
11 px outside a 1024-wide buffer. `[OBSERVED]` arithmetic from the code and data; that the
top vertex is clipped is corroborated by `SPanel::FrameMove2` culling only on
`pos+size >= 0` and `pos <= 800·ratio` (`SControl.cpp:374-377`).

**(C) `3D→2D` projection of a floating label** (§6): for a non-mounted character at
camera sight length 11 (`TMScene.cpp:2296`), `vPosInY` loses `3·11 = 33` px; mounted
gains `3.5·11 = 38.5` px and uses the constant `0.76` instead of `1.16`.

---

<a name="3-primitives"></a>
## 3. The `RenderDevice` 2D primitive catalogue

### 3.1 Vertex layouts `[OBSERVED]` `Structures.h:212-245`

`TMVector3` is three `float` (`Structures.h:96-100`), 12 bytes, no vtable.

**`RDTLVERTEX`** — FVF `0x144` (324), stride **28**:

| offset | type | size | field | meaning |
|---|---|---|---|---|
| 0x00 | float ×3 | 12 | `position` | x,y in device pixels; z always 0.01 |
| 0x0C | float | 4 | `rhw` | always 1.0 |
| 0x10 | uint32 | 4 | `diffuse` | ARGB colour, see §9.1 |
| 0x14 | float | 4 | `tu` | u |
| 0x18 | float | 4 | `tv` | v |

**`RDTLVERTEX2`** — FVF `0x244` (580), stride **36**:

| offset | type | size | field | meaning |
|---|---|---|---|---|
| 0x00 | float ×3 | 12 | `position` | as above |
| 0x0C | float | 4 | `rhw` | 1.0 |
| 0x10 | uint32 | 4 | `diffuse` | ARGB |
| 0x14 | float | 4 | `tu1` | stage 0 u |
| 0x18 | float | 4 | `tv1` | stage 0 v |
| 0x1C | float | 4 | `tu2` | stage 1 u (`D3DTSS_TEXCOORDINDEX = 1`) |
| 0x20 | float | 4 | `tv2` | stage 1 v |

**Device-owned vertex arrays** (`RenderDevice.h:113-117`) — reused static buffers, *not*
per-instance:

| array | type | count | used by |
|---|---|---|---|
| `m_CtrlVertex` | `RDTLVERTEX` | 4 | `RenderRectC`, `RenderRectCoord`, `RenderRectNoTex`, `RenderRectTex`, `RenderRectTexDamage`, **and all three broken `*Tex2*` paths** |
| `m_CtrlProgressVertex` | `RDTLVERTEX` | 10 | `RenderRectProgress2` (fan of up to 9 triangles) |
| `m_CtrlVertex2` | `RDTLVERTEX2` | 4 | `RenderRectTex2` writes here — then draws `m_CtrlVertex` |
| `m_CtrlVertexC2` | `RDTLVERTEX2` | 4 | `RenderRectTex2C` writes UVs here — then draws `m_CtrlVertex` |
| `m_MiniMapVertex2` | `RDTLVERTEX2` | 4 | `RenderRectTex2M` writes here — then draws `m_CtrlVertex` |

Initialisation `[OBSERVED]` `RenderDevice.cpp:208-282` seeds every vertex with
`position=(0,0,0.01)`, `rhw=1.0`, `diffuse=0xAAAAAAAA`, `tu/tv ∈ {0.01,0.99}`. Only
x/y/uv/diffuse are written by the primitives; **z and rhw are never touched again**.

### 3.2 Primitive summary table

| Function | Line | Path | Vertex buffer | FVF / stride | Texture stages | Rotation | Scale | Sets blend? |
|---|---|---|---|---|---|---|---|---|
| `RenderRect` | 1893 | **ID3DXSprite** | internal | sprite-managed | 1 | no (0) | `fScaleX/Y` | sprite-managed |
| `RenderRectRot` | 2759 | **ID3DXSprite** | internal | sprite-managed | 1 | `-fAngle` | `fScaleX/Y` | sprite-managed |
| `RenderRectC` | 1913 | immediate UP | `m_CtrlVertex` | 324 / 28 | 1 | no | `fScaleX/Y` | **no** |
| `RenderRectCoord` | 1942 | immediate UP | `m_CtrlVertex` | 324 / 28 | 1 | no | no | **no** |
| `RenderRectNoTex` | 1971 | immediate UP | `m_CtrlVertex` | 324 / 28 | 1 (null tex) | no | no | `bTrans` |
| `RenderRectTex` | 2043 | immediate UP | `m_CtrlVertex` | 324 / 28 | 1 | yes | `fScale` | `bTrans` |
| `RenderRectTex2C` | 2164 | immediate UP | **`m_CtrlVertex`** (bug) | 580 / 36 | 2 | yes | `fScale` | yes (always) |
| `RenderRectTex2` | 2294 | immediate UP | **`m_CtrlVertex`** (bug) | 580 / 36 | 2 | **no** | `fScale` | yes |
| `RenderRectTex2M` | 2374 | immediate UP | **`m_CtrlVertex`** (bug) | 580 / 36 | 2 | **no** | `fScale` | yes |
| `RenderRectTexDamage` | 2454 | immediate UP | `m_CtrlVertex` | 324 / 28 | 1 | **half** (see §3.9) | `fScale` → half-size centred | `bTrans` |
| `RenderRectProgress2` | 2546 | immediate UP | `m_CtrlProgressVertex` | 324 / 28 | 1 (null tex) | no | fixed | yes (always) |
| `RenderGeomControlBG` | 3429 | immediate UP | local `v[4]` | 324 / 28 | 1 | no | `fScale` | additive |

Fan/winding: all quads are submitted as `D3DPT_TRIANGLEFAN` with `PrimitiveCount = 2`
(except the progress ring, `1..8`). Vertex order is **TL, TR, BR, BL** for every quad —
consistent with `CULLMODE = NONE` (CCW/CW cannot matter; do not enable culling in GL/VK).

### 3.3 `RenderRect` (sprite, 1): `[OBSERVED]` `RenderDevice.cpp:1893-1911`

```c
srcRect   = { (int)iStartX, (int)iStartY, (int)(iStartX+iCX), (int)(iStartY+iCY) };
rotCenter = { iCX/2 + iStartX, iCY/2 + iStartY };   // texture-space
destPoint = { iDestX, iDestY };                     // top-left, device px
scaleVec  = { fScaleX, fScaleY };
if (pTexture && m_pSprite) {
    m_pSprite->Begin();                                  // flags = 0 !
    m_pSprite->Draw(pTexture, &srcRect, &scaleVec, &rotCenter, 0.0f, &destPoint, 0xFFFFFFFF);
    m_pSprite->End();
}
```

* Colour is hard-wired **opaque white** `0xFFFFFFFF` — no tinting possible.
* Source rect is truncated to integers, so sub-pixel scrolling is impossible.
* Rotation is 0 → `rotCenter` is inert (§2.6 and unknown #3).
* `m_pSprite` is created once in `Initialize` (`:181-188`) and re-created on device reset.
* Used by: `RenderGeomRectImage` for `RENDER_IMAGE` (normal panels/buttons) and for
  guild marks, and by `RenderRectRot` internally? No — only those two call sites
  `[OBSERVED]` (grep: `RenderRect(` appears at `:2853`, `:3224`).

### 3.4 `RenderRectRot` (sprite, rotated 1): `[OBSERVED]` `:2759-2777`

Same as above but `m_pSprite->Draw(..., &rotCenter, -fAngle, &destPoint, 0xFFFFFFFF)` with
`rotCenter = { nCenX, nCenY }` supplied by the caller. `RenderGeomRectImage` always
passes `nWidth/2, nHeight/2` — the **centre of the destination rect**, not of the source
rect, which is why this is the only rotation path that visibly works.

Used for:
* `RENDER_IMAGE_STRETCH` (all stretched panels, SButtons, progress-bar backgrounds):
  `scale = (nWidth/srcW, nHeight/srcH)`, rotation `ipControl->fAngle`.
* The `sSanc` and `sLegend` decorations (texture 338, 35×35 sub-rects, §4.6).
* The `bClip == 1` variant that trims the source rect vertically by `fTop/fBottom`
  (`:2869-2885`) — the only "clipping" in the UI (§9.5).

### 3.5 `RenderRectC` (coloured quads, font-specific): `[OBSERVED]` `:1913-1940`

Immediate mode with `m_CtrlVertex`; UV scaling hard-codes the **font** atlas
`512 × 64` (`m_nFontTextureSize`, `m_nFontTextureSizeY`), which is why it cannot be used
for general textures:

```
v0 = (destX,               destY,               0.01)  uv( startX/512,               startY/64 )
v1 = (destX + cx*sx,       destY,               0.01)  uv((startX+cx+0.5)/512,      (startY+0.5)/64 )
v2 = (destX + cx*sx,       destY + cy*sy,       0.01)  uv((startX+cx+0.5)/512,      (startY+cy+0.5)/64 )
v3 = (destX,               destY + cy*sy,       0.01)  uv( startX/512,              (startY+cy+0.5)/64 )
diffuse[0..3] = dwColor
SetTexture(0, pTexture); SetFVF(324); DrawPrimitiveUP(TRIANGLEFAN, 2, m_CtrlVertex, 28);
```

No blend state is set — the caller must have set it (`TMFont2::Render` calls
`SetRenderStateBlock(2)` first, §9.2).

### 3.6 `RenderRectCoord` (tiled): `:1942-1969`

```
v0=(destX,destY)            uv(0,0)
v1=(destX+cx,destY)         uv(fU,0)
v2=(destX+cx,destY+cy)      uv(fU,fV)
v3=(destX,destY+cy)         uv(0,fV)
```

`RENDER_IMAGE_TILE` passes `fU = nWidth/srcW`, `fV = nHeight/srcH`, so UVs exceed 1 and the
texture is tiled by wrap mode. `[OBSERVED]` No sampler state with
`D3DSAMP_ADDRESSU/V = WRAP` is ever set for the UI stage 0 — the default
(`D3DTADDRESS_WRAP`) applies, so tiling works, but so does everything else. Flag for
GL: use `GL_REPEAT`.

### 3.7 `RenderRectNoTex` (solid fill): `:1971-2041`

```
v[TL]=(x,y) v[TR]=(x+cx,y) v[BR]=(x+cx,y+cy) v[BL]=(x,y+cy)   // z/rhw from init
diffuse[all] = dwColor
SetTexture(0, nullptr);
if (bTrans == 1) {
    stage0 ALPHAOP = 4 (MODULATE); ALPHABLENDENABLE=1;
    SRCBLEND=SRCALPHA(5); DESTBLEND=INVSRCALPHA(6); ALPHAFUNC=8 (ALWAYS);
    ALPHATESTENABLE=0; ZWRITEENABLE=0;
} else {
    ALPHABLENDENABLE = 0;                    // fully opaque, alpha ignored
}
SetFVF(324); DrawPrimitiveUP(TRIANGLEFAN, 2, m_CtrlVertex, 28);
```

The `ALPHAREF` ping-pong at the head and tail (0xFF000000 / 0xF000 / 0xDD / 0xD,
`:1987-2001` and `:2026-2040`) is a lazily-restored VGA-dependent constant; it has no
effect here because `ALPHATESTENABLE` is 0 in the `bTrans` path and the tail value is
overwritten by the next primitive that cares.

### 3.8 `RenderRectTex` (stretch + rotate, immediate): `:2043-2162`

Reads the real texture size with `GetLevelDesc(0)` (default `512×512` if `pTexture` is
null). Two geometry branches:

**No rotation (`|fAngle| ≤ 0.1`)** — TL/TR/BR/BL from `(iDestX,iDestY)` and
`(DestCX*fScale, DestCY*fScale)`.

**Rotation** — about the destination-rect centre:

```
fRX = DestCX*fScale/2;      fRY = DestCY*fScale/2;
fCenterX = iDestX + fRX;    fCenterY = iDestY + fRY;
s = sin(fAngle); c = cos(fAngle);
v0 = ( -fRX*c + fRY*s + fCenterX , -fRX*s - fRY*c + fCenterY )
v1 = (  fRX*c + fRY*s + fCenterX ,  fRX*s - fRY*c + fCenterY )
v2 = (  fRX*c - fRY*s + fCenterX ,  fRX*s + fRY*c + fCenterY )
v3 = ( -fRX*c - fRY*s + fCenterX , -fRX*s + fRY*c + fCenterY )
```

UVs:

```
fEndX = iStartX+iCX) / texW          (plus a 1e-4 truncation to 4 decimals if m_iVGAID==1)
fEndY = (iStartY+iCY) / texH
u0 = iStartX/texW + 0.5/texW         v0 = iStartY/texH + 0.5/texH
u1 = fEndX - 0.5/texW                v1 = same as v0
u2 = u1                              v2 = fEndY - 0.5/texH
u3 = u0                              v3 = v2
```

The truncation `(float)(int)((nEndX/fTexWidth)*10000.0f) * 0.000099999997f`
(`:2103-2104`) is a VGA-only rounding hack (1/10000 texture-space quantisation).

Then `ALPHAREF` ping-pong, `SetTexture(0,pTexture)`, `bTrans` handling identical to
`RenderRectNoTex`, `SetFVF(324)`, fan.

### 3.9 `RenderRectTexDamage` (centred, half-rotation): `:2454-2544`

Geometry is **centred on `(iDestX,iDestY)`** and the size is `DestCX*fScale*0.5`:

```
half = fScale*0.5
v[TL]=(destX - DestCX*half, destY - DestCY*half)
v[TR]=(destX + DestCX*half, destY - DestCY*half)
v[BR]=(destX + DestCX*half, destY + DestCY*half)
v[BL]=(destX - DestCX*half, destY + DestCY*half)
```

For `|fAngle| ≤ 0.1` the UVs are the plain rect (`startX/texW … (startX+cx)/texW`). For a
non-zero angle it uses a **fixed 45° corner-rotated UV set** that does not depend on
`fAngle` at all (`:2488-2495`) — the sprite is not actually rotated, only its texture is
swapped for a diagonal arrangement. `[OBSERVED]` `fAngle` is always 0 at the call site
(`TMFont3::Render` passes `0.0f`), so this branch is currently unreachable.

This is the primitive used for **floating damage numbers, "miss", EXP text** (§5.8).

### 3.10 `RenderRectTex2C` / `RenderRectTex2` / `RenderRectTex2M` (two textures)

All three set up two texture stages and FVF 580, and all three are **broken in this port**
`[OBSERVED]` — documented so they are not accidentally imitated:

| defect | where |
|---|---|
| Positions written into `m_CtrlVertexC2` / `m_CtrlVertex2` / `m_MiniMapVertex2` but `DrawPrimitiveUP` is passed `m_CtrlVertex` | `:2286`, `:2366`, `:2446` |
| `RenderRectTex2C` rotation branch writes vertices 2 of 4 into `m_CtrlVertex` (wrong buffer) | `:2206-2207` |
| `if (pTexture) { pTexture2->GetLevelDesc(...) }` — tests one pointer, dereferences the other | `:2298-2304`, `:2378-2384` |
| `RenderRectTex2`/`2M` ignore `fAngle` entirely | `:2306-2313`, `:2386-2393` |
| `2C` computes `fEndX2/fEndY2` from `iStartX+iCX` (the *first* rect) instead of `iStartX2+iCX2` | `:2236-2237` |

Semantics if repaired:
* **2C** — two independent source rects (`iStartX/Y/..` and `iStartX2/Y2/..`) mapped onto
  one destination quad; stage 0 uses texcoord set 0, stage 1 uses set 1
  (`D3DTSS_TEXCOORDINDEX 1`), and stage 1 is `MODULATE`.
* **2** / **2M** — one source rect for both textures (`pTexture` at set 0, `pTexture2` at
  set 1); `2M` exists only because it keeps a *separate* vertex array named for the
  minimap (`m_MiniMapVertex2`), i.e. it was the minimap's two-layer blit (map + fog/light
  overlay). Identical code otherwise.

Grep shows **no call sites** anywhere in the ported source `[OBSERVED]`.

### 3.11 `RenderRectProgress2` (solid progress ring): `:2546-2757`

Draws a **border ring of 8 trapezoid segments** filled clockwise from the top-centre,
using a triangle fan whose centre vertex is `(x+cx/2, y+cy/2)`. `fProgress` is bucketed
into 8 bands of 0.125; each band appends one more outer vertex:

```
fSX = x + cx/2        fSY = y + cy/2        fWidth = cx/2      fHeight = cy/2
vertex 0 = centre
outer polygon (clockwise from top):  (SX,SY-H) (SX+W,SY-H) (SX+W,SY+H) (SX,SY+H) (SX-W,SY+H) (SX-W,SY) (SX-W,SY-H) …
```

`PrimitiveCount` is `1..8`, no texture (`SetTexture(0,0)`), colour from `dwColor`, blend
`SRCALPHA/INVSRCALPHA`, `ZWRITEENABLE 0`, `ALPHATEST 0`. **No call sites** in the ported
source. Dead code; do not port unless a 755/760 feature needs it.

### 3.12 `RenderGeomControlBG` (additive glow backing): `:3429-3489`

Used behind a 3D item icon (`RENDER_3DOBJ`) as a coloured halo. Local `RDTLVERTEX v[4]`
with `z = 0.1f`, `rhw = 1.0f`. Geometry is **centred** on `(nPosX,nPosY)`:

```
halfW = ipControl->m_fWidth  * 0.5f * fScale      (×2 if dwColor == 0xFFFFFFFF)
halfH = ipControl->m_fHeight * 0.5f * fScale
v[TL]=(x-halfW, y-halfH, 0.1) uv(0.0, 0.0)
v[TR]=(x+halfW, y-halfH, 0.1) uv(0.1, 0.0)     <-- note the 0.1, not 0.5/… (degenerate U)
v[BL]=(x-halfW, y+halfH, 0.1) uv(0.0, 1.0)
v[BR]=(x+halfW, y+halfH, 0.1) uv(1.0, 1.0)
```

Note the vertex order is TL,TR,BL,BR — a **TRIANGLESTRIP** (`:3487`), unlike every other
primitive. U for the top-right vertex is `0.1` while the bottom-right is `1.0`, so the
halo texture is sampled with a skewed quad `[OBSERVED]` — almost certainly a decompiler
artifact or an original bug; keep or fix deliberately.

Colour animation: a linear ramp over 2 s

```
t     = (serverTime - m_dwStartTime) % 2000
ratio = ((t * 0.0005 - 0.5) * 2) * PI   // -PI..+PI (misnamed radians, used as a scalar)
ratio = ratio * 0.34 + 0.64             // 0.30 .. 0.98 .. (sawtooth)
colour= ARGB(dwColor) * ratio           // per channel, truncated
```

Blend: `SRCALPHA`, `DESTBLEND = 2 (ONE)` → **additive**. Texture index chosen from a
small table of the control's `dwBGColor` (`:3351-3362`): 123 default, 56 for
`0xFFFFFFAA`/`0xFFFF0000`, 273 for `0xFFFFFFFF`, 122 for `0xFF8800BB`/`0xFF444488`.

### 3.13 Draw pseudocode per control type

```
RenderGeomControl(ctl):
  if !ctl or !ctl.bVisible: return
  set ALPHAREF (VGA-dependent ping-pong)
  SetRenderStateBlock(0)                      # ZEN 1, FOG off, LIGHT off, CULL none,
                                              # ALPHABLEND **off**, stage0 = MODULATE(DIFFUSE,TEX)
  switch ctl.eRenderType:
    RENDER_TEXT, RENDER_SHADOW:
        ctl.pFont->Render(ctl.nPosX, ctl.nPosY + (W==640 ? -1 : 0), ctl.eRenderType)
    RENDER_IMAGE, RENDER_IMAGE_TILE, RENDER_IMAGE_STRETCH, RENDER_TEXT_FOCUS:
        RenderGeomRectImage(ctl)                       # §3.14
        if ctl.strString[0] and ctl.pFont:             # button caption
            if ctl.nTextureSetIndex == 526:
                font->Render(posX + nWidth - 7*len, posY + nHeight - 12, type)   # right-aligned
            else:
                font->Render(posX + (nWidth - 6*len)/2, posY + (nHeight-12)/2, type)  # centred
    RENDER_3DOBJ:
        ... mesh preview (needs SetRenderStateBlock(1), ZFUNC 8, world matrix from pixels)
        ... optional RenderGeomControlBG halo
  set ALPHAREF (VGA ping-pong again)
```

Note the caption maths uses a **fixed 6 px per character** (7 px for set 526) to centre
text — it ignores real glyph widths, so proportional-font captions are systematically
off-centre `[OBSERVED]` (`:3306,3313`).

### 3.14 `RenderGeomRectImage` — the texture-set resolution `[OBSERVED]` `:2833-3251`

```
dwColor = WYDCOLOR_SCALE_ALPHA(ctl.dwColor, g_nUIFadeAlpha)        # §9.1

if ctl.nTextureSetIndex >= 0:                     # --- A: normal set
    set = GetUITextureSet(ctl.nTextureSetIndex)
    if !set or set.nCount < ctl.nTextureIndex or !set.pTextureCoord: return
    coord = set.pTextureCoord[ctl.nTextureIndex]
    tex   = GetUITexture(coord.nTextureIndex, 2000)
    if eRenderType == RENDER_IMAGE:               # 2
        RenderRect(coord.nStartX, coord.nStartY, coord.nWidth, coord.nHeight,
                   coord.nDestX + ctl.nPosX, coord.nDestY + ctl.nPosY, tex, 1, 1)
    elif eRenderType == RENDER_IMAGE_STRETCH:     # 4
        sx = ctl.nWidth / coord.nWidth ; sy = ctl.nHeight / coord.nHeight
        if ctl.bClip:  RenderRectRot(src trimmed by fTop/fBottom, ...); return
        if ctl.sSanc > 0:  decorative 35x35 from texture 338 at (35*(sSanc-1), 0)
        if ctl.sLegend > 0: decorative 35x35 from texture 338 at a legend table offset
        RenderRectRot(coord rect, coord.nDestX+ctl.nPosX, coord.nDestY+ctl.nPosY,
                      ctl.nWidth/2, ctl.nHeight/2, ctl.fAngle, tex, sx, sy)
    elif eRenderType == RENDER_IMAGE_TILE:        # 3
        RenderRectCoord(coord.nDestX+pos, coord.nDestY+pos, ctl.nWidth, ctl.nHeight, tex,
                        dwColor, ctl.nWidth/coord.nWidth, ctl.nHeight/coord.nHeight)
elif ctl.nTextureSetIndex < -2:                   # --- B: abs(set), stretched
    set = GetUITextureSet(-ctl.nTextureSetIndex); coord = set.pTextureCoord[ctl.nTextureIndex]
    tex = GetUITexture(coord.nTextureIndex, 2000)
    RenderRectTex(coord rect, coord.nDestX+pos, coord.nDestY+pos,
                  ctl.nWidth, ctl.nHeight, tex, dwColor, bTrans=1, ctl.fAngle, 1.0)
    return
else:                                             # --- C: -1 / -2
    if 0 <= ctl.nMarkIndex <= 64 and m_stGuildMark[nMarkIndex].pTexture:  # guild mark, §7
        ...
    else:
        RenderRectNoTex(pos, size, dwColor, bTrans = (nTextureSetIndex == -2))
```

**Texture-set coordinate record** `[OBSERVED]` `TextureManager.h:22-37`,
`TextureManager.cpp:426-464`:

| field | meaning |
|---|---|
| `nTextureIndex` | index into the global UI texture table (512 slots) |
| `nStartX/nStartY` | source rect origin **inside that atlas texture**, in texels |
| `nWidth/nHeight` | source rect size, in texels |
| `nDestX/nDestY` | offset added to the control position when drawing (the "hotspot"), in device px |

Loaded from `UI/UITextureSetList.txt` (text, `fscanf` format
`"%s\r\nSetIndex: %d\r\nItemCount: %d\r\n"` then `"%d,%d,%d,%d,%d,%d,%d"` per item).
748 has **446 sets, indices 0..456 with 87, 129, 281–289 missing** `[OBSERVED]`
(measured); `GetUITextureSet()` only bounds-checks `0..600` and returns the zeroed entry
for a missing index (`nCount = 0`, `pTextureCoord = nullptr`), which
`RenderGeomRectImage` then rejects — i.e. **a missing set draws nothing, silently**.

### 3.15 The negative-index convention (corrected)

| `nTextureSetIndex` | Behaviour | Evidence |
|---|---|---|
| `>= 0` | normal set from `UITextureSetList.txt` | `:2837` |
| `-1` | *image* controls: falls into branch C and **draws an opaque `dwColor` rectangle** (`bTrans = 0`). *Text* controls never reach here (they are `RENDER_TEXT`), which is why the note "−1 = no image" holds for text. | `:3239-3248` |
| `-2` | branch C, `bTrans = 1` → **alpha-blended `dwColor` fill** | `:3240-3241` |
| `< -2` | `GetUITextureSet(abs(i))`, drawn **stretched** through `RenderRectTex` | `:3133-3164` |

Also `SPanel::FrameMove2` only submits a panel at all when
`nTextureSetIndex >= 0 || (dwColor & 0xFF000000)` `[OBSERVED]` `SControl.cpp:367` — i.e.
a `−1` panel with no alpha in its colour is not drawn.

### 3.16 The `nMarkIndex` guard is off by one

`if (nMarkIndex >= 0 && nMarkIndex <= 64)` `[OBSERVED]` `:3168` while
`m_stGuildMark[64]` is `TextureManager.h:97` → index 64 reads one element past the array.
Fix to `< 64` in a reimplementation.

---

<a name="4-control-layer"></a>
## 4. The control / geometry layer

### 4.1 Scene `.bin` → control objects `[OBSERVED]` `TMScene.cpp:315-...`

`ReadRCBin()` loops: read `int nControlType`; switch; read the matching struct
(`UIBinary.h`) and construct the control. Parents are resolved **by id**
(`FindControl(nParentID)` then `AddChild`), so scene binaries carry an explicit tree.
`BinPanel` is 10 × int32 = 40 bytes + 4-byte type = 44 bytes per record:

```
nID, nParentID, nTextureSetIndex, nStartX, nStartY, nWidth, nHeight,
nColor (ARGB), nFillType (RENDERCTRLTYPE), nPickable
```

`[UNKNOWN]` — open question #4 (affects who parses the scene `.bin`, not how you draw):
the record at file offset 0 parses cleanly as
`id=258, parent=0, texSet=-2, x=0, y=0, w=240, h=240, colour=0x77000000, fill=4, pick=0`,
but the next 4 bytes (offset 44) are `0x00000C00`, **not** a valid `CONTROL_TYPE`; and a
fixed 44-byte stride over the whole file fails for 528 of 804 candidate records
(measured). The panel at offset 2784, by contrast, *is* followed by a valid type (`1`).
So `BinPanel` is probably **not** 40 bytes in the 748 format, or some record types carry
extra payload. Resolve with a full-file parser that asserts `ftell == file size` at EOF
and prints every record's type/offset; then correct `UIBinary.h`. Until then, treat
`.bin` panel coordinates as read (they match the code's expectations, §3.14) but do not
trust that every control is reached.

### 4.2 `GeomControl` — the render unit `[OBSERVED]` `GeomObject.h:7-49`

One instance is embedded in each control (`SPanel::m_GCPanel`, `SText::m_GCText…4`,
`SButton::m_GCObj`, `SGridControl::m_GCObj`, `SCursor::m_GCPanel`, `S3DObj::m_GCObj`).
Each frame `FrameMove2()` writes its position/size/layer and appends the pointer to the
draw list — **no allocation, no copying** (except `SCursor`'s pickup item, which copies).

| field | type | meaning |
|---|---|---|
| `eRenderType` | `RENDERCTRLTYPE` | dispatch key |
| `nPosX/nPosY` | float | absolute device pixels (already parent-accumulated) |
| `nWidth/nHeight` | float | destination size |
| `nLayer` | int | draw-list index 0..29 (= tree depth, §10) |
| `fAngle` | float | radians, used by STRETCH/guild/preview |
| `fScale` | float | extra scale for 3D previews |
| `fLeft/fTop/fRight/fBottom` | float | vertical clip for `bClip` |
| `dwColor` | uint32 ARGB | modulation |
| `bVisible` | int | gate |
| `nTextureSetIndex` | int | see §3.15 |
| `nTextureIndex` | int | which coord inside the set |
| `sLegend` / `sSanc` | short | decoration overlays (§4.6) |
| `strString[256]` | char | caption for image controls |
| `pFont` | `TMFont2*` | text renderer |
| `m_pNextGeom` | `GeomControl*` | **intrusive** singly-linked draw list |
| `m_fWidth/m_fHeight` | float | used only by `RenderGeomControlBG` |
| `dwBGColor` | uint32 | selects the halo texture |
| `nMarkIndex` | int | guild-mark slot or −1 |
| `nMarkLayout` | int | 1 = gold backing, 2 = silver backing, 3 = none |
| `bClip` | int | vertical (source-rect) clipping |

Default `dwColor = 0xFFAAAAAA`, `nTextureSetIndex = -1`, `nLayer = -1`,
`nMarkIndex = -1`, `nMarkLayout = 3` `[OBSERVED]` `GeomObject.cpp:5-38`.

### 4.3 `stGeomList` and the intrusive list `[OBSERVED]` `SControl.cpp:3002-3051`

```c
struct stGeomList { GeomControl* pHeadGeom; GeomControl* pTailGeom; };

int AddRenderControlItem(list[30], ctl, layer) {
    if (layer >= 30) return 0;              // silently dropped!
    ctl->nLayer = layer;
    if (list[layer].pHeadGeom) list[layer].pTailGeom->m_pNextGeom = ctl;
    else                       list[layer].pHeadGeom = ctl;
    list[layer].pTailGeom = ctl;            // append -> submission order
    return 1;
}
void RemoveRenderControlItem(list[30], ctl, layer) { ...unlink by identity... }
```

`RenderControl` **clears `m_pNextGeom`** for every consumed node
(`ObjectManager.cpp:616-617`), so the list is rebuilt from scratch each frame; the same
`GeomControl` can legally appear twice in one frame (cursor + panel), which is why the
list is intrusive and not a vector.

### 4.4 `SControl` / `SPanel` / `SControlContainer` traversal

`SControlContainer::FrameMove` `[OBSERVED]` `SControlContainer.cpp:259-311` walks the
`TreeNode` (first-child / next-sibling) and calls
`FrameMove2(m_pDrawControl, vParentPos, vControlLayer, 0)`; descending into `m_pDown`
adds the parent's position to `vParentPos` and increments `vControlLayer`.

Two **observed bugs** in that loop, both of which a reimplementation should *not* copy:

1. `vControlLayer` is incremented on descent but **never decremented on ascent**
   (`:302-304`), so the layer is "number of descents so far this frame", not the node
   depth. Once it reaches 30, every later control is dropped by
   `AddRenderControlItem`. The cursor is nevertheless forced to layer **29**
   (`:308`) as a deliberate "always on top".
2. On ascent it subtracts the position of the node it just *moved to*:
   `pCurrentControl = pCurrentControl->m_pTop; vParentPos.x -= pCurrentControl->m_nPosX;`
   (`:301-303`) instead of the node it left. Nested children therefore accumulate the
   wrong parent offset.

`SPanel::FrameMove2` `[OBSERVED]` `SControl.cpp:365-382`:

```
nPosX = parentPos + m_nPosX; nPosY = parentPos + m_nPosY
nWidth = m_nWidth; nHeight = m_nHeight; nLayer = parentLayer
draw only if (pos+size).x >= 0 && (pos+size).y >= 0
        && pos.x <= 800*W_ratio && pos.y <= 600*H_ratio
```

i.e. an **axis-aligned off-screen test only** (no per-pixel clipping).

`SCursor::FrameMove2` `[OBSERVED]` `SControl.cpp:472-523`: position = mouse position,
layer = 29, `m_bAlwaysOnTop = 1`; if `m_nCursorType == 2` it is parked at (−100,−100)
(hardware cursor mode) `:477-481`.

### 4.5 `SText::FrameMove2` — alignment `[OBSERVED]` `SControl.cpp:843-989`

Vertical: always
`nPosY = parent.y + m_nPosY + (m_nHeight − 16·H_ratio)/2 + 2`, and
`nWidth/nHeight` are copied from the control (so the horizontal cull box is the control's).

Horizontal, by `m_dwAlignType`:

| type | formula |
|---|---|
| 0 | `x = parent.x + m_nPosX + 8·W_ratio` (left, 8 px gutter) |
| 1 | `x = parent.x + m_nPosX + (m_nWidth − TextExtent(str).cx)/2` (**GDI measured**) |
| 2 | `x = parent.x + m_nPosX + m_nWidth − (8·len + 8)·W_ratio` (right, 8 px per char) |
| 3 | `x = parent.x + m_nPosX + 2·W_ratio` |
| 4 | `x = parent.x + m_nPosX + 8·W_ratio + 104` (fixed 104 px shift) |

Only type 1 measures the real string, and it does so through the **shared GDI DC**
(`GetTextExtentPoint32(g_pDevice->m_hDC, ...)`) — a CPU-side call per control per frame.

`m_cComma == 2` splits the string at commas into up to 4 sub-strings (`m_GCText..4`),
each drawn with its own `TMFont2`; their X positions are chained by half-widths
(`:899-903`). `m_cComma == 1` inserts thousands separators (`:676-733`).

### 4.6 `sSanc` / `sLegend` decorations

Both draw a 35×35 sub-rect of UI texture **338** behind the control's own image
`[OBSERVED]` `RenderDevice.cpp:2889-3105`:

* `sSanc >= 1`: source `(35·(sSanc−1), 0)`, size 35×35.
* `sLegend`: a lookup table of source origins — `5..12 → (315/350/385/420, 0)`,
  `13..115 → no decoration`, `116..125 → (0,35) … (315,35)` in 35 px steps.

`[INFERRED]` these are the "sanction"/"legend" glow rings around upgraded items.

### 4.7 `SCursor` and the mouse cursor

Two mutually exclusive cursor schemes `[OBSERVED]` `NewApp.cpp:250-262, 855-871`:

* `SCursor::m_nCursorType == 0` → software cursor: `SetCursor(0)` hides the OS cursor and
  the UI draws `SCursor` as a normal panel (texture set **0** `[Cursor]`, 5 frames of
  32×32, `m_GCPanel.nTextureIndex = 0`), at layer 29.
* `m_nCursorType == 2` → hardware cursor: `LoadCursor(IDC_CURSOR1/2)` and
  `SetCursor()` per `m_GCPanel.nTextureIndex` (0 → CURSOR1, 1 → CURSOR2); the software
  panel is parked off-screen at (−100,−100) but still submitted.
* Item pickup (`ECursorStyle::TMC_CURSOR_PICKUP`) copies the item's `GeomControl` into
  `m_GeomItem`, positions it at `cursor + item.pos − 12` and submits **both** the item and
  the cursor (`SControl.cpp:488-518`); a 3D item gets `fScale = 1.5` and a 3 rad/s spin
  (`:504-507`).

---

<a name="5-text"></a>
## 5. Text rendering

### 5.1 `TMFont2` — the GDI rasteriser `[OBSERVED]` `TMFont2.h`, `RenderDevice.cpp:564-599`

One `TMFont2` per text control (`SText` owns 4). Each instance owns **one D3D texture**
and uploads into it on every `SetText`.

**Shared GDI assets, created once on device restore** (`RenderDevice.cpp:564-599`):

```
m_bmi.bmiHeader.biSize        = 40
biWidth  = 512        (m_nFontTextureSize)
biHeight = -512       (negative => TOP-DOWN DIB, row 0 = top)
biPlanes = 1, biBitCount = 32, biCompression = BI_RGB(0)
m_hDC       = CreateCompatibleDC(0)
m_hbmBitmap = CreateDIBSection(m_hDC, &m_bmi, 0, &m_pBitmapBits, 0, 0)   // 512*512*4 = 1 MiB
m_hFont     = CreateFont(m_nFontSize /*14 @1024*/, 0,0,0, g_nFontBold /*500*/,
                         0,0,0,            // italic, underline, strikeout
                         1,                // DEFAULT_CHARSET  (== the ANSI/CP949 page)
                         4,                // OUT_TT_PRECIS
                         0,                // CLIP_DEFAULT_PRECIS
                         4,                // ANTIALIASED_QUALITY
                         2,                // VARIABLE_PITCH | FF_DONTCARE
                         g_szFontName /*"Tahoma"*/);
SelectObject(m_hDC, m_hbmBitmap); SelectObject(m_hDC, m_hFont);
SetTextColor(m_hDC, COLORREF 0xFFFFFF);   // white glyphs
SetBkColor  (m_hDC, 0x000000);            // black background, BK mode = OPAQUE
```

`g_szFontName = "Tahoma"`, `g_nFontBold = 500` are the compiled defaults
(`TMGlobal.cpp:21-22`); `font.txt` (line 1 = face, line 2 = weight) overrides them if
present (`RenderDevice.cpp:576-592`). **There is no `font.txt` in `client_748`**
`[OBSERVED]` → Tahoma 500 always.

### 5.2 `TMFont2::SetText` step by step `[OBSERVED]` `TMFont2.cpp:29-275`

1. **Brightness compensation** (`:37-54`): if `m_nBright > 58`,
   `v = m_nBright − 40`, then `R,G,B = max(0, ch − v)` per channel; alpha untouched.
   `SText::SetTextColor` repeats the same maths (`SControl.cpp:788-802`).
2. **Copy + chunk** into 1–3 lines of ≤ 42 bytes (`:65-120`), see §5.5.
3. **Clear the DIB band**: `FillRect(m_hDC, {0,0,512, m_nFontSize*m_nLineNumber}, BLACK)`
   (`:126-131`) — note the height is `fs·n`, one row short of `(fs+1)·n`.
4. **Per line** (`:132-178`):
   * `TextOut(m_hDC, 0, line·(fs+1), "  …44 spaces…  ", 44)` — blanks the band.
   * `TextOut(m_hDC, 0, line·(fs+1), str + " ", len+1)` — the actual glyphs.
   * if `bCheckZero`: re-`TextOut` a mask string that is `/` where the text had `'0'` and
     space elsewhere, with `BK mode TRANSPARENT`, restoring `OPAQUE` after — the
     "slashed zero" trick.
   * `GetTextExtentPoint32(m_hDC, str, len, &size)` → `m_szStringSize[line] = size.cx`
     (the **device-pixel width used as the quad width**; excludes the trailing space).
5. **Create the destination texture on first use** (`:181-244`):
   * `m_bSavage == 1` → `D3DXCreateTexture(..., 512, 64, 1, 0, D3DFMT_A4R4G4B4, MANAGED)`.
   * else → read `UI/minimap.wyt` into the static `m_pBuffer` **once**, append
     `"TRUEVISION-XFILE"`, and
     `D3DXCreateTextureFromFileInMemoryEx(dev, buf, len+18, 512, 64, 1, 0,
        D3DFMT_A4R4G4B4, MANAGED, filter=1 /*POINT*/, mipfilter=1, colorkey=0xFF000000, 0,0, &tex)`.
6. **Upload** (`:249-273`):

```c
m_pTexture->LockRect(0, &lr, nullptr, 0);        // whole 512x64, D3DFMT_A4R4G4B4, pitch = 1024
for (y = 0; y < m_nLineNumber*(m_nFontSize+1); ++y) {      // up to 45 / 63 / 75 rows
    uint16* dst = (uint16*)(lr.pBits + y*lr.Pitch);
    for (x = 0; x < 512; ++x) {
        uint8 a = (m_pBitmapBits[x + y*512] & 0xFF) >> 4;  // BLUE channel of BGRA, 8->4 bit
        dst[x] = (a <= 0) ? 0 : ((uint16)a << 12) | 0x0FFF; // A4R4G4B4: alpha in high nibble,
    }                                                       // RGB forced to white (0xFFF)
}
m_pTexture->UnlockRect(0);
```

Packing rules summarised:

| aspect | value |
|---|---|
| DIB pixel format | 32 bpp `BI_RGB`, memory order **B,G,R,X** (little-endian `0xXXRRGGBB`) |
| alpha source | `B` byte (`&0xFF`), i.e. the blue channel, then `>>4` → 0..15 |
| DIB stride | 512 × 4 = **2048** bytes |
| texture format | `D3DFMT_A4R4G4B4` (`0x0000F000` = A, RGB = 0xF each) |
| texture size | 512 × 64, 1 mip level, `D3DPOOL_MANAGED` |
| texture pitch | 512 × 2 = **1024** bytes |
| rows written | `m_nLineNumber × (m_nFontSize + 1)` = 15 / 30 / 45 @1024, 21·n @1280, 25·n elsewhere |
| texels written | `512 × (n·(fs+1))` — 512×15 … 512×75 |

**Arithmetic bug (OOB write) `[OBSERVED]`:** at any width other than 640/800/1024/1280
the font is 24 px → 3 chunks × 25 rows = **75 rows** written into a **64**-row texture,
i.e. 11 rows past the end of the locked rect (= 11 × 1024 bytes of overflow). Only
strings longer than 84 bytes (3 chunks) trigger it. 1366×768, 1440×900, 1600×900,
1920×1080 all hit the 24 px case.

### 5.3 Why a *minimap* file is the font atlas `[OBSERVED]` + `[INFERRED]`

`UI/minimap.wyt` (`MiniMap_Path`, `TMPaths.h:41`) is **not** used as a glyph atlas. It is
a 4-byte-magic container around a plain 128×128 24 bpp uncompressed **TGA**
(`evidence/hex/ui-minimap-wyt.txt`: magic `"WT10"`, then `idLength=0, colorMapType=0,
imageType=2, w=128, h=128, bpp=24, descriptor=0`; file size 49182 = 4 + 18 + 49152 + 8).

It serves only to make `D3DXCreateTextureFromFileInMemoryEx` **create a
512×64 `A4R4G4B4` texture** without a `D3DXCreateTexture` call, exactly as the `m_bSavage`
branch does with a plain `D3DXCreateTexture`. Its pixel content is irrelevant: the very
first `SetText` overwrites rows `0 .. n·(fs+1) − 1`, and no quad ever samples rows beyond
`n·(fs+1)` — at 1024×768 that is row 44 of 64.
`[INFERRED]` the original authors used it as a convenient "make me a texture of this size
and format" hack and the file was copied along; the identical file exists in the 755/760
asset sets.

Consequence for a reimplementation: **allocate an empty 512×64 A8 (or A4R4G4B4-
compatible) texture; do not decode `minimap.wyt`.**

### 5.4 `TMFont2::Render` — atlas → screen `[OBSERVED]` `TMFont2.cpp:282-371`

```
if (m_nPosX != -1) nPosX = m_nPosX;      // per-object override
if (m_nPosY != -1) nPosY = m_nPosY;
nPosY += 1;                              // global 1 px down
fade  = m_bNoFade ? 255 : g_nUIFadeAlpha
col   = WYDCOLOR_SCALE_ALPHA(m_dwColor,      fade)     // text colour
shade = WYDCOLOR_SCALE_ALPHA(m_dwShadeColor, fade)     // outline colour, default 0xFF000000
SetRenderStateBlock(2);                                // §9.2
for each line n:
    w = m_szStringSize[n]                              // GDI-measured pixel width
    if (nRenderType != RENDER_TEXT /*0*/):             // i.e. only RENDER_SHADOW(1)/TEXT_FOCUS(6)
        RenderRectC(0, n*(fs+1), w, fs, nPosX+1, nPosY+1 + n*fs, tex, shade, m_fSize, m_fSize)
    RenderRectC(0, n*(fs+1), w, fs, nPosX,   nPosY   + n*fs, tex, col,   m_fSize, m_fSize)
```

* **No outline** separate from the shadow: the "outline" is a **1 px bottom-right drop
  shadow** drawn as a second full quad of the same atlas cell in `m_dwShadeColor`.
  It is skipped entirely for plain text (`eRenderType == RENDER_TEXT == 0`).
* **Italics / bold / underline**: never used — `CreateFont` passes 0 for italic,
  underline and strikeout, and no `lfEscapement`. Any "italic" look in the art comes
  from the font face, not the renderer.
* **Character spacing**: whatever GDI produced (Tahoma at `lfHeight = fs`,
  `VARIABLE_PITCH`). There is no manual kerning, no advance table, no per-glyph quad:
  the whole line is one stretched quad of the GDI bitmap.
* **Scaling**: `m_fSize` multiplies the quad (not the font), so `m_fSize = 2.0`
  magnifies a 14 px bitmap 2× with point sampling → blocky text.
* `m_bMultiLine` (0 by default) switches line spacing from `n*fs` to `n*(fs+1)`.

### 5.5 String chunking and multi-byte handling `[OBSERVED]`

```
len = strlen(s)                        // BYTES, not characters
if len > 84: 3 chunks (42/42/rest)
elif len > 42: 2 chunks (42/rest)
else: 1 chunk
```

The 42-byte cut is DBCS-aware via `IsClearString()` (`Basedef.cpp:1295-1317`), which
scans assuming "byte < 0 means a 2-byte character follows" and reports whether offset
`target` is a character boundary:

```c
for (pos = 0; pos < len; ++pos) {
    if (str[pos] >= 0) { if (pos >= target) return 1; }
    else { if (pos == target) return 0; if (pos == target+1) return 1; ++pos; }
}
return 1;
```

If the boundary is clean the chunk takes 42 bytes, otherwise 41 (the trailing 2-byte char
is pushed to the next chunk). This is the **only** multi-byte code in the renderer, and it
is a single-byte-ANSI heuristic: the glyphs themselves are produced by `TextOut` with
`DEFAULT_CHARSET`, i.e. the **system ANSI code page** applies at draw time
(`[INFERRED]` pre-edit: CP949 on a Korean Windows, CP1252 on Western).

Encoding evidence in the assets `[OBSERVED]`:
* `UI/strdef.bin` = 440 × 128-byte records + 4-byte checksum (56324 bytes exactly),
  every byte XOR `0x5A`, filler `0xE2`. In this English 748 build the strings are plain
  ASCII ("Not enough space for items.", "Your ID is not too long.", …) — see
  `evidence/hex/ui-strdef-xor5a.txt`. It is the *message box* string table
  (`g_pMessageStringTable`, `Basedef.cpp:300-327`), not a font table.
* `UI/UIString.txt` = `"<index>\t<Name>"` ASCII lines loaded by `ReadUIString()`
  (`Basedef.cpp:1178-1199`) into `g_UIString[500][64]`, which `.bin` controls reference by
  `nStringIndex` (`TMScene.cpp:487,544`). Names use `_` for spaces
  (`BASE_UnderBarToSpace`, `SControl.cpp:604`).
* Korean 748 builds would contain CP949 bytes in the same files; **no conversion is done
  anywhere** — the bytes go straight into `TextOut` with `DEFAULT_CHARSET`.

`[UNKNOWN]` How a CP949 string that straddles the 42-byte chunk boundary is handled when
`IsClearString` says "not clear": the code then takes 41 bytes and the lead byte of the
split pair becomes the first byte of the next chunk, re-pairing correctly — this is
correct for **CP949 only** (where a lead byte is always ≥ 0x81). Verify with a Korean
build by rendering > 42-byte Korean strings and comparing glyphs.

### 5.6 Colour and fade

* Text colour is **per-vertex** (`diffuse`), the atlas is always white → any ARGB works.
* Default text colour `0xFFFFFFFF`, default shade `0xFF000000`
  (`TMFont2.cpp:10-11`).
* `g_nUIFadeAlpha` (0..255) multiplies **alpha only** through
  `WYDCOLOR_SCALE_ALPHA()` (`TMGlobal.h:26-35`): `a' = a*fade/255`; `fade >= 255` returns
  the colour unchanged, `fade <= 0` zeroes alpha.
* Blink animation example: the player's own name label sets
  `SetTextColor(((int)(sin(t·π/2000)·255) << 16) | 0xFF000000)` — a red pulse whose
  green/blue are zero (`TMHuman.cpp:7480-7482`).

### 5.7 `TMFont3` — world-anchored floating text `[OBSERVED]` `TMFont3.cpp`

`TMFont3` is a `TreeNode` added to `TMScene::m_pExtraContainer`, i.e. rendered in the
**3D pass** (`RenderObject`), not the UI pass — so it can be depth-interleaved with world
geometry, and it forces `ZENABLE 0` anyway (`:219`).

Constructor `(szText, x, y, colour, fSize, delay, dir, lifeTime, bViewHalf, nType)`:

| `nType` | glyph source | `m_nTextureSetIndex` | initial `m_fScale` |
|---|---|---|---|
| 0 | **GDI** via an internal `TMFont2 m_Font2` | — | 0.70 |
| 1 | digit strip set | 137 `[Green_Number]` | 0.60 |
| 2 | digit strip set | 138 | 0.70 |
| 3 | digit strip set | 139 | 0.70 |
| 4 | digit strip set | 140 | 0.70 |
| 5 | digit strip set | 141 | 1.10 (rising, scale 1.0 after 25 %) |
| 6 | digit strip set | 141 | 0.80 (rising, 0.6 after 25 %) |

For `nType > 0` the string is parsed character-by-character into up to **11 sprites**:

```
'0'..'9' -> texIndex 0..9      '+' -> 10      '-' -> 12      ':' -> 13
'm'      -> single sprite texIndex 11, len = 1
'?'      -> single sprite texIndex 14/11/11/12/11 depending on the set
'E'      -> prefix sprite texIndex 11, len -= 2
'C'      -> prefix sprite texIndex 15, len -= 1
anything else -> texIndex = -1 (invisible), advance 5 px
advance  = (coord[prev].nWidth * m_fWidthRatio) * m_fScale
```

`Render()` (`:202-297`) draws each sprite with `RenderRectTexDamage`, tinting with
`0xFFFFFF | (alpha<<24)` where
`alpha = (1 − m_fVisualProgress) * 255`, and `fScale = m_fWidthRatio * m_fCurrScale`.
The X of each digit is skewed by distance from the string centre:

```
fTemp = (i − (len/2 − 1)) / (len/2 − 1)
x = dest.nDestX + m_stNum[i].nX
    + ((−fTemp·width)·halfLen · 0.30) · (0.55 − m_fCurrScale)      // types != 5,6
    /  ((−fTemp·width)·halfLen · 1.00) · (1.30 − m_fCurrScale)      // types 5,6
    + m_vecPosition.x
```

which produces the expanding/contracting "spread" of damage numbers.
`FrameMove` (`:299-399`) drives life: visible from 30 % of `m_dwLifeTime` (or 40 % when
`m_bViewHalf`), rises/falls `100 px·speed·progress·m_fHeightRatio` along `sDir`
(1..9 = the 8 compass directions plus "9 = up and 50 px left"), and
`m_fCurrScale = sin(progress·π)·m_fScale` for types 5/6 else `(progress+0.2)·m_fScale`.

### 5.8 Where each is used

| Use | Class | Pass |
|---|---|---|
| All UI labels, buttons, chat, tooltips, editable text | `TMFont2` (via `SText`/`SButton`/`SEditableText`/`SListBoxItem`) | UI |
| Over-head names, guild names, kill counters, chat balloons | `SText` controls positioned by `LabelPosition()` | UI |
| Floating damage / "miss" / "Exp +N" / "Cp -N" | `TMFont3` types 1–6 (digit strips) and type 0 (GDI) | 3D |
| FPS / object-count debug overlay | `RenderDevice::m_pFont` (`m_bNoFade = 1`), drawn in `Unlock(1)` at `(10, H−15)` | after `EndScene`? No — before it, `RenderDevice.cpp:412-438` |
| `m_pDXFont`, `m_pDXFontLarge` | **never created, never used** — only `OnLostDevice`/`OnResetDevice` on null (`:609-612, 664-667`) | — |

`m_pDXFont`/`m_pDXFontLarge` are vestigial `ID3DXFont` members from the 755/760 build
`[OBSERVED]`; a reimplementation needs no `ID3DXFont` path at all.

---

<a name="6-overlays"></a>
## 6. 3D-projected 2D overlays

### 6.1 The projection helper `[OBSERVED]` `Basedef.cpp:3039-3069`

```c
BASE_Get3DTo2DPos(fX, fY, fZ, &px, &py):
    vecPos = (fX, fY, fZ)
    vTemp  = TransformCoord(vecPos, m_matView)
    vClip  = TransformCoord(vTemp,  m_matProj)          // NOTE: TransformCoord, i.e. /w is folded in,
                                                        //      *not* Transform
    if (vClip.z < 0 || vClip.z >= 1) return 0            // behind camera / past far plane
    px = (vClip.x + 1) * (W - m_nWidthShift) / 2
    py = (-vClip.y + 1) * (H - m_nHeightShift) / 2
    if (px <= 0 || px >= W-shift || py <= 0 || py >= H-shift) return 0     // off-screen
    return 1
```

`D3DXVec3TransformCoord` divides by `w`, so `vClip` is NDC in `[-1,1]` for x/y and `[0,1]`
for z. The truncating `(int)` cast is a floor for positive values.

### 6.2 Over-head labels `[OBSERVED]` `TMHuman.cpp:7413-7500`

```c
vecPos = ( m_vecPosition.x,
           m_vecPickSize[m_nSkinMeshType].y * m_fScale + m_fHeight + 0.2f,   // eye/head height
           m_vecPosition.y )                       // NOTE: z, y swapped - WYD world is XZ-plane
vTemp  = TransformCoord(vecPos, m_matView)
vClip  = TransformCoord(vTemp,  m_matProj)
if (vClip.z < 0 || vClip.z >= 1) -> hide all labels
else:
    int x = (int)((vClip.x + 1) * (W - m_nWidthShift) / 2)
    if (m_cMount)  y = (int)((-vClip.y + 0.76f) * (H - m_nHeightShift - 1) / 2) + (int)(sightLength*3.5f)
    else           y = (int)((-vClip.y + 1.16f) * (H - m_nHeightShift - 1) / 2) - 3*(int)sightLength
    if (x/y outside the screen) hide all labels
```

The `+1.16 / +0.76` constants and the `±sightLength` terms are empirical offsets that push
the label above the head; `m_fSightLength` is the camera distance (default 11.0,
`TMScene.cpp:2296`; 3.5..15.5 in practice) `[OBSERVED]`.

Labels are ordinary `SText` controls; X is centred with the **6-pixels-per-character**
estimate (`TMHuman.cpp:7622`):

```
nameLabel.SetRealPos( x − (6*(len2+2))*W_ratio/2 ,  ... )
guildMark.SetRealPos( x − (6*(len2+2))*W_ratio/2 − 10 , ... )
```

Visibility rules (`LabelPosition`, `:7304-7747`): hidden while `m_cHide`, for other
players' shadows unless the local player wears the "jewel glasses", for `m_sHeadIndex ∈
{216, 226, 298}` unless hovered, when `g_bEvent`, when not hovered and not a target and
the player has no guild/party/merchant/class flag, and when the projection is off-screen.

### 6.3 HP bars, mount bars, kill counters

All are `SControl`s (`SProgressBar`, `SText`, `SPanel` with a guild mark) whose positions
are set from the same projected `(vPosInX, vPosInY)` `[OBSERVED]` `TMHuman.cpp:7589-7629`:

```
m_pProgressBar->SetRealPos(x − BASE_ScreenResize(35.0f), y − 12)      // player HP bar (70 px wide)
m_pMountHPBar ->SetRealPos(x − BASE_ScreenResize(35.0f), y − 12)      // mount HP
m_pChatMsg    ->SetRealPos(x − m_pChatMsg->m_nWidth/2,  y − 60)
m_pAutoTradeDesc->SetRealPos(x − 150*W_ratio/2, y − 3*H_ratio)
```

### 6.4 3D objects inside the UI window (`RENDER_3DOBJ`)

`[OBSERVED]` `TMMesh.cpp:283-311`: the object is placed by a world matrix built **from
screen pixels**:

```
fWidthRatio = 6.3   (6.26 if viewport.Width/viewport.Height < 1.26)
fX = nX / (viewport.Width  / fWidthRatio) − fWidthRatio/2
fY = nY / (viewport.Height / 4.96)        − 2.48
yaw   = atan2(fX, 50)      pitch = atan2(fY, 50)
world = Identity · RotYawPitchRoll(yaw + ctl.fAngle, pitch − π/2, 0)
      · Scale(0.52·fScale) · Translate(−fX − c·0.52·fScale, −fY − c·0.52·fScale, c.y·0.52·fScale)
```

combined with the `SetMatrixForUI` view/projection (§2.4) → a 3D model rendered at a
precise pixel position, unaffected by the 3D camera. `fScale` is further scaled by
`1/m_fWidthRatio · 0.9` and `BASE_ScreenResize` in `RenderGeomControl` (`:3330-3342`).

### 6.5 Z-order of overlays relative to the world

Because `RenderObject()` runs first with `ZENABLE 1` and `RenderControl()` second,
**all UI is drawn after (on top of) all world geometry**, including `TMFont3` damage
numbers. If a reimplementation wants damage numbers to be occluded by walls, that is a
behavioural change (the original forces `ZENABLE 0` for them, `TMFont3.cpp:219`).

### 6.6 The minimap

The minimap is **not** a projected overlay: it is an ordinary `SPanel` (control id 289)
drawn in the UI pass, whose texture is UI texture slot 4 — a 3×3 mosaic of 128×128 map
tiles built once at scene load by `GenerateTexture` — and whose source window is
re-computed every frame from the player position. Full formulas and the −45° diamond
geometry are in §8.1 and §2.8 B. Markers on top of it are `-2` fill panels placed with a
fixed ±45° rotation (`TMFieldScene.cpp:6917-6941`).

---

<a name="7-guild-marks"></a>
## 7. Guild marks

`[OBSERVED]` `TextureManager.h:10-20`, `TextureManager.cpp:1398-1444`,
`TMFieldScene.cpp:22961-22980` and `:24738-24762`.

* Pool: `stGuildMark m_stGuildMark[64]` = `{ IDirect3DTexture9* pTexture; int nGuild;
  uint32 dwLastRenderTime; }` — 64 slots, `.nGuild = -1` when free.
* Size fixed: `GUILD_MARK_WIDTH = 16`, `GUILD_MARK_HEIGHT = 12`.

**Wire format** `[OBSERVED]`: the mark arrives as a **16×12, 24 bpp Windows BMP**,
632 bytes, fetched over HTTP (`InternetReadFile(m_hHttpFile, strMarkBuffer, 632, ...)`)
`TMFieldScene.cpp:24738-24762`. Validation (`Guildmark_IsCorrectBMP`, `:22961-22980`):

```
BITMAPFILEHEADER.bfType    == 19778 ('BM')
BITMAPFILEHEADER.bfSize    == 630 or 632
BITMAPINFOHEADER.biWidth   == 16
BITMAPINFOHEADER.biHeight  == 12
BITMAPINFOHEADER.biBitCount== 24
```

(16·12·3 = 576 + 14-byte file header + 40-byte info header = 630; the 632 variant has two
extra padding bytes.) `biHeight` is positive → bottom-up rows, and D3DX handles the flip.

**Texture creation** `[OBSERVED]` `TextureManager.cpp:1398-1422`:

```c
D3DXCreateTextureFromFileInMemoryEx(dev, szMarkBuffer, 632,
    /*W*/16, /*H*/12, /*mips*/0, /*usage*/0,
    g_pDevice->m_eFormat,          // == the back-buffer format (X8R8G8B8 on 32bpp)
    D3DPOOL_MANAGED, 1, 1, /*colorkey*/0, NULL, NULL, &m_stGuildMark[i].pTexture);
```

* **No alpha**: `colorkey = 0`, so the mark's background (usually black) becomes opaque
  black, not transparent. The mark is drawn *over* a coloured backing rectangle, and the
  black background is hidden by drawing the backing first (§ below).
* Guard bug `[OBSERVED]`: `if (nIndex < 0 || nIndex > MAX_GUILD_MARK) return 0;` allows
  `nIndex == 64` → one past the array.

**Slot eviction** `[OBSERVED]` `TMFieldScene.cpp:22930-22959`: the least-recently-*rendered*
slot (`dwLastRenderTime == 0` first, else the smallest time) is released and reused;
`dwLastRenderTime` is refreshed every time `RenderGeomRectImage` draws the mark (`:3234`).

**Drawing** `[OBSERVED]` `RenderDevice.cpp:3167-3236` — only for
`nTextureSetIndex ∈ {-1,-2}` panels that carry `nMarkIndex >= 0`:

```
iX = posX − 2;  iY = posY − 2;  iCX = 20;  iCY = 16                 // 2 px border
if (m_fWidthRatio != 1.0) { iCX = m_fWidthRatio*18.5; iCY = m_fHeightRatio*14.5; }
+ per-resolution nudges:  H=0.8 -> y+1 | W=1.6 or 2.0 -> x−1
if (nMarkLayout == 1) RenderRectNoTex(..., 0xFFFFD700, bTrans=1)     // gold backing
if (nMarkLayout == 2) RenderRectNoTex(..., 0xFFC0C0C0, bTrans=1)     // silver backing
RenderRect(0,0, 16,12,                                              // ID3DXSprite, opaque white
           posX − (16*W_ratio − 16), posY − (12*H_ratio − 12),
           markTexture, W_ratio, H_ratio)
m_stGuildMark[i].dwLastRenderTime = timeGetTime()
```

`nMarkLayout` is set from the guild index by `Guildmark_Link` (`TMFieldScene.cpp:22982-22994`):
guild 509/9 → 1 (gold), guilds 526–531 and 3–8 → 2 (silver), else 3 (no backing).

---

<a name="8-dynamic-textures"></a>
## 8. Dynamic textures (`GenerateTexture`)

`[OBSERVED]` `TextureManager.cpp:1186-1381`, constants `TextureManager.h:8,99-100`,
`RenderDevice.cpp:619-623`.

* `MAX_DYNAMIC_TEXTURE = 72` (`m_ppDynamicTexture[72]`, `m_dwDynamicLastUsedTime[72]`),
  **but `GenerateTexture` does not use them**: the destination is
  **`m_ppUITexture[nDestUIIndex]`**, i.e. a slot of the ordinary 512-entry UI texture
  table, which is normally loaded from disk but here is built at runtime.
* `DYNAMIC_TEXTURE_WIDTH/HEIGHT` are `static int`, default **256×256**, and are raised to
  **512×512** in `RestoreDeviceObjects()` when `d3dCaps.MaxTextureWidth > 256`
  (`RenderDevice.cpp:619-623`). `[OBSERVED]`
* **Coordinate halving:** when the size is 256, `nWidth, nHeight, nDestX, nDestY` are all
  divided by 2 at entry (`:1188-1194`), and the blit uses a stride-2 gather
  (`pSrc[2*W*(i+srcY) + 2*j + srcX]`) instead of `memcpy` — a 2× downscale + crop.
  At 512 the copy is row-wise `memcpy(2*width)` / `memcpy(4*width)` (16/32 bpp).

Algorithm:

```
1. if (!m_ppUITexture[src]) LoadUITexture(src, -1); if still null -> 0
2. LockRect the source surface; read D3DSURFACE_DESC (format, width)
3. if (!m_ppUITexture[dst])  create DYNAMIC_W x DYNAMIC_H, 1 level,
                             SrcDesc.Format, D3DPOOL_MANAGED
   else if (format differs)  release + recreate
4. LockRect the destination surface
5. per-row copy of an nWidth x nHeight rect from (nSrcX,nSrcY) to (nDestX,nDestY),
   clamped so it cannot run past DYNAMIC_W/H
6. unlock both
```

Two bugs `[OBSERVED]`: the surface is locked **before** the possible release/recreate in
step 3 (`lpSrcSurface` leaks and the destination lock at `:1268` happens on a stale
surface if the format changed); and the early `return 0` at `:1270` returns while both
surfaces are still locked.

**The only caller** `[OBSERVED]` `TMGround.cpp:3746-3767` (`TMGround::SetMiniMapData`):

```c
for (nY = 0; nY < 3; ++nY)
  for (nX = 0; nX < 3; ++nX) {
      sprintf(szMapName, "UI\\m%02d%02d.wyt", m_vecOffsetIndex.x + nX - 1,
                                              m_vecOffsetIndex.y + nY - 1);
      nSrc = GetUITextureIndex(szMapName);  if (nSrc < 0) nSrc = 13;   // fallback tile
      GenerateTexture(4, nSrc, nX<<7, (2-nY)<<7, 0, 0, 128, 128);
  }
```

i.e. the **minimap is a 3×3 mosaic of 128×128 tiles stitched into UI texture slot 4**,
whose name in `UITextureSetList.txt` is `[N_minimap]` (set 11, `4,0,0,384,384,0,0`).
At 512×512 the mosaic occupies 384×384 at the top-left `[OBSERVED]`; at 256×256 it
occupies 192×192 — but set 11 still asks for a 384×384 source rect, so UVs reach 1.5 and
the 256 path samples outside the mosaic `[INFERRED]` (bug; 16bpp / old-GPU only).

`[OBSERVED]` No other gameplay feature calls `GenerateTexture` in this 748 build — the
"combine icons in the inventory" use mentioned in the task brief does not exist here;
`itemicon.bin`-driven icons are ordinary set lookups. `[UNKNOWN]` whether 755/760 used it
for icon compositing; resolve by grepping the 755 source tree for other call sites.

### 8.1 What the mosaic is used for: the minimap's scrolling UV window

The generated 512×512 (or 256×256) texture is **UI texture slot 4**, which texture set 11
`[N_minimap]` references with `4,0,0,384,384,0,0`. Panel 289 then *animates that coord
record every frame* instead of blitting `[OBSERVED]` `TMGround::FrameMove`
(`TMGround.cpp:3271-3297`):

```c
DYN = TextureManager::DYNAMIC_TEXTURE_WIDTH            // 512 (or 256)
nPosX = (player.x - ground.m_vecOffset.x + 128) / (512 / DYN)
nPosY = (player.y - ground.m_vecOffset.y + 256) / (512 / DYN)
coord = GetUITextureSet(11)->pTextureCoord[0]
coord->nStartX = nPosX - (DYN/8.0f) * m_fMiniMapScale
coord->nStartY = (DYN - nPosY) - (DYN/8.0f) * m_fMiniMapScale
coord->nWidth  = (DYN/4.0f) * m_fMiniMapScale
coord->nHeight = (DYN/4.0f) * m_fMiniMapScale
coord->nStartY -= 4
```

With `m_fMiniMapScale = 0.6` (small/minimised map, `TMFieldScene.cpp:13188`) the window is
76×76 texels; with `1.5` (large map, `:13151`) it is 192×192. So the "minimap blit" is
**one texture-space window query per frame**, not a render-to-texture: the mosaic is built
once at scene load (`SetMiniMapData`) and the panel samples a moving 76²/192² sub-rectangle
of it, stretched into the panel's 137×170 rotated diamond (§2.8 B).

Consequences:

* The minimap is **not** rotated in texture space — the *panel quad* is rotated −45°, and
  the UV window is axis-aligned in the mosaic. `[OBSERVED]`
* Player markers (`m_pInMiniMapPosPanel[256]`, `SPanel(-2, …, g_MinimapPos[i].dwColor, …)`)
  are positioned with a fixed ±0.7071 rotation and `+78` centre offset
  (`TMFieldScene.cpp:6917-6920`), i.e. they are transformed to the same diamond space
  in C++.
* Because the coord record is *mutated globally*, the window applies to every user of set
  11 (only panel 289 exists) `[OBSERVED]`.

---

<a name="9-alpha"></a>
## 9. Alpha, colour, blending, depth, clipping

### 9.1 Colour encoding helpers `[OBSERVED]` `TMGlobal.h:17-35`

```c
WYDCOLOR_ALPHA(c) = c & 0xFF000000
WYDCOLOR_RED(c)   = (c & 0x00FF0000) >> 16
WYDCOLOR_GREEN(c) = (c & 0x0000FF00) >> 8
WYDCOLOR_BLUE(c)  =  c & 0x000000FF
WYD_RGBA(r,g,b,a) = (a | 0xFF000000) | (b & 0xFF) | ((g<<8) & 0xFF00) | ((r<<16) & 0xFF0000)
WYDCOLOR_SCALE_ALPHA(c, fade) : alpha' = alpha*fade/255, RGB untouched
```

**Gotcha:** `WYD_RGBA`'s 4th argument is expected **already shifted into the high byte**
(`a | 0xFF000000` forces alpha to 0xFF when `a` has no high bits). Callers pass
`dwA = dwColor & 0xFF000000`. So the "A" parameter is really "alpha bits".

### 9.2 The three render-state blocks (`SetRenderStateBlock`) `[OBSERVED]` `RenderDevice.cpp:1604-1826`

Only the fields that matter for 2D are listed; `—` = not touched (inherits).

| State | block 0 (UI default) | block 1 (3D) | block 2 (font / 2D blend) | block 3 (pre-UI) |
|---|---|---|---|---|
| `ZENABLE` | 1 | 1 | **0** | 0 |
| `ZWRITEENABLE` | — | 1 | — | — |
| `ZFUNC` | — | 4 (LESSEQUAL) | — | — |
| `ALPHABLENDENABLE` | **0** | 1 | **1** | 0 |
| `SRCBLEND` | 5 (SRCALPHA) | 2 (ONE) | 5 (SRCALPHA) | 5 |
| `DESTBLEND` | **7 (DESTALPHA)** | 6 (INVSRCALPHA) | **6 (INVSRCALPHA)** | 6 |
| `ALPHATESTENABLE` | — | 1 | 1 | 1 |
| `ALPHAREF` | — | 0xDD / 0xFF000000 (VGA) / 0xD | **8** | 8 |
| `ALPHAFUNC` | — | 7 (GREATEREQUAL) | 7 (GREATEREQUAL) | 7 |
| `LIGHTING` | 0 | 1 | — | — |
| `FOGENABLE` | 0 | `m_bFog` | 0 | 0 |
| `CULLMODE` | 3 (CCW) | 3 | 3 | 3 |
| `FILLMODE` | — | — | 3 (SOLID) | 3 |
| `CLIPPING` | — | — | 1 | 1 |
| `MULTISAMPLEANTIALIAS` | — | — | 0 | 0 |
| `STENCILENABLE` | — | — | 0 | 0 |
| stage0 `COLOROP` | 2 SELECTARG1, `COLORARG1 = 2 (TEXTURE)` → **texture only, diffuse ignored** | 4 MODULATE(TEXTURE, DIFFUSE) | **4 MODULATE(TEXTURE, DIFFUSE)** | 4 |
| stage0 `ALPHAOP` | 2 SELECTARG1(TEXTURE) | 2 SELECTARG1(TEXTURE) | **4 MODULATE(TEXTURE, DIFFUSE)** | 4 |
| stage0 `MINFILTER` | 2 (LINEAR) | 2 | **1 (POINT)** | 2 (LINEAR) |
| stage0 `MAGFILTER` | 2 (LINEAR) | 2 | **1 (POINT)** | 2 (LINEAR) |
| stage0 `MIPFILTER` | 1 (POINT) | 2 | 1 | 2 |
| stage1/2 `COLOROP` | — | 1 DISABLE | 1 | 1 |
| `FVF` | 322 | — | — | — |

Consequences a reimplementation must reproduce:

* **Font glyphs are POINT-sampled** (block 2), which is why 14 px GDI text stays crisp.
* Block 0 selects the **texture only** (`SELECTARG1(TEXTURE)`) — as written, the vertex
  diffuse is *not* used for RGB. See the callout below: this contradicts the way the code
  passes `dwColor` everywhere and is one of the two highest-risk ambiguities in §9.
* General UI images drawn through `RenderRectRot`/`RenderRect` (ID3DXSprite) run under
  block **0**: `ALPHABLENDENABLE = 0`, bilinear, texture-only colour — see §9.4
  for what `ID3DXSprite` does on top of that.
* `RenderRectTex`/`RenderRectNoTex`/`RenderRectTexDamage` **do** set blending themselves:
  `bTrans = 1` → `SRCALPHA / INVSRCALPHA`, alpha test off, `ZWRITE 0`.
* `RenderRectCoord` (TILE) and `RenderRectC` **do not** touch blending: they inherit
  block 0 ⇒ **opaque**.
* `RenderGeomControlBG` uses **additive** (`SRCALPHA / ONE`) with
  `ALPHAOP = SELECTARG1(TEXTURE)`, i.e. the halo's diffuse alpha is ignored and only the
  texture's alpha matters (`:3484-3485`).
* D3D constants used above: `D3DBLEND` 2 = ONE, 5 = SRCALPHA, 6 = INVSRCALPHA,
  7 = DESTALPHA; `D3DTOP` 1 = DISABLE, 2 = SELECTARG1, 3 = SELECTARG2, 4 = MODULATE;
  `D3DTA` 0 = DIFFUSE, 1 = CURRENT, 2 = TEXTURE; `D3DCMP` 7 = GREATER_EQUAL,
  8 = ALWAYS; `D3DTEXF` 1 = POINT, 2 = LINEAR.

> ### ⚠ Callout — block 0 cannot produce tinted solid fills (unknown #2)
>
> Literally, `SetRenderStateBlock(0)` leaves stage 0 as
> `COLOROP = SELECTARG1(TEXTURE)`, `ALPHAOP = SELECTARG1(TEXTURE)`. With
> `SetTexture(0, nullptr)` (which `RenderRectNoTex` does) the texture argument is white
> `(1,1,1,1)`, so a `-2` "solid fill" would come out **opaque white**, ignoring
> `dwColor` completely, and every `RenderRectTex` tint would be discarded.
>
> That contradicts the code's intent everywhere it passes a colour: the 748 field scene
> uses `-2` panels with `dwColor = 0x77000000` (`FieldScene2.bin` record 0, a 240×240
> black screen-veil at full alpha 0x77) and the minimap markers are `SPanel(-2, …,
> g_MinimapPos[i].dwColor, …)` `[OBSERVED]` `TMFieldScene.cpp:2008`.
>
> Two readings are possible:
> 1. **Literal** — the states above really are what D3D9 was told, and those fills were
>    white in 2007 too (unlikely, but possible for the `-2` convention).
> 2. **Intended** — block 0 was meant to be `MODULATE` (`4`, as blocks 1/2/3 are), and the
>    `2u` constants are a decompilation/transcription artifact.
>
> `[UNKNOWN]` — resolve with one of: (a) `GetRenderState(D3DRS_*)/GetTextureStageState`
> logging proxy around the shipped `WYD.exe` while a `-2` panel is on screen;
> (b) disassemble the block-0 setup in `WYD.exe` and read the immediates;
> (c) screenshot a `-2` panel in the original and sample its RGB.
> **Recommendation for the port:** implement reading 2 (`MODULATE`), i.e. tint solid
> fills and stretched images by `dwColor` (this is also what every other chapter of this
> project assumes), and record the deviation in the port's notes until (a)–(c) settle it.

### 9.3 `dwColor` modulation path (intended)

```
control.dwColor  ──► WYDCOLOR_SCALE_ALPHA(·, g_nUIFadeAlpha)  ──► vertex diffuse
                                                                    │
texture RGB ───────────────────────────────────────────────────────►× ──► framebuffer
texture A   ───────────────────────────────────────────────────────►× ──► blend factor
```

Intended: UI colour = texture × tint, **alpha = texture alpha × tint alpha** (block 2's
`ALPHAOP = MODULATE(TEXTURE, DIFFUSE)`). At the default `g_nUIFadeAlpha = 255` the tint
alpha passes through unchanged. **This holds only where the stage-0 ops are `MODULATE`** —
see the callout in §9.2 for the block-0 discrepancy.

### 9.4 The `ID3DXSprite` question (biggest unknown)

`RenderRect`/`RenderRectRot` call `m_pSprite->Begin()` with **flags = 0** `[OBSERVED]`
(`:1907`, `:2773`). `ID3DXSprite::Begin(D3DXSPRITE_ALPHABLEND)` is the flag that makes the
sprite set up `ALPHABLENDENABLE = TRUE`, `SRCALPHA/INVSRCALPHA` and premultiply the vertex
colour. Without it the runtime leaves blending alone and only replaces the transform and
the texture-stage setup.

Under block 0 that means `ALPHABLENDENABLE = 0` and `COLOROP = SELECTARG2 (DIFFUSE)` —
the sprite would be drawn **fully opaque in white** (diffuse is `0xFFFFFFFF` in the
`Draw` call), ignoring both the texture RGB *and* the tint. Since panels are visibly
textured *and* alpha-blended in the shipped client, either (a) the D3DX9 version used here
enables blending regardless, or (b) some other state survives.

`[UNKNOWN]` — decide with one experiment: capture a frame in the original binary
(RenderDoc/PIX, or a `IDirect3D9` proxy that logs `GetRenderState` inside
`Begin`/`End`), or run the port with `m_pSprite->Begin(D3DXSPRITE_ALPHABLEND)` and diff
the screenshots. The pragmatic, high-confidence reading of the *intent* is:
**RENDER_IMAGE and RENDER_IMAGE_STRETCH panels are alpha-blended, tinted by `dwColor`,
and point-sampled** — implement that and verify against screen captures.

### 9.5 Depth testing

`[OBSERVED]` The sequence actually is:

```
ObjectManager::RenderControl:  SetRenderState(ZENABLE, 0)        // ObjectManager.cpp:602
   for each control:           RenderGeomControl -> SetRenderStateBlock(0)
                                  -> SetRenderState(ZENABLE, 1)  // RenderDevice.cpp:1608
                               ... draw ...
                               (TMFont2 path sets ZENABLE 0 again via block 2)
ObjectManager::RenderControl:  SetRenderState(ZENABLE, 1)        // :624
```

So **depth testing is re-enabled by block 0 for every control** and `ZFUNC` is
`LESSEQUAL` (set by block 1, `:1712`, never reset by block 0). All hand-built UI quads
carry `z = 0.01`, the halo carries `z = 0.1`, and `ZWRITEENABLE = 1` (from block 1).

Why the UI still draws on top and overlaps correctly:

* `z = 0.01` in a 16-bit-or-deeper depth buffer with near 0.966 / far 70 maps to a
  *very* small depth value, i.e. closer than any world fragment → every UI pixel passes
  against the 3D scene.
* Against other UI quads the comparison is `new_z (0.01) <= stored_z (0.01)` → **equal
  passes**, and the new fragment also writes `0.01`. So "last drawn wins" — identical to
  depth-test-off, as long as all quads share z. This holds for every primitive except
  `RenderGeomControlBG` (`z = 0.1`), which would be *rejected* if drawn after a `0.01`
  quad at the same pixel `[INFERRED]` — in practice the halo is drawn before its own
  control's icon? No: `RenderGeomControl` draws the mesh first, then the halo
  (`:3344-3364`), so the halo can lose to the icon where they overlap. Flagged as a
  likely original bug.
* `RENDER_3DOBJ` re-enters block 1 and sets `ZFUNC = 8 (ALWAYS)` (`:3324`) so the preview
  model always wins over earlier UI.

Practical requirement for a GL/Vulkan port: **disable depth test and depth write for the
whole 2D UI pass** and rely purely on submission order (§10). That reproduces the observed
result except for the `0.1` halo case above, which you should decide explicitly. Do not
enable depth writes: they would poison the depth buffer for the next frame's 3D pass
(the original clears the depth buffer each frame in `Lock()`).

### 9.6 Clipping

`[OBSERVED]` There is **no scissor and no stencil** anywhere in the codebase:
`D3DRS_SCISSORTESTENABLE` never appears; `D3DRS_STENCILENABLE` is only ever set to 0
(block 2/3). What exists instead:

1. **Per-control containment tests** before submission — `SPanel::FrameMove2`
   (`SControl.cpp:374-377`) and `SText::FrameMove2` (`:854-860, 956-959`) drop controls
   whose rect is entirely off-screen, using
   `pos+size >= 0 && pos <= 800·W_ratio / 600·H_ratio`.
2. **`GeomControl.bClip == 1`** — a vertical *source-rect* trim, not a real clip
   (`RenderDevice.cpp:2869-2885`): the source rect becomes
   `(startX, startY + fTop, width, fBottom − fTop)` and the destination Y is shifted by
   `fTop`, with the same scale factors. This is how scrolling lists "clip" their rows —
   it changes what is sampled, not what is written.
3. Grid/list controls shrink their contents rather than clipping them
   (`SGridControl::FrameMove2`, `SControl.cpp` 5000+).

So: nothing in the UI can be partially clipped horizontally. A GL port should **not** add
a scissor where the original has none, or overlapping windows will look different.

---

<a name="10-ordering"></a>
## 10. Ordering and overlap

`[OBSERVED]` Ordering is a two-level sparse list, `stGeomList m_pDrawControl[30]`
(`MAX_DRAW_CONTROL = 30`, `SControlContainer.h:7`):

```
layer index  = value passed as `inParentLayer` when the control calls AddRenderControlItem
layer order  = 0, 1, 2, …, 29   (ascending; higher = drawn later = on top)
within layer = append order = order in which FrameMove2() was called
               = pre-order tree traversal of the control tree
siblings     = REVERSED creation order, because TreeNode::AddChild() prepends
               (TreeNode.cpp:120-129: pChild->m_pNextLink = m_pDown; m_pDown = pChild)
```

So the intended semantics are "**z = depth in the control tree; ties = reverse creation
order**", and the cursor is pinned at layer 29 (`SControlContainer.cpp:308`).

Because of the two traversal bugs in §4.4 (layer never decremented, wrong parent offset),
the *actual* layer is not the depth — but the ordering it produces is still a
monotonically-increasing function of traversal position, so the practical drawing order in
this port is: **root children in reverse creation order, then each one's children
immediately after it**, all clamped to layers 0..29 with everything past 29 dropped.

Recommended reimplementation:

```
for each control in tree, pre-order:
    depth   = number of ancestors (root = 0)
    z       = depth*1000 + reverse_sibling_index
draw UI sorted by (z, submission order within the same control)
cursor  = z = +infinity (or the highest layer)
tooltips/desc panels: they are ordinary SPanel/SText children of a panel, so their
    "on top" behaviour comes from being a *child* (depth +1) of the hovered panel,
    not from any explicit priority field.
```

There is **no `nPriority`** field. `m_bAlwaysOnTop` exists on `SControl`
(`SControl.h:89`) but is only assigned (`SCursor::FrameMove2:486`,
`SControl.cpp:112-116`) and never read by the renderer `[OBSERVED]`.
`GeomControl.nZ` exists but is never read `[OBSERVED]`.

Modal handling (`m_pModalControl[8]`) affects **input routing only**
(`SControlContainer.cpp:41-51`), never drawing.

Layer budget: because `MAX_DRAW_CONTROL = 30` and layers are also used as the cursor's
"on top" slot, a port with a deeper UI tree must raise the constant or switch to a real z
index. `[INFERRED]` 748's real control trees are shallow (the `.bin` parent chains are
id-based and mostly one or two levels), but this was not measured — walking `FieldScene2.bin`
after the parser is fixed (§4.1) is the way to confirm it before choosing a z format.

---

<a name="11-reimplementation"></a>
## 11. Reimplementation notes (OpenGL / Vulkan)

### 11.1 Geometry and coordinate space

* Render 2D UI in **window pixels, origin top-left, Y down**. Either flip Y when building
  the projection (`ortho(0, W, H, 0, -1, 1)`) or emit vertices pre-flipped and use
  `glClipControl`/negative viewport height in Vulkan. Do **not** reproduce
  `SetMatrixForUI`'s X flip.
* One shader for all 2D: `in vec2 pos_px; in vec4 colour; in vec2 uv;`
  `uniform vec2 screen;` → `clip = vec2(pos_px.x/screen.x*2-1, 1-pos_px.y/screen.y*2)`.
  This reproduces `XYZRHW` + `rhw = 1` exactly, including the fact that z is ignored.
* Keep a separate 3D-in-UI path for `RENDER_3DOBJ`: build the model's world matrix from
  pixels using the `TMMesh::RenderForUI` constants (§6.4) with an orthographic projection
  covering `x ∈ [-3.15, 3.15]`, `y ∈ [-2.48, 2.48]`.

### 11.2 Batching

The original issues **one `DrawPrimitiveUP` per quad** (and one
`ID3DXSprite::Begin/Draw/End` per sprite) — an entire 1024×768 field UI frame costs
roughly 300–600 draw calls `[INFERRED]` from the per-control quad count. A port should:

1. Keep a single dynamic vertex buffer with a **CPU-side batching layer** that
   accumulates quads and flushes on texture change (texture-set changes are far rarer
   than quads).
2. Preserve submission order exactly (§10) — the batch must be a FIFO with no depth sort.
3. Keep the six-index pattern per quad (`TRIANGLEFAN` of 4 vertices ≡ two triangles
   `0,1,2 / 0,2,3`) so that the degenerate winding never matters (culling must be off, as
   in the original).
4. Reproduce `DESTBLEND = INVSRCALPHA` premultiplied-style math: `dst = src*srcA + dst*(1-srcA)`.
   For the additive halo (`DESTBLEND = ONE`): `dst = src*srcA + dst`.

### 11.3 Glyph handling — the one place to redesign

The original uploads a **fresh 512×64 texture per `SetText` call and per font object**
(~64 KiB × number of text controls, plus a lock/convert/upload per frame per changed
string). Reproduce the *appearance*, not the mechanism:

* Build a **process-wide glyph atlas** (single R8 or R8G8B8A8 texture, e.g. 1024×1024)
  rasterised once per font size with the same GDI-equivalent settings
  (Tahoma 14 px, weight 500, `ANTIALIASED`, grayscale coverage).
* Cache glyphs by code point; the original's per-control 512×64 scratch texture exists
  only because GDI renders whole strings.
* Sample with **POINT/NEAREST** filtering to match `MAGFILTER = POINT` (§9.2) so the
  pixel pattern is identical; bilinear will visibly soften text.
* Emit **one quad per glyph** instead of one stretched quad per line — but keep the same
  pixel metrics: the original's line advance is `m_nFontSize + 1` rows and the quad is
  exactly `GetTextExtent(str).cx` px wide, so total advance per glyph must come from the
  same font metrics, not from a kerning table.
* Recolour via vertex colour (`texture × tint`), never by re-rasterising.
* The 1 px drop shadow = the same quads offset by `(+1,+1)` in `m_dwShadeColor`, drawn
  **before** the main quads.
* Keep the global `+1` px Y offset and the 640-wide `−1` correction for pixel parity.
* Avoid texture uploads per frame entirely: only the atlas creation touches the GPU once,
  and dynamic strings (chat, damage numbers) become pure vertex work.

If exact-pixel parity with the original matters more than speed, the only faithful method
is to keep a GDI (or FreeType with identical hinting/metrics) rasterisation path at
`m_nFontSize` px into an 8-bit coverage bitmap and upload *that* once per string into a
pooled atlas region — but no per-frame full-surface lock/copy.

### 11.4 Texture sampling and palette quirks to preserve

* UI atlases are `D3DFMT_A4R4G4B4`/`A1R5G5B5`/`A8R8G8B8` depending on the `.wys` alpha
  class and the desktop bit depth (`TextureManager.cpp:278-292, 340-350`). At 32 bpp with
  `cAlpha == 'A'` the format is `D3DFMT_UNKNOWN` → D3DX picks from the file. Decoding is
  another chapter's problem; the draw side only needs "the texture as loaded, sampled with
  point filtering and `WRAP` addressing (for `RENDER_IMAGE_TILE`)".
* Bilinear (block 0 / block 3) applies to stretched panels; point applies to fonts and
  (because block 2 stays current) to any `RenderRectC`/`RenderRectNoTex`/`RenderRectTex`
  call made while block 2 is active — in practice the font and the damage sprites.
* Transparent colour key `0xFF000000` is used when creating TGA textures
  (`TextureManager.cpp:265`, `RenderRect...` n/a) — i.e. **black is the transparent
  colour** for keyed UI art. Reproduce as `alpha = (texel == 0) ? 0 : 1` if you drop the
  key; better, keep the key at upload time.
* `RENDER_IMAGE_TILE` relies on `ADDRESSU/V = WRAP` (never set explicitly → D3D default).

### 11.5 State the port must force per frame

```
depth test        OFF, depth write OFF      (UI pass)
cull              OFF                        (original CULLMODE = CCW but winding is
                                              inconsistent; never enabled in practice)
blend             SRC_ALPHA, ONE_MINUS_SRC_ALPHA   (and SRC_ALPHA, ONE for the halo)
alpha test        ref 8/255, GREATER_EQUAL, ON for text; OFF for bTrans fills
scissor           OFF
srgb              OFF (colours are used raw; gamma is applied with SetDeviceGammaRamp,
                  RenderDevice.cpp:719-748 / 293-314 — i.e. a hardware ramp, not a shader)
```

`SetGamma()` writes the D3D gamma ramp from `m_nBright`
(`ramp[i] = clamp(m_nBright*0.02*i*256)`), so the *whole screen* is brightness-scaled on
the GPU — a port must either keep an equivalent ramp (`vkSetGamma`/`SetDeviceGammaRamp`)
or fold it into the final blit, otherwise colours will not match.

### 11.6 Migration checklist

| Original behaviour | Required in GL/VK |
|---|---|
| `XYZRHW` immediate quads | CPU-built screen-space vertices, `pos/screen` → NDC |
| `D3DPT_TRIANGLEFAN`, 4 verts | 2 tris per quad, no culling |
| per-quad `DrawPrimitiveUP` | batched dynamic VBO, flush on texture change |
| `ID3DXSprite` (RenderRect/Rot) | same quad path; rotation about the destination centre |
| font scratch texture per control | shared glyph atlas, NEAREST |
| `A4R4G4B4` font atlas (4-bit alpha) | R8 atlas is fine; point-sample |
| `DESTBLEND = INVSRCALPHA` | `ONE_MINUS_SRC_ALPHA` |
| additive halo (`ONE`) | `ONE, ONE` |
| no depth / no scissor | same |
| gamma ramp | keep or emulate |
| `.bin` positions in raw pixels | **do not** rescale |
| `BASE_ScreenResize` positions | `x*W/800`, `y*H/600` (non-uniform) |

---

<a name="appendix-a"></a>
## Appendix A — symbol index (file:line, primary evidence)

| Symbol | Location |
|---|---|
| `RenderDevice` ctor / statics | `RenderDevice.cpp:18-142` |
| `SetViewPort` | `RenderDevice.cpp:343-358` |
| DIB + HFONT + FONTCONFIG | `RenderDevice.cpp:564-599` |
| `SetRenderStateBlock` 0/1/2/3 | `RenderDevice.cpp:1604-1826` |
| `SetProjectionMatrix` | `RenderDevice.cpp:1839-1854` |
| `SetMatrixForUI` | `RenderDevice.cpp:1856-1872` |
| `GetPickRayVector` | `RenderDevice.cpp:1874-1891` |
| `RenderRect` / `RenderRectRot` | `RenderDevice.cpp:1893-1911` / `2759-2777` |
| `RenderRectC` / `RenderRectCoord` / `RenderRectNoTex` | `1913-1940` / `1942-1969` / `1971-2041` |
| `RenderRectTex` / `...Tex2C` / `...Tex2` / `...Tex2M` | `2043-2162` / `2164-2292` / `2294-2372` / `2374-2452` |
| `RenderRectTexDamage` / `...Progress2` | `2454-2544` / `2546-2757` |
| `RenderGeomRectImage` | `RenderDevice.cpp:2833-3251` |
| `RenderGeomControl` | `RenderDevice.cpp:3253-3427` |
| `RenderGeomControlBG` | `RenderDevice.cpp:3429-3489` |
| Vertex structs | `Structures.h:96-100, 212-230` |
| `GeomControl` | `GeomObject.h:7-49`, `GeomObject.cpp:5-73` |
| `stGeomList`, `MAX_DRAW_CONTROL` | `SControlContainer.h:7,36` |
| `Add/RemoveRenderControlItem` | `SControl.cpp:3002-3051` |
| `SControlContainer::FrameMove` | `SControlContainer.cpp:259-311` |
| `SPanel::FrameMove2` | `SControl.cpp:365-382` |
| `SText::FrameMove2` (alignment) | `SControl.cpp:843-989` |
| `SCursor::FrameMove2` | `SControl.cpp:472-523` |
| `TMFont2::SetText` / `Render` | `TMFont2.cpp:29-275` / `282-371` |
| `TMFont3` ctor / `Render` / `FrameMove` | `TMFont3.cpp:5-196` / `202-297` / `299-399` |
| `TextureManager::GenerateTexture` | `TextureManager.cpp:1186-1381` |
| `TextureManager::LoadGuildTexture` | `TextureManager.cpp:1398-1422` |
| `TextureSet`/`GuildMark` structs | `TextureManager.h:10-37` |
| `SControlContainer` / `ObjectManager::RenderControl` | `SControlContainer.cpp:7-22`, `ObjectManager.cpp:591-626` |
| Frame submission order | `NewApp.cpp:716-742` |
| `BASE_Get3DTo2DPos` | `Basedef.cpp:3039-3069` |
| `BASE_ScreenResize` | `Basedef.cpp:33-36` |
| `IsClearString` (DBCS boundary) | `Basedef.cpp:1295-1317` |
| `strdef.bin` loader | `Basedef.cpp:300-327` |
| `UIString.txt` loader | `Basedef.cpp:1178-1199` |
| `TMGround::SetMiniMapData` | `TMGround.cpp:3746-3767` |
| `Guildmark_IsCorrectBMP` / download | `TMFieldScene.cpp:22961-22980` / `24738-24762` |
| Head-label projection | `TMHuman.cpp:7413-7500` |
| Colour helpers | `TMGlobal.h:17-35` |

## Appendix B — evidence files

| File | Content |
|---|---|
| `evidence/hex/ui-minimap-wyt.txt` | `UI/minimap.wyt` header hexdump, TGA field decode, size arithmetic |
| `evidence/hex/ui-strdef-xor5a.txt` | `UI/strdef.bin` raw + XOR-0x5A decoded records |
| `evidence/hex/ui-fieldscene2-panel289.txt` | the real minimap control record from `FieldScene2.bin` |
| `evidence/hex/ui-uitxturesetlist-sets.txt` | texture sets 0, 11, 137, 291 verbatim |
| `evidence/hex/ui-uistring-head.txt` | `UI/UIString.txt` first 192 bytes |
| `evidence/hex/_gen_ui_evidence.js` | generator for the five files above |
