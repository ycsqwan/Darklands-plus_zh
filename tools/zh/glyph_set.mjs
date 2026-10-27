/* glyph_set.mjs -- which characters a Chinese build must put in the font atlas.
 *
 * The ImGui atlas is the one choke point: every Chinese glyph drawn has to be baked
 * into it. This counts the distinct characters the translated cards need, and what
 * that costs at the size the game's text is baked at (34 px, 2x oversampled for the
 * normal set, 1x for the big-screen set).
 */
import fs from 'node:fs';
import path from 'node:path';

const args = process.argv.slice(2);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 && i + 1 < args.length ? args[i + 1] : d; };
const MOD = path.resolve(opt('--mod', 'mods/chinese/MSGFILES'));

const chars = new Map();          // char -> count
const LO = 0x20, HI = 0x7f;

function utf8Len(b, i) {
  const c = b[i];
  if (c < 0xc2 || c > 0xf4) return 0;
  const n = c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
  if (i + n > b.length) return 0;
  for (let k = 1; k < n; k++) if ((b[i + k] & 0xc0) !== 0x80) return 0;
  return n;
}

function walk(buf) {
  let i = 0;
  while (i < buf.length) {
    const n = utf8Len(buf, i);
    if (n) {
      const s = buf.subarray(i, i + n).toString('utf8');
      for (const ch of s) chars.set(ch, (chars.get(ch) || 0) + 1);
      i += n;
    } else i++;
  }
}

for (const f of fs.readdirSync(MOD)) {
  if (!f.toLowerCase().endsWith('.msg')) continue;
  walk(fs.readFileSync(path.join(MOD, f)));
}

const all = [...chars.entries()];
const cjk = all.filter(([c]) => {
  const cp = c.codePointAt(0);
  return (cp >= 0x3400 && cp <= 0x4dbf) || (cp >= 0x4e00 && cp <= 0x9fff) ||
         (cp >= 0xf900 && cp <= 0xfaff) || (cp >= 0x3000 && cp <= 0x303f) ||
         (cp >= 0xff00 && cp <= 0xffef) || (cp >= 0x2000 && cp <= 0x206f);
}).sort((a, b) => b[1] - a[1]);

console.log(`distinct characters in the translated cards: ${all.length}`);
console.log(`  of them CJK + CJK punctuation: ${cjk.length}`);

const px = 34;
function cost(glyphs, size, os, sets) {
  const cell = Math.ceil(size * os) + 2;                 // oversampled raster + padding
  const px2 = glyphs * cell * cell;
  const side = Math.ceil(Math.sqrt(px2 * sets));
  return { cell, total: px2 * sets, side };
}
for (const [label, size, os, sets] of [['normal set, 8 typefaces, 2x', px, 2, 8], ['normal set, 1 typeface, 2x', px, 2, 1],
                                       ['normal set, 8 typefaces, 1x', px, 1, 8], ['normal set, 1 typeface, 1x', px, 1, 1]]) {
  const c = cost(cjk.length, size, os, sets);
  console.log(`  ${label}: cell ${c.cell}px -> ${(c.total / 1e6).toFixed(1)} M pixels -> atlas ${c.side}px square`);
}
console.log(`\nImGui's default atlas is 4096x4096 = 16.8 M pixels; MaxTextureSize is what the GPU allows.`);
console.log(`\nmost frequent: ${cjk.slice(0, 40).map(([c, n]) => c).join('')}`);
console.log(`rarest: ${cjk.slice(-20).map(([c, n]) => c).join('')}`);
