# Windows ZIP notes

## Current project reality

The app uses macOS-only APIs (`CoreText`, `CoreGraphics`, Cocoa frameworks, `osascript` folder picker, `open` launcher). `make` on this repo does **not** produce `ps2.exe`.

Packaging ≠ porting. The Windows script only zips an existing build.

## When a Windows build exists

Minimum ship folder:

| File | Purpose |
| :--- | :--- |
| `ps2.exe` | App binary |
| `SDL2.dll` | Must match the SDL2 version/toolchain used at link time |
| `README.txt` | Short launch notes (script writes this) |

Optional later: VC++ redistributable note, game controller drivers (usually not needed for XInput pads).

## Suggested future Windows build (not part of packaging)

Only if the user separately asks to port:

- Replace CoreText renderer with SDL_ttf or FreeType
- Folder picker via Win32 `IFileDialog` or a tiny cross-platform helper
- Launch via `ShellExecuteW` / `CreateProcessW`
- Scan `.exe` / `.lnk` instead of `.app`
- Build with CMake + MSVC or MinGW + SDL2

Until that lands, refuse to invent Windows artifacts.
