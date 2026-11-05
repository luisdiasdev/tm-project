# Documentation

This tree records how the 748 port was approached, what was learned from the
original client, and what is still missing.

## Start here

- [`port-748/overview.md`](port-748/overview.md) — goal, branch, commits, method.
- [`port-748/remaining-work.md`](port-748/remaining-work.md) — what to do next.
- [`port-748/pitfalls.md`](port-748/pitfalls.md) — mistakes that waste time.

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
- [`tools/capture_and_click.ps1`](tools/capture_and_click.ps1) — launch the
  client, click a client-area point, capture the window.

## Data

- [`port-748/reference/`](port-748/reference/) — generated inventory and
  missing-binding reports. Regenerate with `tools/ui_inventory.py`.
- [`port-748/screenshots/`](port-748/screenshots/) — TMProject vs original 748
  captures for server select and the Close modal.
