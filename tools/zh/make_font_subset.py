"""Build the CJK font subset used by the Chinese build.

    python tools/zh/make_font_subset.py --src "WenQuanYi Bitmap Song 12px.ttf"

Collects every character that actually appears in the translated material
(cards, translation table, data tables), adds the small ranges the renderer
always needs, then subsets the source font down to just those glyphs.

The source font is WenQuanYi Bitmap Song 12px (GPL v2 + font embedding
exception).  It is not committed: point --src at your own copy.
"""
import argparse
import collections
import os
import sys

from fontTools import subset
from fontTools.ttLib import TTFont

# the console on Windows defaults to GBK; keep diagnostics from crashing
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
OUT = os.path.join(ROOT, 'port', 'third_party', 'fonts', 'wqy-12px-cjk.ttf')

# Ranges taken wholesale, whether or not the text uses them: ASCII, Latin-1,
# general punctuation (dashes, curly quotes, ellipsis), arrows, box drawing,
# geometric shapes, CJK symbols and fullwidth forms.
# Keep in sync with ranges_cjk in port/ui/ui.cpp -- a character missing from
# there is never rasterised, and shows up as '?' even when the glyph exists.
EXTRA_RANGES = [
    (0x20, 0x7e), (0xa0, 0xff),
    (0x2000, 0x206f), (0x2190, 0x21ff), (0x2500, 0x257f),
    (0x25a0, 0x25ff), (0x2600, 0x26ff), (0x2700, 0x27bf),
    (0x3000, 0x303f), (0xfe30, 0xfe4f), (0xff00, 0xffef),
]

# Ranges the renderer is able to display.  Used only to classify what is
# missing; the Han blocks are filled from the text, not taken wholesale.
RENDER_RANGES = EXTRA_RANGES + [
    (0x3400, 0x4dbf), (0x4e00, 0x9fff), (0xf900, 0xfaff),
]

SOURCES = [
    os.path.join(ROOT, 'mods', 'chinese', 'ui_zh.tsv'),
    os.path.join(ROOT, 'mods', 'chinese', 'mod.txt'),
    os.path.join(ROOT, 'mods', 'chinese', 'untranslated.txt'),
    # this fork's own run table: the runs the shared table gets wrong
    os.path.join(ROOT, 'tools', 'zh', 'overrides.tsv'),
    os.path.join(ROOT, 'port', 'ui', 'gamedata.cpp'),
]


def collect():
    cnt = collections.Counter()

    def add(path):
        if not os.path.isfile(path):
            return
        with open(path, encoding='utf-8', errors='replace') as fh:
            for ch in fh.read():
                o = ord(ch)
                # card files carry the game's own colour/format codes as raw
                # bytes >= 0x80; they are not text and must not reach the font
                if o < 0x20 or o == 0x7f or o == 0xfffd:
                    continue
                cnt[ch] += 1

    msgdir = os.path.join(ROOT, 'mods', 'chinese', 'MSGFILES')
    if os.path.isdir(msgdir):
        for fn in sorted(os.listdir(msgdir)):
            add(os.path.join(msgdir, fn))
    for p in SOURCES:
        add(p)
    return cnt


def cmap_of(path):
    f = TTFont(path, fontNumber=0)
    cov = set()
    for t in f['cmap'].tables:
        cov |= set(t.cmap.keys())
    return f, cov


def in_ranges(cp, ranges):
    return any(a <= cp <= b for a, b in ranges)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', required=True, help='WenQuanYi Bitmap Song 12px .ttf')
    ap.add_argument('--out', default=OUT)
    args = ap.parse_args()

    if not os.path.isfile(args.src):
        sys.exit('source font not found: %s' % args.src)

    cnt = collect()
    used = {ord(c) for c in cnt}
    extra = {c for a, b in EXTRA_RANGES for c in range(a, b + 1)}
    uni = sorted(used | extra)
    print('used chars = %d, plus fixed ranges = %d' % (len(used), len(uni)))

    opts = subset.Options()
    opts.layout_features = ['*']
    opts.name_IDs = ['*']
    opts.name_legacy = True
    opts.name_languages = ['*']
    opts.notdef_outline = True
    opts.recalc_bounds = True
    opts.drop_tables = ['GSUB', 'GPOS', 'GDEF', 'BASE', 'vhea', 'vmtx', 'VORG', 'DSIG']
    opts.passthrough_tables = False
    opts.recalc_timestamp = False
    opts.hinting = True

    font, _ = cmap_of(args.src)
    ss = subset.Subsetter(options=opts)
    ss.populate(unicodes=uni)
    ss.subset(font)
    subset.save_font(font, args.out, opts)

    # verify what came out
    out_font, cov = cmap_of(args.out)
    _, src_cov = cmap_of(args.src)

    need_han = {ord(c) for c in cnt if 0x4e00 <= ord(c) <= 0x9fff}
    miss_han = sorted(need_han - cov)
    renderable = [ord(c) for c in cnt if in_ranges(ord(c), RENDER_RANGES)]
    # dropped by the subset even though the source font has it -> our bug
    miss_dropped = sorted(x for x in renderable if x not in cov and x in src_cov)
    # absent from the source font entirely -> needs a fallback font
    miss_absent = sorted(x for x in renderable if x not in cov and x not in src_cov)
    beyond = sorted(x for x in used if not in_ranges(x, RENDER_RANGES))

    def fmt(xs):
        return ' '.join('U+%04X' % x for x in xs)

    print('OUT=%s size=%d' % (args.out, os.path.getsize(args.out)))
    print('cmap=%d numGlyphs=%d' % (len(cov), out_font['maxp'].numGlyphs))
    print('need Han=%d missing Han=%d' % (len(need_han), len(miss_han)))
    if miss_han:
        print('  MISSING HAN: ' + fmt(miss_han))
    print('need used=%d dropped-by-subset=%d absent-from-source=%d'
          % (len(used), len(miss_dropped), len(miss_absent)))
    if miss_dropped:
        print('  DROPPED (widen EXTRA_RANGES): ' + fmt(miss_dropped))
    if miss_absent:
        print('  ABSENT FROM SOURCE FONT (needs a fallback font): ' + fmt(miss_absent))
    if beyond:
        print('  outside render ranges, will not be drawn: ' + fmt(beyond))
    print('full ASCII 0x20-0x7E present: %s'
          % all(c in cov for c in range(0x20, 0x7f)))
    return 1 if (miss_han or miss_dropped) else 0


if __name__ == '__main__':
    sys.exit(main())
