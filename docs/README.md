# Documentation

This tree records how the 748 port was approached, what was learned from the
original client, and what is still missing.

## Start here

- [`port-748/overview.md`](port-748/overview.md) — goal, branch, commits, method.
- [`port-748/remaining-work.md`](port-748/remaining-work.md) — what to do next.
- [`port-748/pitfalls.md`](port-748/pitfalls.md) — mistakes that waste time.
- [`rendering/README.md`](rendering/README.md) — **complete rendering
  specification**: how every texture, mesh, animation and world asset is loaded
  and decoded, the shaders, the render states, the ordered pass list, and a
  guide to rebuilding it all on OpenGL/Vulkan. ~8 000 lines, every claim tagged
  OBSERVED / INFERRED / UNKNOWN.

## Rendering

- [`rendering/README.md`](rendering/README.md) — index, evidence-tag rules,
  the sixteen facts that change a port, corrections to older notes, and the bug
  list found while documenting.
- [`rendering/10-reimplementation-guide.md`](rendering/10-reimplementation-guide.md)
  — start here to *build* it: target architecture, matrix conversion, asset
  pipeline, per-pass pipeline objects, shader plan, work plan, parity harness.
- [`rendering/01-overview.md`](rendering/01-overview.md) — architecture, axes,
  units, resolution policy, camera, timing.
- [`rendering/02-textures.md`](rendering/02-textures.md) — `.wyt`/`.wys`/DDS/TGA,
  the proprietary fourCCs, texture lists, mip and eviction policy.
- [`rendering/03-meshes.md`](rendering/03-meshes.md) — `.msh`, `.msa`, `.bon`,
  vertex/index formats, mesh registries.
- [`rendering/04-animations.md`](rendering/04-animations.md) — `.ani` keyframes,
  sampling, bone palettes, skinning maths, attachments.
- [`rendering/05-shaders.md`](rendering/05-shaders.md) — every shader asset,
  bytecode versions, disassembly, constant map, what is dead.
- [`rendering/06-render-states-and-materials.md`](rendering/06-render-states-and-materials.md)
  — the four state blocks, lighting, fog, blending, depth, swap chain, gamma.
- [`rendering/07-pipeline-and-order.md`](rendering/07-pipeline-and-order.md) —
  the ordered pass list, per-pass states, sorting and culling.
- [`rendering/08-scene-and-world-assets.md`](rendering/08-scene-and-world-assets.md)
  — terrain `.trn`/`.dat`, tile lookup, sky, water, decoration, placement.
- [`rendering/09-ui-and-text.md`](rendering/09-ui-and-text.md) — 2-D projection,
  quads, control layer, GDI font atlas, overlays, z-order.
- [`rendering/evidence/`](rendering/evidence/) — 143 raw hexdumps, whole-asset
  parse proofs, shader disassemblies and the scripts that regenerate them.

## UI rewrite evaluation

- [`ui-rewrite/README.md`](ui-rewrite/README.md) — verdict, scores, effort.
- [`ui-rewrite/01-current-architecture.md`](ui-rewrite/01-current-architecture.md)
  — how the UI works today, end to end.
- [`ui-rewrite/02-pain-points.md`](ui-rewrite/02-pain-points.md) — measured
  failure modes.
- [`ui-rewrite/03-proposals.md`](ui-rewrite/03-proposals.md) — framework vs
  dynamic vs JSON/YAML.
- [`ui-rewrite/04-refactor-plan.md`](ui-rewrite/04-refactor-plan.md) — phases,
  effort, risks, gates.
- [`ui-rewrite/05-target-format.md`](ui-rewrite/05-target-format.md) — JSON
  schema, bindings, actions, tooling.
- [`ui-rewrite/06-measurements.md`](ui-rewrite/06-measurements.md) — raw
  numbers and reproduction.

## Newer `.pane` UI system

- [`wyd-pane/README.md`](wyd-pane/README.md) — start here.
- [`wyd-pane/01-container-and-crypto.md`](wyd-pane/01-container-and-crypto.md)
  — exact `.pane` / `.wyp` delta cipher.
