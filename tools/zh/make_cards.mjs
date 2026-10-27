#!/usr/bin/env node
/* make_cards.mjs -- build a Darklands+ Chinese card-text mod.
 *
 * The card texts live in MSGFILES, a catalogue of 419 `$*.MSG` files, 3756 cards.
 * Each card is: 5 header bytes (text box top, left, 0, right edge, 0) then the
 * text bytes then a 0 terminator. The text is a byte string: bytes 0x20..0x7F are
 * printable ASCII, everything else is a control code (newline, colour, hyperlink,
 * icon) that must survive untouched.
 *
 * A run is a maximal sequence of printable bytes. Runs are the unit of translation:
 * the source table maps an English run to its Chinese, and a run is replaced whole.
 * Runs left out of the table stay English on purpose (option bullets "...", numbers).
 *
 * Because the replacement is a whole run, `$PlaceName` and friends survive: the game
 * expands variables after the text is read, so the table keys keep the `$` names.
 *
 * Output: one `.MSG` per catalogue entry, holding the translated payload. The mod
 * system reads a `.MSG` in `mods/<mod>/MSGFILES/` as the raw payload, so this is
 * byte-exact and needs no escaping (a `.TXT` would be escaped 4x larger).
 *
 * Usage:
 *   node tools/zh/make_cards.mjs [--msgfiles DIR] [--table FILE] [--out DIR]
 *                                [--report FILE] [--only NAME] [--dry]
 *
 * Defaults read the dkch project's run table and the installed game; neither is
 * modified. The dkch folder is only ever read.
 */
import fs from 'node:fs';
import path from 'node:path';

const args = process.argv.slice(2);
function opt(name, def) {
  const i = args.indexOf(name);
  return i >= 0 && i + 1 < args.length ? args[i + 1] : def;
}
const flag = (n) => args.includes(n);

const MSGFILES = path.resolve(opt('--msgfiles', 'D:\\goggame\\Darklands\\DARKLAND\\MSGFILES'));
const TABLE = path.resolve(opt('--table', 'D:\\dkch\\zh\\work\\translations.tsv'));
const OUT = path.resolve(opt('--out', 'mods/chinese/MSGFILES'));
const REPORT = path.resolve(opt('--report', 'mods/chinese/untranslated.txt'));
const ONLY = opt('--only', null);
const DRY = flag('--dry');

/* ---------------------------------------------------------------- table ---- */

function readTable(file) {
  const raw = fs.readFileSync(file);
  let text = raw.toString('utf8');
  if (text.charCodeAt(0) === 0xfeff) text = text.slice(1); // BOM
  const map = new Map();
  const lines = text.split(/\r?\n/);
  let head = null;
  for (const line of lines) {
    if (!line) continue;
    const tab = line.indexOf('\t');
    if (tab < 0) continue;
    const en = line.slice(0, tab);
    const zh = line.slice(tab + 1);
    if (!head) { head = { en, zh }; continue; }
    if (!en) continue;
    map.set(en, zh);
  }
  return { map, head };
}

/* ------------------------------------------------------------- catalogue --- */

function readCatalogue(file) {
  const b = fs.readFileSync(file);
  if (b.length < 2) throw new Error(`${file}: too small`);
  const count = b[0] | (b[1] << 8);
  const out = [];
  for (let i = 0; i < count; i++) {
    const at = 2 + i * 24;
    if (at + 24 > b.length) break;
    const name = b.subarray(at, at + 12).toString('latin1').replace(/\0+$/, '');
    const len = b.readUInt32LE(at + 16);
    const off = b.readUInt32LE(at + 20);
    if (off < 0 || len < 0 || off + len > b.length) continue;
    out.push({ name, payload: b.subarray(off, off + len) });
  }
  return out;
}

/* A card file payload: [nb:u8] then nb cards of [5 header bytes][text][0]. */
function parseCards(payload) {
  if (!payload.length) return null;
  const nb = payload[0];
  const cards = [];
  let p = 1;
  for (let i = 0; i < nb; i++) {
    if (p + 5 > payload.length) return null;
    const header = Buffer.from(payload.subarray(p, p + 5));
    p += 5;
    let q = p;
    while (q < payload.length && payload[q] !== 0) q++;
    if (q >= payload.length) return null;
    cards.push({ header, text: payload.subarray(p, q) });
    p = q + 1;
  }
  return { nb, cards, end: p };
}

function buildPayload(cards) {
  const parts = [Buffer.from([cards.length])];
  for (const c of cards) {
    parts.push(c.header);
    parts.push(c.text);
    parts.push(Buffer.from([0]));
  }
  return Buffer.concat(parts);
}

/* ------------------------------------------------------------- translate --- */

const PRINT_LO = 0x20, PRINT_HI = 0x7f;

/* Split into runs of printable bytes; everything else is a one-byte control. */
function* runs(text) {
  let i = 0;
  while (i < text.length) {
    const c = text[i];
    if (c >= PRINT_LO && c <= PRINT_HI) {
      let j = i;
      while (j < text.length && text[j] >= PRINT_LO && text[j] <= PRINT_HI) j++;
      yield { kind: 'run', buf: text.subarray(i, j) };
      i = j;
    } else {
      yield { kind: 'ctrl', buf: text.subarray(i, i + 1) };
      i++;
    }
  }
}

