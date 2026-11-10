# 01 — Rendering Architecture Overview (WYD 748 client port)

Purpose: orient a newcomer to *who owns what* in the WYD client renderer, the coordinate/unit conventions, the screen-resolution policy, the camera model and the timing model, before reading the ordered pass list in `07-pipeline-and-order.md`.

Last updated: 2026-11-10

## Confidence summary

Tag occurrences in this document (a tag on a table row or paragraph covers every claim in it):

| Tag | Count |
|---|---|
| `[OBSERVED]` | 72 |
| `[INFERRED]` | 9 |
| `[UNKNOWN]` | 2 |

Biggest unknowns:

1. **The scene-level render orchestration is not in the decompiled source.** No `TMScene::Render` / `TMScene::RenderFrame` / `TMFieldScene::RenderFrame` exists in `Projects/TMProject/*.cpp` (grep for `RenderFrame` and `::Render()` returns only leaf-object and `D3DDevice::Render` stubs). The port renders *everything* through the generic tree walk `ObjectManager::RenderObject()` (`ObjectManager.cpp:628-660`). Whether the shipped 748 binary had additional per-scene passes outside that walk is unresolved; see `07-pipeline-and-order.md` §2.4.
2. Runtime depth-stencil format actually selected by `CD3DEnumeration` (16 vs 24 bit) — affects Z precision parity.
3. What `JBlur` (motion blur / bloom) actually did in the DX8 original: the class survives as an empty stub (`JBlur.cpp:12-66`).
4. Whether the `m_nWidthShift`/`m_nHeightShift` fields (always 0 here) implemented letterboxing in the original.
5. `UNKNOWN`: original sky-mesh index / visibility semantics — the port sets `m_pSky->m_bVisible = 0` (`TMFieldScene.cpp:1931`) but `TMSky::Render()` never tests `m_bVisible` (`TMSky.cpp:125-224`).

## Table of contents