- [`wyd-pane/02-text-format.md`](wyd-pane/02-text-format.md) — grammar and full
  panel/element key reference.
- [`wyd-pane/03-widget-framework.md`](wyd-pane/03-widget-framework.md) —
  `CGuiElement`/`CGuiPanel` class model, vtables, lifecycle.
- [`wyd-pane/04-widget-reference.md`](wyd-pane/04-widget-reference.md) —
  per-widget draw/input behaviour.
- [`wyd-pane/05-textures-and-guimat.md`](wyd-pane/05-textures-and-guimat.md) —
  guimat aliases, `.wyt`, `.wyp`, `.wys`.
- [`wyd-pane/06-reimplementation-notes.md`](wyd-pane/06-reimplementation-notes.md)
  — practical porting guidance.
- [`wyd-pane/tools/`](wyd-pane/tools/) — codec, JSON parser/emitter, schema.
- [`wyd-pane/decoded-panels/`](wyd-pane/decoded-panels/) — all shipped panels
  decoded to text.

## UI modernization plan (using the `.pane` format)

- [`ui-modernization/README.md`](ui-modernization/README.md) — verdict, effort,
  milestones, decision summary.
- [`ui-modernization/01-scope-and-assets.md`](ui-modernization/01-scope-and-assets.md)
  — 2026 hybrid architecture, panel inventory, asset coverage, 748 feature map.
- [`ui-modernization/02-architecture.md`](ui-modernization/02-architecture.md)
  — target layering, modules, binding/input/draw models, integration points.
- [`ui-modernization/03-plan-and-effort.md`](ui-modernization/03-plan-and-effort.md)
  — work packages, estimates, scope levers, test strategy.
- [`ui-modernization/04-risks-and-decisions.md`](ui-modernization/04-risks-and-decisions.md)
  — unknowns, risks, decision log, go/no-go.

## Reference

- [`port-748/build-and-test.md`](port-748/build-and-test.md) — build, run,
  capture and click the client.
- [`port-748/boot-fixes.md`](port-748/boot-fixes.md) — the minimum changes that
  made the 748 client boot (strdef, packet packing, curse, paths).
- [`port-748/assets-and-formats.md`](port-748/assets-and-formats.md) — UI scene
  layout, texture lists, `strdef.bin`, `curse.bin`, packet packing.
- [`port-748/control-ids.md`](port-748/control-ids.md) — the `TM*` namespace,
  how the 229 field bindings were derived, and what is still unbound.
- [`port-748/server-select.md`](port-748/server-select.md) — the runtime-built
  748 server window.
- [`port-748/message-box.md`](port-748/message-box.md) — the 748 modal layout
  and texture sets.
- [`port-748/renderer.md`](port-748/renderer.md) — D3D9 render-target cleanup.
- [`reverse-engineering/rea-workflow.md`](reverse-engineering/rea-workflow.md) —
  exact REA/Ghidra workflow used here.
- [`reverse-engineering/recovered-addresses.md`](reverse-engineering/recovered-addresses.md)
  — function/string addresses and constructor conventions.

## Tools

- [`tools/ui_inventory.py`](tools/ui_inventory.py) — parse UI `.bin` files,
  validate EOF, cross-check control IDs and texture sets.
- [`tools/rewrite_control_ids.py`](tools/rewrite_control_ids.py) — the
  dry-run/verified ID rewriter used for `TMFieldScene.cpp` (byte-preserving).
- [`tools/restore_non_utf8.py`](tools/restore_non_utf8.py) — repair raw bytes
  replaced by U+FFFD in `TMFieldScene.cpp`.
- [`tools/generate_serverlist.py`](tools/generate_serverlist.py) — write the
  client's `serverlist.bin` and `sn.bin` (localhost by default; `--dump` to
  inspect an existing pair).
- [`tools/capture_and_click.ps1`](tools/capture_and_click.ps1) — launch the
  client, click a client-area point, capture the window.

## Data

- [`port-748/reference/`](port-748/reference/) — generated inventory and
  missing-binding reports. Regenerate with `tools/ui_inventory.py`.
- [`port-748/screenshots/`](port-748/screenshots/) — TMProject vs original 748
  captures for server select and the Close modal.
