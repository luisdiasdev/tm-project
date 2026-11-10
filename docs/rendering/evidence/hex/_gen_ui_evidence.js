// Generator for docs/rendering/evidence/hex/ui-*.txt
// Run: node _gen_ui_evidence.js
const fs = require('fs');
const D = 'E:/wyd/test-setup/client_748/';
function hex(b, off, len) {
  const o = [];
  for (let i = 0; i < len; i++) o.push(b[off + i].toString(16).padStart(2, '0'));
  return o.join(' ');
}

/* ---------- 1. UI/minimap.wyt ---------- */
const m = fs.readFileSync(D + 'UI/minimap.wyt');
let s = 'File: ' + D + 'UI/minimap.wyt\nSize: ' + m.length + ' bytes\n\n';
s += '[0x0000..0x003F]\n' + hex(m, 0, 64) + '\n\n';
s += '[0x0000..0x0003] magic         = "' + m.slice(0, 4).toString('latin1') + '"\n';
s += '[0x0004..0x0015] TGA header (18 bytes):\n';
s += '  idLength      = ' + m[4] + '\n';
s += '  colorMapType  = ' + m[5] + '\n';
s += '  imageType     = ' + m[6] + '  (2 = uncompressed true-colour)\n';
s += '  cmapFirst     = ' + m.readUInt16LE(7) + '\n';
s += '  cmapLength    = ' + m.readUInt16LE(9) + '\n';
s += '  cmapDepth     = ' + m[11] + '\n';
s += '  xOrigin       = ' + m.readUInt16LE(12) + '\n';
s += '  yOrigin       = ' + m.readUInt16LE(14) + '\n';
s += '  width         = ' + m.readUInt16LE(16) + '\n';
s += '  height        = ' + m.readUInt16LE(18) + '\n';
s += '  pixelDepth    = ' + m[20] + ' bpp\n';
s += '  descriptor    = 0x' + m[21].toString(16) + '\n\n';
s += 'Payload: width*height*3 = 128*128*3 = ' + (128 * 128 * 3) + ' bytes at 0x0016\n';
s += '4 + 18 + 49152 = ' + (4 + 18 + 49152) + '; file size ' + m.length +
     ' => ' + (m.length - (4 + 18 + 49152)) + ' trailing zero bytes.\n';
s += 'No TGA footer: the loader strips the 4-byte magic and appends "TRUEVISION-XFILE\\0"\n';
s += 'so that D3DX accepts the stream as a TGA (TMFont2.cpp:198-236).\n\n';
s += '[tail, last 30 bytes]\n' + hex(m, m.length - 30, 30) + '\n';
fs.writeFileSync('ui-minimap-wyt.txt', s);

/* ---------- 2. UI/strdef.bin ---------- */
const d = fs.readFileSync(D + 'UI/strdef.bin');
let t = 'File: ' + D + 'UI/strdef.bin\nSize: ' + d.length + ' bytes  (440 strings * 128 bytes + 4-byte checksum = ' +
        (440 * 128 + 4) + ')\nObfuscation: every byte XOR 0x5A (BASE_ReadMessageBin, Basedef.cpp:319-324)\n\n';
t += '[0x0000..0x003F] raw\n' + hex(d, 0, 64) + '\n\n';
const dd = Buffer.from(d.slice(0, 56320).map(x => x ^ 0x5A));
t += '[0x0000..0x003F] after XOR 0x5A\n' + hex(dd, 0, 64) + '\n\n';
t += 'Decoded records 0..3 (128 bytes each, ANSI/CP949, NUL padded):\n';
for (let r = 0; r < 4; r++) {
  const seg = dd.slice(r * 128, (r + 1) * 128);
  const z = seg.indexOf(0);
  t += '  [' + r + '] "' + seg.slice(0, z < 0 ? 128 : z).toString('latin1') + '"\n';
}
t += '\n0xE2 filler before XOR becomes 0xB8 after: record 0 is an unused filler record\n';
t += 'in this English 748 build. Later records contain plain ASCII sentences.\n';
fs.writeFileSync('ui-strdef-xor5a.txt', t);

