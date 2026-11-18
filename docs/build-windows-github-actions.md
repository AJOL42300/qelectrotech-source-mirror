# Windows build on GitHub Actions

The workflow `.github/workflows/build-windows.yml` builds QElectroTech for
Windows on GitHub's servers (Qt 6.8.3 with MinGW, CMake and Ninja, without
KDE Frameworks and without tests) and publishes a portable folder as a zip.

## Start a build

- Automatically: push to a branch named `ajol42300-*` of the fork.
- By hand: GitHub, tab **Actions**, workflow **Build Windows (zip)**,
  **Run workflow**, then choose the branch. With the GitHub command line:

```sh
gh workflow run build-windows.yml --ref <branch>
gh run watch
```

## Get the zip

- Web: open the run, then **Artifacts** at the bottom of the summary page.
- Command line:

```sh
gh run download <run-id> --name "QElectroTech-win64-qt6.8.3-<commit>"
```

The zip holds a `QElectroTech` folder: `bin/QElectroTech.exe` with the Qt
libraries, and `elements/`, `titleblocks/`, `examples/`, `lang/` (the `.qm`
files built by `lrelease`). Unzip it anywhere and run `bin/QElectroTech.exe`.
The folder is self-contained: the job runs `windeployqt` (through
`QET_DEPLOY_QT=ON` and `cmake --install`), which copies the Qt DLLs, the
plugins (`plugins/platforms/qwindows.dll`..., found through `bin/qt.conf`) and the MinGW runtime DLLs
(`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`). Before
zipping, the job checks these files, then starts the packaged exe with
`--version` and a minimal `PATH` (no Qt, no MinGW): if a DLL or plugin is
missing, the job fails and no zip is published. Do not run the exe from the
`build` folder: it is not deployed there and depends on the `PATH`.
Artifacts are kept 90 days by default.

## Notes

- The `elements` submodule is fetched with the checkout
  (`submodules: recursive`).
- SQLite is compiled in the job from the official amalgamation, because the
  history of saves needs its session extension; it is linked statically.
- The existing `windows-build.yml` (MSYS2, installer, tests) is unchanged.
