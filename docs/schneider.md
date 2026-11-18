# Schneider Electric: breakers, contactors, front views and accessories

Vector redrawings (no photo, logo or manufacturer text) of Schneider
Electric circuit breakers, residual current devices (RCD), disconnectors and
TeSys D contactors, each as an IEC symbol and as a front view, with their
accessories. The
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
| `accessories` | MT250 motor mechanism (LV431541 220/240 V AC, LV431543 24/30 V DC), MN undervoltage release (LV429410), MX shunt trip (LV429387), OF/SD/SDE/SDV contact (29450), iOF contact (A9A26924), iMN undervoltage release (A9A26960), iMX+OF shunt trip with contact (A9A26946), rotary handle (LV429337), terminal shield (LV429517) |
| `tesys_d` | TeSys D contactors 3P, built-in 1NO+1NC: LC1D09P7, LC1D12P7, LC1D18P7, LC1D25P7, LC1D32P7, LC1D38P7 (230 V AC coil), LC1D09BD (24 V DC coil) |
| `tesys_d_accessories` | LADN front-mounting auxiliary blocks: LADN22 (2NO+2NC), LADN11 (1NO+1NC), LADN20 (2NO), LADN02 (2NC) |

The accessories are separate elements, so they can be placed next to a device
or on another folio. The handle and the shield are mechanical: they have a
front view without terminals (the `--check-elements` warning "0 terminals" is
expected).

### Scale

Acti9 and TeSys D: one 9 mm module is 30 px, so an 18 mm pole is 60 px
(3.33 px/mm).
ComPacT NSX and INS: the 35 mm pole pitch is 120 px (3.43 px/mm). The two
differ by 3 %; this is a drawing choice, not a measurement. Each front view
shows the device body, the toggle, the position indicator, the test button of
the RCDs and the rating as dynamic text.

## TeSys D contactors and terminal numbers

The contactor symbol has the three power poles (terminals `1/L1`-`2/T1`,
`3/L2`-`4/T2`, `5/L3`-`6/T3`), the coil (`A1`, `A2`), the built-in normally
open contact (`13`, `14`) and normally closed contact (`21`, `22`), joined by
a dashed mechanical link. The front view has the same 12 terminal names:
power terminals at the top and bottom, coil and auxiliary terminals on the
flanks. The numbers are drawn next to the terminals on the symbols and on the
front views. The front view follows a public photo of an LC1D09 (rev. 115):
power terminals `1/L1`, `3/L2`, `5/L3` on the top and `2/T1`, `4/T2`, `6/T3`
on the bottom, a row of control terminals above the black front cover
(`13 NO`, `21 NC`, `A1`) and another under it (`14 NO`, `22 NC`, `A2`), a
round recess with a cross-head screw per terminal, a mounting lug, side
latches, a green band with the designation and the coil voltage, and a
decorative check pattern in the marking zone. The terminal names are the
same as before; only the positions of `A1`, `A2`, `13`, `14`, `21`, `22`
moved from the flanks to these two rows.

