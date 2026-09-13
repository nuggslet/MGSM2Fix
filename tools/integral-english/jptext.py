"""Turn the game's font codes into Japanese you can read.

`audit_text.game_text` prints a string as `<8113><812E>` because this game
stores **font indices**, not Shift-JIS: there is no encoding to decode and no
table anywhere on the disc that maps a code to a character. `rendertext.py`
solved reading ONE string by drawing it. This solves reading them in bulk.

    py jptext.py                     work/japanese-inventory.tsv -> ...-readable.tsv
    py jptext.py --hex 8213825382288 24d
    py jptext.py --sample 20         print 20 of the longest lines, decoded

HOW THE TABLE WAS BUILT, AND WHY YOU CAN TRUST IT

*Kana are arithmetic, not a transcription.* Rendering the contiguous ranges
showed the two kana banks are laid out in the standard order, one code per
character including the small and voiced forms, so:

    0x8101 + i  ->  hiragana, from U+3041 (ぁ)
    0x8201 + i  ->  katakana, from U+30A1 (ァ)

Checked against strings whose reading was already known:
`<8213><8253><8228><824D><9006><8249>` comes out コントローラ, and the
commentary line `<8115><8149><812B><8113><812E>...` comes out さらにこの
インテグラル. That is 69% of every glyph used in the inventory, exact.

*The `0x90` bank is a hand transcription, and it verifies itself.* 238 glyphs,
read off labelled contact sheets (`work/lab_90_*.png`). The check that it is
right is that consecutive codes spell real words used by the game:

    906A 906B 906C 906D 906E   地雷探知機   the Mine Detector
    9059 905A 905B 905C 905D   精神安定剤   Diazepam
    9055 9056 9057             風邪薬       cold medicine
    904A 904B 904C 904D        光学迷彩     optical camouflage

A mis-transcribed glyph would break a word, which is a far stronger test than
squinting at a 12x12 bitmap twice.

WHAT IS STILL UNREADABLE - AND WHERE IT LIVES, WHICH IS NOW KNOWN

Codes from `0x9600` up are "bank 1", which `rendertext.glyph` refuses with
"bank 1 lives elsewhere; not located". It is not in `font.res`, and it is not
appended after bank 0 either - glyph index 392 lands back among bank 0's own
kanji, which is how that guess was ruled out.

**Bank 1 is a per-block glyph table, and the block carries it.** A `.gcx`
script ends with a font blob (`parse_gcx` has always read it as `font`), and
`0x9A01 + i` indexes it directly. Proven, not inferred: `abst`'s caption
decodes as 作戦⟪9A01⟫⟪9A02⟫ and glyphs 0 and 1 of that blob are 記 and 録 -
作戦記録. The glyphs after them are 諸島沖孤廃棄占拠等, which is the mission
log's own vocabulary in the order the text first needs it.

That also explains why one code means different characters in different places:
`⟪9A01⟫` is 記 in `abst`, 端 in `s07b` (コントローラ端子1) and 年 in `roll`
(1980年代). The code is an index into whatever block is loaded, so there is no
global table to build and never was.

`RADIO.DAT` works the same way. **Correction, 2026-09-10:** this file used to
say its bank-1 codes "never leave `0x9601`-`0x96FF` - 255 entries". They do.
A commentary fragment's blob holds up to **441** glyphs and the codes run on
into `0x97xx`; the claim only looked true because `japanese-inventory.tsv`'s
scanner silently dropped every code it did not recognise, `0x97xx` included.
And the index is not `code - 0x9601` but `zen_index` - see
`radiomap.bank1_index`, which is the single copy of that rule.

**It is all readable now.** `bank1-glyphs.tsv` beside this file names 1,214
shapes, every one read against a decoded sentence, and the export has zero
unresolved codes. Read `RADIO.DAT` through `radiotext.py`, not the inventory.
"""
import argparse
import hashlib
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from workdir import WORK

CODE = re.compile(r'<([0-9A-F]{4})>')

