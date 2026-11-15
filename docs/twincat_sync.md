# Exchange I/O names with TwinCAT

QElectroTech (QET) can compare the I/O tables of the PLC cards of a project
with the I/O tree of a TwinCAT 3 project, and copy names and comments in
either direction, on request. Nothing runs by itself, and TwinCAT is never
started: QET only reads and writes the project files.

## Quick start

1. Open the project, then **Project > Exchange I/O names with TwinCAT...**.
2. Browse to the TwinCAT project (`.tsproj`, `.xti`, `.sln` or the folder
   that holds them) and press **Read and compare**. The path is saved in the
   QET project.
3. For each difference, choose the direction: **QET -> TwinCAT**,
   **TwinCAT -> QET** or nothing. The buttons *All QET -> TwinCAT*,
   *All TwinCAT -> QET* and *None* set every row at once.
4. **Preview** lists what would change on each side, and **Apply** does it.

If a QET card has no TwinCAT box with the same identifier, the top table of
the dialog lets you pair it with a box by hand.

## What is compared

A QET *card* is a PLC master symbol with rows in its I/O table. A TwinCAT
*box* is an EtherCAT device, coupler or terminal. A *channel* is a process
data entry of a box (Inputs / Outputs).

| QET                              | TwinCAT                              |
|----------------------------------|--------------------------------------|
| card label, e.g. `BOX 03.10`     | box name without its type, `BOX 03.10 (EL1008)` |
| row address `X01(3)`, or the row position | n-th channel of the same direction |
| function text of the row         | name of the channel                  |
| comment of the row               | comment of the channel               |
| row type (digital, analog...)    | kind of the channel                  |

The channel number is the number at the end of the address (`X01(3)` is
channel 3). Without one, the position of the row among the rows of the same
direction is used. Positions do not move when channels are renamed.

The differences are:

| Difference               | QET -> TwinCAT | TwinCAT -> QET |
|--------------------------|:--------------:|:--------------:|
| Box identifier differs   | yes            | no             |
| Channel names differ     | yes            | yes            |
| Channel comments differ  | yes            | yes            |
| Channel only in TwinCAT  | no             | adds a row     |
| Channel only in QET      | report only    | report only    |
| Digital against analog   | report only    | report only    |
| Card without a box       | report only    | report only    |
| Box without a card       | report only    | report only    |

A name that TwinCAT gives by itself (`Channel 1`, `Input 3`, `AI Standard
Channel 2`...) never overwrites a name written in QET.

## The structure of the TwinCAT tree is never changed

The TwinCAT tree describes the real hardware. QET never adds, removes, moves
or reorders anything in it, and never changes addresses, types,
`ProductRevision` or the layout of the process data. Only these values are
written, on nodes that exist:

- the name of a box, which keeps its `(type)` suffix: `BOX 03.10 (EL1008)`
  gets the QET identifier in front of the type;
- the name of a channel (a process data variable);
- the comment of a channel.

On the QET side, TwinCAT -> QET can add rows to the I/O table of an existing
card, and change names and comments. It never creates cards.

## Writing to the TwinCAT project

The write is a minimal text patch: the file is scanned, only the characters
of the changed values are replaced, and everything else (indentation, line
endings, BOM, attribute order) stays byte for byte as it was. Before any
file is replaced:

- QET checks that the file did not change since it was read (SHA-256). If it
  did, nothing is written and you are asked to read again;
- the patched text is scanned again and compared with the original: if
  anything other than the intended values differs, the write is refused;
- a backup `<file>.<yyyyMMdd-HHmmss>.bak` is created next to every file to
  be changed, before the first write;
- the files are replaced one by one, and if one fails the others are put
  back from their backups.

**Reload the project in TwinCAT XAE afterwards.** XAE does not see the
change by itself, and saving from XAE over the changed files would drop it.
Close the project in XAE, or accept the reload prompt, before applying.

### Renaming and the references to a name

A box or channel name can appear in other places of the project: the owner
of a link (`OwnerA`, `OwnerB`), the variable links (`VarA`, `VarB`), the
`IoItemPathName` of a mapping. When a name changes, QET rewrites these
references so that they follow it. A reference QET does not know how to
rewrite makes the dialog show a **warning** with its location; the preview
lists every reference that will be rewritten. Check the warnings before you
apply.

Names in safety projects that embed a box name are not rewritten.

## From TwinCAT to QET

The rows changed by TwinCAT -> QET are applied by one undo command
(**Exchange I/O names with TwinCAT**): one *Undo* restores every card. The
QET project is modified only if something differs, and the QET step is run
after the TwinCAT write has succeeded.

## Command line report

A read-only report of the differences, with no window:

```
qelectrotech --twincat-report <project.qet> <report.csv> [project.tsproj]
```

The third argument is optional if the project already has a TwinCAT path.
The CSV uses `;`, UTF-8 with a byte-order mark, and the columns `kind`,
`label`, `qet`, `twincat`. Nothing is written to either project.

The same CSV is a handy way to review the differences in a spreadsheet; it
is a report, not an import format.

## Project file

The path is stored in the QET project as

```xml
<twincat_sync path="C:/Dev/Machine/Machine.tsproj"/>
```

The element is written only when a path is set, so projects that do not use
this feature are saved exactly as before, and old projects open unchanged.

## Limits

- QET reads the XML of the `.tsproj` / `.xti` directly. The layout of
  TwinCAT 3 files is not documented: channel names are read from the `Name`
  of the process data (`Pdo`) and comments from its first `Entry`. If your
  TwinCAT version stores them elsewhere, the differences will be wrong:
  check with **Preview** and the `.bak` file first, on a copy.
- Rewriting of links is best effort (see above).
- TwinCAT is never started. The scripts of TwinCAT's Automation Interface
  are not used.
- A channel or card present on one side only can only be reported.
