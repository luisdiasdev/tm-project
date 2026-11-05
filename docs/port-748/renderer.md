# Renderer Notes (D3D9)

## Render-target textures (fixed)

`TextureManager::InitRenderTargetTexture()` used:

```cpp
D3DXCreateTexture(..., 1 /* D3DUSAGE_RENDERTARGET */, D3DPOOL_MANAGED, ...)
```

D3D9 requires `D3DUSAGE_RENDERTARGET` resources to be in `D3DPOOL_DEFAULT`;
`MANAGED` returns `D3DERR_INVALIDCALL`. The old code also created
`m_pOriginTexture` twice without releasing it. Every boot logged:

```
Error in Init Render Target Texture
```

No code samples these surfaces (`m_pOriginTexture`, `m_pAccumTexture`,
`m_pRenderTexture`, `m_pBloomTexture*` are write-only). The function is now a
no-op returning success, with the old body kept under `#if 0` for reference.
`WYD.log` is clean.

If 755/760 post-processing is ever enabled, implement these correctly:

- use `D3DPOOL_DEFAULT` for render-target textures,
- release every surface/texture in `ReleaseTexture()` / device-lost handling,
- recreate on `RestoreRenderTargetTexture()`.

## Sprite/GDI font

The UI font is GDI-rendered into a bitmap and uploaded every `TMFont2::SetText`
(`TMFont2.cpp`). It loads `UI/minimap.wyt` as the glyph atlas
(`MiniMap_Path`) and `RenderDevice::m_nFontTextureSize`. 748 has this file and
the same format, so text rendering itself is not version-specific.

## Negative texture sets

`RenderGeomRectImage`:

- `nTextureSetIndex >= 0`: normal set.
- `nTextureSetIndex < -2`: set `abs(index)`.
- `-2`: solid fill with `dwColor`; `-1`: no image (text only).

`SPanel::FrameMove2` only submits a panel when
`nTextureSetIndex >= 0 || (dwColor & 0xFF000000)`. For 748, generated frame
panels use `0xFFFFFFFF`, so they draw.

## D3D8 vs D3D9

The original 748 client is DX8, but TMProject is already a D3D9 codebase and
runs the 748 assets. Do not port API-level DX8 calls; the version delta that
breaks the port is data/IDs/packets.