# bank 0x90: punctuation, then the kanji the game's own UI text needs, in the
# order those strings first used them. Read from work/lab_90_*.png.
GLYPH_90 = {
    0x9001: ' ', 0x9002: '、', 0x9003: '。', 0x9004: '，', 0x9005: '々',
    0x9006: 'ー', 0x9007: '〜', 0x9008: '‥', 0x9009: '（', 0x900A: '）',
    0x900B: '「', 0x900C: '」', 0x900D: '『', 0x900E: '』',
    0x9010: '％', 0x9011: '＆',
    0x9016: '／', 0x9017: '…', 0x9018: '×', 0x901B: '○', 0x901F: '愛',
    0x9020: '飲', 0x9021: '双', 0x9022: '眼', 0x9023: '鏡', 0x9025: '段',
    0x9026: '行', 0x9027: '書', 0x9028: '渓', 0x9029: '谷', 0x902A: '核',
    0x902B: '保', 0x902C: '存', 0x902D: '庫', 0x902E: '大', 0x902F: '雪',
    0x9030: '原', 0x9031: '暗', 0x9032: '視', 0x9033: '闇', 0x9034: '見',
    0x9035: '赤', 0x9036: '外', 0x9037: '線', 0x9038: '熱', 0x9039: '持',
    0x903A: '物', 0x903B: '体', 0x903C: '識', 0x903D: '別', 0x903E: '有',
    0x903F: '毒', 0x9040: '身', 0x9041: '守', 0x9042: '半', 0x9043: '減',
    0x9044: '調', 0x9045: '味', 0x9046: '料', 0x9047: '産', 0x9048: '使',
    0x9049: '用', 0x904A: '光', 0x904B: '学', 0x904C: '迷', 0x904D: '彩',
    0x904E: '隠', 0x904F: '撮', 0x9050: '影', 0x9051: '力', 0x9052: '回',
    0x9053: '復', 0x9054: '開', 0x9055: '風', 0x9056: '邪', 0x9057: '薬',
    0x9058: '止', 0x9059: '精', 0x905A: '神', 0x905B: '安', 0x905C: '定',
    0x905D: '剤', 0x905E: '手', 0x905F: '温', 0x9060: '度', 0x9061: '形',
    0x9062: '状', 0x9063: '変', 0x9064: '化', 0x9065: '必', 0x9066: '要',
    0x9067: '不', 0x9068: '明', 0x9069: '捨', 0x906A: '地', 0x906B: '雷',
    0x906C: '探', 0x906D: '知', 0x906E: '機', 0x906F: '写', 0x9070: '演',
    0x9071: '習', 0x9072: '納', 0x9073: '長', 0x9074: '丈', 0x9075: '夫',
    0x9076: '麻', 0x9078: '匂', 0x9079: '凍', 0x907A: '付', 0x907B: '押',
    0x907C: '構', 0x907D: '離', 0x907E: '発', 0x907F: '砲', 0x9080: '連',
    0x9081: '続', 0x9082: '破', 0x9083: '片', 0x9084: '榴', 0x9085: '弾',
    0x9086: '前', 0x9087: '方', 0x9088: '投', 0x9089: '無', 0x908A: '誘',
    0x908B: '導', 0x908C: '射', 0x908D: '十', 0x908E: '字', 0x908F: '操',
    0x9090: '作', 0x9091: '対', 0x9092: '空', 0x9093: '装', 0x9094: '備',
    0x9095: '照', 0x9096: '準', 0x9097: '移', 0x9098: '動', 0x9099: '人',
    0x909A: '指', 0x909B: '向', 0x909C: '性', 0x909D: '設', 0x909E: '置',
    0x909F: '敵', 0x90A0: '接', 0x90A1: '近', 0x90A2: '爆', 0x90A3: '閃',
    0x90A4: '界', 0x90A5: '奪', 0x90A6: '電', 0x90A7: '子', 0x90A8: '妨',
    0x90A9: '害', 0x90AA: '狙', 0x90AB: '撃', 0x90AC: '入', 0x90AD: '応',
    0x90AE: '答', 0x90AF: '願', 0x90B0: '煙', 0x90B1: '草', 0x90B2: '吸',
    0x90B3: '過', 0x90B4: '注', 0x90B5: '意', 0x90B6: '可', 0x90B7: '倍',
    0x90B8: '率', 0x90B9: '箱', 0x90BA: '頭', 0x90BB: '錬', 0x90BC: '的',
    0x90BD: '増', 0x90BE: '幅', 0x90BF: '映', 0x90C0: '像', 0x90C1: '器',
    0x90C2: '材', 0x90C3: '場', 0x90C4: '所', 0x90C5: '確', 0x90C6: '事',
    0x90C7: '周', 0x90C8: '囲', 0x90C9: '源', 0x90CA: '生', 0x90CB: '兵',
    0x90CC: '戦', 0x90CD: '中', 0x90CE: '少', 0x90CF: '遅', 0x90D0: '繊',
    0x90D1: '維', 0x90D2: '防', 0x90D3: '屈', 0x90D4: '曲', 0x90D5: '者',
    0x90D6: '透', 0x90D7: '軍', 0x90D8: '携', 0x90D9: '帯', 0x90DB: '食',
    0x90DC: '時', 0x90DD: '総', 0x90DE: '合', 0x90DF: '感', 0x90E0: '冒',
    0x90E1: '治', 0x90E2: '服', 0x90E3: '一', 0x90E4: '間', 0x90E5: '起',
    0x90E6: '緊', 0x90E7: '急', 0x90E8: '解', 0x90E9: '除', 0x90EA: '扉',
    0x90EB: '敷', 0x90EC: '位', 0x90ED: '表', 0x90EE: '示', 0x90EF: '製',
    0x90F0: '彼', 0x90F1: '女', 0x90F2: '専', 0x90F3: '音', 0x90F4: '抑',
    0x90F5: '冷', 0x90F6: '却', 0x90F7: '暖', 0x90F8: '全', 0x90F9: '抜',
    0x90FA: '約', 0x90FB: '秒', 0x90FC: '後', 0x90FD: '範', 0x90FE: '特',
    0x90FF: '殊',
}

