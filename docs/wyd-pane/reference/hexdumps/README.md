# Hexdumps and round-trip report

- `notice-encrypted-first256.txt` — shipped ciphertext.
- `notice-decrypted-first256.txt` — plaintext (UTF-16LE BOM + `Panel`).
- `hud-encrypted-first256.txt` / `hud-decrypted-first256.txt` — second sample.
- `roundtrip-report.txt` — every shipped `.pane` / `.panel`, its size, SHA-256
  prefix, BOM check and `encrypt(decrypt(x)) == x` result.

Observation: all `.pane` files share the same ciphertext prefix because all of
them start with the same plaintext header lines. Differences begin at the first
panel-specific value (`OffsetX`/`OffsetY`), typically around offset `0xAE`.
