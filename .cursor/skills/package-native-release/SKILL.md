---
name: package-native-release
description: >-
  Build redistributable macOS DMG and Windows ZIP packages for the PS2 System
  Menu (SDL2 C++ native app), including bundling SDL dylibs/DLLs and optional
  GitHub Releases upload. Use when the user asks to make a DMG, Windows zip,
  installer, distributable build, release artifact, or package the app for
  other devices.
---

# Package native release (macOS DMG + Windows ZIP)

Package this repo's native app so other people can run it without compiling.

**Defaults for this project**

| Setting | Value |
| :--- | :--- |
| App name | `PS2 System Menu` |
| Bundle / binary | `PS2.app` / `ps2` (mac), `ps2.exe` (Windows when present) |
| Build | `make` / `make bundle` |
| Output dir | `dist/` |

Do **not** rewrite app source for cross-platform as part of this skill. Packaging only.

## Workflow checklist

```
Progress:
- [ ] 1. Confirm platform request (mac DMG / Windows ZIP / both / GitHub Release)
- [ ] 2. Build + verify locally
- [ ] 3. Run packaging script(s)
- [ ] 4. Smoke-check the artifact
- [ ] 5. (Optional) Upload with gh release
```

## macOS DMG

**Run from repo root:**

```bash
bash .cursor/skills/package-native-release/scripts/make-macos-dmg.sh
```

Script does:

1. `make clean && make bundle`
2. Copies Homebrew `libSDL2` + `libSDL3` into `PS2.app/Contents/Frameworks/`
3. Rewrites install names / `@rpath` so the `.app` runs without Homebrew
4. Ad-hoc codesigns the bundle
5. Builds `dist/PS2-System-Menu-<version>-macos-<arch>.dmg` via `hdiutil`

**Overrides (env):**

- `APP_BUNDLE` — default `PS2.app`
- `VERSION` — default from `Info.plist` or `2.0`
- `DMG_VOLUME_NAME` — default `PS2 System Menu`
- `SKIP_BUILD=1` — package existing bundle only

**Smoke check:** mount the DMG, open the app once, quit, detach.

**Common failure:** app crashes on another Mac → SDL dylibs not bundled or rpaths wrong. Re-run the script; do not hand-copy a bare `ps2` binary into a DMG.

## Windows ZIP

This codebase is currently macOS-native (CoreText / Cocoa / `open`). A Windows ZIP is only valid when a Windows build already exists.

**If `ps2.exe` (or `dist/windows/ps2.exe`) is missing:**

1. Tell the user packaging cannot invent a Windows binary.
2. Stop. Do not fabricate stubs or claim Windows support.

**When a Windows build exists, run:**

```bash
bash .cursor/skills/package-native-release/scripts/make-windows-zip.sh
```

Script expects:

- `dist/windows/ps2.exe` (or set `WINDOWS_EXE`)
- SDL2 runtime DLL beside the exe (copies from `SDL2_DLL` if provided)

Produces: `dist/PS2-System-Menu-<version>-windows-x64.zip`

**Manual Windows layout (if building elsewhere):**

```
ps2.exe
SDL2.dll          # required; ship the DLL matching the toolchain
README.txt        # short run notes
```

Zip that folder. No installer required for v1.

## GitHub Release upload

Only after artifacts exist in `dist/` and `gh` auth works:

```bash
VERSION=2.0
gh release create "v${VERSION}" \
  dist/PS2-System-Menu-*-macos-*.dmg \
  dist/PS2-System-Menu-*-windows-*.zip \
  --title "v${VERSION}" \
  --notes "macOS DMG and Windows ZIP. Unofficial fan project; not affiliated with Sony."
```

If `gh auth status` fails, stop and ask the user to run `gh auth login -h github.com`. Do not paste tokens into the skill or chat logs.

Omit the Windows glob if that artifact was not built.

## Agent rules

- Prefer the scripts in `scripts/` over one-off shell; fix the script if it breaks.
- Never commit `dist/`, `*.dmg`, `*.zip`, `PS2.app`, or Homebrew-copied dylibs unless the user explicitly asks.
- Never claim the app is Windows-ready unless a real Windows binary was packaged.
- Keep macOS packaging working even if Windows is skipped.

## Extra detail

- DMG layout / codesign notes: [macos-dmg.md](macos-dmg.md)
- Windows zip expectations: [windows-zip.md](windows-zip.md)
