# Schneider Electric breakers: front views and accessories

Vector redrawings (no photo, logo or manufacturer text) of Schneider
Electric circuit breakers, residual current devices (RCD) and disconnectors,
each as an IEC symbol and as a front view, with their accessories. The
elements are in
`extra-elements/10_electric/20_manufacturers_articles/schneider`, the
generator is `misc/schneider/gen_schneider.py` (reproducible: it removes the
folder and writes it again), and the example is `examples/schneider_example.qet`
(`misc/schneider/gen_example.py`).

```
python misc/schneider/gen_schneider.py extra-elements/10_electric/20_manufacturers_articles/schneider
python misc/schneider/gen_example.py
```

## Content

Each product has a folder holding `<stem>.elmt` (IEC symbol) and
`<stem>_realistic.elmt` (front view). The terminal names are the same in both,
so a conductor keeps its meaning. One terminal per pole at the top (`1`, `3`,
`5`, `7`) and at the bottom (`2`, `4`, `6`, `8`); the RCDs with the neutral on
the left use `N1`/`N2` first. The origin is the first top terminal, every
terminal is on the 10 px grid.

| Folder | Products (reference) |
| --- | --- |
| `acti9_ic60` | iC60N 1P / 2P / 3P / 4P (A9F74110, A9F74202, A9F74310, A9F74410) |
| `acti9_rcd` | iDPNa Vigi 1P+N (A9D34616), Vigi iC60 3P (A9V11325), iID 2P (A9R41225), iID 4P (A9R41440) |
| `acti9_isolators` | iSW 3P 40 A (A9S65340), STI 2P 25 A (A9N15651) |
| `compact_nsx` | NSX100B 3P TMD100 (LV429540), NSX250N 3P frame (C25N3), NSX250N 3P TM250 (C25N3TM250C, with an `_accessories` front view) |
| `compact_ins` | INS250 3P / 4P (31106 / 31107) |
| `accessories` | MT250 motor mechanism (LV431541 220/240 V AC, LV431543 24/30 V DC), MN undervoltage release (LV429410), OF/SD/SDE/SDV contact (29450), iOF contact (A9A26924), rotary handle (LV429337), terminal shield (LV429517) |

The accessories are separate elements, so they can be placed next to a device
or on another folio. The handle and the shield are mechanical: they have a
front view without terminals (the `--check-elements` warning "0 terminals" is
expected).

### Scale

Acti9: one 9 mm module is 30 px, so an 18 mm pole is 60 px (3.33 px/mm).
ComPacT NSX and INS: the 35 mm pole pitch is 120 px (3.43 px/mm). The two
differ by 3 %; this is a drawing choice, not a measurement. Each front view
shows the device body, the toggle, the position indicator, the test button of
the RCDs and the rating as dynamic text.

## The "220 A" disconnector

Schneider has no 220 A rating. The reading used here: the **ComPacT NSX250**
with its **MT250** motor mechanism (the "220" of `C25N3220`, a reference found
in the real project, could not be verified online), or the **INS250**
disconnector. Both are drawn; the NSX250N TM250 carries the full set of
accessories. To be confirmed by the author of the request.

## Linking accessories by label prefix

An accessory whose label starts with the label of a device, followed by a
separator (`.`, `-` or `_`), belongs to that device: `-QS1` for the
disconnector, `-QS1.1` for the motor mechanism, `-QS1-OF` for the contact.

1. **Compatible list as articles with quantity 0.** Each device is delivered
   with its compatible accessories as article blocks (the Articles tab, blocks
   1 to 4: `manufacturer_reference_auxiliaryN`, `quantity_auxiliaryN`...),
   quantity `0`, block description "Compatible accessory". The motor
   mechanisms for 220 V AC and 24 V DC are exclusive and share one block.
   There are four blocks only, so the handle (the 5th group) is in the
   compatibility list (`COMPAT` in the generator) but not seeded on the NSX250.
2. **The quantity becomes 1** (the number of distinct linked labels for the
   reference) when an accessory is placed with the same prefix, and returns to
   0 if it is deleted or renamed.
