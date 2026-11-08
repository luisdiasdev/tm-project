# Samples

- `notice.encrypted.pane` — shipped encrypted file.
- `notice.decrypted.txt` — same file after the delta transform and UTF-16LE
  decode.
- `logo.encrypted.pane` — a second shipped file; its first lines differ only in
  the `OffsetX` value.
- `logo.decrypted.txt` — decoded text.
- `notice.json`, `logo.json` — the same panels parsed into the ordered JSON
  model used by `tools/pane_json.py`.

Use these to verify a decoder:

```text
python ../tools/pane_codec.py decode notice.encrypted.pane /tmp/notice.txt
python ../tools/pane_codec.py encode /tmp/notice.txt /tmp/notice.pane
# /tmp/notice.pane must be byte-identical to notice.encrypted.pane
```
