# 06 — Render states, materials, lighting, fog, blending and swap chain (WYD 748 / TMProject D3D9 port)

Purpose: the complete, implementable description of every device/state/material/light/fog/blend/depth decision
this client makes, with D3D9 → OpenGL 3.3+ and Vulkan 1.1+ equivalents, so the renderer can be rebuilt from
this document alone.

Last updated: 2026-11-10

---

## Confidence summary

| Tag | Count | Notes |
|---|---|---|
| `[OBSERVED]` | ~120 | every state value below was read in `RenderDevice.cpp` / `D3DDevice.cpp` / `TMMesh.cpp` / `TMGround.cpp` / `TMEffect*.cpp`, or taken from a hexdump / log |
| `[INFERRED]` | ~10 | meaning of decompiled enum literals (verified against the `D3D*` enums), intent of "shadow" leftovers, per-pass ordering |
| `[UNKNOWN]` | 4 | see below |

Biggest unknowns:

1. **`[UNKNOWN]`** Whether the *original* 748 exe ever enables a stencil buffer. The port requests
   `D3DFMT_D16` first (§6.1) which has 0 stencil bits and never sets `D3DRS_STENCILENABLE` to 1, so in the
   port stencil is provably unused; the 748 exe's device creation parameters are not recoverable from the
   (obfuscated) binary without running it. Resolved by: `apitrace`/API-Monitor the 748 client and inspect
   `CreateDevice`'s `AutoDepthStencilFormat`.
2. **`[UNKNOWN]`** The numeric value of `Config.bright` in a default install (range 0..100, default 50 is
   only the compiled-in default). Resolved by: read `client_748`'s config file.
3. **`[UNKNOWN]`** Whether `TMLight` dynamic point lights (indices ≥ 2) are lit in hardware on the target
   GPU: `LightEnable` supports up to `MaxActiveLights` (8 typically) and the code never checks the cap.
   Resolved by: `D3DCAPS9.MaxActiveLights` dump on the target machine.
4. **`[UNKNOWN]`** The exact contents of `m_dwClearColor` for every sky state (`m_nState` 0..15) — only
   0..3, 10..13 are assigned in the source; the rest keep whatever the previous scene left. Resolved by:
   log `m_dwClearColor` once per frame with `LogRenderState`-style instrumentation.

---

## Table of contents

