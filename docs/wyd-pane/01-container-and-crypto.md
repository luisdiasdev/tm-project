# 01 — Container and encryption

## 1. Summary

A `.pane` file is a **UTF-16LE text file** (BOM `FF FE`) whose bytes have been
passed through a fixed, position-dependent **delta transform**.

There is no key, no compression, no header and no checksum. The transform is
symmetric in structure: decryption is a backward pass, encryption a forward
pass.

The transform is implemented in `FUN_00407580` at `0x407580`. The same function
is used for `.wyp` files, which are encrypted PNGs.

Note: the distribution also contains one **plaintext** file with the `.panel`
extension, `UI/Panel/game/ItemOptionStone.panel`. It starts directly with
`FF FE` and is not encrypted. The game only selects the decrypt path for the
exact `.pane` extension, so treat `.panel` as an unencrypted `.pane` source
copy.

## 2. Decryption

Let `C` be the file bytes (ciphertext) and `P` the plaintext. All arithmetic is
modulo 256 (`unsigned char`).

```text
P[0] = C[0] - 14
P[i] = C[i] - C[i-1] - (i % 14)     for i >= 1
```

Important: `C[i-1]` is the **original** byte, not the already-decrypted one. The
game processes from the end of the file backwards, so index `i-1` has not been
modified yet.

```python
def decrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    for i in range(n - 1, 0, -1):
        b[i] = (b[i] - b[i - 1] - (i % 14)) & 0xFF
    if n:
        b[0] = (b[0] - 14) & 0xFF
    return bytes(b)
```

## 3. Encryption

The exact inverse; this is what a repacker needs:

```text
C[0] = P[0] + 14
C[i] = P[i] + C[i-1] + (i % 14)     for i >= 1
```

Here `C[i-1]` is the already-computed ciphertext byte.

```python
def encrypt(data: bytes) -> bytes:
    b = bytearray(data)
    n = len(b)
    if n:
        b[0] = (b[0] + 14) & 0xFF
    for i in range(1, n):
        b[i] = (b[i] + b[i - 1] + (i % 14)) & 0xFF
    return bytes(b)
```

## 4. Ghidra evidence

`FUN_00407580` reads a whole file, runs this loop, and keeps the result in
memory:

```c
while (uVar5 = uVar5 - 1, 0 < (int)uVar5) {
    pcVar1 = (char *)(buf + uVar5);
    *pcVar1 = *pcVar1 + (((char)(uVar5 / 0xe) * '\x0e' -
              *(char *)(buf + -1 + uVar5)) - (char)uVar5);
}
**(char **)(ecx + 4) -= 0xe;
```

Because `(char)(i/14) * 14` is congruent to `i - (i % 14)` modulo 256, the
expression reduces exactly to the formula above. The final statement applies
the `i = 0` case (`P[0] = C[0] - 14`).

The full decompiled function is in
[`reference/pseudocode/0x407580.c`](reference/pseudocode/0x407580.c).

## 5. Round-trip proof

Every shipped `.pane` starts with the same ciphertext prefix, because every
panel starts with the same plaintext header lines:

```text
ciphertext: 0D 0C 5E 61 C6 CB 3F 46 B3 BC 32 3D 56 63 6D 6E EB EE FF 04 …
plaintext:  FF FE 50 00 61 00 6E 00 65 00 6C 00 0D 00 0A 00 7B 00 0D 00 …
            BOM   P     a     n     e     l    CR    LF    {     CR    LF
```

A file is valid when:

1. its length is even;
2. after decryption it starts with `FF FE`;
3. the rest decodes as UTF-16LE;
4. `encrypt(decrypt(file)) == file`.

All 63 shipped `.pane` files pass all four checks. The verification script and
the per-file report are in
[`reference/hexdumps/roundtrip-report.txt`](reference/hexdumps/roundtrip-report.txt).

## 6. Where the loader is called

`FUN_00482c10` (`0x482c10`) is the panel-file entry point:

1. it takes a requested name and appends/checks the `.pane` extension
   (string `".pane"` at `0x883080`);
2. if the extension is `.pane`, it calls `FUN_00407580` to decrypt;
3. it hands the decrypted buffer to the UTF-16 text parser.

`FUN_00482a20` (`0x482a20`) does the analogous job for the `ui/Default.guimat`
resource and other panel resources.

The `.pane` extension string has xrefs at `0x481b15`, `0x482aab` and `0x482cad`.

## 7. Related containers

`FUN_004aea40` (`0x4aea40`) is the texture loader. It dispatches on extension:

| Extension | Branch | Meaning |
|---|---|---|
| `.WYT` | 1 | `WT10` magic + TGA body; the loader strips the magic and appends the TGA 2.0 signature in memory |
| `.WYS` | 2 | custom DDS-like container; repaired in memory |
| `.WYP` | 3 | **same delta cipher as `.pane`**; plaintext is a PNG |

All 4 shipped `.wyp` files decrypt to `89 50 4E 47 0D 0A 1A 0A` (PNG).