3. **Outside the list**: the accessory is not written as an article and a
   warning is given (an error with `--strict`, which writes nothing and exits
   with 2). A device without a list (INS250: the compatibility could not be
   verified) says "no list".
4. **An accessory without a parent** (no label prefix matching a device) is a
   warning.

The tool is `misc/schneider/qet_accessory_articles.py`:

```
python misc/schneider/qet_accessory_articles.py project.qet -o out.qet
python misc/schneider/qet_accessory_articles.py project.qet --in-place
python misc/schneider/qet_accessory_articles.py project.qet --check --strict
python misc/schneider/qet_accessory_articles.py project.qet -o out.qet --prune-zero
```

Nothing is written without `-o` or `--in-place`. Exit codes: 1 unreadable
file, 2 `--strict` and errors, 3 `--check` and changes needed. It is
idempotent: the blocks it manages (reference in the list, or the marker with
an empty reference) are rewritten at every run, so there is no duplicate;
other blocks of the user are kept and take one of the four places.
Every element carrying the parent label (the symbol and the front view, even
on several folios) receives the same blocks, which is compatible with the
synchronisation of the fields of elements sharing a label. The tests are in
`misc/schneider/test_qet_accessory_articles.py`.

`--prune-zero` removes the blocks that are not linked, which is needed for the
parts list (below).

## Quantity 0 in the parts list

The current export (`sources/bomexport.cpp`, `groupedQuery`) does not handle a
quantity of 0: with "sum quantities" a block of quantity 0 gives a line with
a total of 0; without it, the line counts as 1, which is wrong. The code was
not changed (it would change the behaviour and need a setting). Until then, run
the tool with `--prune-zero` before exporting the parts list.

### Proposed C++ patch (not applied)

1. In the `part()` query of `bomexport.cpp`, for the main part and for each
   auxiliary block, add a condition to the WHERE clause that excludes a
   quantity equal to 0, behind a setting off by default:

   ```
   AND NOT (TRIM(q) <> '' AND TRIM(q) NOT GLOB '*[^0-9.,]*'
            AND CAST(REPLACE(q, ',', '.') AS REAL) = 0)
   ```

   The nomenclature and summary queries of `projectdatabase.cpp` (about lines
   5349-5420) have to be checked the same way.
2. The refusal of the link is done in the code that sets the label of an
   element: when the new label has a parent prefix, and the reference of the
   element is a Schneider accessory that is not in the parent's article blocks,
   refuse (or warn in the consistency report); otherwise set
   `quantity_auxiliaryN` of the parent to 1 through a `QUndoCommand` on the
   project's undo stack, and put it back to 0 on deletion or rename. This needs
   a new merge id in `dev_doc/ID_of_QUndoCommand.txt`.

## Where the example comes from

"The example" is ambiguous. Three candidates: the real project
`Schema 22.04.26.qet` (a welding cabinet, with a generic `repartiteur-gris`
element carrying C25N3220, LV429517, LV429410, LV429540, LV431543),
`examples/industrial.qet` (A9D34616, A9F74202, A9N15651, GV2ME08) and
`examples/affuteuse_250h.qet` (IC60N D32, C60ND1A). All the references found
are covered except GV2ME08 (TeSys, not drawn) and C25N3220 (not found).
`examples/schneider_example.qet` has six folios: the symbols, the Acti9 front
views, the NSX front views, the INS250, the NSX250 with its accessories linked
by label, and the front views of the NSX250 with its accessories.

## Verified and uncertain

The references were checked on public pages of the manufacturer; compatibility
lists come from these pages only. Not verified: C25N3220; the width of the iID
2P; the INS250 dimensions (taken equal to the NSX250); the terminal names of
the accessories (C/NC/NO, M1/M2, D1/D2 are assumed); the motor mechanism and
auxiliary contacts for the INS250; the other iC60 accessories (shunt trip,
etc.), neither drawn nor listed; the IP degree of LV429337 (IP40 on one page,
IP43 on another). The Vigi iC60 and iDPNa are discontinued or replaced. The
iC60 plus Vigi assembly is not drawn. The Vigi has no list, so an iOF labelled
under a Vigi gives "no list". The layouts and colours are generic.
