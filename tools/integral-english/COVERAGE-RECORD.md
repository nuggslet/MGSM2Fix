> Historical/reference record. For current instructions see [README.md](README.md),
> [BUILDING.md](BUILDING.md), and [NextSteps.md](NextSteps.md). Dated counts,
> machine paths, deployment state, and completed-work estimates below are not current instructions.

# Text coverage evidence (2026-09-04, VR section added 2026-09-06, updated 2026-09-09)

The current patch is not a complete English port. The title's disc-swap copy
(`en_menu3`) is ported but raw-disc only - the collection patches that block
itself, so it is not deployed here (README, "Why `en_menu3` is raw-disc only");
the Mission
Log and the disc-change abstract (both in `abst`) were ported on 2026-09-05
(`en_abst`, seen on screen the same day), and the VR disc on 2026-09-06
(seven PPFs since the MOVIE captions on 2026-09-07; see "The VR disc" below and the README section of the same name).
Two more things landed on 2026-09-08: `en_pad2`, the controller-port subtitle in
the Psycho Mantis room, which was the last main-disc string with a USA
counterpart and no family (all five of its call sites, across `s07b` and the
Integral-only `s07br`); and USA's four MISSION LOG location-name spellings,
inside `en_abst`. **Ten families now, 20 main-disc PPFs and 27 in all.**
The expanded scan closes the old tool's disc-1-only coverage gap for stage
inventory; it does not establish that every visible string has been audited.

**Since 2026-09-07 there is a stronger tool for the main discs: `mainsweep.py`.**
`audit_text.py` inventories *candidates* by framing and says so; it cannot tell
whether a candidate has an English counterpart, which is why about 160 of them
sat unclassified here. `mainsweep.py` does for discs 1 and 2 what `vr_sweep.py`
does for the VR disc: it pairs every GCL string with the USA disc's by the
command that owns it, so "Integral is Japanese here and USA has English" becomes
a comparison between two discs rather than a judgement about bytes. Run on
retail on both sides, deliberately, so a gap cannot hide behind a patch that is
already deployed.

**Result, both discs, identical:** twelve owners hold Japanese strings whose
owner has English on the USA disc. Nine are inside stages a patch family already
owns (`abst`, `preope`, `option`, `title`, `change`, `demosel`). Two are Japanese
on the USA disc in identical numbers, so there is nothing to port: `cmd 4AD9`,
the location titles, 12 Japanese and 58 English on **both** discs, and
`chara 9302` in `rank`, 1 and 30 on both. One was neither, and it is now the
`en_pad2` family, **built and deployed 2026-09-08**:

| | |
|---|---|
| stage | `s07b` on both discs, and `s07br` |
| owner | `chara 2D0A` — `CHARA_2D0A_2ND` → `NewSecond`, `game/second.c` |
| what it is | the subtitle drawn when the controller moves to port 2 for the Psycho Mantis fight: コントローラ端子1のコントローラを｜使用してください。 |
| Integral | 55 bytes of Japanese at **two** call sites in `s07b` and one in `s07br` |
| USA | English at the second `s07b` site, `PLUG CONTROLLER INTO \| CONTROLLER PORT 1.` (42 bytes); the first site is the identical Japanese |

**The table this replaces was wrong in two ways, and reading the caller is what
showed it.** `second.c` takes one string per spawn, so there is no record 0 and
record 1 to index: `s07b` holds two separate *spawns*, in two branches of its
script, and USA translated the **later** one. USA's English is shorter than the
slot, so the port is length-preserving — no container resized, no stage
relocated. All five sites are ported on the user's instruction, because both
branches hand the same message to the same actor and shipping USA's
inconsistency would leave Japanese on screen. `HANDOFF-ARCHIVE.md` §5.11 has the
reasoning and the verification.

**A second limit of this sweep, found the same day.** `mainsweep.py` compares
the **82 stage names both discs share**, so every one of the 13 Integral-only
stages is outside its universe. The third copy of that string, in `s07br`, was
therefore invisible to it and was found only by looking for the same owner in
the Integral-only stages by hand. A count from this tool means "among shared
stages"; Integral-only stages need their own pass, and none has been done.

