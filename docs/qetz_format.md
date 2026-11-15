# The `.qetz` project file

A `.qetz` is a QElectroTech project saved as a zip archive. It holds the
same project as a `.qet`, split into files, with the pictures stored once
each as binary files instead of base64 text inside the XML. The full
description of the archive and of its database is
[`sources/container/QETZ-FORMAT.md`](../sources/container/QETZ-FORMAT.md);
this page is the short guide for users.

## Why

A large project is mostly base64 text. The reference project used to
measure this (58 folios, 3868 symbols, 1414 wires, 498 pictures) is 70 MB
as a `.qet`, 59 MB of it pictures, many of them the same picture several
times. As a `.qetz` it is 11 MB: the 498 pictures become 84 files (one per
distinct picture, named by its SHA-256), compressed once.

## Using it

- **Open**: *File > Open* takes a `.qetz` as it takes a `.qet`.
- **Save as .qetz**: *File > Save as* offers *Zipped project QElectroTech
  (*.qetz)* first; choosing a name ending in `.qet` saves a plain `.qet`.
  Saving a project opened from a `.qetz` writes a `.qetz` again. *Save as*
  is the conversion from `.qet` to `.qetz`; *File > Save a copy for an
  older QElectroTech* writes the `.qet` back.
- **Convert from the command line**, without opening the program window:

  ```sh
  qelectrotech --convert-to-qetz project.qet  project.qetz
  qelectrotech --convert-to-qet  project.qetz project.qet
  ```

  The input file is never modified. The output must end in `.qetz` or
  `.qet` and must not be the input. A project open elsewhere (its lock file
  beside it) is not overwritten unless `--ignore-lock` is given.
  `--resave <in> <out>` does the same with the output type taken from the
  extension, and also writes `.qetdb`.

## Compatibility

- A `.qet` opens and saves exactly as before; nothing is converted unless
  you ask.
- `.qetz` -> `.qet` writes the file a plain `.qet` save of the project
  gives. On the reference project the result of `.qet` -> `.qetz` -> `.qet`
  is byte for byte the file `--resave` writes straight from the `.qet`, and
  the project's counts (`--info`) and every picture are the same.
- A `.qetz` is written to a temporary file and renamed over the old one
  only when complete (`QSaveFile`), so a crash leaves the previous file.
- A version of QElectroTech that does not know `.qetz` cannot open it. Use
  *File > Save a copy for an older QElectroTech*, or `--convert-to-qet`.
- Tools that read the project XML directly (scripts, other programs) must
  read `project.xml` and `folios/*.xml` from the archive, or be given a
  `.qet` made with `--convert-to-qet`. Pictures are files in `images/`,
  referenced by `qetz-file`.

## The zip library

The archive is read and written in memory with the bundled
[miniz](../sources/container/miniz/README.md) (MIT licence, two source
files, compatible with the GPL of QElectroTech): no new dependency to
install.

## Measures

Reference project, `RelWithDebInfo` build (MinGW, Qt 6.8.3), headless
`--info` / `--resave`, on a busy shared Windows machine, so read these as
orders of magnitude. CPU time of the process, open = open and build every
folio:

| | `.qet` | `.qetz` |
|---|---|---|
| File size | 70.2 MB | 11.2 MB |
| Open (CPU) | 104 s | 85 s |
| Re-save, folios unchanged (wall) | 144 s | 25 s |

Opening is still dominated by building the 3868 symbols and 1414 wires, not
by reading the file. The save is faster because a folio that was not
modified is copied as it was read. Wall times of the same runs moved by
more than a factor of two with the machine's load.

## Tests

`tst_qetzip` and `tst_qetcontainer` cover the archive and its content;
`tst_cliconvert` converts a project to `.qetz` and back with the real
binary and compares the result.
