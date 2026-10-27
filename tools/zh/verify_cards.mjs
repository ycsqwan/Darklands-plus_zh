/* verify_cards.mjs -- check the generated mod against the original catalogue.
 *
 * The checks that matter:
 *   1. every payload re-parses to the same card count, and every card keeps its
 *      5 header bytes -- the header holds the text box geometry, a translation
 *      must not move the box;
 *   2. the control-code skeleton is byte-identical: strip the printable runs and
 *      what is left must match the original exactly, so no newline, colour or
 *      hyperlink byte was invented or dropped;
 *   3. the CJK bytes that were inserted are valid UTF-8.
 */
import fs from 'node:fs';
import path from 'node:path';

const args = process.argv.slice(2);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 && i + 1 < args.length ? args[i + 1] : d; };

const MSGFILES = path.resolve(opt('--msgfiles', 'D:\\goggame\\Darklands\\DARKLAND\\MSGFILES'));
const MOD = path.resolve(opt('--mod', 'mods/chinese/MSGFILES'));
const LO = 0x20, HI = 0x7f;

function readCatalogue(file) {
  const b = fs.readFileSync(file);
  const count = b[0] | (b[1] << 8);
  const out = [];
  for (let i = 0; i < count; i++) {
    const at = 2 + i * 24;
    const name = b.subarray(at, at + 12).toString('latin1').replace(/\0+$/, '');
    const len = b.readUInt32LE(at + 16), off = b.readUInt32LE(at + 20);
    out.push({ name, payload: b.subarray(off, off + len) });
  }
  return out;
}

function parseCards(p) {
  const nb = p[0], cards = [];
  let q = 1;
  for (let i = 0; i < nb; i++) {
    const header = Buffer.from(p.subarray(q, q + 5)); q += 5;
    let e = q; while (e < p.length && p[e] !== 0) e++;
    if (e >= p.length) return null;
    cards.push({ header, text: p.subarray(q, e) }); q = e + 1;
  }
  return cards;
}

/* How many bytes of a valid UTF-8 sequence start at i, or 0 if not one.
 * The game's colour codes are 0x80..0x8f and 0xff, all invalid UTF-8 lead
 * bytes, so this cleanly tells a Chinese glyph from a control code -- the same
 * rule dkch's TextDecoder uses. */
function utf8Len(text, i) {
  const c = text[i];
  if (c < 0xc2 || c > 0xf4) return 0;
  let n;
  if (c < 0xe0) n = 2;
  else if (c < 0xf0) n = 3;
  else n = 4;
  if (i + n > text.length) return 0;
  for (let k = 1; k < n; k++) if ((text[i + k] & 0xc0) !== 0x80) return 0;
  return n;
}

const isTextByte = (text, i) => (text[i] >= LO && text[i] <= HI) || utf8Len(text, i) > 0;

/* Everything that is not a text run, kept in order. */
function skeleton(text) {
  const out = [];
  let i = 0;
  while (i < text.length) {
    if (isTextByte(text, i)) { while (i < text.length && isTextByte(text, i)) i += utf8Len(text, i) || 1; out.push('|'); }
    else { out.push(text[i]); i++; }
  }
  return out;
}

/* Text runs, as byte ranges. */
function runsOf(text) {
  const out = [];
  let i = 0;
  while (i < text.length) {
    if (isTextByte(text, i)) {
      const s = i;
      while (i < text.length && isTextByte(text, i)) i += utf8Len(text, i) || 1;
      out.push({ s, e: i });
    } else i++;
  }
  return out;
}

/* Decode the run as UTF-8 and confirm it holds no lone/invalid byte. */
function validUtf8(buf) {
  try {
    const s = new TextDecoder('utf-8', { fatal: true }).decode(buf);
    return !s.includes('\ufffd');
  } catch { return false; }
}

const cat = readCatalogue(MSGFILES);
let files = 0, cards = 0, badParse = 0, badHeader = 0, badSkeleton = 0, badUtf8 = 0, changed = 0;
const samples = [];

for (const e of cat) {
  const dest = path.join(MOD, e.name.replace(/\.MSG$/i, '') + '.MSG');
  if (!fs.existsSync(dest)) { badParse++; continue; }
  const out = fs.readFileSync(dest);
  const a = parseCards(e.payload), b = parseCards(out);
  if (!a || !b) { badParse++; continue; }
  if (a.length !== b.length) { badParse++; continue; }
  files++; cards += a.length;

  for (let i = 0; i < a.length; i++) {
    if (!a[i].header.equals(b[i].header)) badHeader++;
    const sa = skeleton(a[i].text), sb = skeleton(b[i].text);
    if (sa.length !== sb.length || sa.some((v, k) => v !== sb[k])) badSkeleton++;
    const orig = a[i].text, now = b[i].text;
    const ro = runsOf(orig), rn = runsOf(now);
    if (ro.length !== rn.length) { badSkeleton++; continue; }
    for (const r of rn) if (!validUtf8(now.subarray(r.s, r.e))) { badUtf8++; break; }
    if (!orig.equals(now)) {
      changed++;
      if (samples.length < 6 && orig.length > 25) {
        samples.push({ file: e.name, orig: orig.toString('latin1'), now: now.toString('utf8') });
      }
    }
  }
}

console.log(`files checked: ${files}/${cat.length}   cards: ${cards}`);
console.log(`cards changed: ${changed}`);
console.log(`failures -> parse/count: ${badParse}  header: ${badHeader}  skeleton: ${badSkeleton}  utf8: ${badUtf8}`);
if (samples.length) {
  console.log('\n-- samples --');
  for (const s of samples) {
    console.log(`\n[${s.file}]`);
    console.log(`  EN: ${JSON.stringify(s.orig)}`);
    console.log(`  ZH: ${JSON.stringify(s.now)}`);
  }
}
process.exitCode = badParse || badHeader || badSkeleton || badUtf8 ? 1 : 0;