**A description is not always the string its table points at.** Six item and
weapon slots swap or rewrite their text with the game state, mapped 2026-09-07
from the only two functions that print one (README, "Descriptions that change
with the game state"). Five are ported or deliberately matched to USA. The
sixth has no counterpart and **stays Japanese**: on VERY EASY the FA-MAS slot
becomes the MP5 SD outright - label and description both - and USA has neither
that weapon nor that difficulty. The description is 103 bytes at RAM 0x80011B04;
the label is an inline literal in the menu code, not a table entry. Confirmed on
screen 2026-09-07. Note that neither sweep could have
found it - it is an executable string, so `mainsweep.py` does not see it, and
`audit_text.py` reads the executables only for the save-title probes.

**One more text set, found and partly ported 2026-09-07.** The inventory's
side-column abbreviations are a separate block of names in the executable,
already Latin on both discs, which is why no sweep or audit had flagged them.
Comparing Integral's against USA's entry by entry, one differed: item 22 was
`SCARF` where USA has `HANDKER`. That one was changed on the user's instruction
and is the port's first replacement of Integral's **own English** rather than of
its Japanese (README, "Amendment, 2026-09-07"). The rest of the block already
matched. Integral's weapon names carry one entry USA does not have at all,
`MP 5 SD`.

**The blind spot this file used to share with every sweep - now swept, on the
main discs.** All of them - `mainsweep.py`, `vr_sweep.py`, `jpsweep.py`,
`audit_text.py` - look for *Japanese*, so a string that is already English on
both discs and simply **says something different** passed all of them
unremarked. `mainsweep.py --diff-english` asks that question directly
(2026-09-08): 15 replace hunks over disc 1's 82 shared stages, 8 with
player-readable text, and one residue after triage - the `abst` location names,
where **four** pairs differ and not the three the documents listed. The new one
is `Cmnd rm` against USA's `Cmnd room`. Disc 2 is identical.

| Integral | USA |
|---|---|
| `Tank Hanger` | `Tank Hangar` |
| `Medi rm` | `Medi room` |
| `Cmnder rm` | `Cmnder room` |
| `Cmnd rm` | `Cmnd room` |

**All four now read USA's**, asked and answered 2026-09-08: `USA_LOCATION_NAMES`
in `abst_build.py` takes USA's whole command, +12 bytes, the stage still 88
sectors. That is the second application of amendment 4b after `SCARF` against
`HANDKER`, and the builder's verifier now re-parses the list and asserts it
equals its source record for record. Note that `mainsweep.py` reads **retail**,
so it still reports these four - it now prints `!!` beside any finding in a
stage a patch family owns, which is what stops them being ported twice.

**The VR disc has not been swept this way, and one input has to change first.**
USA's VR disc carries five languages, so the diff must take only the English arm
of a language branch. With that done, a per-owner fuzzy match reports `FAMAS`
against USA's `FA-MAS` in the mission titles - and that is a **non-finding**,
because the deployed `vr_en_missions.ppf` holds 142 `FA-MAS` and no `FAMAS`: the
port already writes USA's spelling. The English-against-English question must
therefore be asked of the **deployed** bytes, the opposite of the discipline the
Japanese question needs, or a sweep rediscovers the port's own work.

**So the claim this file can now make** is that on the main discs, every string
whose owning command has English on the USA release is either already ported or
Japanese on the USA disc too — the last exception, `s07b`, was ported on
2026-09-08. That is a measurement, and it is a measurement over the 82 stage
names the two discs share (see the limit noted above).
What it still does not cover is texture lettering, executable UI beyond the
probes below, and runtime language branches.


## The file-level blind spot, and what it hid (2026-09-09)

**Every sweep in this project until 2026-09-09 read exactly one file: `STAGE.DIR`**
(plus the four executables). Nothing had ever looked inside the other seven
files on a disc. The figures above - "153 Japanese strings a disc" and the rest -
are therefore true *of the stage archives* and were presented as though they were
true of the disc. They are not.

The prompt was a question from the user about an Integral-exclusive Japanese
developer-commentary codec channel. There is one, it is enormous, and no tool
here could see it.

### Every file on a disc, and whether anything had read it

Integral disc 1 against USA disc 1, whole-file scans (not samples):

| file | Integral d1 | delta vs USA | Japanese found | swept before today |
|---|---:|---:|---|---|
| `/MGS/DEMO.DAT` | 258,744,320 | +141,312 | ~1.8 KB, real text | no |
| `/MGS/VOX.DAT` | 196,173,824 | +159,744 | ~1.0 KB, real text | no |
| `/MGS/STAGE.DIR` | 75,132,928 | +3,239,936 | 153 strings deployed | **yes - the only one** |
| `/MGS/ZMOVIE.STR` | 47,517,696 | +10,240 | none | no (FMV stream) |
| `/DUMMY3M.DAT` | 27,648,001 | 0 | none | as relocation scratch only |
| `/MGS/RADIO.DAT` | 11,198,464 | **+9,421,613** | **megabytes** | **no** |
| `/MGS/BRF.DAT` | 5,724,160 | −73,728 | none | no - now verified clean |
| `/MGS/FACE.DAT` | 3,508,224 | 0 | none | no - byte-identical stats |
| `SLPM_862.47` | 641,024 | — | 5 strings | yes |

`BRF.DAT` and `FACE.DAT` are the reassuring rows: 380,941 bytes of English text
in Integral's `BRF.DAT` against USA's 382,771, and `FACE.DAT` identical on both
counts, so the briefing data and the codec portraits carry nothing Japanese.
That was assumed before and is measured now.

### `RADIO.DAT`: the codec, and the commentary

Codec dialogue is not in `STAGE.DIR` at all. `menu/radiomes.c` loads it from
`RADIO.DAT` by sector, with a fragment size packed into the radio code, so no
GCL sweep could ever have reached it.

**Integral's `RADIO.DAT` is 6.3x the size of USA's** - 11,198,464 bytes against
1,776,851 - and it splits cleanly in two:

| region | bytes | content |
|---|---:|---|
| `0x0000000`–`0x042C54C` | 4,375,884 | the story codec, **English and Japanese together** |
| `0x042C54C`–`0x0AAC050` | **6,814,468** | **Japanese only - no English dialogue line anywhere in 6.5 MB** |

The English half is USA's script, complete and essentially unchanged: 35,273
dialogue lines / 1,145,926 bytes in Integral against USA's 35,193 / 1,143,269.
**So Integral's codec is already in English** - it is the runtime language
setting that chooses, which is exactly what `[Game] EnglishText` exists to hold
(README, "Unlocks"; the collection's language race). Nothing there needs porting.

The Japanese-only half is the developer commentary. It was read by rendering it
with the game's own font (`rendertext.py`), because none of it is Shift-JIS:

* 「ニンジャにつづきスネークも　装衣えを用意すること」
* 「、デモはゲーム中とは別モデルでやる予定だったので」 - the cutscenes were
  planned to use a different model from the in-game one
* 「さらにこのインテグラル　では」 - *furthermore, in this Integral…*
* 「られたメモリをどうやりくりするか」 - juggling the memory they were given;
  this exact 32-byte run occurs **328 times**, so conversations share boilerplate

95.1% of that region's 2 KB blocks are distinct, so it is real content and not a
repeated pattern. `d0 03`, a Japanese text control code, appears **64,087** times
in Integral's file against **4** in USA's.

### The two small pockets

Both are Integral-only and both are real text, rendered to confirm it:

* **`DEMO.DAT`**, ~1.8 KB across 258 MB, 0 in USA's: 「そしてテロリストの」 -
  story narration.
* **`VOX.DAT`**, ~1.0 KB across 196 MB, 0 in USA's: 「エンジンやプロペラのノイズ」
  - sound-design commentary, sitting beside the audio it describes.

### What this does and does not mean for the port

It is **not** a porting gap. The commentary, the Japanese codec track and both
small pockets are Integral-exclusive: USA never shipped any of it, so there is no
English to copy and the standing rule (port English where English exists, never
invent) leaves every byte of it alone. The port's scope - menus, screens and the
executable's own strings - is unchanged.

What it changes is what this document may claim. The honest statement is:

> Of the text this port covers, nothing with a USA counterpart is still
> Japanese. Of the text on the disc, several megabytes are Japanese, almost all
> of it Integral-exclusive commentary that has no English source and would be
> **translation** rather than porting.

That second sentence had never been written down, and the first had been
standing in for it.

## What Japanese is still there, and why (measured 2026-09-08)

Every figure above is about what the port *covers*. This is the complement: what
a player still meets on the deployed discs - **in the stage archives.** It does
not cover `RADIO.DAT`, `DEMO.DAT` or `VOX.DAT`; the section above this one is
where those are counted, and `RADIO.DAT` alone holds more Japanese than every
figure in this section put together. `py jpremain.py` produces it, and it
is the only tool here that reads **deployed** bytes rather than retail - retail
sectors with every deployed PPF overlaid, and the STAGE.DIR entry followed for
the four families that relocate their stage into DUMMY3M (`en_abst`, `en_brf`,
`en_option`, `en_preope`). Reading the retail LBA would return the unpatched
stage and quietly overstate what is left.

**Totals, stage scripts:**

| | plain English | mixed | **Japanese** |
|---|---:|---:|---:|
| disc 1 | 3,282 | 4 | **153** |
| disc 2 | 3,282 | 4 | **153** |
| VR disc | 10,809 | 183 | **38** |
| all three | 24,373 | 191 | **344** |

`mixed` means letters and glyph codes together, and it is a bucket rather than a
verdict because both cases occur: `<9A0E>Tokyo Game Show, Spring '98<9A0F>` is
English in Integral's own typographic quotes (most of the VR disc's 183), while
`<9009>...<900B>NORMAL<9...>` is Japanese with an English word inside it.

### Disc 1 and disc 2 (identical), 153 each

| owner | stage | n | what it is | why it stays |
|---|---|---:|---|---|
| `chara 53C7` | `abst` | 31 | Integral's **Japanese** location list in `demo.gcx` | Integral-only; its English list is the one the port now gives USA's spellings |
| `chara D44E` | `rank` | 30 | the ranking screen's commentary | Integral-only feature; no USA counterpart (§5.9) |
| `chara D3C0` | `rank` | 16 | more of the same | as above |
| `chara 04F2` | `rank` | 2 | as above | as above |
| `chara CF79` | `title` | 22 | title-stage text, including the disc-swap block | `en_menu3` ports the swap strings but is **raw-disc only** - the collection patches those same bytes (§5.3) |
| `chara D44E` | `title` | 21 | the **1P MODE** pages | Integral-only; 21 Japanese pages before the mode starts |
| `chara B757` | `roll` | 12 | the staff roll | credits, Integral's own |
| `cmd 4AD9` | 12 gameplay stages | 12 | location titles, one per stage | **Japanese on the USA disc too** - USA never translated them |
| `cmd EC9D` | `ending`, `endingr`, `s12a` | 4 | debug/ending strings | Japanese on the USA disc too |
| `chara 566F` | `abst` | 2 | the caption under READ MISSION LOG? | kept by rule - USA draws nothing there; `KEEP_PROMPT_CAPTION` (§5.9, still open) |
| `chara 81C7` | `camera` | 1 | a PHOTO ALBUM prompt | USA leaves the slot empty |

Nothing in that table has a USA English counterpart that the port is refusing to
use. The two categories that could ever change are the `title` rows, which need
the raw-disc variant, and the `abst` caption, which is an open question.

### VR disc, 38

| owner | stage | n | what it is |
|---|---|---:|---|
| `chara 976C` | `option` | 22 | Integral-only option rows; USA's seven help lines are ported, the rest have no counterpart |
| `chara D44E` | `vrsave`, `vrtitle` | 9 | debug windows; USA carries the identical Japanese |
| `chara 5667` | `vrtitle` | 4 | the PocketStation help line, its prompt and はい/いいえ - USA's fifth EXTRA item is STAFF CREDIT, a different feature (§6) |
| `chara 81C7` | `camera` | 1 | the same PHOTOGRAPHING prompt as the main discs |

**These figures count GCL records, and on 2026-09-11 that was shown to be
the wrong unit for one class of text.** Three VR overlays (`vrsave`,
`selectvr`, `vrtitle`) each carry a memory-card caption module in their
`.rodata` - not records, so not in this table - and the byte inventory that
did see them judged per *string*: the same Japanese caption sits in
`vrtitle`, where USA has it in Japanese too, so the copies in `vrsave` and
`selectvr`, where USA has English, inherited "identical in USA" and were
never compared as stages. `vr_en_memcard` ports those two; `vrtitle`'s stays
by rule. `py overlaydiff.py [--vr]` is the per-stage check that would have
caught it, and its 2026-09-11 run over both discs and both executables found
nothing else of the kind (NextSteps §26).

### The executables

Measured the same way, on the deployed executable:

* **item and weapon descriptions: 0 Japanese.** 26 item and 11 weapon strings,
  all English. The frozen Ration/Ketchup pair reads `Frozen.|Melt it before|you
  use.` - USA has its own text for those, so they were ported after all.
* **the MP5 SD description: 1 Japanese**, at file `0x2304`, immediately past
  `ARENA_B`'s exclusive end so the repack never touches it. Integral-only weapon
  on VERY EASY only; USA has neither (README, "Descriptions that change with the
  game state").
* **the memory-card message pool: 4 of 17 Japanese** - the four progress lines
  USA draws nothing for (now saving, save complete, now loading, load complete).
  The other 13 are English.

So per main disc the true total is **153 + 5 = 158**, and the VR disc's own
executable pools hold the same shape of leftovers (`vr_en_savemsg`'s two
untranslated indices, the MP5, the mine-detector difficulty line).

### Not covered by any of this

Overlay `.rodata` string pools were outside every tool here until
2026-09-11; `overlaydiff.py` covers them now, per stage, as English USA has
that Integral lacks. Texture lettering - Japanese drawn as art rather than
stored as text - is still
outside every tool here. The VR camera's EXORCISE textures are the known case
and are deferred; nothing else has been inventoried.

## The list itself: `jplist.py` (2026-09-09)

The two sections above describe what is left and where; neither was a *list*.
`py jplist.py` writes one - every untranslated Japanese string on all three
discs, one per line, to `work/japanese-inventory.tsv`:

    disc  source                      offset      bytes glyphs kana_kanji  text
    disc1 STAGE.DIR/abst/chara 566F   0x0            48     24         10  #{<9090><90CC>...
    disc1 RADIO.DAT                   0x90287F      152     76         70  <8113><812E>...

**190,180 strings, 3,318,254 kana/kanji glyph slots**, in a 32 MB file. It is
regenerated rather than committed - the repository keeps the counts, the method
and the tool.

| disc | source | strings | kana/kanji |
|---|---|---:|---:|
| disc 1 | `RADIO.DAT` | 94,246 | 1,652,457 |
| disc 1 | `DEMO.DAT` | 576 | 4,612 |
| disc 1 | `VOX.DAT` | 338 | 2,674 |
| disc 1 | `STAGE.DIR` | 98 | 764 |
| disc 2 | `RADIO.DAT` | 94,246 | 1,652,457 |
| disc 2 | `DEMO.DAT` | 318 | 2,538 |
| disc 2 | `VOX.DAT` | 229 | 1,798 |
| disc 2 | `STAGE.DIR` | 98 | 764 |
| VR | `STAGE.DIR` | 31 | 190 |
| | **total** | **190,180** | **3,318,254** |

`BRF.DAT` and `FACE.DAT` appear nowhere, and that is a result rather than an
omission - see below.

### What makes it a list of *Japanese* and not of bytes

Three tests, each of which was forced by a wrong answer earlier in this project:

1. **A run needs kana or kanji, not just high bytes.** `CORE` is the `0x81`,
   `0x82` and `0x96` banks; `0x80`, `0x90`, `0x9A`, `0xC1`, `0xC2` and `0xD0`
   are allowed *inside* a run without counting toward its length, because Latin
   letters, punctuation, button glyphs and text control codes all appear inside
   Japanese strings. That is what keeps the MP5 SD description (Latin name,
   Japanese body) in the list and pure-Latin `Tank Hanger` out of it.
2. **Repeated glyphs are not prose.** `BRF.DAT` matched 56 runs, every one a
   single code repeated - `<8283><8283><8283>…`. A run needs four distinct
   glyphs and no glyph taking more than half of it.
3. **Anything the USA disc also has is dropped.** This is the test that does the
   real work: it removed all 56 of `BRF.DAT`'s runs and all of `FACE.DAT`'s,
   because they are image data present on both releases. It would equally remove
   text USA left Japanese - a different category from Integral-exclusive
   content, and one the stage-archive sections above track separately.

### Where it disagrees with `jpremain.py`, and which to believe

`jplist.py` lists 98 stage-archive strings a disc where `jpremain.py` reports
153. The difference is `0x9Axx`: it holds real glyphs, but it is also where
Integral keeps its typographic quotes, so counting it would classify
`<9A0E>Tokyo Game Show, Spring '98<9A0F>` as Japanese. `jplist.py` therefore
excludes it and loses strings built only from that bank, such as the `cmd 4AD9`
location titles.

**For the stage archives, `jpremain.py` is the authority** - it works on complete
parsed records and weighs glyphs against Latin letters, which is the better test
where there is no binary to guard against. `jplist.py` earns its place on the
raw files, which nothing else reads at all.

### What the list is for

Not porting. Every string in it is Integral-exclusive, so there is no USA English
to copy and the standing rule leaves all of it alone. The list exists because
"how much untranslated Japanese is on this disc, and where exactly" had no answer
here until now, and because anyone who ever wants that commentary in English
needs a starting point - which is a translation project, not this one.

## Reading it: `jptext.py`, and how the font actually works (2026-09-09)

`jplist.py` gives a list; its `text` column is font codes, which is not something
a person can read. `py jptext.py` converts it to
`work/japanese-readable.tsv`, decoding **3,682,776 of 4,225,090 glyphs
(87.2%)**:

    STAGE.DIR/title/chara CF79   DISC 1 をセットしてください。
    STAGE.DIR/rank/chara 04F2    クリアデータを保存しますか?
    RADIO.DAT                    このエレベータは⟪9608⟫⟪9609⟫に移動しているわけ
                                 ではなく、回りのテクスチャをスクロールさせること
                                 により⟪9608⟫⟪960A⟫しています。

### Kana are arithmetic

Rendering the contiguous code ranges showed both kana banks are in standard
order, one code per character including small and voiced forms:

    0x8101 + i  ->  hiragana from U+3041 (ぁ)
    0x8201 + i  ->  katakana from U+30A1 (ァ)

and the top bits `0x6000` are style flags that must be masked off first, exactly
as `zen_index` does - without that, `0xD006` (a styled `ー`) reads as an unknown
code and コントローラ comes out as コントロ⟪D006⟫ラ. Verified against strings
whose reading was already known. 69% of all glyph uses, exact, no transcription.

### The `0x90` bank is a transcription that checks itself

238 glyphs read off labelled contact sheets. The proof it is right is that
consecutive codes spell the game's own words - `906A`-`906E` 地雷探知機,
`9059`-`905D` 精神安定剤, `9055`-`9057` 風邪薬, `904A`-`904D` 光学迷彩 - and a
single misread glyph would break a word. It is also confirmed independently by
arithmetic: `zen_index(0x90E2)` is glyph 394, and glyph 394 draws 服, which is
what the table says.

### Bank 1: solved, and it is why no global table exists

Codes from `0x9600` up are "bank 1", which `rendertext.py` has always refused
with "bank 1 lives elsewhere; not located". It is not in `font.res`, and not
appended after bank 0 either - glyph index 392 lands back among bank 0's kanji,
which ruled that guess out.

**Bank 1 is a per-block glyph table carried by the block itself.** A `.gcx`
script ends with a font blob - `parse_gcx` has always read it as `font` - and
`0x9A01 + i` indexes it directly. Proven: `abst`'s caption is
作戦⟪9A01⟫⟪9A02⟫, and glyphs 0 and 1 of that blob are **記** and **録**. The
glyphs after them are 諸島沖孤廃棄占拠等 - the mission log's own vocabulary, in
the order the text first needs it.

This is why the same code means different characters in different places:
`⟪9A01⟫` is 記 in `abst`, 端 in `s07b` (コントローラ端子1) and 年 in `roll`
(1980年代). There is no global table to build, and never was.

`RADIO.DAT` behaves the same way: 1,908 of its strings begin a fresh run at
`0x9601`, and the *same* commentary sentence appears at three offsets using
different bank-1 codes each time.

**Corrected 2026-09-10.** This paragraph used to add that its bank-1 codes "never
leave `0x9601`-`0x96FF` (255 entries)". They do: a commentary fragment's blob
holds up to **441** glyphs and the codes run on into `0x97xx`. The claim only
looked true because `japanese-inventory.tsv`'s scanner drops every code it does
not recognise, `0x97xx` included - so the evidence for it was manufactured by the
same bug it was describing. The index is `zen_index`, not `code - 0x9601`; see
`radiomap.bank1_index` and §19 of `HANDOFF-ARCHIVE.md`.

### Finishing bank 1: three sources done, the commentary measured

Bank 1 is a per-block table, so there is no global mapping to build - each
`(stage, code)` pair is its own question. The 90 distinct glyph *shapes* the
remaining Japanese actually uses were identified by pairing two weak signals:

* **an OCR shortlist.** `glyphocr.py` renders every JIS level 1 character at
  96px, area-averages to 12x12 and scores by zero-mean normalised correlation.
  Measured against the 238 hand-transcribed `0x90` kanji - a real labelled test
  set from the same font - it gets **47% top-1, 59% top-3**. Not usable alone,
  and the errors say why: 鏡→鎌, 線→緑, 減→滅. At 12x12 those are the same
  picture.
* **the decoded context.** With 87% of each sentence already readable, the gap
  is usually forced. 「⟪9A50⟫入ドック」 narrows a shortlist to almost nothing;
  「作戦⟪9A01⟫⟪9A02⟫」 is 作戦記録; 「変更内容を⟪910B⟫書き保存」 pins 上; and
  the staff roll is MGS1's own opening text, which pins 二/万/千 outright:
  「1980年代、世界には常時六万発以上の核兵器が存在した。」
  **Corrected 2026-09-10:** that glyph was transcribed 五 and is 六. The roll gives
  no way to tell them apart at 12 pixels; `RADIO.DAT` does, twice (第六感, and
  レイブンは六人もの人間を運んだ against a 四人運び record), and so does the
  history: the stockpile peaked near 60,000, not 50,000.

82 of the 90 shapes fell to that combination. The result:

| source | kana/kanji | unresolved | readable |
|---|---:|---:|---:|
| `DEMO.DAT` | 7,150 | 0 | **100%** |
| `VOX.DAT` | 4,472 | 0 | **100%** |
| `STAGE.DIR` | 1,718 | 0 | **100%** |
| `RADIO.DAT` | 3,304,914 | 0 | **100%** |

*(superseded 2026-09-10. The table above counts what `japanese-inventory.tsv` holds, and the inventory holds only **85.2%** of the commentary's glyph instances: its scanner ends a run at any code it does not recognise, and `0x91xx` and `0x97xx` carry real text. The export no longer uses it for `RADIO.DAT` - `radiotext.py` walks the game's own records instead, which reaches all 125 commentary fragments and finds more text than the inventory in every one of them. **The finished figures: 68,242 lines, 3,923,944 kana/kanji, zero unresolved glyph codes** - across all three discs, VR included (§20). The fragment map those figures rest on has a per-fragment regression guard as of §21: `py radiotext.py --check`, with `--selftest` to prove it still catches a slipped base. §19 of `HANDOFF-ARCHIVE.md` has the account, including the bank-1 index bug that was naming every `0x97xx` glyph one position too far along.)*

**And it found two errors in the hand transcription.** `0x9027` was read as 告
and is 書 - 「上書き保存」 and 「解説書」 both demand it - and `0x90E4` was read
as 問 and is 間, because 「1⟪90E4⟫」 in the rank screen is 1週間. Neither was
visible by squinting at the glyph a second time; both were obvious the moment a
sentence had to make sense. That is the argument for context over eyesight, and
it applies to the 238 as much as to the 90.

### `RADIO.DAT`'s glyph tables: located, and the identifications propagate

Two things were missing to read the commentary: where each conversation's glyph
table sits, and what its glyphs are. The first is now solved and the second
turned out to be half-solved already.

**The identifications propagate, because the master font is shared.** Of the 82
bank-1 shapes identified for the stage archives, **78 appear inside
`RADIO.DAT`** as byte-identical 36-byte bitmaps, 150-220 times each. So
`shape -> character` is **global** even though `code -> shape` is per-block:
identify a glyph once anywhere and it is identified everywhere it is reused.
That is what makes the remaining work additive rather than per-block.

**The table sits immediately after the conversation's text.** Proven by a
constraint with exactly one solution in 11 MB. The second line of the first
conversation is
`⟪9603⟫眼では見えないでしょうけど、#N⟪9604⟫から⟪9605⟫⟪9606⟫も赤外線が⟪9607⟫ているのよ`,
whose English counterpart in the same file reads "You probably can't see them
with your naked eyes, but there are infrared beams coming out of that wall." So
`9606` is 本 and `9607` is 出 - both already known - and they are at adjacent
indices, which means their bitmaps must be 36 bytes apart. Searching the whole
file for 本 immediately followed by 出 returns **one** offset, `0x265`. With 本
at index 5 that puts the table base at **`0x1B1`** - directly after the text
records, which end at `0x1B0`. Decoding the conversation from that base:

    ⟪仕⟫⟪掛⟫けられているわ           …が仕掛けられているわ
    ⟪肉⟫眼では…⟪壁⟫から⟪何⟫本も赤外線が出ているのよ
    それに⟪触⟫れると扉が⟪閉⟫まって毒ガスが⟪噴⟫き出してくる…

Every gap is now a single plausible character rather than a mystery, and the
sentences match their English line for line. The rule is therefore:

    table base   = end of the conversation's text records
    glyph index  = code - 0x9601

which is the same shape as the `.gcx` case (`0x9A01 + i` into the script's own
font blob), just with the table inline instead of at the end of the file.

**What is left is only identification, and it is now countable.** Because shapes
dedupe globally, the cost is not 541,920 uses or ~1,900 blocks - it is the number
of *distinct* shapes in the file, each of which needs naming once. The
`glyphocr.py` shortlist plus surrounding context is the method that worked for
the stage archives' 90, and the commentary has the strongest context of all: its
own English translation sits in the same file for the story half, and the
commentary half is prose about making a game.

**The number is 1,735.** Walking outwards in 36-byte steps from each of the
8,840 known-glyph occurrences recovers **319 glyph runs, 57,007 cells and 1,813
distinct shapes**, of which 78 are already identified. So finishing the
commentary means naming **1,735 more glyphs**, once each - not 541,920 uses and
not ~1,900 blocks. For a few hours of Japanese prose that is the expected size
of a kanji set, which is a good sign the count is real.

Treat it as a **lower bound**: the walk only finds a table that contains at
least one of the 78 anchors, so tables built entirely from rarer kanji are not
counted yet. A first attempt to avoid that by sweeping all 36 byte-phases over
the whole file reported 1.9 million "shapes" and is recorded here as a
cautionary result - without an anchor to start from, the test cannot tell a font
cell from any other 36 bytes, and the answer was garbage rather than merely
imprecise.

### 2026-09-11: six readings corrected by an independent table

Every bank-1 shape was hashed and looked up in the MGS1 Translation Toolkit's
codec-glyph table (`graphicsData`, 6,877 bitmaps; NextSteps §27): all 1,214
present, 1,208 agree, 6 differ. Their table is OCR with acknowledged errors, so
each of the six was re-read here rather than taken - and **all six went their
way**: 京→涼, 綺→華, 瀕→餓, 綿→緻, 輌→輛, 〝→”. The sentences make sense after
every swap, which is why the sentence test never caught them; they are a name,
two near-synonyms, a variant form and a quotation mark. The last three were
kept at first and re-tested only when the user looked at the bitmaps: a
compound character has to be scored on the half that differs, not the whole
tile its two candidates share. `bank1-glyphs.tsv` carries all six. Nothing of
theirs is copied; CREDITS.md.

### DONE (2026-09-10): the 12.8% is identified, and the count was wrong

**Everything from here to "The dump, and the scope it is drawn to" is the record
of the problem before it was solved. Read it for the method, not the numbers.**

The commentary needed **1,200** more glyph identifications, not 1,735; the larger
figure came from a fragment map that was wrong for 93% of strings and so
manufactured bitmaps no font contains. `radiomap.py` rebuilt the map from the
game's own record-list parser plus the 192 fragment extents its radio codes
declare, and `glyphsheets.py` / `glyphfill.py` / `glyphreview.py` named all
1,200. Coverage is **100.00%** and the dump is regenerated. §18 of
`HANDOFF-ARCHIVE.md` has the full account, including the five characters this work
proved wrong in the tables above.

### Why `RADIO.DAT`'s 12.8% looked like it needed 1,735 more identifications

The commentary uses **541,920 bank-1 glyphs**, in tables of up to 255 entries in
each of ~1,900 conversation blocks. Two of the three obstacles below are now
gone - the table location is solved and the identifications propagate - so read
this list as the record of what the problem looked like before that:

1. the glyph block inside a `RADIO.DAT` conversation has to be located - the
   `.gcx` case is solved and this one is not yet parsed;
2. every distinct shape across all those blocks has to be *recognised*, and
   locating a table yields bitmaps, not characters;
3. at 47% top-1, OCR cannot do step 2 alone, and there is far too much of it for
   the context trick, which needs a human reading each sentence.

So the honest position is that three of the four sources are done and the
commentary is bounded and costed rather than finished. Publishing a 47%-accurate
kanji table would put wrong Japanese into a document that looks authoritative,
which is worse than leaving `⟪9601⟫` visible.

### What remains is recognition, not reverse engineering

Locating a block's table yields **bitmaps, not characters** - something still has
to say which character a 12x12 bitmap is. By hand that is easy for the stage
archives (421 bank-1 glyph uses in total) and impossible for the commentary
(541,920 uses, up to 255 distinct glyphs in each of ~1,900 blocks). That wants
bitmap matching against a reference font, and it is the one piece of work between
this and a fully readable disc.

`DEMO.DAT` and `VOX.DAT` use no bank-1 glyph at all and come out **fully
readable** - and reading them corrected an earlier guess in these notes: their
text is story narration and dialogue (「そしてテロリストの核発射」,
「け入れられない場合核を発射すると」), not the sound-design commentary the
single rendered sample had suggested.

### The 1,735: no font can look them up, so they are rendered for transcription

*(The count was wrong - it is 1,214, and the 1,735/1,813 figures came from a fragment map that was wrong for 93% of strings. All of them are named as of 2026-09-10. The method below - render them and read them - is what worked, with one change: read each glyph against a decoded **sentence**, not on its own. §18-19 of `HANDOFF-ARCHIVE.md`.)*

Two reference fonts were tried and both are recorded because the second one was
the *right* idea and still failed:

| reference | how it was matched | top-1 |
|---|---|---:|
| MS Gothic / Meiryo, 96px rasterised then area-averaged to 12x12 | zero-mean normalised correlation, ink-ratio gate | **47%** |
| **Shinonome 12-dot** (MIT/public domain, native JIS X 0208 at exactly 12 dots) | exact bitmap, then Hamming with ±1 shifts | **0% exact, 5.9% fuzzy** |

Shinonome should have been the answer - a native 12-dot bitmap font is the
apples-to-apples comparison, and 6,879 glyphs of it were parsed and tested. Not
one game glyph matches it exactly, and fuzzy matching is *worse* than the
outline font. **Konami drew their own 12x12 design**, so there is no font on
earth to look these up in, and the remaining work is recognition by eye.

`glyphsheets.py` therefore renders them for a person - or a multimodal model -
to read:

    py glyphsheets.py     -> work/glyphs-to-identify.pdf   8 pages, 1,200 glyphs
                          -> work/glyphpages/pageNN.png    the same, as images
                          -> work/glyphpages/pageNN.txt    decoded sentences per glyph
                          -> work/glyphs-to-identify.tsv   id, occurrences, char, shape_hex
                          -> work/glyph-answers.tsv        78 known, to score the pass

Glyphs are ordered **by how often they occur**, so a partial pass buys the most
text - the first few hundred cover most of the commentary's characters. Fill the
`char` column and `jptext.load_shape_table()` picks it up; naming a glyph once
names it everywhere, because `shape -> character` is global.

**Expect 85-95% accuracy from isolated glyphs, and know where the errors will
be.** The 238-glyph pass ran at ~99%, but only because consecutive codes there
spelled words - context was doing the work. Two of its errors (書 read as 告,
間 as 問) survived a second look at the bitmap and fell instantly to a sentence
that had to make sense. At 12x12, 線/緑, 鏡/鎌 and 間/問 are the same picture.
So a transcription pass should be **checked against context afterwards**, not
trusted on its own.

### Rendering sentences instead: better, and blocked on one number

Reading a sentence beats reading a glyph, for the same reason context beat
eyesight above: each image carries several unknown kanji, and the kana are
already known exactly, so a transcript whose kana do not line up is *detected*
rather than silently accepted. That needs each conversation's table base, and
the base work is close but not finished:

* conversations grouped: **389**; bases pinned: **333**
* the alignment signal is **phase** - at the true base every glyph cell's
  bottom row is blank, because the font leaves a bottom margin. Picking the
  phase took known-glyph agreement from 3 conversations to **317**
* conversation 0's base comes out `0x1B1`, matching the value proved
  independently from the 本/出 adjacency
* but repeated sentences - 1,085 groups appearing 2+ times with *different*
  code assignments - agree only **61.5%** of the time after coordinate ascent
  over ±4 index shifts

Some of that 38% is not misalignment: the grouping key strips codes, so two
sentences with the same kana skeleton and genuinely different kanji are counted
as disagreeing. Separating those two causes is the next step, and until it is
done sentence rendering would produce a confident-looking document with wrong
glyphs in it - which is the failure mode this whole exercise is trying to avoid.

## The dump, and the scope it is drawn to (2026-09-09)

The figures above count Japanese on the disc. This is the part that matters for
anyone who wants to *do* something about it: the Japanese **USA has no
counterpart for**, which is the user's scope as of 2026-09-09 and is much
narrower than the totals.

`RADIO.DAT` is the case that needed splitting. It holds two halves - a
story-codec region whose Japanese is a subtitle track for conversations USA
ships in English, and a commentary region with no English dialogue anywhere in
it:

| region | rows (disc 1) | kana/kanji | |
|---|---:|---:|---|
| story codec `0x0`-`0x042C54C` | 16,869 | 282,280 | USA covers it - out of scope |
| **commentary `0x042C54C`-`0x0AAC050`** | **77,361** | **1,369,719** | in scope |

`jplist.py` had already applied the same test to the other files by subtracting
runs that also appear in USA's copy - which is what empties `BRF.DAT` and
`FACE.DAT` completely. `RADIO.DAT` escaped it because USA's codec text is plain
ASCII, so there were no font-code runs to subtract against.

**The commentary is byte-identical on both discs.** sha256 of
`0x042C54C`-`0x0AAC050` is `93b96e43…` on disc 1 and disc 2 alike, while the
story regions differ. So the identical 77,361 line count on both discs is
correct rather than a bug, disc 2's `RADIO.DAT` dump duplicates disc 1's, and
the glyphs only have to be identified once.

### What `dumpjp.py` produces

156,379 lines over 5,589 pages, every line drawn from the game's own glyph
bitmaps, so the images are exact by construction - there is no recognition step
in the picture path:

    work/jpdump/disc1_RADIO_DAT.pdf   77,361 lines  2,763 pages
    work/jpdump/disc1_DEMO_DAT.pdf       576 lines     21 pages
    work/jpdump/disc1_VOX_DAT.pdf        338 lines     13 pages
    work/jpdump/disc1_STAGE_DIR.pdf       98 lines      4 pages
    work/jpdump/index.tsv            156,379 rows

The text column of `index.tsv` is only as complete as the glyph identification
(87.2%), and it upgrades retroactively: name a shape in
`glyphs-to-identify.tsv` and every line that uses it reads correctly from then
on, in every block.

Verified mechanically rather than visually, because the session's image budget
was spent: 4,000 sampled lines render with none blank or sparse, and every glyph
cell of a line matches the font byte for byte. **Nobody has looked at a page
yet** - the scale and lines-per-page are `--scale` and `--lines-per-page` and
are cheap to change before a re-render.

## Reproduce the inventory

```powershell
py audit_text.py --game D:/Steam/SteamApps/common/MGS1 --executables D:/mgsbuild/integral-english-work/work --output D:/mgsbuild/repro4/text-audit.json
```

This reads deployed PPFs, follows relocated stage entries, inventories GCL
STRING framing candidates and scans main-game overlay pointers and short
LUI/ADDIU or LUI/ORI address sequences. JSON retains stage/PPF/executable hashes,
offsets, raw bytes, decoded candidates, stage differences and extraction errors.
It writes only the requested report. Run it again after changing deployed PPFs.

| Image | Integral named stages | USA named stages | Stage extraction errors |
|---|---:|---:|---:|
| Main disc 1 | 95 | 96 | 0 |
| Main disc 2 | 95 | 96 | 0 |
| VR | 105 | 105 | 0 |

**The Integral-only stages are swept now too** (`mainsweep.py --integral-only`,
2026-09-08). Each of the 13 pairs onto the USA stage it is a variant of - the
base name without the trailing `r`, or `init` for `init_ve` - and across all 13,
on both discs, there is **exactly one** Japanese string whose base-stage owner
has English on the USA disc: the `s07br` copy of the controller-port line, which
`en_pad2` ports. So the hole this tool's shared-name universe left is measured
and closed rather than merely known.

Each main disc has 82 shared stage names and 13 Integral-only names:
`d18ar`, `endingr`, `init_ve`, `s03ar`, `s03dr`, `s03er`, `s07br`, `s07cr`,
`s09ar`, `s18ar`, `s19ar`, `s19br`, `s20ar`. **These are outside `mainsweep.py`'s
universe**, which is the two discs' shared names; `s07br` is the one so far known
to hold portable text, and `en_pad2` covers it. What the `*r` stages are for has
not been established - `s07br`'s overlay source is byte-identical to `s07b`'s.

**A new lead, 2026-09-12, still not a conclusion.** The decomp's C source never
references any of these thirteen names as string literals — `game/loader.c`'s
`NewLoader(const char *dir)` just takes whatever name it is handed, so the
choice of destination is made by the *caller's* GCL script data, not by C code
the decomp covers. Byte-searching `int1_stage.dir` for each of the 13 names
found where they are referenced: `s07br` appears inside `s07a`, `s07c`, `s09a`,
`s07cr`, `s09ar` and `select2`, always as the identical 26-byte run
`c8 bb 09 07 06 "s07br\0" 50 6d 04 06 7d f9 50 73 03 02` — the same opcode,
argument count and string byte-for-byte in every owner. That uniformity across
five unrelated rooms is the opposite of what a hand-authored, room-specific
story branch would look like, and is instead what a **shared/common data table
compiled into multiple overlays** looks like — e.g. the developer stage-select
menu (`game/select.c`'s `NewSelect`/`isStageSelectionMenu`, which walks a GCL
list of stage-name strings for QA jump-to-any-stage) linked into several
builds. That would make at least this appearance of `s07br` a debug-tooling
artefact rather than proof of in-story reachability — but the opcode `c8 bb`
has not been identified against any known GCL command, so this is a
lead to disassemble further, not a finding. The same search also turned up two
`*r` names that are **not** Integral-exclusive — `s08br` and `s17ar` are
referenced the same way and evidently exist on the USA disc too — which means
the `r` suffix is a general MGS1 pattern (present in the original game) and
Integral's 13 are additions to an existing convention, not an invention of
their own. Whoever picks this up next should identify the `c8 bb` opcode (by
finding the same byte pattern near a command the decomp *does* cover) before
concluding either way.
Integral's VR ISO was located by its PVD and `SLPM_862.49` path at container
base `0x57592000`; USA VR is at `0xD39B7000`. The older 106-stage count included
one more than the 105 named entries actually enumerated; use 105 for inventory.

## What the scan can and cannot prove

- `jpsweep.py` compares pointer slots at equal offsets on disc 1. Equal offsets
  do not establish matching tables across differently compiled overlays. The
  camera pairing was independently verified; other pairings need evidence.
- GCL scanning is a framing heuristic, including data that overflows an
  OPTION's one-byte length. Opcode-like bytes inside operands can yield false
  positives. A candidate needs structural and caller verification before use.
- Address-reference scanning also yields code/data false positives. It does
  not model all register flow or discover every indirect reference.
- The font strips `0x6000` style flags. `0x80xx` can be Latin and `0x9001` a
  space; other glyphs depend on the font bank. `unresolved_glyphs` is therefore
  not a Japanese-language classification. USA also contains non-ASCII glyphs.
- VR overlay load bases are not established by this tool, so its VR reference
  scan is explicitly disabled. VR GCL candidates and stage inventory are read.
  The VR port established them separately — `0x800C11A0` for Integral and
  `0x800C4350` for USA, both `_bss_objend` — but `audit_text.py` has not been
  taught them, so its VR numbers below come from the port's own tools.
- Texture lettering, executable UI beyond the save-title probes, runtime
  language branches, collection-provided replacements and screen reachability
  still need targeted inspection. Zero extraction errors is not zero gaps.

The report is a reproducible investigation index. Completing the translation
census still requires verifying residual candidates against callers, fonts,
the USA path and reachable screens. No translations are inferred from it.

## Save-slot title encoding

The original USA executable also stores full-width Shift-JIS Latin. The old
suggestion that Integral's title should be replaced with an ASCII USA title
was incorrect for these inputs:

| Token | Integral executable offsets | USA executable offsets | Bytes |
|---|---|---|---|
| Full-width MGS prefix | `0x2AF4`, `0x31EC` | `0x3264`, `0x9E6D8` | `826c82668272` |
| Full-width Dock | `0x8F410` | `0x91B68` | `8263828f8283828b` |
| Full-width [NM] | `0x2AC0` | `0x2B44` | `816d826d826c816e` |

Integral appends `81e7` (the integral sign) to the MGS prefix. This is product
branding. `source/menu/datasave.c`'s `makeTitle` also constructs full-width time
digits; the caption tables are separate from the save-title formatting path.
No blanket ASCII conversion is warranted. These byte probes do not claim that
every possible location or runtime save title has been displayed and tested.

## Additional retained caption

The camera stage's first GCL STRING at script `+0x1B8` contains
`90639a019a029a038152910b9027810d902b902c8117813e8119810bc03f`
in Integral; USA's corresponding record is empty. This is separate from the
six retained `camsave` overlay slots and was missing from the earlier list.
It stays unchanged under the existing no-invented-translation rule.

The retained recap bytes and ranking/location glyphs must also be interpreted
through their callers. In particular, the rebuilt preope keeps unread retail
recap bytes while its MG2 renderer uses the appended English blob. Counting
encoded strings in the file overstates what remains visible in Japanese.

## The VR disc (inventoried and ported 2026-09-06)

Read from the VR binaries by `vrlib.py` and the six `vr_*.py` builders, not by
`audit_text.py`. Every figure is what the builders report on a clean run.

| where the text is | how it is stored | Integral | ported |
|---|---|---|---|
| in-mission windows | `chara 0xD44E` (`vrwindow`) commands in each stage's `scenerio.gcx` | 1813 windows in 94 stages | **1808 in 92 stages** |
| item / weapon / capture-mode pools | executable string arenas behind tables at `0x8009C11C`, `0x8009C304` and `0x80011F0C` | 3 pools | all, minus MP5 and frozen items (no USA text) |
| save and load messages | executable tables at `0x8009C884` / `0x8009C8B4` | 12 + 12 | 20; indices 1 and 9 stay Japanese (USA draws nothing) |
| option help lines | `chara 0x976C` option `-e` in the `option` stage | 31 records | 7 (1, 2, 3, 5, 6, 12, 26); the rest are Integral-only rows |
| KEY CONFIG | eight label textures in the option stage's DAR, plus quad geometry in the overlay | 8 labels | all 8, with USA's rectangles and `key_syukan` +11; **verified on screen 2026-09-07** in all three button types, and the row's selection highlight widened to match (README, "The VR KEY CONFIG on screen"). Its help line under the controller stays Japanese: USA leaves records 17..25 empty |
| EXTRA menu help lines | `chara 0x5667` option `-t` in `vrtitle` | 11 records | 4 (records 2–5) |
| PHOTOGRAPHING memory-card messages | string table in the `camera` overlay at `+0x608` / `+0x638` / `+0x668` / `+0x708` | 4 groups | all but the two Japanese prompts and the two USA-empty slots |
| MOVIE selection captions | `chara 0xFAA8` option `-t` in the `movie` stage | 3 captions (4 records) | **all 3** (6 records), the two TGS ones as USA's two lines |

Known to remain Japanese on the VR disc, each because USA has no counterpart:
the PocketStation help line, prompt and はい/いいえ (USA's fifth EXTRA item is
STAFF CREDIT, a different feature); save/load indices 1 and 9; two camera
prompts; `vrtitle`'s four and `vrsave`'s one debug window, where USA carries the
identical Japanese; MP5, the frozen items and the mine-detector line.

The last VR item that was **blocked rather than absent** is now ported: the two
TGS **MOVIE captions**. USA draws each as two lines where Integral drew one, and
the count is neither the record count nor the position table but two calls in the
clip-selection code, one of which Integral does not make; the port retargets that
call at a 16-word stub that lights both of a clip's lines, exactly as USA does
(README "The MOVIE selection captions"). Nothing on the VR disc with a USA
counterpart is still Japanese for a reason other than the list above.

Not yet inventoried on the VR disc: texture lettering outside the eight KEY
CONFIG labels (the camera's EXORCISE textures are known and deferred), and any
string reached only through the five-language selection in USA's executable
other than the English pool the port reads.