| Product | Numbers | Status |
| --- | --- | --- |
| Contactors LC1D09 to D38, poles | 1/L1, 2/T1, 3/L2, 4/T2, 5/L3, 6/T3, coil A1/A2 | standard marking |
| Contactor built-in contacts | 13-14 NO, 21-22 NC | count 1NO+1NC verified; the numbers follow the EN 50012 convention (ASSUMED) |
| LADN22 | 53-54 NO, 61-62 NC, 71-72 NC, 83-84 NO, left to right | VERIFIED: [FA130288](https://www.se.com/au/en/faqs/FA130288/), [FA116636](https://www.se.com/us/en/faqs/FA116636/) |
| LADN11, LADN20, LADN02 | the same positions as LADN22 (53-54 + 61-62; 53-54 + 83-84; 61-62 + 71-72) | ASSUMED (subset of the LADN22 marking) |
| 29450 (OF/SD/SDE/SDV) | 11, 12, 14 (the OF1 slot) | ASSUMED; the marking depends on the slot (21-22-24, 31-32-34, 81-82-84, 91-92-94 for the others) |
| A9A26924 (iOF) | 11, 12, 14 | ASSUMED; the public diagram is a picture only |

The LADN "G" variants use another marking (31-32, 41-42, 53-54, 63-64) and
are not drawn. The contactors are compatible with the four LADN blocks: they
are seeded as articles of quantity 0 and take quantity 1 when a block is
labelled with the prefix of the contactor (`-KM1`, `-KM1.1`), like the NSX
accessories below.

Sources (public pages, checked): LC1D09BD
(<https://www.se.com/uk/en/product/LC1D09BD/>: 45 x 77 x 95 mm, 24 V DC coil,
9 A AC-3, 1NO+1NC), LC1D38P7 (<https://www.se.com/uk/en/product/LC1D38P7/>:
38 A, 230 V AC coil, 1NO+1NC, 45 mm), LADN22
(<https://www.se.com/uk/en/product/LADN22/>: 2NO+2NC, screw clamp).

## Colours

The front views use the colours of the public product photos, as far as an
element allows it. An element (`.elmt`) can only use the named colours of the
element editor, so the Schneider green (about #3DCD58 on the photos) is drawn
with the nearest one, `HTMLGreenLimeGreen` (#32CD32). Housings are
`HTMLGrayGainsboro` (light) or `HTMLGrayDimGray` (dark), terminals and screws
`HTMLGraySilver`, label windows white, and the status flags red or yellow
by function. The green is a plain accent band: no logo is drawn. All the
colours and shapes are ASSUMED from the photos (not measured), and the
generator keeps them in a few constants (`GREEN`, `LIGHT`, `DARK`...).

### Photo-inspired shapes (rev. 113, 115)

The Acti9 modular front views (breakers, RCDs, isolators) follow a public
photo of an iC60N 3P: a white housing with a light terminal stage at the top
and at the bottom, a round recess with a cross-head screw per pole, two
yellow fixing clips on the top, a white label band with a green line and a
check pattern in the marking zone, one white handle per pole with a green
position window and a dark tie bar, lightly shaded flanks. The RCDs keep
their test button and sensitivity text; the fuse isolator keeps its carriers.
The photos were only looked at, nothing is copied: no logo, no manufacturer
text, no raster. Shapes, proportions and colours are ASSUMED.

### Auxiliaries and releases (rev. 114)

Each auxiliary has a clear IEC symbol and a front view, with numbered
terminals that are visible (not hidden under the body) and the commercial
reference written on the module:

| Reference | Product | Terminals | Source / status |
| --- | --- | --- | --- |
| A9A26960 | Acti9 iMN undervoltage release, 220-240 V AC, 2 modules | D1, D2 | reference and voltage VERIFIED ([se.com](https://www.se.com/uk/en/product/A9A26960/)); terminal marking ASSUMED |
| A9A26946 | Acti9 iMX+OF shunt trip with OF contact, 220-415 V AC | C1, C2 + 11, 12, 14 | reference and voltage VERIFIED ([se.com](https://www.se.com/uk/en/product/A9A26946/)); marking ASSUMED |
| LV429387 | ComPacT NSX MX shunt trip, 220/240 V AC | C1, C2 | VERIFIED ([se.com](https://www.se.com/il/en/product/LV429387/)); marking ASSUMED |
| LV429410 | ComPacT NSX MN undervoltage release, 24 V DC | D1, D2 | VERIFIED earlier; marking ASSUMED |
| 29450, A9A26924 | OF/SD/SDE/SDV and iOF contacts | 11, 12, 14 | see the table above |

Other Acti9 references seen but not drawn: A9A26948 (iMX+OF 12-24 V AC),
A9A26969 (iMNx 220-240 V AC), A9A26927 (iSD fault contact, discontinued on
1 September 2023). The coil voltage selects the reference, so one element per
voltage is needed: only one voltage per release is drawn so far.

Every element of the library carries `manufacturer` (Schneider Electric),
`manufacturer_reference`, `designation` and `description` in its element
information, so the nomenclature can use it directly; the iC60N 3P, the other
Acti9 devices and the NSX devices also list these releases as compatible
accessories of quantity 0.

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
`examples/schneider_example.qet` has eight folios: the symbols, the Acti9 front
views, the NSX front views, the INS250, the NSX250 with its accessories linked
by label, the front views of the NSX250 with its accessories, and the TeSys D
contactors with their LADN blocks (symbols with terminal numbers, front
views).

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

Contactors: the height and depth of LC1D12 to D38 (77 x 95 mm taken from the
D09 and D38 pages), the layout of the coil and auxiliary terminals on the
flanks of the front view, the numbers 13-14/21-22 of the built-in contacts,
the marking of LADN11/20/02, the marking of the 29450 slots other than OF1,
the iOF 11-12-14, the LADN dimensions (generic 45 mm block), and every colour
(palette limited to the named colours). Photo-inspired shapes (rev. 113 and
115) and the new releases (rev. 114): the compatibility of iMN/iMX+OF with each
Acti9 device (taken as the same list as the iOF, from the public pages), the
terminal marking D1/D2 and C1/C2 of the releases, the 2-module width of the
iMX+OF, the choice of one voltage per release. Not done: the other coil
voltages and the iSD/iMNx/iMX+SD references. Not done: LADC22 and other LADN
variants, ratings above D38 (LC1D40A and up), the other TeSys ranges (F, K),
and a terminal-number overlay on the breaker symbols. `build/elements` is a
copy of the library, refreshed by copying the `schneider` folder.