1. [The two state entry points](#1-the-two-state-entry-points)
2. [State caching arrays](#2-state-caching-arrays)
3. [Lighting](#3-lighting)
4. [Fog](#4-fog)
5. [Alpha / blending model](#5-alpha--blending-model)
6. [Depth and stencil](#6-depth-and-stencil)
7. [Culling, fill mode, multisampling, depth hacks](#7-culling-fill-mode-multisampling-depth-hacks)
8. [Swap chain, clear colour, device loss](#8-swap-chain-clear-colour-device-loss)
9. [Gamma](#9-gamma)
10. [State-dump debug helpers](#10-state-dump-debug-helpers)
11. [Reimplementation notes (OpenGL / Vulkan)](#11-reimplementation-notes-opengl--vulkan)

Enum references used throughout (`d3d9types.h`, `[OBSERVED]` at the call sites):

| Enum | Values used by this client |
|---|---|
| `D3DCULL` | `1`=NONE, `2`=CW, `3`=CCW |
| `D3DBLEND` | `1`=ZERO, `2`=ONE, `3`=SRCCOLOR, `4`=INVSRCCOLOR, `5`=SRCALPHA, `6`=INVSRCALPHA, `7`=DESTALPHA, `9`=DESTCOLOR |
| `D3DCMPFUNC` | `4`=LESSEQUAL, `7`=GREATEREQUAL, `8`=ALWAYS |
| `D3DTOP` | `1`=DISABLE, `2`=SELECTARG1, `4`=MODULATE, `5`=MODULATE2X, `6`=MODULATE4X, `7`=ADD, `11`=ADDSMOOTH, `18`=MODULATEALPHA_ADDCOLOR, `24`=DOTPRODUCT3 |
| `D3DTA` | `0`=DIFFUSE, `1`=CURRENT, `2`=TEXTURE, `3`=TFACTOR |
| `D3DFOGMODE` | `0`=NONE, `3`=LINEAR |
| `D3DFILLMODE` | `2`=WIREFRAME, `3`=SOLID |
| `D3DSHADEMODE` | `1`=FLAT, `2`=GOURAUD |
| `D3DSAMP_*FILTER` | `0`=NONE, `1`=POINT, `2`=LINEAR, `3`=ANISOTROPIC |
| `D3DTSS_TCI_*` | `0`=PASSTHRU, `1`…=texcoord sets, `0x30000`=CAMERASPACEREFLECTIONVECTOR |

---

## 1. The two state entry points

### 1.1 `RenderDevice::InitializeRenderingState()` — `RenderDevice.cpp:1575-1602`

`[OBSERVED]` — it does **not** set a single `D3DRS_*`. It only:

```cpp
D3DXMatrixIdentity(&m_matWorld);
m_pd3dDevice->SetTransform(D3DTS_WORLD, &m_matWorld);   // :1577-1578
// then invalidates the three cache arrays:
m_dwRenderStateList[0..255]        = -1  (0xFFFFFFFF)    // :1580-1585
m_dwSamplerStateList[0..7][0..13]  = -1                  // :1587-1593
m_dwTextureStageStateList[0..7][0..28] = -1              // :1595-1599
```

So after `InitializeRenderingState()` **every** D3D9 render/sampler/texture-stage state is whatever the
D3D9 runtime default is. The engine then relies on `SetRenderStateBlock()` being called before any draw.
This is an important difference from a "typical" D3D9 app: **there is no global init of blend/Z/depth state.**

Consequence: on the very first frame, the state is the D3D9 default
(`ZENABLE=TRUE, ZWRITEENABLE=TRUE, ZFUNC=LESSEQUAL, CULLMODE=CCW, ALPHABLENDENABLE=FALSE,
ALPHATESTENABLE=FALSE, LIGHTING=TRUE, FOGENABLE=FALSE, DITHERENABLE=FALSE, SHADEMODE=GOURAUD,
SRCBLEND=ONE, DESTBLEND=ZERO, COLORVERTEX=FALSE, MULTISAMPLEANTIALIAS=TRUE, …`) until the first
`SetRenderStateBlock()` at `NewApp.cpp:730`.

### 1.2 The main 3D block — `SetRenderStateBlock(1)` (`RenderDevice.cpp:1632-1759`)

Called once per frame from `NewApp::RenderScene()` (`NewApp.cpp:730`) after `SetViewVector()` and before
`m_pObjectManager->RenderObject()`.

| # | Call | Value | Meaning | GL equivalent | Vulkan equivalent |
|---|---|---|---|---|---|
| 1 | `SetRenderState(D3DRS_ALPHABLENDENABLE)` | 1 | enable blending for everything from here on | `glEnable(GL_BLEND)` | `VkPipelineColorBlendAttachmentState.blendEnable=VK_TRUE` |
| 2 | `D3DRS_ALPHATESTENABLE` | 1 | enable alpha test | `glEnable(GL_ALPHA_TEST)` (compat) / `discard` in shader | fragment `discard` |
| 3 | `D3DRS_ALPHAFUNC` | 7 = GREATEREQUAL | pass if `alpha >= ALPHAREF` | `glAlphaFunc(GL_GEQUAL, r)` | `if (a < r) discard;` |
| 4 | `D3DRS_ALPHAREF` | `0xDD` (32-bit) / `0xD` (16-bit); `0xFF000000`/`0xF000` if `m_iVGAID==1` | alpha threshold (221/255 ≈ 0.867) | `glAlphaFunc` ref = `0xDD/255` | uniform float 0.867 |
| 5 | `D3DRS_AMBIENT` | `0x33FFFFFF` | global ambient = white with alpha byte 0x33 | `GL_LIGHT_MODEL_AMBIENT = (1,1,1,1)` (D3D's alpha nibble is unused here) | constant in shader |
| 6 | `D3DRS_ZENABLE` | 1 | depth test on | `glEnable(GL_DEPTH_TEST)` | `depthTestEnable=VK_TRUE` |
| 7 | `D3DRS_ZFUNC` | 4 = LESSEQUAL | `z <= zbuf` passes | `glDepthFunc(GL_LEQUAL)` | `depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL` |
| 8 | `D3DRS_ZWRITEENABLE` | 1 | depth writes on | `glDepthMask(GL_TRUE)` | `depthWriteEnable=VK_TRUE` |
| 9 | `D3DRS_LIGHTING` | 1 | fixed-function lighting on | emulate (see §3) | shader |
| 10 | `D3DRS_SPECULARENABLE` | 0 | no per-vertex specular | `glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,…)`; effectively specular = 0 | shader |
| 11 | `D3DRS_CULLMODE` | 3 = CCW | cull counter-clockwise faces (keep CW) | `glFrontFace(GL_CW); glCullFace(GL_BACK); glEnable(GL_CULL_FACE)` | `frontFace=VK_FRONT_FACE_CLOCKWISE`, `cullMode=VK_CULL_MODE_BACK_BIT` (with D3D-style Y-down clip → see §11) |
| 12 | `D3DRS_SHADEMODE` | 2 = GOURAUD | smooth shading | `glShadeModel(GL_SMOOTH)` | n/a |
| 13 | `D3DRS_COLORVERTEX` | 1 | vertex diffuse/alpha used as material | `glEnable(GL_COLOR_MATERIAL)` | shader |
| 14 | `D3DRS_NORMALIZENORMALS` | 1 | renormalise normals after transform | `glEnable(GL_NORMALIZE)` | `normalize()` in shader |
| 15 | `D3DRS_DITHERENABLE` | 1 | ordered dither (colour quantisation) | `glEnable(GL_DITHER)` | n/a (no-op on modern HW) |
| 16 | `D3DRS_SRCBLEND` | 2 = ONE | **premultiplied** alpha | `glBlendFunc(GL_ONE, GL_INVSRCALPHA…)` see next row | `srcColorBlendFactor=VK_BLEND_FACTOR_ONE` |
| 17 | `D3DRS_DESTBLEND` | 6 = INVSRCALPHA | | `glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)` | `dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA` |
| 18 | `D3DRS_FOGENABLE` | `m_bFog` (1 by default; `TMSky` clears it for one weather state, `TMSky.cpp:391`) | fog on | `glEnable(GL_FOG)` (compat) / shader | shader |
| 19 | `D3DRS_FOGCOLOR` | `m_dwClearColor` | fog colour = current clear colour | `glFogfv(GL_FOG_COLOR,…)` | uniform |
| 20 | `D3DRS_FOGVERTEXMODE` | 3 = LINEAR | **per-vertex** linear fog (not table fog) | `glFogi(GL_FOG_MODE, GL_LINEAR); glFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH)` | shader, per-vertex |
| 21 | `D3DRS_FOGSTART` | `floatBits(m_fFogStart + sightLength - 8.0f)` | see §4.3 | `glFogf(GL_FOG_START,…)` | uniform |
| 22 | `D3DRS_FOGEND` | `floatBits(m_fFogEnd + sightLength - 15.0f)` | see §4.3 | `glFogf(GL_FOG_END,…)` | uniform |
| 23 | `D3DRS_RANGEFOGENABLE` | 0 | distance = \|z\| in view space, not radial distance | (GL default) | shader |
| 24 | `D3DRS_DIFFUSEMATERIALSOURCE` | 1 = MATERIAL (set twice, same value) | material diffuse used | `glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE)` semantics | material UBO |
| 25 | `D3DRS_SPECULARMATERIALSOURCE` | 2 = COLOR1, then **1 = MATERIAL** (last write wins) | material specular used | idem | material UBO |
| 26 | `D3DTSS_COLOROP(0)` | 4 = MODULATE | `tex0 * diffuse` | combiners / shader: `c = t0.rgb * vColor.rgb` | shader |
| 27 | `D3DTSS_COLORARG1(0)` | 2 = TEXTURE | | | |
| 28 | `D3DTSS_COLORARG2(0)` | 0 = DIFFUSE | | | |
| 29 | `D3DTSS_ALPHAOP(0)` | 2 = SELECTARG1 | alpha = tex0 alpha | `c.a = t0.a` | shader |
| 30 | `D3DTSS_ALPHAARG1(0)` | 2 = TEXTURE | | | |
| 31 | `D3DTSS_ALPHAARG2(0)` | 1 = CURRENT | (unused with SELECTARG1) | | |
| 32 | `D3DTSS_TEXCOORDINDEX(0)` | 0 | texcoord set 0 | `glTexCoordPointer` index / attribute | attribute location |
| 33 | `D3DSAMP_MINFILTER(0)` | 2 = LINEAR | | `glTexParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR…)` | `VkSamplerCreateInfo` |
| 34 | `D3DSAMP_MAGFILTER(0)` | 2 = LINEAR | | `GL_LINEAR` | `VK_FILTER_LINEAR` |
| 35 | `D3DSAMP_MIPFILTER(0)` | 2 = LINEAR | trilinear | `GL_LINEAR` mip filter | `mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR` |
| 36 | `D3DTSS_COLOROP(1)` | 1 = DISABLE | stage 1 off by default (objects enable it per-mesh) | – | – |
| 37 | `D3DTSS_ALPHAOP(1)` | 2 = SELECTARG1 | | | |
| 38 | `D3DTSS_TEXCOORDINDEX(1)` | 1 | texcoord set 1 | | |
| 39 | `D3DSAMP_MIPFILTER(1)` | 0 = NONE | **no mipmapping on stage 1** → aliasing/shimmer on the detail layer | `glTexParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR)` (no mip) | `mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST` + `maxLod=0` |
| 40 | `D3DTSS_COLOROP(2)` | 1 = DISABLE | stage 2 off | – | – |

Note the **`ONE / INVSRCALPHA`** pair at rows 16-17: opaque geometry (alpha = 1 from the texture or the
material) is unaffected, but anything with `src alpha < 1` gets *additive-ish* blending instead of the usual
`SRCALPHA / INVSRCALPHA`. `[INFERRED]`: this is a leftover from the DX8-era "premultiplied texture" pipeline;
switching to `SRCALPHA/INVSRCALPHA` will visibly change translucent world geometry.

### 1.3 The UI/2D block — `SetRenderStateBlock(0)` (`RenderDevice.cpp:1606-1631`)

Used before UI quads (`RenderGeomRectImage` `:2887`, `RenderGeomControl` `:3273`).

| Call | Value | Meaning |
|---|---|---|
| `D3DRS_ZENABLE` | 1 | depth test on (UI is drawn with `D3DFVF_XYZRHW` and z≈0.01… see §6) |
| `D3DRS_FOGENABLE` | 0 | UI is **never** fogged |
| `D3DRS_LIGHTING` | 0 | UI is unlit |
| `D3DRS_SPECULARENABLE` | 1 | (irrelevant, lighting off) |
| `D3DRS_CULLMODE` | 3 = CCW | |
| `D3DRS_SHADEMODE` | 2 = GOURAUD | |
| `D3DRS_AMBIENT` | `0x0AAAAAA` | near-black ambient (note: this is a malformed D3DCOLOR — alpha nibble `0x0`, RGB `0xAAAAAA` = 2/3 grey) |
| `D3DRS_SRCBLEND` | 5 = SRCALPHA | standard alpha blending (used by the transparent colour quads) |
| `D3DRS_DESTBLEND` | 7 = DESTALPHA | **not** INVSRCALPHA — `[INFERRED]` a quirk; with `ALPHABLENDENABLE=0` (next row) it does not matter until a UI element turns blending on |
| `D3DRS_COLORVERTEX` | 1 | |
| `D3DRS_DITHERENABLE` | 1 | |
| `D3DRS_ALPHABLENDENABLE` | 0 | off by default; `RenderRectNoTex(...,bTrans=1)` turns it on per quad |
| `D3DTSS_COLOROP(0)` | 2 = SELECTARG1, `COLORARG1`=2 TEXTURE | UI quads are **texture only**, vertex colour ignored for RGB |
| `D3DTSS_ALPHAOP(0)` | 2 = SELECTARG1, `ALPHAARG1`=2 TEXTURE | alpha from texture |
| `D3DSAMP_MINFILTER/MAGFILTER` | 2/2 = LINEAR | UI is linear-filtered |
| `D3DSAMP_MIPFILTER` | 1 = POINT | nearest mip |
| `D3DTSS_TEXCOORDINDEX(0)` | 0 | |
| – | `SetFVF(322)` | `D3DFVF_XYZ|DIFFUSE|TEX1`, 28-byte stride (`RDTLVERTEX`) — later calls re-set 324 (`XYZ|DIFFUSE|TEX1` with rhw = `0x144`) |

### 1.4 The UI-overlay block — `SetRenderStateBlock(3)` (`:1792-1823`)

Applied by `NewApp.cpp:732` **before `RenderControl()`**, i.e. every UI element is drawn under it.

| Call | Value |
|---|---|
| `D3DRS_ZENABLE` | **0** (no depth test for UI) |
| `D3DRS_ALPHABLENDENABLE` | **0** |
| `D3DRS_SRCBLEND` / `DESTBLEND` | 5 (SRCALPHA) / 6 (INVSRCALPHA) |
| `D3DRS_ALPHATESTENABLE` | 1 |
| `D3DRS_ALPHAREF` | **8** |
| `D3DRS_ALPHAFUNC` | 7 = GREATEREQUAL |
| `D3DRS_FILLMODE` | 3 = SOLID |
| `D3DRS_CULLMODE` | 3 = CCW |
| `D3DRS_STENCILENABLE` | **0** |
| `D3DRS_CLIPPING` | 1 |
| `D3DRS_MULTISAMPLEANTIALIAS` | **0** (UI is not anti-aliased) |
| `D3DRS_ANTIALIASEDLINEENABLE` | 0 |
| `D3DRS_VERTEXBLEND` | 0 |
| `D3DRS_INDEXEDVERTEXBLENDENABLE` | 0 |
| `D3DRS_FOGENABLE` | **0** |
| `D3DTSS_COLOROP(0)` | 4 = MODULATE, ARG1=TEXTURE, ARG2=DIFFUSE |
| `D3DTSS_ALPHAOP(0)` | 4 = MODULATE, ARG1=TEXTURE, ARG2=DIFFUSE |
| `D3DSAMP_MINFILTER/MAGFILTER/MIPFILTER` | 2/2/2 (trilinear) |
| `D3DTSS_TEXTURETRANSFORMFLAGS(0)` | 0 |
| `D3DTSS_COLOROP(1)` / `ALPHAOP(1)` | 1 / 1 (DISABLE) |

### 1.5 `SetRenderStateBlock(2)` (`:1760-1791`)

Used by `TMFont2.cpp:302`, `TMFont3.cpp:208,210`, and the 3-D item preview (`RenderDevice.cpp:2289, 2369,
2449`). Differences vs block 3: `ALPHABLENDENABLE=1`, `ALPHAREF=8`, `D3DRS_FILLMODE=3`,
`D3DSAMP_MINFILTER/MAGFILTER=1/1` (**POINT** sampling — that is why the default bitmap font looks crisp),
`D3DSAMP_MIPFILTER=1`, stage-0 `COLORARG2=DIFFUSE`, `ALPHAARG2=DIFFUSE` (i.e. MODULATE by vertex colour).

`TMFont3.cpp:208-210` calls block 2 and then immediately block 1 — the block-2 settings survive only for the
duration of the font setup code in between.

---

## 2. State caching arrays

`[OBSERVED]` `RenderDevice.cpp:674-717` — every state goes through an inline cache:

```cpp
void RenderDevice::SetRenderState(D3DRENDERSTATETYPE s, DWORD v) {
    if ((int)s >= 0 && (int)s < 256 && m_dwRenderStateList[s] != v) {
        m_dwRenderStateList[s] = v;
        m_pd3dDevice->SetRenderState(s, v);
    }
}
void RenderDevice::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD v) {
    if (stage >= 8) return;
    if (t >= 0 && t < 29 && m_dwTextureStageStateList[stage][t] != v) { …; m_pd3dDevice->SetTextureStageState(…); }
}
void RenderDevice::SetTexture(DWORD stage, IDirect3DBaseTexture9* p) { … m_pTexture[stage] != p … }
void RenderDevice::SetSamplerState(DWORD stage, D3DSAMPLERSTATETYPE s, DWORD v) {
    if (stage >= 8) return;
    if (s >= 0 && s < 14 && m_dwSamplerStateList[stage][s] != v) { …; m_pd3dDevice->SetSamplerState(…); }
}
```

| Array | Size | Init | Notes |
|---|---|---|---|
| `m_dwRenderStateList[256]` | 256 | `-1` (0xFFFFFFFF) in both ctor `:71` and `InitializeRenderingState` `:1580-1585` | `D3DRENDERSTATETYPE` max is `D3DRS_BLENDOPALPHA`=207, so 256 covers all |
| `m_dwTextureStageStateList[8][29]` | 8×29 | ctor: **0** (`:80`); `InitializeRenderingState`: `-1` (`:1595-1599`) | `D3DTEXTURESTAGESTATETYPE` has 33 values; **`type >= 29` is silently dropped** (`D3DTSS_CONSTANT`=32 would be lost — never used) |
| `m_dwSamplerStateList[8][14]` | 8×14 | `-1` (`:86`, `:1591`) | exactly matches `D3DSAMPLERSTATETYPE` 0..13 |
| `m_pTexture[8]` | 8 | `nullptr` (`:81`) | pure caching of the bound texture, not of sampler state |

**What breaks if you ignore the cache:** nothing visually. The cache exists only to save D3D9 API calls
(state changes were expensive on 2007-era drivers). Two real caveats if you reimplement it:

1. **The caches make the D3D9 calls idempotent, so the *first* call after a device reset must not be cached.**
   `InitializeRenderingState()` sets everything to `-1` precisely so the first `SetRenderStateBlock()` re-emits
   everything. If you port the cache, you must invalidate it on `Reset()` (`Reset3DEnvironment` does call
   `RestoreDeviceObjects()` → `InitializeRenderingState()` `[OBSERVED]` `D3DDevice.cpp:770-800` +
   `RenderDevice.cpp:614`).
2. **A stale cache hides missing state.** Because `m_dwRenderStateList` is also the *only* record of state,
   bugs like "block 1 was skipped this frame" do not show up as missing D3D calls; they show up as
   *wrong* values in the log (§10). Useful side effect: this array is a ready-made per-frame state snapshot.
3. The `-1` sentinel means a legitimate state value of `0xFFFFFFFF` would be re-issued every frame.
   `D3DRS_ALPHAREF` is set to `0xFF000000`, not `0xFFFFFFFF`, so this never happens.

Ctor note: `m_dwTextureStageStateList` is initialised to **0** (`:80`) while the other two are `-1`; then
`InitializeRenderingState()` sets all three to `-1`. Since `Initialize()` calls `InitializeRenderingState()`
before the first frame, the 0-initialisation is dead.

---

## 3. Lighting

### 3.1 The two fixed-function lights

`[OBSERVED]` ctor `RenderDevice.cpp:89-111`:

```cpp
for i in 0..1:
    memset(m_light[i], 0);
    m_light[i].Type    = D3DLIGHT_DIRECTIONAL;
    m_light[i].Ambient = (0,0,0,0);                     // alpha also 0 after memset
m_light[0].Direction = normalize(-10, 10, -6)  = (-0.6511, 0.6511, -0.3907)
m_light[1].Direction = normalize(10, -14, 6)   = ( 0.5488,-0.7683,  0.3293)
```

`[OBSERVED]` `SetRenderStateBlock(1)` `:1634-1667` **overwrites** every frame:

```cpp
m_light[0].Diffuse = m_light[0].Specular = m_colorLight;        // default (1,1,1,1)
m_light[1].Diffuse = m_light[1].Specular = m_colorBackLight;    // default (0.69,0.69,0.69,1)
if (current scene is FIELD && ((TMFieldScene*)scene)->m_bShowBoss == 1)
    m_light[0].Diffuse = m_light[0].Specular = (0,0,0,0);       // and m_bShowBoss = 0 afterwards
m_light[0].Direction = (1, -1, -1);                             // NOT normalised
m_light[1].Direction = (0,  0,  1);
for i in 0..1: SetLight(i, &m_light[i]); LightEnable(i, 1);
```

`D3DLIGHT9` fields actually used: `Type`, `Diffuse`, `Specular`, `Ambient` (always 0), `Direction`.
`Position`, `Range`, `Falloff`, `Attenuation0/1/2` are 0 and irrelevant for a directional light.

`RenderDevice::SetLight()` (`:452-462`) does the same `SetLight`+`LightEnable` loop but is **never called**
(`[OBSERVED]` grep: definition only). Dead method.

> ⚠ **Flag.** `Direction = (1,-1,-1)` is not normalised although `D3DLIGHT9.Direction` is supposed to be a
> unit vector; D3D9 divides by `|D|` in practice on most drivers, but the *documented* behaviour is
> undefined. A GL/Vulkan reimplementation must decide: normalise (physically sane) or use as-is (bug-compat).
> Ditto `m_light[1].Direction = (0,0,1)` which happens to be unit length.

### 3.2 The colour animation (`m_colorLight`, `m_colorBackLight`)

Both are `D3DCOLORVALUE` members of `RenderDevice` (defaults 1,1,1,1 and 0.69,0.69,0.69,1 —
`RenderDevice.cpp:130-135`). They are re-assigned by `TMSky::FrameMove` per weather/night state
(`[OBSERVED]` `TMSky.cpp:264-513`), e.g.

| Sky state | `m_colorLight` | `m_colorBackLight` |
|---|---|---|
| sunset/night (`:264`) | (1.0, 0.8, 0.8, 1) | (0.3, 0, 0, –) |
| night (`:285`) | (1,1,1,1) | (0.6,0.6,0.6) |
| (`:298`) | (0.89,0.89,0.89) | (0,0,0) |
| (`:311`) | (1,1,1) | (0.4,0.4,0.4) |
| (`:338`) | (0.8,0.8,0.8) | (0.6,0.6,0.6) |
| (`:350`) | (1,1,1) | (0.2,0.3,0.2) |
| generic (`:361`/`:365`) | (1,1,1,1) | (0.4,0.4,0.4) |
| interpolated (`:483-491`) | lerp of `m_LightVal[]` | – |

with the 4-entry table `[OBSERVED]` `TMSky.cpp:13-19`:

```cpp
D3DCOLORVALUE TMSky::m_LightVal[4] = {
  { 0.70, 0.70, 0.70, 0.1 },   // day
  { 0.30, 0.30, 0.30, 1.0 },   // night
  { 0.50, 0.40, 0.20, 1.0 },   // dusk
  { 0.26, 0.34, 0.34, 1.0 }    // dawn
};
```

(Source comment on that table: `// NOTE: this is clearly not float values.` — a decompiler warning to ignore;
the values are valid floats, the *alpha* values 0.1 and 1.0 are what look odd.)

> ⚠ **Flag (real bug).** The `RenderDevice` constructor sets only `m_colorLight.r/g/b` and
> `m_colorBackLight.r/g/b` (`:130-135`); **both `.a` members are left uninitialised** (the object is created
> with `new RenderDevice(...)` and there is no covering `memset`). `m_colorLight.a` is later assigned by a few
> `TMSky` states only (`TMSky.cpp:280, 293, 306, 314, 346, 353, 364, 401-410`). Because block 1 copies the
> whole `D3DCOLORVALUE` into `D3DLIGHT9.Diffuse/Specular`, the lights' alpha is indeterminate until the sky
> assigns it — harmless for D3D8/9 lighting (alpha is unused) but a landmine for a port that packs light
> colour into a `vec4`. Initialise it to 1.0 in any reimplementation.

These colours feed the **fixed-function** lighting (`D3DLIGHT9`) and are also used as multipliers by many
`TMEffect*`/`TMObject` classes when they synthesise a `D3DMATERIAL9.Emissive`
(e.g. `TMObject.cpp:142-144`: `Emissive = m_colorBackLight*0.3 + result*0.4`;
`TMTree.cpp:107-109`: `Emissive = m_colorBackLight*0.4 + color*0.4`).

`m_colNormalAmbient` (declared `RenderDevice.h:105`) is **never written or read** — dead member `[OBSERVED]`.

### 3.3 Dynamic point lights

`TMLight` (`TMLight.cpp`) creates `D3DLIGHT_POINT` lights starting at index **2**
(`TMLight::m_dwBaseLightIndex = 2`), with `Attenuation0 = Attenuation1 = Attenuation2 = 0.001`,
`Range = 5.0` (`7.0` for index 6 in the select-char scene), `Ambient = Specular = Diffuse = colour`,
and enables/disables them with `LightEnable(index, …)` (`TMLight.cpp:41, 77, 81`).
They are owned by `TMObjectContainer` (`TMObjectContainer.cpp:685: new TMLight(colorValue, 1)`) and by
`TMHuman`'s `m_pLightContainer` (`TMHuman.cpp:6265-6281`, used to brighten characters standing in light).
`[OBSERVED]`

No cap check against `D3DCAPS9.MaxActiveLights` — with more than 6 simultaneous lights (indices 2..7) D3D9
silently drops the extra ones. `[INFERRED]`

### 3.4 Vertex-shader light direction (`SetVertexShaderConstantF(1, …)`)

`[OBSERVED]` `RenderDevice.cpp:1669-1689`:

```cpp
TMCamera* pCamera = g_pObjectManager->m_pCamera;
if (RenderDevice::m_bDungeon != 0 && RenderDevice::m_bDungeon != 4)
    vLightDir = ( pCamera->m_vecCamDir.x - m_light[0].Direction.x,
                  0.5f,
                 (pCamera->m_vecCamDir.z + m_light[0].Direction.z) - 0.3f,
                  0.0f );
else
    vLightDir = ( -m_light[0].Direction.x, 0.5f, m_light[0].Direction.z, 0.0f );
m_pd3dDevice->SetVertexShaderConstantF(1, &vLightDir, 1);
```

Note `y` is hard-wired to **0.5**, and with `m_light[0].Direction = (1,-1,-1)` the non-dungeon branch gives
`(-1, 0.5, -1)` — **not normalised**.

**This value is then overwritten for every skinned mesh**: `CMesh.cpp:633-635`

```cpp
D3DXVECTOR3 vecDir(-1.0f, 1.0f, 1.0f);
D3DXVec3Normalize(&vecDir, &vecDir);          // = (-0.57735, 0.57735, 0.57735)
g_pDevice->m_pd3dDevice->SetVertexShaderConstantF(1, (const float*)&vecDir, 1);
```

So the *effective* VS light direction is the constant `(-0.5774, 0.5774, 0.5774, 0)` — pointing **down** and
toward −x/−z if you consider it a direction *toward* the light, or up/+x/+z as a "light travel direction"
(the shaders use `dp3 N·L` and `lit`, so it is the direction *toward* the light).
`RenderDevice`'s camera-relative computation therefore only affects the fixed-function path.

### 3.5 Brightness: `m_nBright`, `m_bBright`, `CMesh::Render(int)`

| Symbol | Where | Range / meaning |
|---|---|---|
| `RenderDevice::m_nBright` | static, `RenderDevice.cpp:18`, set from `Config.bright` (`NewApp.cpp:220-242`, clamped 0..100) | **gamma-ramp brightness only** (§9) + a UI text colour tweak (`TMFont2.cpp:37-39`, `SControl.cpp:788-792, 1683-1687`: if `> 58`, subtract `m_nBright - 40` from R,G,B per channel) |
| `m_bBright` | **does not exist** in this codebase | – |
| `CMesh::Render(int nBright)` | `CMesh.cpp:36`, `CFrame.cpp:206/224/235` | a **boolean "skip material upload"** flag, *not* a brightness (§3 of `05-shaders.md` §7.4). `0` → `SetMaterial(cAlpha)` runs and uploads `c0/c6/c7/c8`; non-zero → the previous mesh's constants are reused |
| `bBright` (effect particle argument) | `TMEffectParticle.cpp:34` | `m_efAlphaType = (EEFFECT_ALPHATYPE)(bBright == 1)` → selects additive vs alpha blending (§5) |

Exact gamma formula (§9.3) for the *only* real brightness control:

```
ramp[i] = min(65535, (int)((m_nBright * 0.02f * i) * 256.0f))     for i = 0..255
```

---

## 4. Fog

### 4.1 Flags and parameters

| Symbol | Default | Set by |
|---|---|---|
| `m_bFog` | 1 (`RenderDevice.cpp:59`) | `TMSky` sets 0 for some states (`TMSky.cpp:391`) |
| `m_fFogStart` | 60.0 (`:56`) | `TMSky::FrameMove` per weather (`FogList`, `TMSky.cpp:21-38`) |
| `m_fFogEnd` | 156.0 (`:57`) | idem |
| `g_pObjectManager->m_pCamera->m_fSightLength` | per scene: 7.0 / 11.0 / 15.5 / `m_fMaxCamLen` (`TMCamera.cpp:441-466`, `TMScene.cpp:2296`, `TMFieldScene.cpp:14856-14909`) | zoom/mouse-wheel (`EventTranslator.cpp:313-347`) |

`TMSky::FogList[16][2]` `[OBSERVED]` (`TMSky.cpp:21-38`) — start/end per sky state:

```
{14,34} {25,34} {55,70} {25,34} {14,23} {14,22} {16,34} {16,34} {39,70} {19,30} {19,30} {19,70} {29,40} {0,0} {0,0} {0,0}
```

and the demo/underwater overrides: `m_fFogStart/End = 19/30` for `ESCENE_DEMO` (`TMSky.cpp:55-56`),
`1/10` when `m_pCamera->m_bInWater` (`TMSky.cpp:503-504`). After selection, `+5.0` is added to both
(`TMSky.cpp:443-444`), and `m_fFogEnd += sightLength - 10` in one path (`TMSky.cpp:582-583`).

### 4.2 The state values

`[OBSERVED]` `SetRenderStateBlock(1)`:

```cpp
SetRenderState(D3DRS_FOGENABLE,     m_bFog);
SetRenderState(D3DRS_RANGEFOGENABLE, 0);
SetRenderState(D3DRS_FOGCOLOR,      m_dwClearColor);      // NOT m_dwActualClearColor
SetRenderState(D3DRS_FOGVERTEXMODE, 3 /* D3DFOG_LINEAR */);
float fstart = (m_fFogStart + camera->m_fSightLength) - 8.0f;
float fend   = (m_fFogEnd   + camera->m_fSightLength) - 15.0f;
SetRenderState(D3DRS_FOGSTART, *(DWORD*)&fstart);   // float bits into a DWORD state
SetRenderState(D3DRS_FOGEND,   *(DWORD*)&fend);
```

So the **actual** fog range is `depth ∈ [fFogStart + sightLength − 8, fFogEnd + sightLength − 15]`, i.e. the
fog window moves with the zoom: as `sightLength` grows the fog recedes. Because the offsets are −8 and −15
the window *widens* by 7 units as well.

`D3DRS_FOGVERTEXMODE = D3DFOG_LINEAR` (not `D3DFOG_NONE`, not table fog) means D3D computes, **per vertex**:

```
f = (fogEnd - |z_view|) / (fogEnd - fogStart)      clamped to [0,1]
color = (1 - f) * fogColor + f * litColor           // f = 1 → no fog
```

`D3DRS_RANGEFOGENABLE = 0` ⇒ the distance is **|z|** in view space, not the radial distance. This matters on
wide FOV: with `m_fFOVY = 0.25` (0.25 × 180° = 45° → `D3DXToRadian(180)*0.25` = 0.785 rad = 45° vertical FOV,
`RenderDevice.cpp:29`, `:1841-1845`) the difference is small but visible at the screen corners.

### 4.3 Interactions — sky, UI, effects, VS-fog

| Pass | Fogged? | Evidence |
|---|---|---|
| World geometry (`TMMesh` FVF path, ground) | **yes** | `SetRenderStateBlock(1)`; ground re-asserts `FOGVERTEXMODE=3` per draw (`TMGround.cpp:2723-2724`, `TMMesh.cpp:66`) |
| Sky dome/plane | **no** | `TMSky.cpp:170` `SetRenderState(D3DRS_FOGENABLE, 0)` before the sky draw, restored to `m_bFog` at `:199`, and `SetRenderStateBlock(0)`/`(3)` also disable it |
| UI (controls, text, windows) | **no** | blocks 0/2/3 all set `FOGENABLE=0` |
| Billboards / particles / effects | **no** | `TMEffectBillBoard.cpp:145` `FOGENABLE=0` around the draw (restored after) |
| Skinned meshes (vertex shader) | **via `oFog`** — `D3DRS_FOGENABLE` is bypassed, the VS writes the fog factor itself, using **different inputs** than the FF path (see §4.4) | `05-shaders.md` §8 |

### 4.4 The two competing fog formulas (must both be reproduced)

```
Fixed-function (world/ground/objects):
    f = clamp((fFogEnd + sight - 15 - |z_view|) / ((fFogEnd + sight - 15) - (fFogStart + sight - 8)), 0, 1)

Vertex shader skinmesh5..8 (c6 = (1, fogEnd, 1/(fogEnd-fogStart), 0), uploaded in CMesh::SetMaterial):
    fogEnd  = m_fFogEnd * (boneAniIndex == 61 ? 1.4 : 1.0)
    f = clamp((fogEnd - z_view) * (1/(fogEnd - m_fFogStart)), 0, 1)   then min(f, 1.0)

Vertex shader skinmesh1..4:  see 05-shaders.md §7.2 — they read c95, which is never uploaded (port defect).
```

Always `color = lerp(fogColor, color, f)` with `fogColor = D3DRGB(m_dwClearColor)`.

---

## 5. Alpha / blending model

### 5.1 Texture alpha classification (`cAlpha`)

Every texture entry carries a `char cAlpha` (default `'N'` = 78, `TextureManager.cpp:27, 38, 46, 54`).
`'N'` / `'A'` / `'a'` / `'C'` decide the **texture format** as well as the blend mode
`[OBSERVED]` `TextureManager.cpp:278-360`:

| `cAlpha` | Texture format chosen | Meaning `[INFERRED]` |
|---|---|---|
| `'N'` | `X1R5G5B5` on 16-bit/Voodoo, else **`X8R8G8B8`** (`TextureManager.cpp:349-382`) | no alpha channel at all — colour-key transparency |
| `'A'` | `A8R8G8B8` (or `A4R4G4B4` on 16-bit/Voodoo) | alpha channel — normal alpha blending |
| `'a'` | `A1R5G5B5` (or `A4R4G4B4`) | 1-bit alpha |
| `'C'` | `A8R8G8B8` (`A4R4G4B4`) | alpha **used as a colour mask** → additive (`MODULATEALPHA_ADDCOLOR`) |

The mesh renderer reacts to it: `CMesh.cpp:47-50` disables `ALPHATESTENABLE` for `'C'`;
`CMesh.cpp:458-469, 499-510` switch stage 1 to `D3DTOP_MODULATEALPHA_ADDCOLOR` (18) for `'C'`, and to
`D3DTOP_SELECTARG1`/`ADDSMOOTH` otherwise.

### 5.2 `m_bTrans` and the negative `nTextureSetIndex` convention (UI)

`[OBSERVED]` `RenderDevice.cpp:3230-3246`:

```cpp
if (ipControl->nTextureSetIndex >= 0)  { ... draw textured quad ... }
else {
    int bTrans = (ipControl->nTextureSetIndex == -2) ? 1 : 0;
    RenderRectNoTex(..., dwColor, bTrans);
}
```

* `nTextureSetIndex >= 0` → normal textured control.
* `nTextureSetIndex == -1` → **untextured opaque** rectangle of `dwColor`.
* `nTextureSetIndex == -2` → **untextured alpha-blended** rectangle ("glow"/fade), implemented in
  `RenderRectNoTex` (`:1971-2041`) which, for `bTrans == 1`, sets
  `ALPHAOP=4 (MODULATE)`, `ALPHABLENDENABLE=1`, `SRCBLEND=5 (SRCALPHA)`, `DESTBLEND=6 (INVSRCALPHA)`,
  `ALPHAFUNC=8 (ALWAYS)`, `ALPHATESTENABLE=0`, `ZWRITEENABLE=0`; for `bTrans == 0` it sets
  `ALPHABLENDENABLE=0` only (i.e. **it does not restore Z-write** — see the caveat below).
  `RenderRectNoTex` also re-sets `D3DRS_ALPHAREF` before *and* after the draw (`:1987-2001`, `:2026-2040`).

> ⚠ **Flag.** `RenderRectNoTex(bTrans=1)` sets `ZWRITEENABLE=0` and never restores it; the next
> `SetRenderStateBlock(0/1/2/3)` does not set `ZWRITEENABLE` either (block 0/2/3 don't touch it, block 1 does).
> Anything drawn between a transparent UI quad and the next block-1 call therefore has depth writes off.
> Reproduce this order-dependence, or fix it deliberately.

### 5.3 Per-effect blending table

`EEFFECT_ALPHATYPE` (`TMEffect.h:6-12`): `EF_DEFAULT = 0`, `EF_BRIGHT = 1`, `EF_NONEBRIGHT = 2`,
`EF_ALPHA = 3`.

The canonical effect state block (`[OBSERVED]` `TMEffectBillBoard.cpp:133-169`, and near-identical in
`TMEffectBillBoard2/3/4.cpp`, `TMEffectMesh.cpp`, `TMEffectSWSwing.cpp`, `TMShade.cpp`):

| State | `EF_BRIGHT` (additive) | everything else (alpha) |
|---|---|---|
| `D3DRS_ALPHABLENDENABLE` | 1 | 1 |
| `D3DRS_SRCBLEND` | 5 = SRCALPHA | 5 = SRCALPHA |
| `D3DRS_DESTBLEND` | **2 = ONE** | **6 = INVSRCALPHA** |
| `D3DTSS_ALPHAOP(0)` | 2 = SELECTARG1 | **4 = MODULATE** |
| `D3DRS_ALPHAFUNC` | 8 = ALWAYS | 8 = ALWAYS |
| `D3DRS_ALPHATESTENABLE` | 0 | 0 |
| `D3DRS_ZWRITEENABLE` | **0** | **0** |
| `D3DRS_ZENABLE` | (unchanged — still 1) | (unchanged) |
| `D3DRS_FOGENABLE` | 0 | 0 |
| `D3DRS_LIGHTING` | 0 | 0 |
| `D3DRS_CULLMODE` | **1 = NONE** (two-sided billboard) | 1 |
| FVF / stride | `SetFVF(322)`, 24 bytes | idem |
| transform | `SetTransform(256 /* D3DTS_WORLD */, &m_matEffect)` per billboard | idem |

and the restore after the draw (`:157-166`): `CULLMODE=3`, `LIGHTING=1`, `SRCBLEND=2` (ONE),
`ALPHAFUNC=7`, `ALPHATESTENABLE=1`, `ZWRITEENABLE=1`, plus `DESTBLEND=6` (BRIGHT) or `ALPHAOP=2` (others).

`TMEffectParticle.cpp:34` sets the alpha type **from a constructor argument**:
`m_efAlphaType = (EEFFECT_ALPHATYPE)(bBright == 1)` — i.e. `bBright ? EF_BRIGHT : EF_DEFAULT`.
`[OBSERVED]` So `TMEffectParticle(pos, …, bBright=1, …)` renders additively.

### 5.4 Other blend combinations present in the codebase

From the exhaustive state inventory `[OBSERVED]` (grouped over all `.cpp`):

| `SRCBLEND` | `DESTBLEND` | Where / meaning |
|---|---|---|
| 2 = ONE | 6 = INVSRCALPHA | block 1 (world), `m_cDefaultAlpha == 1` character pass (`CFrame.cpp:228-229` sets ONE/INVSRCALPHA explicitly) |
| 5 = SRCALPHA | 6 = INVSRCALPHA | standard alpha: UI transparent quads, most effects' non-bright mode, `CMesh.cpp:726-727` (mouse-over highlight) |
| 5 = SRCALPHA | 2 = ONE | **additive** (`EF_BRIGHT`) |
| 5 = SRCALPHA | 7 = DESTALPHA | block 0 default (unused because blending is off) |
| 3 = SRCCOLOR | 6 = INVSRCALPHA, 3 = SRCCOLOR / 4 = INVSRCOLOR | water/sea (`TMSea.cpp`), sky tint |
| 2 = ONE | 2 = ONE | rare "pure add" (level-up flashes) |

`D3DRS_ALPHAREF` values actually used: `0xDD` (=221, default), `0xAA` (=170, `TMObject.cpp:264`),
`0xFF` (=255, 16-bit colorkey path), `8` (blocks 2/3), and the buggy `0xFF000000` / `0xF000` for
`m_iVGAID == 1` (`RenderDevice.cpp:1694-1696, 1990-1992, 2029-2031`).

> ⚠ **Flag.** `D3DRS_ALPHAREF` is an 8-bit quantity. `0xFF000000` truncates to `0x00` on most drivers and
> `0xF000` truncates to `0x00` as well (only the low 8 bits are used), i.e. on `m_iVGAID == 1` hardware the
> alpha test effectively becomes "alpha >= 0" = always pass. `[INFERRED]` Never reconstructed; it only
> triggers when the adapter description string contains "MX" (→ `m_iVGAID = 0`) or doesn't match
> NVIDIA/RADEON (`m_iVGAID = 1`), `D3DDevice.cpp:555-562`.

---

## 6. Depth and stencil

### 6.1 Depth buffer format

`[OBSERVED]` `D3DDevice.cpp:256-321` (`BuildPresentParamsFromSettings`):

```cpp
m_d3dpp.Windowed               = m_d3dSettings.IsWindowed;
m_d3dpp.BackBufferCount        = 1;
m_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
m_d3dpp.EnableAutoDepthStencil = m_d3dEnumeration.AppUsesDepthBuffer;   // = 1 (RenderDevice ctor :45)
m_d3dpp.hDeviceWindow          = m_hWnd;
if (!g_nAntialias) m_d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
m_d3dpp.AutoDepthStencilFormat = m_d3dSettings.DepthStencilBufferFormat();
```

and the format comes from `CD3DEnumeration::BuildDepthStencilFormatList`
(`D3DEnumeration.cpp:535-569`) which walks, **in this order**,
`D3DFMT_D16, D3DFMT_D15S1, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D32`
and the caller picks element **0** (`D3DDevice.cpp:384`, `:504`). ⇒

**The depth buffer is `D3DFMT_D16` (16-bit, no stencil) on essentially every device.**

### 6.2 Is stencil used?

**No.** `[OBSERVED]`:
* the depth/stencil surface has 0 stencil bits (§6.1);
* the *only* stencil calls in the entire codebase are
  `SetRenderState(D3DRS_STENCILENABLE, 0)` at `RenderDevice.cpp:1771` and `:1803` (blocks 2 and 3);
* no `D3DRS_STENCILFUNC`, `STENCILREF`, `STENCILMASK`, `STENCILWRITEMASK`, `STENCILFAIL/ZFAIL/PASS`,
  `TWO_SIDED_STENCIL` anywhere (`[OBSERVED]` exhaustive grep).

⇒ In GL: `glDisable(GL_STENCIL_TEST)`; in Vulkan: `depthStencilState.stencilTestEnable = VK_FALSE`
(stencil attachment still required by the format, but `VK_FORMAT_D16_UNORM` has none).

### 6.3 Z-write / Z-test per pass

| Pass | `ZENABLE` | `ZWRITEENABLE` | `ZFUNC` |
|---|---|---|---|
| default after device creation | 1 | 1 | 4 (LESSEQUAL) |
| block 0 (UI) | 1 | (unchanged → 1) | (4) |
| block 1 (world) | 1 | **1** | **4** |
| block 2 (font/preview) | **0** | (unchanged) | – |
| block 3 (UI overlay) | **0** | (unchanged) | – |
| 3-D char/item preview inside UI | 1 | 1 | **8 = ALWAYS** (`RenderDevice.cpp:3324`, `:3372`), restored to 4 at `:3367`, `:3405` |
| billboards / particles | 1 | **0** | 4 |
| `RenderRectNoTex(bTrans=1)` | – | **0** (not restored!) | – |
| `CMesh` mouse-over overlay | – | (unchanged) | – |

UI quads are `D3DFVF_XYZRHW` (`322`/`324`) with `z = 0.01f` and `w = 1.0f` for the vertex-0 template
(`RenderDevice.cpp:208-282`: every `m_CtrlVertex[i].position` starts at `(0,0,0.01)` and
`rhw = 1.0f`), so even the "`ZENABLE=1`" path in block 0 puts UI at the far plane of the ortho-ish projection
set by `SetMatrixForUI` (`:1856-1872`: fov 0.1, aspect × 0.94, near 10, far 100, camera at z=+50).

### 6.4 Shadow handling (`m_nShadowTextureSize`, `m_nOldShadowX/Y`, `TMLight`, `TMGround`)

`[OBSERVED]`:

* `m_nShadowTextureSize` is **written only** (128 when `ps_1_1` exists, 64 otherwise — `RenderDevice.cpp:534,
  539, 1568`) and **never read**. There is **no** render-to-texture shadow map, no `SetRenderTarget` to a
  shadow surface, no projected shadow texture pass anywhere (`grep SetRenderTarget` → only the save/restore of
  `m_pOldRenderTarget` in `RestoreDeviceObjects`).
* `m_nOldShadowX` / `m_nOldShadowY` do not exist in this codebase (`[OBSERVED]` grep).
* `TMGround.cpp` contains **no** shadow code (`[OBSERVED]` grep for `hadow` → 0 hits).
* Character shadows are ordinary **flat billboard "blob" effects**: `TMHuman` creates `m_pShadow` as a
  `TMShade`/`TMEffectBillBoard` with `m_efAlphaType = EF_BRIGHT` (`TMHuman.cpp:9788`) and
  `TMShade` renders a dark, alpha-blended quad on the ground (`TMShade.cpp`, `m_efAlphaType == EF_DEFAULT`
  by default and `EF_BRIGHT` for the coloured variants). `TMLight` is a *light*, not a shadow.
* `TMObjectContainer.cpp` / `TMHouse.cpp` use `TMEffectBillBoard(… nTextureIndex …)` for light pools on the
  ground, not shadows.

⇒ A port needs **no** shadow-map infrastructure to match 748; it needs a textured blob quad.

---

## 7. Culling, fill mode, multisampling, depth hacks

### 7.1 Culling

| Value | Where | Effect |
|---|---|---|
| 3 = CCW | block 1 default, restored after most special cases (`CMesh.cpp:656`, `TMObject.cpp:745`, `TMEffectBillBoard.cpp:157`) | cull CCW = keep CW-front-facing triangles |
| 1 = NONE | billboards/particles (`TMEffectBillBoard.cpp:151`), `CMesh.cpp:647-648` (skinmesh draw is **two-sided**: `SetRenderState(CULLMODE, NONE)` twice in a row, so it is effectively unconditional) | |
| 2 = CW | weapon/equip parts (`CMesh.cpp:582`), `TMObject.cpp:304` for `nAlpha == 2`, `RenderDevice.cpp:304` in `Lock`-adjacent code paths | cull CW = keep CCW |

`[OBSERVED]` **Skinned meshes are always drawn two-sided** (`CMesh.cpp:647-648`) — the `if (m_pParentSkin->
m_nMeshType == 1) CULLMODE=CW` at `:642-645` is immediately overwritten. So the port never culls characters.

### 7.2 Fill mode

`D3DRS_FILLMODE`: 3 = SOLID everywhere (`RenderDevice.cpp:1769, 1801`, `TMGround.cpp:3266`) except
`TMGround.cpp:2719` where `if (m_bWire == 1) SetRenderState(D3DRS_FILLMODE, 2)` — a **debug wireframe** of
the terrain (restored to 3 at `:3266`). `m_bWire` is a debug flag; keep it behind a debug switch.

### 7.3 Multisampling / AA

`[OBSERVED]`
* `g_nAntialias` (config) drives `BuildPresentParamsFromSettings`'s loop over
  `CheckDeviceMultiSampleType` (`D3DDevice.cpp:301-320`): it picks the **highest** `D3DMULTISAMPLE_TYPE` in
  `1..g_nAntialias` for which both the back-buffer and depth formats are supported, and stores
  `MultiSampleQuality = qualityLevel - 1`.
* MSAA is turned **off** for UI: `D3DRS_MULTISAMPLEANTIALIAS = 0` in blocks 2 and 3.
* `g_nAntialias` is forced to 0 when `m_iVGAID == 2` (RADEON) **and** `g_nUseBlur > 0`
  (`D3DDevice.cpp:264-265`) — a workaround for a blur+MSAA conflict.
* `D3DRS_ANTIALIASEDLINEENABLE = 0` (blocks 2/3) → no line AA.

### 7.4 Depth hacks

`[OBSERVED]` there is **no** `D3DRS_ZBIAS`, `D3DRS_SLOPESCALEDEPTHBIAS`, `D3DRS_DEPTHBIAS` or
`D3DRS_POINTSIZE` call anywhere. Depth fighting between decals and ground is avoided by:
* `D3DFVF_XYZRHW` + z = 0.01 for 2-D,
* the near/far planes (`g_ClipNear * 1.4f` .. `g_ClipFar`, `RenderDevice.cpp:1841-1845`),
* `SetRenderStateBlock(1)`'s `ZFUNC=LESSEQUAL`,
* z-offset in the *geometry itself* (`m_fHeight + 1.0f` for the sky, `TMGround` height map +0.1 scale).
If you want coplanar decals to behave identically you must keep `LESS_OR_EQUAL` — switching to `LESS` will
make repeated geometry (hands, overlays, the same mesh drawn twice for the "god glow") z-fight.

---

## 8. Swap chain, clear colour, device loss

### 8.1 Present parameters (`D3DDevice.cpp:256-321`)

| Field | Windowed | Fullscreen |
|---|---|---|
| `Windowed` | `TRUE` | `FALSE` |
| `BackBufferWidth/Height` | client rect | `DisplayMode().Width/Height` |
| `BackBufferFormat` | `PDeviceCombo()->BackBufferFormat` (== desktop format) | chosen combo |
| `BackBufferCount` | **1** | 1 |
| `SwapEffect` | `D3DSWAPEFFECT_DISCARD` | same |
| `EnableAutoDepthStencil` | `TRUE` | TRUE |
| `AutoDepthStencilFormat` | `D3DFMT_D16` (first in list) | same |
| `MultiSampleType / Quality` | from AA loop | same |
| `FullScreen_RefreshRateInHz` | 0 | `Fullscreen_DisplayMode.RefreshRate` |
| `PresentationInterval` | **0** (`D3DPRESENT_INTERVAL_IMMEDIATE`, no vsync) | `m_d3dSettings.PresentInterval()` (chosen from the enumerated list) |
| `Flags` | `D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL` and, when `!g_nAntialias`, `| D3DPRESENTFLAG_LOCKABLE_BACKBUFFER` | idem |

No `D3DPRESENTFLAG_DEVICECLIP`, no `D3DPRESENTFLAG_VIDEO`, no `D3DSWAPEFFECT_COPY`.

### 8.2 Present

`[OBSERVED]` `RenderDevice.cpp:440-448`:

```cpp
if (FAILED(m_pd3dDevice->EndScene())) return 0;
if (bEnd == 1) {
    if (m_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST)
        m_bDeviceLost = 1;
}
```

`Present` with all-null arguments ⇒ no dirty rect, no vsync override, presents the whole back buffer.
`bEnd == 1` only on the final frame of a frame (the AVI/loading paths call `Unlock(0)` so they never present).

### 8.3 Clear

`[OBSERVED]` `RenderDevice.cpp:385-403`:

```cpp
if (bClear) {
    if (g_pCurrentScene && g_pCurrentScene->m_pSky) {
        if (m_bDungeon != 0 && m_bDungeon != 3 && m_bDungeon != 4) m_dwActualClearColor = 0;
        else if (sky->m_nState == 0) m_dwActualClearColor = 0xFF335599;
        else if (sky->m_nState == 1) m_dwActualClearColor = 0xFF333333;
        else if (sky->m_nState == 2) m_dwActualClearColor = 0xFF441100;
        else if (sky->m_nState == 3) m_dwActualClearColor = 0xFF222222;
    }
    m_pd3dDevice->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET,
                        m_dwActualClearColor, 1.0f, 0);
}
```

* Clear flags: **colour + depth**, never stencil. `Z = 1.0f`, `Stencil = 0`.
* The colour is a `D3DCOLOR` (ARGB). For states ≥ 4 the value is whatever the *previous* frame/scene left in
  `m_dwActualClearColor` — a latent bug (`SendMessage`-style stale state) worth reproducing only if you are
  chasing an exact pixel match in odd weather states.
* `m_dwClearColor` (used for `D3DRS_FOGCOLOR` and re-copied into `m_dwActualClearColor` by block 1) is
  changed by scenes: `TMSelectCharScene.cpp:113` = `0x444433`, `TMSelectServerScene.cpp:138` = `0`,
  `TMSky.cpp` per state (`:507-513` = the same four values as above), `TMSky.cpp:413` =
  `0xFFFFFFFF` (white flash), `TMSky.cpp:481`/`:606` = computed from an RGB triple:
  `dwB | (dwG << 8) | (dwR << 16)` — note this drops alpha → the clear colour has **A = 0**; harmless for
  `Clear` (D3D ignores alpha for the RT) but it *does* affect `D3DRS_FOGCOLOR` if it is read as ARGB
  (fog uses RGB only). `[OBSERVED]`
* Frame order: `NewApp::RenderScene` calls `Lock(1)` (→ Clear) **before** `SetRenderStateBlock(1)`, so the
  clear on frame *n* uses the value computed by `Lock` (sky state) — the `m_dwActualClearColor = m_dwClearColor`
  inside block 1 (`:1726`) only matters for the *next* frame or for a `Lock(0)` invocation.

### 8.4 Device loss / reset

`[OBSERVED]`

```
Lock(bClear):
  if (m_bDeviceLost) {
      hr = TestCooperativeLevel();
      if (hr == D3DERR_DEVICELOST)     return 0;             // skip the frame
      if (hr != D3DERR_DEVICENOTRESET) return hr;
      if (m_bWindowed) { re-read adapter display mode; m_d3dpp.BackBufferFormat = that; }
      return Reset3DEnvironment();
  }
```

`Reset3DEnvironment` (`D3DDevice.cpp:770-800`):
`InvalidateDeviceObjects()` → `IDirect3DDevice9::Reset(&m_d3dpp)` → re-read back-buffer desc →
`RestoreDeviceObjects()`.

`RenderDevice::InvalidateDeviceObjects()` (`:628-672`) releases: texture manager textures, mesh-manager
meshes, object-manager device objects, all `m_pVertexShader[8]` + `m_pVertexDeclaration[8]`, all
`m_pVSEffect[4]`, all `m_pPSEffect[6]`, the GDI font bitmap/DC/font, the saved render target and depth
surface, `m_pDXFont`/`m_pDXFontLarge` (`OnLostDevice`), and the D3DX sprite.
It does **not** release `m_pVDEffect[4]` (leak, harmless).
`RenderDevice::RestoreDeviceObjects()` (`:527-626`) re-creates everything **and re-runs
`InitializeRenderingState(); InitVertexShader();`** but *not* `InitPixelShader()` (commented out).

---

## 9. Gamma

`[OBSERVED]` `RenderDevice.cpp:719-748` (`SetGamma`), called once from `RenderDevice::Initialize()` `:284`:

```cpp
if (RenderDevice::m_nBright != 50) {
    static D3DGAMMARAMP gamma;  memset(&gamma, 0, sizeof(gamma));
    m_pd3dDevice->GetGammaRamp(1, &gamma);          // NOTE: index 1, then SetGammaRamp(0, …)
    for (int i = 0; i < 256; i++) {
        int nVal = (int)((((float)m_nBright * 0.02f) * (float)i) * 256.0f);
        if (nVal > 0xFFFF) nVal = 0xFFFF;
        gamma.red[i] = gamma.green[i] = gamma.blue[i] = (WORD)nVal;
    }
    m_pd3dDevice->SetGammaRamp(0, 0, &gamma);
    if (m_d3dpp.Windowed) { HDC hdc = GetDC(hWnd); SetDeviceGammaRamp(hdc, &gamma); ReleaseDC(hWnd, hdc); }
}
```

Exact ramp: **`ramp[i] = min(65535, (int)(m_nBright * 0.02 * i * 256))`**, identical for R/G/B, i.e.

| `m_nBright` | value at i=255 | interpretation |
|---|---|---|
| 50 (default) | `50*0.02*255*256 = 65 280` → **not applied at all** (early-out) | unity |
| 0 | 0 | black screen |
| 25 | 32 640 (≈ 0.498) | half brightness |
| 45 | 58 752 (≈ 0.897) | slightly dark |
| 58 | 75 776 → clamped to 65 535 at i ≥ 250 | near-unity then clipped |
| 100 | 130 560 → clamped to 65 535 from i ≈ 128 | saturating (blown highlights) |

Notes:
* It is a **D3D gamma ramp** (`IDirect3DDevice9::SetGammaRamp`), *plus* — in windowed mode — the Win32
  `SetDeviceGammaRamp` on the desktop DC. It is **not** a LUT applied to the rendered image, so an
  OpenGL/Vulkan port must either use `glSetDeviceGammaRamp`-equivalent platform APIs
  (`SetDeviceGammaRamp` on Win32, `wl_display`/`XRRSetCrtcGamma` on Linux) or apply the same ramp as a
  post-process.
* `GetGammaRamp(1, …)` reads **swap-chain 1** while `SetGammaRamp(0, …)` writes **swap-chain 0** — an
  off-by-one; only the "read" is affected (the ramp is then fully overwritten anyway).
* `m_nBright == 50` means "don't touch the ramp", which is why an unmodified install has no gamma change.
* `Finalize()` (`:289-315`) restores the ramp using the same formula with `m_nBright = 50` (i.e. writes a
  *unity-by-formula* ramp `50*0.02*i*256 = 256*i` = identity), plus `SetDeviceGammaRamp` when windowed.

---

## 10. State-dump debug helpers

Three functions exist, all writing a text file next to the exe (`[OBSERVED]` `RenderDevice.cpp:2779-2831`,
paths in `TMPaths.h:32-34`):

| Function | Output file | Content |
|---|---|---|
| `LogRenderState()` | `RenderStateLog.txt` | 256 lines `Index:<n> Val=<v>` from `m_dwRenderStateList` |
| `LogSamplerState()` | `SamplerStateLog.txt` | 8×14 lines `Stage:<s> State:<t>  Val=<v>` from `m_dwSamplerStateList` |
| `LogTextureStageState()` | `TextureStageStateLog.txt` | 8×29 lines `Stage:<s> State:<t>  Val=<v>` from `m_dwTextureStageStateList` |

They faithfully dump the **cache**, which is also the live state (the cache is authoritative, see §2), so this
is a cheap way to capture the exact state at any point:

* add `g_pDevice->LogRenderState(); g_pDevice->LogSamplerState(); g_pDevice->LogTextureStageState();`
  immediately after the `SetRenderStateBlock(n)` you want to inspect (or inside `TMMesh::Render` right before
  `DrawIndexedPrimitive`) — rebuild, run one frame, read the three files;
* the values are **decimal**; `-1` (`4294967295`) means "never set since the last
  `InitializeRenderingState()`";
* the arrays are public members, so a debugger watch on
  `g_pDevice->m_dwRenderStateList[27]` (`D3DRS_ALPHABLENDENABLE`) is equally quick.

None of the three is called anywhere in the shipping code (`[OBSERVED]`), so they cost nothing until used.

---

## 11. Reimplementation notes (OpenGL / Vulkan)

### 11.1 The state block is the pipeline

The engine has **four** state blocks and every draw belongs to exactly one of them. Map them to four
pipeline objects (Vulkan) or four state-bundle-ish groups (GL):

| Block | Vulkan pipeline deltas | GL state deltas |
|---|---|---|
| 0 (UI opaque) | depth test on/`LEQUAL`/write on, blending off, cull back + `frontFace CW`, fog off, unlit, `SELECTARG1` texture-only | `glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(1); glDisable(GL_BLEND); glEnable(GL_CULL_FACE); glFrontFace(GL_CW);` + texture-only fragment program |
| 1 (world) | depth on/`LEQUAL`/write on, blend `ONE / ONE_MINUS_SRC_ALPHA`, alpha-test `GEQUAL 0.867`, cull back+frontFace CW, fog on, lit, `MODULATE(tex, diffuse)` | `glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(1); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_CULL_FACE); glFrontFace(GL_CW); glCullFace(GL_BACK);` |
| 2 (font/preview) | depth off, blend `SRCALPHA / INVSRCALPHA`, alpha-test `GEQUAL 8/255`, point-filtered, `MODULATE` ×2 args | `glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);` + `GL_NEAREST` samplers |
| 3 (UI overlay) | depth off, blending **off**, alpha-test `GEQUAL 8/255`, trilinear, `MODULATE` | `glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);` + `GL_LINEAR_MIPMAP_LINEAR` |

Do **not** unify them: block 2 vs 3 differ only in point-vs-trilinear filtering and blending, and that
difference is what makes the bitmap font crisp.

### 11.2 Fixed-function lighting replacement

```
litColor = ambientGlobal(D3DRS_AMBIENT)                     // 0x33FFFFFF → (1,1,1)
         + matEmissive                                      // D3DMATERIAL9.Emissive
         + matAmbient * lightAmbient[i]                     // lightAmbient = 0 in this engine
         + Σ_i saturate(dot(N, -lightDir_i)) * matDiffuse * lightDiffuse[i]
specular: disabled (D3DRS_SPECULARENABLE = 0)
attenuation: n/a (directional lights), point lights use 1/(0.001 + 0.001 d + 0.001 d²)
```
with `lightDiffuse[0] = m_colorLight`, `lightDiffuse[1] = m_colorBackLight`, `lightDir[0] = (1,-1,-1)`
(unnormalised!), `lightDir[1] = (0,0,1)` — and the extra `TMLight` point lights (indices 2+, `Range` 5/7,
`Ambient=Specular=Diffuse=colour`) if you want the torch/lantern pools of light.
`D3DRS_COLORVERTEX = 1` ⇒ the *vertex* diffuse/alpha multiplies the material diffuse
(`D3DMCS_COLOR1` is the D3D9 default for `DIFFUSEMATERIALSOURCE` unless overridden; here it is overridden to
MATERIAL for diffuse and MATERIAL for specular — so vertex colour is **not** used for the material, only as
the interpolated gamma-ramp-ish tint that D3D adds when `COLORVERTEX` is on).

### 11.3 Texture-stage emulation

All 8 stages with `D3DTSS_COLOROP/ALPHAOP ∈ {DISABLE, SELECTARG1, MODULATE, MODULATE2X, MODULATE4X, ADD,
ADDSMOOTH, MODULATEALPHA_ADDCOLOR, DOTPRODUCT3}` and arguments from `{DIFFUSE, CURRENT, TEXTURE, TFACTOR}`.
Implement one über-shader:

```glsl
vec4 stageOp(vec4 cur, vec4 diff, vec4 tex, vec4 tf, int op, int a1, int a2) {
    vec4 r1 = pick(a1, cur, diff, tex, tf);
    vec4 r2 = pick(a2, cur, diff, tex, tf);
    switch (op) {
      case 1: return cur;                                   // DISABLE
      case 2: return r1;                                    // SELECTARG1
      case 4: return r1 * r2;                               // MODULATE
      case 5: return vec4(min(r1.rgb * r2.rgb * 2.0, 1.0), r1.a * r2.a);  // MODULATE2X
      case 6: return vec4(min(r1.rgb * r2.rgb * 4.0, 1.0), r1.a * r2.a);  // MODULATE4X
      case 7: return vec4(min(r1.rgb + r2.rgb, 1.0), r1.a);               // ADD
      case 11:return vec4(r1.rgb * r2.rgb + (1.0 - r1.rgb) * r2.rgb /*ADDSMOOTH approx*/, r1.a);
      case 18:return vec4(r1.rgb * r1.a + r2.rgb, r1.a);    // MODULATEALPHA_ADDCOLOR
      case 24:return vec4(vec3(dot(r1.rgb*2.0-1.0, r2.rgb*2.0-1.0)), 1.0); // DOTPRODUCT3
      default:return cur;
    }
}
```
(`ADDSMOOTH` is `src1 + src2 - src1*src2`; the exact D3D definition is
`result = src1 + src2 - src1*src2`.) Get these right and the terrain/water/sky/UI all fall out.

### 11.4 Fog

* Use **per-vertex** fog where D3D did: compute `f` in the vertex shader from view-space `z` and pass it as
  an interpolated scalar, then `color = mix(fogColor, color, f)`. Fragment-side `glFog` will look different
  because the FF path used `D3DFOG_LINEAR` with `FOGVERTEXMODE`.
* Keep two independent fog parameter sets (§4.4) — the FF one (`FOGSTART/END` from block 1) and the
  skinned-mesh one (`c6` from `SetMaterial`).
* The **sky is never fogged**, and neither is the UI, so `f` must be 0/disabled for those pipelines.

### 11.5 Blending / alpha test order

D3D9 order of operations with `ALPHATESTENABLE=1` and `ALPHABLENDENABLE=1`: *test, then blend*. On modern
hardware alpha-to-coverage/MSAA interacts badly with `discard`; since MSAA is disabled for UI and the world
uses `ALPHAREF = 0xDD` (very high), keep `discard` and accept the (already aliased) edges.

### 11.6 Gamma and colour space

* D3D9 with `D3DFMT_X8R8G8B8`/`A8R8G8B8` back buffers and no `D3DPMISCCAPS_POSTBLENDSRGBCONVERT` → blending is
  done in **gamma space** (sRGB-encoded values). Re-create this by **not** using an sRGB swapchain / sRGB
  render target; if you use a linear-space pipeline you will need to encode back and the
  `ONE / INVSRCALPHA` and additive passes will not match.
* The brightness control is a display gamma ramp (§9) — implement it as a platform gamma ramp, not as a
  shader multiply, or the screenshot comparison will differ in the mid-tones.

### 11.7 Swap chain details to mirror

* `D3DSWAPEFFECT_DISCARD` + `BackBufferCount = 1` ⇒ 2 images in flight in Vulkan
  (`minImageCount = 2`, `VK_PRESENT_MODE_IMMEDIATE_KHR` in windowed to match `PresentationInterval = 0`).
* vsync: **off** in windowed mode, from the enumerated `PresentInterval` list in fullscreen. If you want the
  original's tearing behaviour you must use `IMMEDIATE`/`MAILBOX`, not `FIFO`.
* Clear = colour + depth (`Z = 1.0`), no stencil, colour from §8.3 (sky-state driven, four hard-coded values
  plus 0 for dungeons).
* Depth = **16-bit** (`D16_UNORM`). Using 24/32-bit changes nothing visible *except* z-fighting of the
  coplanar passes in §7.4 — so D16 is what reproduces the original's fighting.

### 11.8 Things to delete, not port

* `D3DRS_STENCIL*` (never enabled, no stencil bits).
* `m_nShadowTextureSize` and any shadow-map plan (write-only, no consumer).
* `SetLight()` (`RenderDevice.cpp:452`) — dead method.
* `m_colNormalAmbient` — dead member.
* `D3DRS_VERTEXBLEND` / `INDEXEDVERTEXBLENDENABLE` (always 0 — skinning is in the VS).
* `D3DRS_POINTSIZE`, `ZBIAS`, `DEPTHBIAS`, `TWO_SIDED_STENCIL`, `SEPARATEALPHABLENDENABLE`,
  `BLENDOP*`, `SRGBWRITEENABLE` — **never used** (`[OBSERVED]` exhaustive grep of `D3DRS_`).