function translate(text, map) {
  const parts = [];
  const missing = [];
  let replaced = 0;
  for (const r of runs(text)) {
    if (r.kind !== 'run') { parts.push(r.buf); continue; }
    const en = r.buf.toString('latin1');
    const zh = map.get(en);
    if (zh !== undefined) {
      parts.push(Buffer.from(zh, 'utf8'));
      replaced++;
    } else {
      parts.push(r.buf);
      if (en.trim()) missing.push(en);
    }
  }
  return { buf: Buffer.concat(parts), missing, replaced };
}

/* A CJK ideograph in UTF-8 starts with 0xE3..0xE9 (U+3400..U+9FFF) or 0xEF. */
function hasCjk(buf) {
  for (const b of buf) if (b >= 0xe3 && b <= 0xe9) return true;
  return false;
}

/* ----------------------------------------------------------------- main ---- */

const { map, head } = readTable(TABLE);
console.log(`table: ${TABLE}`);
console.log(`  header: ${JSON.stringify(head.en)} => ${JSON.stringify(head.zh)}`);
console.log(`  ${map.size} runs`);

const cat = readCatalogue(MSGFILES);
console.log(`catalogue: ${MSGFILES}  ${cat.length} entries`);

if (!DRY) fs.mkdirSync(OUT, { recursive: true });

const missingAll = new Map(); // run -> count
const seenAll = new Map(); // run -> count, every run in the card text
const hist = new Map(); // runs-per-card -> cards
let files = 0, cards = 0, filesTranslated = 0, runsHit = 0, runsMiss = 0, bytesIn = 0, bytesOut = 0;
let cardsEmpty = 0, cardsBulletOnly = 0, cardsWithCjk = 0;
const skipped = [];

for (const e of cat) {
  if (ONLY && e.name.toUpperCase() !== ONLY.toUpperCase()) continue;
  const parsed = parseCards(e.payload);
  if (!parsed) { skipped.push(e.name); continue; }
  const outCards = [];
  let anyZh = false, cardRuns = 0, cardBullets = 0;
  for (const c of parsed.cards) {
    const { buf, missing, replaced } = translate(c.text, map);
    if (hasCjk(buf)) anyZh = true;
    runsHit += replaced;
    runsMiss += missing.length;
    for (const m of missing) missingAll.set(m, (missingAll.get(m) || 0) + 1);
    let n = 0, bullets = 0;
    for (const r of runs(c.text)) {
      if (r.kind !== 'run') continue;
      const en = r.buf.toString('latin1');
      seenAll.set(en, (seenAll.get(en) || 0) + 1);
      n++;
      if (en === '...') bullets++;
    }
    cardRuns += n; cardBullets += bullets;
    hist.set(n, (hist.get(n) || 0) + 1);
    if (n === 0) cardsEmpty++;
    else if (bullets === n) cardsBulletOnly++;
    if (hasCjk(buf)) cardsWithCjk++;
    outCards.push({ header: c.header, text: buf });
  }
  const payload = buildPayload(outCards);
  files++; cards += parsed.cards.length;
  if (anyZh) filesTranslated++;
  bytesIn += e.payload.length; bytesOut += payload.length;
  if (!DRY) {
    const dest = path.join(OUT, e.name.replace(/\.MSG$/i, '') + '.MSG');
    fs.writeFileSync(dest, payload);
  }
}

console.log(`\nfiles written: ${files}${DRY ? ' (dry run)' : ''} -> ${OUT}`);
if (skipped.length) console.log(`skipped (not a card file): ${skipped.length} ${skipped.slice(0, 5).join(', ')}${skipped.length > 5 ? '...' : ''}`);
console.log(`cards: ${cards}  cards with Chinese: ${cardsWithCjk}  files with Chinese: ${filesTranslated}/${files}  no printable run: ${cardsEmpty}  bullet-only: ${cardsBulletOnly}`);
console.log(`runs: ${runsHit} replaced, ${runsMiss} left English`);
console.log(`distinct runs in card text: ${seenAll.size}`);
console.log(`payload bytes: ${bytesIn} -> ${bytesOut} (${(bytesOut / bytesIn).toFixed(2)}x)`);

const unseen = [...map.keys()].filter((k) => !seenAll.has(k));
console.log(`table runs never seen in card text: ${unseen.length}`);
if (unseen.length && unseen.length <= 30) for (const k of unseen) console.log(`  ${JSON.stringify(k)}`);

const topHist = [...hist.entries()].sort((a, b) => a[0] - b[0]).slice(0, 12);
console.log(`runs-per-card histogram (runs: cards): ${topHist.map(([k, v]) => `${k}:${v}`).join(' ')}`);

if (missingAll.size) {
  const top = [...missingAll.entries()].sort((a, b) => b[1] - a[1]);
  console.log(`\ndistinct untranslated runs: ${top.length}`);
  if (!DRY) {
    const body = top.map(([r, n]) => `${n}\t${r}`).join('\n');
    fs.mkdirSync(path.dirname(REPORT), { recursive: true });
    fs.writeFileSync(REPORT, body + '\n');
    console.log(`report: ${REPORT}`);
  } else {
    for (const [r, n] of top.slice(0, 40)) console.log(`  ${n}\t${r}`);
  }
}
