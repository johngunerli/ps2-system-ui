---
name: package-native-release
description: >-
  Package the PS2 System Menu native app into a redistributable macOS DMG
  (with bundled SDL dylibs) or Windows ZIP. Use when the user asks to make a
  DMG, Windows zip, installer, release artifact, or package the app for other
  devices.
---

# Package native release

Teach the agent how to produce installable artifacts for this repo. Do not add packaging shell scripts to the repo unless the user asks.

## Project defaults

| Setting | Value |
| :--- | :--- |
| App | PS2 System Menu |
| macOS bundle / binary | `PS2.app` / `ps2` |
| Build | `make` / `make bundle` |
| Output | `dist/` (gitignored) |

## macOS DMG

Run on macOS only.

1. `make clean && make bundle`
2. Create `PS2.app/Contents/Frameworks/`
3. Copy Homebrew `libSDL2-2.0.0.dylib` (from `otool -L` / `brew --prefix sdl2-compat` or `sdl2`) into Frameworks
4. Also copy `libSDL3.dylib` (and `libSDL3.0.dylib` if present) — `sdl2-compat` dlopens SDL3 at runtime
5. `chmod u+w` the copied dylibs
6. `install_name_tool -change` the binary’s SDL2 path to `@rpath/libSDL2-2.0.0.dylib`
7. `install_name_tool -add_rpath @executable_path/../Frameworks` on the binary
8. Set dylib ids to `@rpath/...`
9. `xattr -cr PS2.app` then `codesign --force --deep --sign - PS2.app`
10. Stage a temp folder with `PS2.app` + symlink `Applications` → `/Applications`
11. `hdiutil create -volname "PS2 System Menu" -srcfolder <stage> -ov -format UDZO dist/PS2-System-Menu-<version>-macos-<arch>.dmg`

Verify with `otool -L PS2.app/Contents/MacOS/ps2` — SDL2 must be `@rpath/...`, not `/opt/homebrew/...`.

## Windows ZIP

This codebase is macOS-native today (CoreText / Cocoa). Do not invent a Windows binary.

Only if `ps2.exe` and `SDL2.dll` already exist:

1. Put them in a folder with a short `README.txt` (keep DLL beside the exe)
2. Zip to `dist/PS2-System-Menu-<version>-windows-x64.zip`

If no Windows build exists, say so and stop.

## GitHub Release (optional)

After artifacts exist and `gh auth status` works:

```bash
gh release create "v<version>" dist/*.dmg dist/*.zip --title "v<version>" --notes "Unofficial fan project; not affiliated with Sony."
```

Omit globs for missing platforms. If auth fails, ask the user to run `gh auth login`.

## Rules

- Prefer one-shot shell in the terminal over committing `.sh` helpers
- Never commit `dist/`, `*.dmg`, `*.zip`, or `PS2.app` unless asked
- Never claim Windows support without a real Windows binary