# bank 0x91: the second kanji page. Four were identified from the UI text
# (ご了承下さい, 上書き保存, 主観移動可能モード, 期限は1週間); the rest came out
# of RADIO.DAT's commentary once `radiotext.py` started reading the text the
# inventory's scanner had been dropping - each is settled by a sentence, and
# they are the reason it was dropping it (an unrecognised code ends a run).
GLYPH_91 = {
    0x9101: '気', 0x9102: '絶', 0x9103: '安', 0x9104: '属',
    0x9106: '布', 0x9107: '完', 0x9108: '了', 0x910B: '上',
    0x910C: '自', 0x910D: '拳', 0x910E: '銃', 0x910F: '能',
    0x9110: '初', 0x9111: '期',
}

# Bank 1 is PER-BLOCK, so it is keyed by (stage, code): the same code is a
# different character in a different stage - 記 in `abst`, 端 in `s07b`,
# 年 in `roll`. Identified by an OCR shortlist plus the decoded context around
# each use: the staff roll pinned 二/万/五/千 because it is MGS1's own opening
# text, and 変更内容を上書き保存 pinned 更/内/容/上. 82 of the 90 shapes the
# remaining Japanese uses are identified; the other 8 stay as codes.
BANK1 = {
    ('abst', 0x9A01): '記',
    ('abst', 0x9A02): '録',
    ('abst', 0x9A03): '参',
    ('abst', 0x9A6F): '待',
    ('camera', 0x9A01): '更',
    ('camera', 0x9A02): '内',
    ('camera', 0x9A03): '容',
    ('ending', 0x9A01): '最',
    ('ending', 0x9A02): '終',
    ('endingr', 0x9A01): '最',
    ('endingr', 0x9A02): '終',
    ('option', 0x9A03): '画',
    ('option', 0x9A04): '面',
    ('option', 0x9A05): '戻',
    ('option', 0x9A08): '本',
    ('option', 0x9A09): '整',
    ('option', 0x9A0C): '下',
    ('option', 0x9A0D): '遊',
    ('option', 0x9A0E): '最',
    ('option', 0x9A0F): '適',
    ('option', 0x9A10): '態',
    ('option', 0x9A11): '主',
    ('option', 0x9A12): '観',
    ('rank', 0x9A01): '法',
    ('rank', 0x9A02): '説',
    ('rank', 0x9A03): '参',
    ('rank', 0x9A04): '下',
    ('rank', 0x9A05): '最',
    ('rank', 0x9A06): '低',
    ('rank', 0x9A07): '以',
    ('rank', 0x9A09): '受',
    ('rank', 0x9A0B): '成',
    ('rank', 0x9A0C): '血',
    ('rank', 0x9A0D): '清',
    ('rank', 0x9A11): '助',
    ('rank', 0x9A12): '名',
    ('rank', 0x9A13): '差',
    ('rank', 0x9A14): '込',
    ('rank', 0x9A15): '口',
    ('rank', 0x9A16): '取',
    ('rank', 0x9A17): '素',
    ('rank', 0x9A18): '晴',
    ('rank', 0x9A19): '任',
    ('rank', 0x9A1A): '務',
    ('rank', 0x9A1B): '内',
    ('rank', 0x9A1C): '容',
    ('rank', 0x9A1D): '道',
    ('roll', 0x9A01): '年',
    ('roll', 0x9A02): '代',
    ('roll', 0x9A03): '世',
    ('roll', 0x9A04): '常',
    ('roll', 0x9A05): '六',
    ('roll', 0x9A06): '万',
    ('roll', 0x9A07): '以',
    ('roll', 0x9A08): '在',
    ('roll', 0x9A09): '壊',
    ('roll', 0x9A0A): '型',
    ('roll', 0x9A0B): '分',
    ('roll', 0x9A0C): '相',
    ('roll', 0x9A0D): '当',
    ('roll', 0x9A0E): '月',
    ('roll', 0x9A10): '西',
    ('roll', 0x9A11): '暦',
    ('roll', 0x9A12): '日',
    ('roll', 0x9A13): '略',
    ('roll', 0x9A14): '配',
    ('roll', 0x9A15): '数',
    ('roll', 0x9A16): '削',
    ('roll', 0x9A17): '同',
    ('roll', 0x9A18): '現',
    ('roll', 0x9A19): '二',
    ('roll', 0x9A1A): '千',
    ('title', 0x9A01): '主',
    ('title', 0x9A02): '観',
    ('title', 0x9A03): '攻',
    ('title', 0x9A04): '出',
    ('title', 0x9A05): '来',
    ('title', 0x9A06): '同',
    ('title', 0x9A07): '態',
    ('title', 0x9A08): '＋',
    ('title', 0x9A09): '渡',
    ('title', 0x9A0A): '通',
    ('title', 0x9A0B): '常',
    ('title', 0x9A0C): '取',
    ('title', 0x9A0D): '得',
    ('title', 0x9A10): '格',
    ('title', 0x9A11): '闘',
    ('title', 0x9A12): '各',
    ('title', 0x9A13): '承',
    ('title', 0x9A14): '下',
    ('title', 0x9A15): '難',
    ('title', 0x9A16): '易',
    ('title', 0x9A17): '正',
    ('title', 0x9A18): '去',
    ('title', 0x9A19): '記',
    ('title', 0x9A1A): '録',
    ('title', 0x9A1B): '読',
    ('title', 0x9A1C): '真',
    ('title', 0x9A1D): '集',
    ('title', 0x9A1E): '画',
    ('title', 0x9A1F): '面',
    ('title', 0x9A20): '戻',
    ('title', 0x9A26): '杯',
}

