# Decoded panel JSON

Every shipped panel converted with `tools/pane_json.py to-json`.

The JSON preserves:

- the exact attribute order;
- unknown keys;
- empty values;
- comments (collected at the top-level `comments` array).

This is the most convenient form for an editor, a validator or a runtime
importer. To rebuild a game-loadable file:

```bash
python ../tools/pane_json.py from-json game/HUD.pane.json HUD.pane
```

To rebuild decoded text instead:

```bash
python ../tools/pane_json.py from-json game/HUD.pane.json HUD.txt --text
```