/* ---------- 3. UI/FieldScene2.bin, panel 289 (minimap) ---------- */
const b = fs.readFileSync(D + 'UI/FieldScene2.bin');
const OFF = 2784;
let f = 'File: ' + D + 'UI/FieldScene2.bin (size ' + b.length + ')\n';
f += 'Panel record at file offset ' + OFF + ' (0x' + OFF.toString(16) + '), preceding int = 0x00000001 (CTRL_TYPE_PANEL)\n\n';
f += '[0x0AE0..0x0B0F]\n' + hex(b, OFF, 48) + '\n\n';
f += 'Decoded as BinPanel (int32 LE, beginning after the 4-byte control type):\n';
const names = ['nID', 'nParentID', 'nTextureSetIndex', 'nStartX', 'nStartY', 'nWidth', 'nHeight', 'nColor', 'nFillType', 'nPickable'];
for (let k = 0; k < 10; k++) {
  const v = b.readInt32LE(OFF + 4 + k * 4);
  let line = '  +0x' + (k * 4).toString(16).padStart(2, '0') + '  ' + names[k].padEnd(18) + ' = ' +
             String(v).padStart(12) + '  0x' + (v >>> 0).toString(16).toUpperCase().padStart(8, '0');
  if (names[k] === 'nColor') {
    line += '  A=0x' + (v >>> 24).toString(16).padStart(2, '0') +
            ' R=0x' + ((v >>> 16) & 255).toString(16).padStart(2, '0') +
            ' G=0x' + ((v >>> 8) & 255).toString(16).padStart(2, '0') +
            ' B=0x' + (v & 255).toString(16).padStart(2, '0');
  }
  f += line + '\n';
}
f += '\nNotes:\n';
f += '  nID 289 = TMP_MINIMAP (ResourceControl.h:587) -> TMFieldScene.cpp:1980\n';
f += '  nTextureSetIndex -11 -> abs() -> texture set 11 "N_minimap" (ui-uitxtureset-11.txt)\n';
f += '  nFillType 4 = RENDER_IMAGE_STRETCH (Enums.h:10)\n';
f += '  nColor 0x88FFFFFF = 53% white modulation of the minimap blit\n';
f += 'Next 4 bytes after the record (offset ' + (OFF + 44) + ') = 0x' +
     b.readUInt32LE(OFF + 44).toString(16) + ', which is not a valid CONTROL_TYPE:\n';
f += 'see the chapter text ("open question: 748 panel record length").\n';
fs.writeFileSync('ui-fieldscene2-panel289.txt', f);

/* ---------- 4. UI/UITextureSetList.txt, set 11 ---------- */
const txt = fs.readFileSync(D + 'UI/UITextureSetList.txt', 'latin1').split(/\r?\n/);
let g = 'File: ' + D + 'UI/UITextureSetList.txt  (parsed by TextureManager::InitUITextureSetList, TextureManager.cpp:426-464)\n';
g += 'Format per set:  "[Name]" / "SetIndex: N" / "ItemCount: K" / K lines of\n';
g += '                 texIndex,startX,startY,width,height,destX,destY\n\n';
for (const want of [0, 11, 137, 291]) {
  for (let i = 0; i < txt.length; i++) {
    if (new RegExp('^SetIndex:\\s*' + want + '\\s*$').test(txt[i])) {
      const st = i - 1;
      const cnt = parseInt(txt[i + 1].replace(/[^0-9]/g, ''), 10);
      const end = Math.min(st + 3 + Math.min(cnt, 6), txt.length);
      g += '--- line ' + (st + 1) + ' ---\n';
      for (let j = st; j < end; j++) g += '  ' + txt[j] + '\n';
      if (cnt > 6) g += '  ... (' + (cnt - 6) + ' more coord lines)\n';
      g += '\n';
      break;
    }
  }
}
fs.writeFileSync('ui-uitxturesetlist-sets.txt', g);

/* ---------- 5. UI/UIString.txt ---------- */
const u = fs.readFileSync(D + 'UI/UIString.txt');
let v2 = 'File: ' + D + 'UI/UIString.txt  (parsed by ReadUIString, Basedef.cpp:1178-1199)\n';
v2 += 'Format: "<index><TAB><name>" lines; the name is copied into g_UIString[index][64]\n';
v2 += 'and later copied into .bin controls via binButtonData.nStringIndex.\n\n';
v2 += '[0x0000..0x00BF]\n' + hex(u, 0, 192) + '\n\n';
v2 += 'ASCII:\n' + u.slice(0, 192).toString('latin1') + '\n';
fs.writeFileSync('ui-uistring-head.txt', v2);
console.log('wrote 5 files');