# Shape table: 36-byte glyph bitmap (hex) -> character. Bank 1 is per-block, so
# `code -> shape` differs between blocks, but `shape -> character` is GLOBAL -
# proven by the 78 stage-archive shapes turning up byte-identical inside
# RADIO.DAT 150-220 times each. So naming a glyph once names it everywhere.
# `glyphsheets.py` writes the glyphs out for transcription; fill the `char`
# column of work/glyphs-to-identify.tsv and this picks them up.
SHAPES = {}


def shape_key(raw36):
    """the committed table's key for a glyph bitmap.

    A digest, not the bitmap, so `bank1-glyphs.tsv` carries no game data
    (CREDITS.md). 16 hex digits is 64 bits over ~1,200 shapes; there is no
    collision today and `glyphfill.py --publish`, which is what writes that
    table, refuses to write if two shapes ever hash alike.
    """
    return hashlib.sha256(raw36).hexdigest()[:16]


def load_shape_table(path=None):
    """shape -> character, from a transcribed glyphsheets.py TSV.

    `bank1-glyphs.tsv` beside this file is the committed table - 1,214 shapes,
    read by hand and checked against a decoded sentence each. It is the one
    artefact here that cannot be regenerated from the discs, so it lives in the
    repository rather than in `work/`. A `work/glyphs-to-identify.tsv` wins
    when it exists, so a pass in progress overrides the committed copy.

    Two column spellings, and the difference matters. `work/` files carry
    `shape_hex`, the 36 raw font bytes; the committed table carries `shape_id`,
    `shape_key` of those bytes, because **no game data goes in this
    repository** and a glyph bitmap is game data. The identification - which
    character a shape is - is ours and is what ships. Both spellings load, and
    a `shape_hex` file is converted on the way in, so the committed table and a
    freshly generated one are interchangeable for lookup.
    """
    for cand in ([path] if path else
                 [WORK + '/glyphs-to-identify.tsv',
                  os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               'bank1-glyphs.tsv')]):
        if cand and os.path.exists(cand):
            path = cand
            break
    if not path or not os.path.exists(path):
        return SHAPES
    with io.open(path, encoding='utf-8') as fh:
        head = fh.readline().rstrip('\n').split('\t')
        raw = 'shape_hex' in head          # a work/ table; hash it on the way in
        col = 'shape_hex' if raw else 'shape_id'
        if 'char' not in head or col not in head:
            return SHAPES
        ci, si = head.index('char'), head.index(col)
        for line in fh:
            f = line.rstrip('\n').split('\t')
            if len(f) > max(ci, si) and f[ci].strip():
                k = f[si].strip()
                SHAPES[shape_key(bytes.fromhex(k)) if raw else k] = f[ci].strip()
    return SHAPES


