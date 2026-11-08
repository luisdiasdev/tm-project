# Analysis provenance

## Target

| Item | Value |
|---|---|
| File | `WYD.exe` |
| Copies compared | `E:\wyd\test-setup\wydglobal2026\WYD.exe` and `C:\Users\Luis\AppData\Local\Temp\rea-wyd\WYD.exe` |
| SHA-256 | `a058262d36631fd8b674603fb20d54ff18016b8f3dd38249250e953b8efc3c50` |
| Format | PE32, x86 |
| PDB reference | `SD.pdb`, GUID `b551b31f-3da8-475e-9b2d-4a07cacc31ef`, age `1` |
| Provider | Ghidra 12.1.3 via REA |
| Analysis profile | `187191b8b086eb6a29c1a16e74878d8eabd0b3b79420c7198471da3edc72fd1c` |

The E: copy is on exFAT, so the analysis was performed on the NTFS temp copy.
Both copies have the same SHA-256.

## Method

1. Searched strings for `.pane`, `Panel`, `TextureAlias`, `RasterOp`,
   `FontList`, etc.
2. Followed xrefs from the `.pane` extension string (`0x883080`) to the panel
   loader functions.
3. Found the decryption loop in `FUN_00407580` and derived the closed form.
4. Decrypted all `.pane` files and read the text grammar directly.
5. Followed the `element` factory (`FUN_00447900`) and class constructors to
   recover the widget hierarchy.
6. Followed each class's `+0x60` property handler and `+0x0c` draw function.
7. Read the texture loader (`FUN_004aea40`) to confirm `.wyt`/`.wyp`/`.wys`
   handling.

## Reproducing the decoded panels

```bash
python docs/wyd-pane/tools/pane_codec.py decode \
  "UI/Panel/game/HUD.pane" "HUD.txt"
```

or decode the whole tree:

```bash
python docs/wyd-pane/tools/pane_json.py to-json "UI/Panel/game/HUD.pane" "HUD.json"
```

## Verification commands

```bash
# all files: decrypt -> re-encrypt -> compare
python docs/wyd-pane/tools/pane_codec.py --help

# schema validation over every decoded panel
python docs/wyd-pane/tools/pane_json.py validate "UI/Panel/notice.pane"
```

The generated report
[`hexdumps/roundtrip-report.txt`](hexdumps/roundtrip-report.txt) lists the size,
SHA-256 prefix and round-trip status of all 64 shipped files.

## Limitations

- No runtime tracing was needed for the container/format; the text and the
  transform are conclusive.
- Widget semantics are static-analysis results. Draw order and exact visual
  details should be checked against screenshots when implementing a renderer.
- The animation JSON schema is only partially explored; the `.pane` loader
  passes it to the animation manager unchanged.
- The PDB is not available, so function names are `FUN_*`; class names come from
  MSVC RTTI.
- `.wys` was identified and its loader branch located, but its internal pixel
  layout was not fully decoded.