1. [Class ownership map](#1-class-ownership-map)
2. [Per-frame call chain (WinMain → Present)](#2-per-frame-call-chain-winmain--present)
3. [Coordinate systems, units and matrix convention](#3-coordinate-systems-units-and-matrix-convention)
4. [Screen resolution policy and UI scaling](#4-screen-resolution-policy-and-ui-scaling)
5. [Camera](#5-camera)
6. [Timing model](#6-timing-model)
7. [Scene tree = draw order (the single most important structural fact)](#7-scene-tree--draw-order-the-single-most-important-structural-fact)
8. [Glossary of asset extensions](#8-glossary-of-asset-extensions)
9. [Reimplementation notes (OpenGL/Vulkan)](#9-reimplementation-notes-openglvulkan)

---

## 1. Class ownership map

| Class | File | Owns |
|---|---|---|
| `NewApp` | `NewApp.cpp/.h` | The Win32 window, the game loop (`NewApp::Run`, `NewApp.cpp:623-709`), device creation (`InitDevice`, `NewApp.cpp:426-530`) and the **frame entry point** `NewApp::RenderScene` (`NewApp.cpp:716-742`). |
| `D3DDevice` | `D3DDevice.cpp/.h` | `IDirect3D9* m_pD3D`, `IDirect3DDevice9* m_pd3dDevice`, `D3DPRESENT_PARAMETERS m_d3dpp`, `D3DSURFACE_DESC m_d3dsdBackBuffer`, window/resolution handling, present-parameter building, `ToggleFullscreen`, `CaptureScreen`. `[OBSERVED] D3DDevice.h:59-109` |
| `RenderDevice` | `RenderDevice.cpp/.h` | Derives from `D3DDevice`. Owns: render-state cache (`m_dwRenderStateList[256]`, `m_dwTextureStageStateList[8][29]`, `m_dwSamplerStateList[8][14]`), matrices (`m_matWorld`, `m_matView`, `m_matProj`, `m_matWaterView`), fog params, lights, `m_viewport`, the texture manager, the mesh manager, the font (`TMFont2* m_pFont`), the D3DX sprite, the 8 skinning vertex shaders and the UI draw helpers (`RenderRect*`, `RenderGeomControl`). `[OBSERVED] RenderDevice.h:82-164` |
| `TextureManager` | `TextureManager.cpp/.h` | Texture dictionaries (UI texture sets, model textures, effect textures, env textures, guild marks), `DYNAMIC_TEXTURE_WIDTH/HEIGHT`, and the (currently disabled) off-screen targets `m_pOriginTexture`, `m_pAccumTexture`, `m_pBloomTexture[2]`, `m_pBloomTexture1[2]`. `[OBSERVED] TextureManager.h:94-129` |
| `MeshManager` | `MeshManager.cpp/.h` | Common (static) meshes `m_stCommonMesh[]`, skinned meshes `m_stSkinMeshList[]`, bone-animation tables `m_BoneAnimationList[100]`, object masks `m_aObjectMask`. `[OBSERVED] MeshManager.h` |
| `TreeNode` | `TreeNode.cpp/.h` | The universal scene graph node: `m_pDown` (first child), `m_pNextLink`/`m_pPrevLink` (siblings), virtual `FrameMove/Render/OnMouseEvent/...`. **`AddChild` prepends** (`TreeNode.cpp:120-129`). |
| `TMScene` | `TMScene.cpp/.h` | Scene base (derives `TreeNode`). Owns `m_pControlContainer` (all UI), `m_pEffectContainer`, `m_pShadeContainer`, `m_pGroundObjectContainer`, `m_pHumanContainer`, `m_pItemContainer`, `m_pExtraContainer`, `m_pGround`, `m_pSky`, `m_pSun`, `m_pMyHuman`, ground streaming between two `TMGround` pages. `[OBSERVED] TMScene.h:40-70`, ctor `TMScene.cpp:23-260` |
| `TMFieldScene` | `TMFieldScene.cpp/.h` | In-game scene: builds the 748 HUD from `FieldScene2.bin`, creates ground/objects/humans/weather/effects, handles 229 field control IDs, skill/attack/camera logic. Has **no** `Render` override. `[OBSERVED] TMFieldScene.h:24-257` |
| `TMSelectCharScene` | `TMSelectCharScene.cpp/.h` | Character-select scene (`env\character.trn`); **no** `Render` override — its humans/ground draw through the same tree walk. `[OBSERVED] TMSelectCharScene.cpp:104-119` |
| `TMSelectServerScene` | `TMSelectServerScene.cpp/.h` | Server-select scene; creates `m_pSun`, `m_pSnow`, 50 demo players added to `m_pHumanContainer` (`:1184`) and — oddly — renders each demo player **once** during `ResetDemoPlayer()` setup, i.e. outside `BeginScene` (`:1190-1198`). **No** `Render` override. `[OBSERVED] TMSelectServerScene.cpp:261-357, 1023-1199` |
| `TMDemoScene` | `TMDemoScene.cpp/.h` | Opening-AVI/demo scene; skeletal stubs only. `[OBSERVED] TMDemoScene.cpp:4-56` |
| `ObjectManager` | `ObjectManager.cpp/.h` | Owns the *root* `TreeNode m_pRoot`, the single `TMCamera m_pCamera`, game state, and the two driver loops `FrameMove()` (`:328-370`) and `RenderObject()` (`:628-660`), plus `RenderControl()` for the UI (`:591-626`). |
| `TMCamera` | `TMCamera.cpp/.h` | Orbiting 3rd-person camera around `m_pFocusedObject`; produces `m_cameraPos`, `m_vecCamDir`, ground-clamped sight length, and the 9 "visual key" grid cells. |
| `TimerManager` | `TimerManager.cpp/.h` | Single global millisecond clock (`GetServerTime()`), server-time resync, `m_dwDelayTime`. |
| `SControlContainer` / `SControl` / `GeomControl` | `SControlContainer.cpp`, `SControl.cpp`, `GeomObject.h` | Widget tree for UI; each frame `FrameMove()` rebuilds `m_pDrawControl[30]` (bucketed by tree depth) and `RenderControl()` consumes/draws it. |

### 1.1 ASCII ownership diagram

```
NewApp                                  (window + loop, NewApp.cpp)
 ├─ m_pRenderDevice : RenderDevice ─────┐  (D3DDevice : swapchain/device)
 │                                       ├─ m_pTextureManager : TextureManager
 │                                       ├─ m_pMeshManager    : MeshManager
 │                                       ├─ m_pFont           : TMFont2   (font atlas quads)
 │                                       ├─ m_pSprite         : ID3DXSprite
 │                                       └─ m_pVertexShader[8] (skinning VS from Shader\skinmesh*.bin)
 ├─ m_pObjectManager : ObjectManager
 │    ├─ m_pRoot : TreeNode
 │    │    ├─ m_pCamera : TMCamera           (added in ObjectManager ctor, :32-35)
 │    │    └─ <current scene> : TMScene       (added by SetCurrentState, :752)
 │    │         ├─ m_pExtraContainer     : TreeNode   (floating damage text TMFont3)
 │    │         ├─ m_pEffectContainer    : TreeNode   (sun, rain, snow, billboards, skills)
 │    │         ├─ m_pShadeContainer     : TreeNode   (TMShade blob shadows / decals)
 │    │         ├─ m_pGroundObjectContainer : TreeNode
 │    │         │     ├─ m_pObjectContainerList[0] : TMObjectContainer (static world)
 │    │         │     └─ m_pGroundList[0] : TMGround  (terrain)
 │    │         ├─ m_pHumanContainer     : TreeNode   (TMHuman: player, mobs, NPCs)
 │    │         ├─ m_pSky : TMSky                     (skydome mesh)
 │    │         ├─ m_pItemContainer      : TreeNode   (TMItem/TMGate/TMCannon/drops)
 │    │         └─ m_pControlContainer   : SControlContainer  (ALL 2D UI)
 │    │                └─ m_pDrawControl[30] : GeomControl lists  → drawn by RenderControl()
 │    └─ m_pTimerManager, m_pSocketManager, ...
```

(Child creation order is `[OBSERVED] TMScene.cpp:74,78,82,86,90` for the five containers, `TMFieldScene.cpp:1933-1934` for sky and item container, and because `AddChild` prepends, the *iteration* order is the reverse — see §7.)

---

## 2. Per-frame call chain (WinMain → Present)

```
wWinMain                                   NewApp.cpp:1392
 └─ newApp->Initialize(hInstance, nFull)   NewApp.cpp:168-424
      └─ InitDevice()                      NewApp.cpp:426-530   → new RenderDevice(...), Initialize(hWnd)
 └─ newApp->Run()                         NewApp.cpp:623-709   ← THE LOOP
      │  PeekMessage loop; when idle and m_bActive==1:
      ├─ m_pEventTranslator->ReadInputEventData()   NewApp.cpp:651  (DInput mouse/keyboard)
      ├─ m_pEventTranslator->CameraEventData()      NewApp.cpp:652  (camera yaw/pitch/zoom)
      ├─ m_pObjectManager->FrameMove(dwServerTime)  NewApp.cpp:654
      │     ├─ walk scene tree → TreeNode::FrameMove  ObjectManager.cpp:336-362
      │     └─ EffectFrameMove(m_pEffectContainer)   ObjectManager.cpp:364-365
      │           ↳ inside scene FrameMove: SControlContainer::FrameMove  TMScene.cpp:1378-1379
      │             (rebuilds m_pDrawControl buckets)  SControlContainer.cpp:259-311
      ├─ m_pAviPlayer check → RenderScene()          NewApp.cpp:682-685
      │     ├─ m_pd3dDevice->TestCooperativeLevel()  NewApp.cpp:718
      │     ├─ SetViewPort(0,0,W,H)                  NewApp.cpp:722  → SetProjectionMatrix()
      │     ├─ Lock(1)                               NewApp.cpp:724  → Clear + BeginScene
      │     ├─ SetViewVector(cam->m_cameraPos, lookat)  NewApp.cpp:726-729
      │     ├─ SetRenderStateBlock(1)                NewApp.cpp:730  ("3D world" state block)
      │     ├─ ObjectManager::RenderObject()         NewApp.cpp:731  → scene tree Render()
      │     ├─ SetRenderStateBlock(3)                NewApp.cpp:732  ("2D/UI" state block)
      │     ├─ ObjectManager::RenderControl()        NewApp.cpp:733  → SetMatrixForUI + UI quads
      │     ├─ Unlock(1)                             NewApp.cpp:734  → EndScene + Present
      │     ├─ g_pTextureManager->ReleaseNotUsingTexture()   NewApp.cpp:736-737
      │     └─ g_pMeshManager->ReleaseNotUsingMesh()         NewApp.cpp:738-739
      └─ if (m_pObjectManager->m_bCleanUp) CleanUp()  NewApp.cpp:699-700
```

Notes:
- There is exactly **one** `BeginScene`/`EndScene` pair per frame, both inside `RenderDevice::Lock`/`Unlock` (`RenderDevice.cpp:405`, `:440`). `[OBSERVED]`
- `Lock(bClear)` is always called with `bClear == 1` in this port (`NewApp.cpp:724`); `Unlock(bEnd)` always with `bEnd == 1`. `[OBSERVED]`
- The **only** place a viewport is set per frame is `NewApp.cpp:722` (full back buffer). `[OBSERVED]`
- No render-target switching happens anywhere in the frame: `grep SetRenderTarget` finds only `GetRenderTarget` in `RestoreDeviceObjects` (`RenderDevice.cpp:601`). `[OBSERVED]`

---

## 3. Coordinate systems, units and matrix convention

### 3.1 Axes

- **Y is up, X/Z is the ground plane.** `TMObject::SetPosition(fX, fY, fZ)` stores `m_vecPosition.x = fX`, `m_vecPosition.y = fZ`, `m_fHeight = fY` — i.e. the 2-D actor position is (x, z) and the height is a separate scalar. `[OBSERVED] TMObject.cpp:41-49`
- Ground vertices are built as `(tileX, height, tileY)` then transformed by `Scale(2.0, 0.1, 2.0) * Translate(offset.x, 0, offset.y)`. `[OBSERVED] TMGround.cpp:2711-2716, 3137-3146`
- Sky dome is placed at the camera position with `m_fHeight + 1.0f` and rotated by `m_fAngle` (default 180°). `[OBSERVED] TMSky.cpp:161-166`
- All `D3DXMatrixRotationYawPitchRoll` usages pass yaw about **Y**, pitch about **X**, roll about **Z**, in D3D's left-handed convention. `[OBSERVED]` e.g. `TMMesh.cpp:169`, `TMSky.cpp:162`

### 3.2 Handedness and projection

- Projection is `D3DXMatrixPerspectiveFovLH` and view is `D3DXMatrixLookAtLH` with `up = (0,1,0)`, so the whole pipeline is **left-handed, depth range Z ∈ [0, 1]**. `[OBSERVED] RenderDevice.cpp:1834, 1841`, UI `:1859-1867`
- The port must NOT flip handedness: any OpenGL reimplementation needs `glClipControl`-style `[0,1]` depth or an equivalent depth remap, and a Z-flip only if using a right-handed math library. `[INFERRED] from the LH projections above`

### 3.3 Units and scale

| Quantity | Value | Evidence |
|---|---|---|
| Terrain tile size | **2.0 world units** (the ground mesh scales tile index by 2) | `[OBSERVED] TMGround.cpp:2711` |
| Terrain tile height scale | **0.1 units per stored height step** (mask heights are `char`, `*0.1f`) | `[OBSERVED] TMGround.cpp:2724, 3224`, `TMShade.cpp:144-147` |
| Ground page | 64 × 64 tiles = **128 × 128 world units** per `.trn`; two pages (`m_pGroundList[2]`) live at once | `[OBSERVED] TMGround.cpp:2823-2831` loops `0..63`, `TMScene.h:59-60`, `TMFieldScene.cpp:1473-1485` |
| Zone file naming | `env\Field%02d%02d.trn` / `.dat` where the index is `worldPos >> 7` | `[OBSERVED] TMFieldScene.cpp:1466-1471` |
| Character size | `TMHuman::m_vecPickSize[skinType].y ≈ 2.0` (bounding half-height used for labels/picking), so a character is ≈2 units tall ≈ one tile wide | `[OBSERVED] TMHuman.cpp:59-61, 7419` |
| Camera default distance | 15.0 (`m_fMaxCamLen`), 7.0 in the far view mode, 15.5 in view mode 4 | `[OBSERVED] TMCamera.cpp:16, 438-469` |
| Fog near/far (device) | `m_fFogStart = 60`, `m_fFogEnd = 156` at construction, then overridden **every frame** by `TMSky::FrameMove` (weather table `FogList[16][2]`, e.g. `{16,34}` for field, `+5` added) | `[OBSERVED] RenderDevice.cpp:56-57`, `TMSky.cpp:21-39, 422-444` |
| Clip planes | `near = g_ClipNear*1.4 = 0.966`, `far = g_ClipFar` (set to 70 in the scene ctor, 90 in one sky branch) | `[OBSERVED] RenderDevice.cpp:1844-1845`, `TMGlobal.cpp:23-24`, `TMScene.cpp:34`, `TMSky.cpp:254, 412` |
| Vertical FOV | `m_fFOVY = 0.25` × 180° = **45°** | `[OBSERVED] RenderDevice.cpp:29, 1842` |

### 3.4 Matrix convention (row-vector, world = R·S·T with identity parent)

Every static/skinned draw composes the same way (D3D row-vector convention, `v' = v · W · V · P`):

```cpp
D3DXMatrixScaling(&matScale, m_fScaleH, m_fScaleV, m_fScaleH);
D3DXMatrixTranslation(&matPosition, fX, fY, fZ);
D3DXMatrixRotationYawPitchRoll(&mat, fAngle, fAngle2 - D3DXToRadian(90), fAngle3);
D3DXMatrixMultiply(&mat, &g_pDevice->m_matWorld, &mat);   // parent world (always identity here)
D3DXMatrixMultiply(&mat, &mat, &matScale);
D3DXMatrixMultiply(&mat, &mat, &matPosition);
g_pDevice->m_pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(0), &mat);
```
`[OBSERVED] TMMesh.cpp:159-178`

- `g_pDevice->m_matWorld` is a **global parent transform that is set to identity once** (`InitializeRenderingState`, `RenderDevice.cpp:1577-1578`) and is never modified afterwards (grep for `m_matWorld` finds only reads/`SetTransform` of locals). So in practice `W = R · S · T`. `[OBSERVED] RenderDevice.cpp:1577-1578` + grep
- Skinned characters do **not** use `D3DTS_WORLD` at all: the bone matrices are pre-multiplied by the view matrix, transposed, and uploaded as 3 constant registers each starting at `c9`; the projection is in `c2` (transposed), the light direction in `c1`, and the inverse-transposed view in `c92`. `[OBSERVED] CMesh.cpp:625-640`, `RenderDevice.cpp:1848-1851, 1690`
- Note the ubiquitous `- D3DXToRadian(90)` pitch bias on static meshes: WYD meshes are authored facing +X and are rotated by −90° to face +Z. Any reimplementation must keep it or import a compensating rotation. `[OBSERVED] TMMesh.cpp:169`, `TMObject.cpp:542`, `TMSky.cpp:162`; `[INFERRED]` that it is an axis-convention compensation

### 3.5 Where the port must transpose/mirror

- D3D matrices are uploaded **as-is** for `SetTransform` (they are row-major, row-vector). For shader constant uploads the code explicitly transposes (`SetProjectionMatrix`, `CMesh::RenderMesh`). An OpenGL/Vulkan port uses column-major + `v' = P·V·W·v`, i.e. every matrix must be transposed exactly once. `[INFERRED]` from `RenderDevice.cpp:1848-1851` and `CMesh.cpp:628-630`
- UI quads are **pre-transformed** (`D3DFVF_XYZRHW`, FVF 324) and therefore need *no* matrix at all — a port must draw them in a pixel-space orthographic pass. `[OBSERVED] RenderDevice.cpp:2156-2161, 324`

---

## 4. Screen resolution policy and UI scaling

### 4.1 Resolution selection

- A fixed 11-entry table maps `config.txt` `[RES]` (1-based) to width/height/bpp: 640×480/16 … 3200×2400/32. `[OBSERVED] NewApp.cpp:175-193, 264-272`
- If `CheckResolution()` fails, it falls back to entry index 1 (800×600×16). `[OBSERVED] NewApp.cpp:267-272, 1372-1385`
- Font size is picked from the *width*: 640→10, 800→12, 1024→14, 1280→20, else 24. `[OBSERVED] NewApp.cpp:274-292`
- `m_dwCurrScreenX/Y/Bpp/RefreshRate` are the **desktop** mode (from `GetDeviceCaps`), captured only for restoring windowed mode; they are *not* the render-target size. `[OBSERVED] RenderDevice.cpp:466-483`

### 4.2 The virtual canvas is 800×600 (not 1024×768)

```
m_fWidthRatio  = screenWidth  / 800.0f     // RenderDevice.cpp:64
m_fHeightRatio = screenHeight / 600.0f     // RenderDevice.cpp:65
BASE_ScreenResize(size) = screenWidth * size / 800.0f   // Basedef.cpp:33-35
```
`[OBSERVED]`

- UI layout code clamps/positions against `800*m_fWidthRatio` × `600*m_fHeightRatio`, i.e. the full screen. `[OBSERVED] SControl.cpp:374-377, 854-859`
- `SPanel`/`SText` positions are authored in **800×600 logical pixels** and multiplied by the ratios at draw time. `[OBSERVED] SControl.cpp:865-951`
- **Correction to a common assumption:** there is no 1024×768 virtual canvas in this client; the base canvas is 800×600 at 100 % = 800×600. Anything that "looks like" 1024×768 comes from the shipped 748 HUD scene being authored at that pixel size and then scaled by `m_fWidthRatio`. `[OBSERVED]` (ratios above) — the mismatch is why `SControl` code is full of `if (m_fWidthRatio == 1.28f)` special cases (`TMFieldScene.cpp:16767-16782`).

### 4.3 Viewport

```cpp
void RenderDevice::SetViewPort(x, y, w, h) {
    m_viewport = {x, y, w, h, MinZ=0.0f, MaxZ=1.0f};
    m_pd3dDevice->SetViewport(&m_viewport);
    SetProjectionMatrix();          // aspect = (W - m_nWidthShift)/(H - m_nHeightShift)
}
```
`[OBSERVED] RenderDevice.cpp:343-358`

- The viewport is always the full back buffer, origin (0,0). `[OBSERVED] NewApp.cpp:722`
- **No letterboxing / no aspect correction beyond the projection aspect ratio.** `m_nWidthShift`/`m_nHeightShift` are initialised to 0 (`RenderDevice.cpp:52-53`) and every assignment in the codebase sets them to 0 (`TMFieldScene.cpp:552, 628, 12851, …`). `[OBSERVED]`
- Consequence: on non-4:3 resolutions the 3-D projection stretches but the UI (pixel-space quads scaled by the two independent ratios) also stretches — the UI does **not** preserve aspect. `[INFERRED] from the two independent ratios + XYZRHW quads`
- 2-D screen mapping used by picking/labels/ground culling:
  `px = (x+1)/2 * (screenW - widthShift)`, `py = (-y+1)/2 * (screenH - heightShift)`. `[OBSERVED] TMHuman.cpp:7446-7453`, `TMGround.cpp:3232-3233`
  ⚠ decompile bug to flag: `TMGround.cpp:3233` uses `m_nWidthShift` for the **Y** term (`m_dwScreenHeight - m_nWidthShift`). Harmless while both are 0, but do not copy it.

### 4.4 UI matrix vs. pixel-space UI

`SetMatrixForUI()` sets a *pseudo* projection (FOV 0.1 rad, aspect `backbuffer.Width/Height * 0.94`, near 10, far 100) and a view at `(0,0,50)` looking at the origin: `[OBSERVED] RenderDevice.cpp:1856-1872`

- Used for **3-D objects embedded in the UI** (`RENDER_3DOBJ`: character portrait, item icons, boss health orbs) via `TMMesh::RenderForUI`, which fakes an orthographic placement by yaw/pitch derived from the screen pixel: `fYaw = atan2(fX, 50)`, `fPitch = atan2(fY, 50)`. `[OBSERVED] TMMesh.cpp:283-311`, `RenderDevice.cpp:3320-3407`
- All **2-D UI** (panels, text, buttons, bars, icons, labels) does **not** use it — it emits `XYZRHW` quads with `rhw = 1.0` and `z = 0.01`. `[OBSERVED] RenderDevice.cpp:208-283` (vertex init), `:2057-2064` (only x/y written), `:2156`

---

## 5. Camera

### 5.1 Model

- One camera instance, owned by `ObjectManager` and parented to the root node (`ObjectManager.cpp:32-35`). `[OBSERVED]`
- Spherical orbit around `m_pFocusedObject` (normally `m_pMyHuman`), state = `m_fHorizonAngle` (yaw), `m_fVerticalAngle` (pitch), `m_fSightLength` (distance), `m_fSightTarget = 3.0` (look-at height), `m_fCamHeight`, `m_fMinClose`, `m_fMaxCamLen = 15`. `[OBSERVED] TMCamera.cpp:16-47`
- Eye position (`GetCameraPos`, `TMCamera.cpp:78-80`):
  ```
  cam.x = (focus.x - g_fWide) - cos(pitch)*cos(yaw) * sight * 1.2
  cam.z = (focus.y + g_fWide) - cos(pitch)*sin(yaw) * sight * 1.2
  cam.y =  -sin(pitch)*sight + sightTarget + focusHeight + camHeight - 2.0
  ```
  `g_fWide` is a global that is never assigned (always 0.0) so it is a no-op. `[OBSERVED] TMCamera.cpp:78-80`, `TMGlobal.cpp:80` (no writer found)
- Look-at (`GetCameraLookatDir`, `TMCamera.cpp:382-389`):
  `dir = (cos(yaw)cos(pitch), sin(pitch), sin(yaw)cos(pitch))`; `lookAt = camPos + dir`. `[OBSERVED] TMCamera.cpp:314-316`
- View matrix is then built by `RenderDevice::SetViewVector(camPos, lookAt)` each frame with `up=(0,1,0)`. `[OBSERVED] NewApp.cpp:729`, `RenderDevice.cpp:1828-1837`
- View modes (`TMCamera::SetViewMode`): 0 → pitch −0.314, dist 7; 1 → pitch −0.770, dist 15; 2 → pitch −1.047, dist 14; 4 → pitch −0.785, dist 15.5. Yaw defaults to 45°. `[OBSERVED] TMCamera.cpp:430-472`
- Camera *rotate* input: yaw += dx·0.0049, pitch += dy·0.002, pitch clamped to [−0.9854, 0.75] (`EventTranslator.cpp:211-260`); zoom = wheel/240, clamped to [fClose (1.2 or 2.5 when mounted), 15] (`EventTranslator.cpp:311-329`). `[OBSERVED]`

### 5.2 Camera collision / zoom-in rules

- `GetCameraLookatPos()` samples `GroundGetHeight(camera.xz)`; if the ground is above `camY - 0.27`, it shortens `m_fSightLength` by the intersection length (or by 0.5 when the intersection is behind), so the camera slides in instead of entering terrain. `[OBSERVED] TMCamera.cpp:322-355`
- There is **no** ray cast against world meshes/houses — only against the height field. `[INFERRED]` (only `GroundGetHeight` is consulted; no `D3DXIntersect` in `TMCamera.cpp`)
- `m_bInWater` is set from `GroundIsInWater()` and then drives an aggressive fog override (start 1, end 10) and a clear-colour override in `TMSky::FrameMove` (`TMSky.cpp:501-514`). `[OBSERVED]`
- `m_nEarthLevel` / `m_dwSetTime` implement a timed screen-shake flag (set by `EarthQuake`) that restores the previous angles after 1–5 s. `[OBSERVED] TMCamera.cpp:49-63, 409-413`; the actual shake offset is applied by callers, not here. `[INFERRED]`
- `RenderDevice::m_bCameraRot` (config `[CAMERAROTATE]`) only inverts mouse X/wheel signs. `[OBSERVED] NewApp.cpp:232-235`, `EventTranslator.cpp:211-220`

### 5.3 Screen→world ray and culling keys

- `RenderDevice::GetPickRayVector(orig, dir)` inverts `m_matView`, builds the NDC point from `g_pCursor->m_nPosX/Y` divided by `(screen - shift)` and by `m_matProj.m[0][0] / m_matProj.m[1][1]`, giving the usual D3DX pick ray. `[OBSERVED] RenderDevice.cpp:1874-1891`
- `TMCamera::GetCameraPos` also publishes a **3×3 grid of "visual keys"** around the camera (`TreeNode::m_VisualKey1..9`), each key = `(worldX>>5) + ((worldZ>>5) << 16)` — a 32×32-unit cell id. `[OBSERVED] TMCamera.cpp:104-259`; the same formula is used when adding world objects: `unsigned int Key = (intx>>5) + ((inty>>5)<<16)` (`TMObjectContainer.cpp:103`).
- `TMObject::isVisualKey()` returns true when the object's key matches one of the 9 camera cells, and `Render()`/`FrameMove()` early-out when it does not. This **is the renderer's coarse spatial cull**. `[OBSERVED] TMObject.cpp:85-95, 671-684`, `TMTree.cpp:35`, `TMLeaf.cpp:49-56`

### 5.4 `m_bDungeon`

`RenderDevice::m_bDungeon` (0 = outdoors, 1/2/3/4/5 = dungeon variants) is set from the loaded terrain type (`TMGround.cpp:2351-2362`) and switches: clear colour, light colours, fog table, ground second-texture stage, water material, sky visibility, and the water-mesh variant. It is a **global mode switch consulted inside nearly every pass**, so a reimplementation must carry it as a per-frame uniform/flag. `[OBSERVED] TMGround.cpp:2340-2370`, `TMSky.cpp:257-372`, `TMSea.cpp:43-157`, `RenderDevice.cpp:389-398`

---

## 6. Timing model

- **One clock**: `TimerManager::GetServerTime()` returns `m_dwDelayTime + (timeGetTime() - m_dwBaseTime) + m_dwServerTime`, i.e. a millisecond counter that starts at the last server timestamp and is periodically resynced from packet ticks. `[OBSERVED] TimerManager.cpp:47-80`
- `m_dwDelayTime` is a manual global pause offset (used during scene teardown: `TMScene::~TMScene` sets 20000 then 0). `[OBSERVED] TMScene.cpp:264, 283`
- Server resync: if a packet tick differs by >500 ms, the client shifts the clock by ±85 ms (drift correction), evaluated at most every 10 s. `[OBSERVED] NewApp.cpp:1190-1204`
- **No fixed timestep and no frame-delta accumulator.** Each system stores its own "last tick" and subtracts:
  - animation/effects: `nTimeSpent = now - m_dwCreateTime` (`TMEffectBillBoard.cpp:196-203`)
  - rain: `fElapsedTime = (now - m_dwOldServerTime) * 0.1f` (`TMRain.cpp:140`)
  - the render delta is **never** used for interpolation; the renderer simply draws whatever state `FrameMove` produced this iteration. `[OBSERVED]`
- FPS is measured inside `RenderDevice::Unlock` over a 2-second window and can be overlaid on screen (`g_bDebugMsg`). `[OBSERVED] RenderDevice.cpp:412-438`
- `m_dwStartTime` on `RenderDevice` is **not** a frame timer — it is only used by `RenderGeomControlBG` to stamp the first draw. `[OBSERVED] RenderDevice.cpp:3431-3432`
- Practical consequence: the client is frame-rate dependent. On a modern machine, `FrameMove` is called as fast as `PeekMessage` allows and there is no `Sleep`, so effects/animation run faster than in 2007. `[INFERRED]` (no `Sleep`/vsync wait in `NewApp::Run`; `m_d3dpp.PresentationInterval = 0` when windowed, `D3DDevice.cpp:284`)

---

## 7. Scene tree = draw order (the single most important structural fact)

`ObjectManager::RenderObject()` is a plain depth-first walk of the scene tree; each node's `Render()` is called and a non-zero return descends into `m_pDown`, then walks `m_pNextLink` siblings (`ObjectManager.cpp:628-660`). Because `AddChild` **prepends** (`TreeNode.cpp:120-129`, confirmed against the original binary's `0x54ac09` "prepend child" in `docs/reverse-engineering/recovered-addresses.md`), **iteration order is the reverse of insertion order**.

Final order of the field scene's top-level children:

| # | Node | Inserted at | Consequence |
|---|---|---|---|
| 1 | `m_pItemContainer` | `TMFieldScene.cpp:1934` (last) | dropped items & gates are drawn **first** |
| 2 | `m_pSky` | `TMFieldScene.cpp:1933` | skydome |
| 3 | `m_pHumanContainer` | `TMScene.cpp:90` | all characters/NPCs/monsters |
| 4 | `m_pGroundObjectContainer` | `TMScene.cpp:86` | children: `TMGround` (`1921`) then `TMObjectContainer` (`1488`) → **terrain, then static objects** |
| 5 | `m_pShadeContainer` | `TMScene.cpp:82` | blob shadows / ground decals |
| 6 | `m_pEffectContainer` | `TMScene.cpp:78` | sun, rain, snow, target markers, then every spawned effect (newest first) |
| 7 | `m_pExtraContainer` | `TMScene.cpp:74` | floating damage numbers (`TMFont3`) drawn last |

`[OBSERVED]` (all call sites above) + `[INFERRED]` that this equals the original's order, since the walk and the prepend semantics are decompiled from the original.

The far more useful, per-pass description is in `07-pipeline-and-order.md`.

---

## 8. Glossary of asset extensions

| Ext | Meaning | Where loaded |
|---|---|---|
| `.msh` | Skinned/bone-animated mesh (`%s%02d%02d.msh` from the bone-animation name) | `TMSkinMesh.cpp:167-173` |
| `.ani` | One animation clip of a `.bon` set (`%s%04d.ani`) | `MeshManager.cpp:127, 161` |
| `.msa` | Static mesh (vertex/index buffers built by `TMMesh::LoadMsa`); the literal filename comes from `Mesh\MeshList.txt` | `MeshManager.cpp:457, 54-64`, `TMMesh.cpp:481` |
| `.bon` | Bone-animation descriptor for one animation set (`%s.bon`) | `MeshManager.cpp:113` |
| `.wys` | Texture (`.wys` image container) — `mesh\%s.wys` for models, `Effect\%s.wys` for effects | `TMPaths.h:44-45`, `TMMesh.cpp:546` |
| `.wyt` | UI texture atlas (texture *set* file); minimap atlas is `UI\m%02d%02d.wyt` | `TMGround.cpp:3754`, `TMPaths.h:33` |
| `.trn` | Terrain tile map (64×64 tiles: tile index, back-tile index, height, colour) — `env\Field%02d%02d.trn` | `TMGround.cpp` (LoadTileMap), `TMFieldScene.cpp:1470` |
| `.dat` | Static-object placement list for a zone (`env\Field%02d%02d.dat`) | `TMObjectContainer::Load`, `TMFieldScene.cpp:1471` |
| `.bin` | Binary tables: `Mesh\MeshList`(txt)/`EffectTextureList.bin`/`MeshTextureList.bin`/`EnvTextureList3.bin`/`object.bin`/`AttributeMap.dat`/`strdef.bin`/UI scene `.bin`s | `TMPaths.h:26-42`, `ObjectManager.cpp:850` |
| `Shader\skinmesh%d.bin` | Pre-compiled vertex shader for N-bone skinning (1..8) | `RenderDevice.cpp:942` |

---

## 9. Reimplementation notes (OpenGL/Vulkan)

1. **Keep the tree, keep the order.** The scene graph *is* the render list. Reimplement `TreeNode` (prepend + DFS walk) verbatim, or bake the resulting order (see §7 and `07-pipeline-and-order.md`) into an explicit pass list. Do not "improve" it into a sorted queue — transparency ordering in WYD is painter's-order-by-tree.
2. **Y-up, LH, Z∈[0,1], 45° vertical FOV, near 0.966, far ≈70.** Convert with a single `P_clip = R_z * P_lh` style remap if you go RH; never mirror an axis (that would flip all the yaw conventions and the `-90°` mesh bias).
3. **World scale**: 1 unit ≈ 1 m; tile 2 units; page 128 units; character ≈2 units. Camera distances 1.2…15 units. Get this wrong and fog/HLOD constants will not match.
4. **Two independent screen ratios** (`W/800`, `H/600`) for UI, one aspect ratio (`W/H`) for 3-D. UI must be drawn in a pixel-space (pre-transformed) pass, not through the 3-D projection.
5. **`m_bDungeon` is a global render mode** (0,1,2,3,4,5) that changes clear colour, lights, fog, water material and ground tex-stage; expose it as a per-frame constant.
6. **The debug knobs you should keep** for A/B testing against the original: `g_bHideBackground`, `g_bHideEffect`, `g_bHideSkillBuffEffect(2)`, `g_nUIFadeAlpha`, `RenderDevice::m_bFog`, `m_bShowEffects`.
7. **Do not port `JBlur`/render-target code as-is** — it is disabled in this port (`TextureManager.cpp:1446-1453` returns 1 before the `#if 0` block; `JBlur.cpp` is empty). See `07-pipeline-and-order.md` §6.
8. **State-block equivalence**: the client has only four canned state blocks (`SetRenderStateBlock(0..3)`). Reproduce them as four immutable pipeline-state objects and switch between them; per-object tweaks are then deltas on top. Exact contents are tabulated in `07-pipeline-and-order.md` §7.1.
9. **Vertical flip**: D3D screen Y grows downward, and all UI coordinates are top-left based; the projection helper `SetMatrixForUI` assumes it too. If you render UI through an ortho matrix instead of pixel-space quads, add the flip there, not in the world pass.
10. **What this document does not tell you**: the exact vertex-shader bytecode semantics of `skinmesh1..8.bin`. The constant layout is documented (`c1` light dir, `c2` transposed proj, `c9+3i` bone matrices premultiplied by view, `c92` inverse-transposed view) but the shader body is opaque — reimplementation requires writing your own equivalent skinning shader from those constants. `[OBSERVED]` for the layout, `[UNKNOWN]` for the bytecode.