def char_for_shape(raw36):
    """a glyph bitmap -> character, once the shape table has been filled in"""
    if not SHAPES:
        load_shape_table()
    return SHAPES.get(shape_key(raw36))


STYLE = 0x6000      # colour/emphasis bits, not part of the glyph index


def glyph_char(code, stage=None):
    """one code -> one character, or None if the font bank is not located.

    The top bits are style flags and must be masked off first - `zen_index`
    does the same `code &= ~0x6000` before it indexes the font. Without it
    `0xD006` (a styled `ー`) and `0xC243` (a styled `ャ`) look like unknown
    codes, which is what made コントローラ decode as コントロ⟪D006⟫ラ.
    """
    code &= ~STYLE
    if stage is not None and (stage, code) in BANK1:
        return BANK1[(stage, code)]
    if 0x8101 <= code <= 0x8153:
        return chr(0x3041 + code - 0x8101)
    if 0x8201 <= code <= 0x8256:
        return chr(0x30A1 + code - 0x8201)
    if code in GLYPH_90:
        return GLYPH_90[code]
    if code in GLYPH_91:
        return GLYPH_91[code]
    if 0x8000 <= code <= 0x80FF:
        return chr(code & 0xFF)              # the Latin bank, already ASCII
    return None


def decode(text, stage=None):
    """a `<xxxx>`-coded string -> readable Japanese, unknown glyphs as ⟪xxxx⟫"""
    out = []
    for part in re.split(r'(<[0-9A-F]{4}>)', text):
        if not part:
            continue
        m = CODE.fullmatch(part)
        if not m:
            out.append(part)
            continue
        code = int(m.group(1), 16)
        ch = glyph_char(code, stage)
        out.append(ch if ch is not None else '⟪%04X⟫' % code)
    return ''.join(out)


def coverage(text, stage=None):
    """(known, total) glyph codes in a string"""
    codes = [int(c, 16) for c in CODE.findall(text)]
    return sum(1 for c in codes if glyph_char(c, stage) is not None), len(codes)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--in', dest='src', default=WORK + '/japanese-inventory.tsv')
    ap.add_argument('--out', default=WORK + '/japanese-readable.tsv')
    ap.add_argument('--hex', help='decode one run of codes given as raw hex')
    ap.add_argument('--sample', type=int, default=0,
                    help='print the N longest decoded lines instead of writing a file')
    args = ap.parse_args()

    if args.hex:
        raw = bytes.fromhex(args.hex.replace(' ', ''))
        coded = ''.join('<%02X%02X>' % (raw[i], raw[i + 1]) for i in range(0, len(raw) - 1, 2))
        print(decode(coded))
        return 0

    rows = []
    with io.open(args.src, encoding='utf-8') as fh:
        header = fh.readline().rstrip('\n').split('\t')
        for line in fh:
            f = line.rstrip('\n').split('\t')
            if len(f) < 7:
                continue
            rows.append(f)

    if args.sample:
        for f in sorted(rows, key=lambda r: -int(r[5]))[:args.sample]:
            st = f[1].split('/')[1] if f[1].startswith('STAGE.DIR/') else None
            known, total = coverage(f[6], st)
            print('%s %s %s  [%d/%d glyphs known]' % (f[0], f[1], f[2], known, total))
            print('   %s' % decode(f[6], st))
        return 0

    known = total = 0
    with io.open(args.out, 'w', encoding='utf-8', newline='') as fh:
        fh.write('\t'.join(header[:6] + ['japanese', 'codes']) + '\n')
        for f in rows:
            st = f[1].split('/')[1] if f[1].startswith('STAGE.DIR/') else None
            k, t = coverage(f[6], st)
            known += k
            total += t
            fh.write('\t'.join(f[:6] + [decode(f[6], st), f[6]]) + '\n')
    print('%d row(s) -> %s' % (len(rows), args.out))
    print('glyphs decoded: %d of %d (%.1f%%); the rest are bank 1, whose font is'
          ' not located' % (known, total, 100.0 * known / total if total else 0))
    return 0


if __name__ == '__main__':
    sys.exit(main())
