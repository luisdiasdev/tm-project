# Tools

## `pane_codec.py`

Low-level container tool.

```bash
python pane_codec.py dump   <file.pane>
python pane_codec.py decode <file.pane> [out.txt]
python pane_codec.py encode <file.txt>  [out.pane]
```

- `dump` decrypts and prints the UTF-16 text.
- `decode` writes the decoded UTF-16LE text (with BOM).
- `encode` applies the exact inverse transform, producing a game-loadable
  `.pane`.

## `pane_json.py`

Semantic tool: parse a panel into ordered JSON and rebuild it.

```bash
python pane_json.py to-json   <file.pane|file.txt> [out.json]
python pane_json.py from-json <file.json> [out.pane] [--text]
python pane_json.py validate  <file.pane|file.txt>
```

- `to-json` accepts encrypted `.pane` or already-decoded text.
- `from-json` writes an encrypted `.pane` by default; `--text` writes decoded
  UTF-16LE text instead.
- `validate` checks every key against `pane_schema.json`.
- The JSON keeps attributes as an ordered list, so unknown keys and ordering
  survive a round trip.

Verified:

- 64/64 shipped files (`63 .pane` + 1 plaintext `.panel`) parse and re-emit.
- 63/63 `.pane` files rebuild to semantically identical models.
- 0 unknown keys / 0 unknown element types in the shipped data.

## `texture_extract.py`

Convert the texture containers used by the guimat:

```bash
python texture_extract.py <file.wyt> [out.tga]
python texture_extract.py <file.wyp> [out.png]
python texture_extract.py <file.wys> [out.dds]
```

- `.wyt` → `.tga`: removes the 4-byte `WT10` magic and appends the TGA 2.0
  `TRUEVISION-XFILE.` signature.
- `.wyp` → `.png`: applies the same delta cipher as `.pane`.
- `.wys` → raw bytes; the DDS header repair was not fully decoded.

## `pane_schema.json`

Machine-readable reference for:

- panel keys;
- common element keys;
- per-type keys;
- enums and value grammars;
- class, constructor and draw addresses.

Use it for an editor's property grid, a validator, or code generation.

## `pane_audit.py`

Audit a full shipped data set and emit a manifest.

```bash
python pane_audit.py --client ../../wydglobal2026
python pane_audit.py --client ../../wydglobal2026 --json manifest.json --strict
```

- Parses every `.pane`/`.panel`, validates it against `pane_schema.json`, and
  reports unknown keys/types.
- Resolves every `TextureAlias` through the guimat, checks that the texture row
  exists and that the file is present under the client root.
- Resolves every `uu_*` key against `Lang/EN/UIString.txt`.
- Reports duplicate element names per panel (they make name binding ambiguous).
- Emits a JSON manifest (`panels`, element counts, hashes, used textures,
  aliases, fonts, issues) intended as input for a C++ binding generator.

Verified against `wydglobal2026`: 64 files, 1,315 element records, 36 used
textures (all present), 212 `uu_` keys (all present), 0 schema issues.

## Suggested extra tooling

- `pane_editor` — load `to-json`, render the tree, edit properties, save back.
- `pane_atlas` — resolve aliases through the guimat and export a texture
  preview.
- `pane_diff` — compare two panels by key rather than by text.
