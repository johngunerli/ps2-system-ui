# macOS DMG notes

## Why SDL must be bundled

`make` links against Homebrew:

```
/opt/homebrew/opt/sdl2-compat/lib/libSDL2-2.0.0.dylib
```

`sdl2-compat` also **dlopens** `libSDL3.dylib` at runtime (`@loader_path` / `@executable_path`). A DMG that only contains `PS2.app` without those dylibs will launch on the build machine and fail everywhere else.

The packaging script copies both into `Contents/Frameworks/` and sets:

- binary: `@rpath/libSDL2-…`
- rpath: `@executable_path/../Frameworks`

## Codesigning

After `install_name_tool`, binaries must be re-signed. The script uses ad-hoc signing (`codesign --sign -`). That is enough for local / GitHub Release downloads on many Macs; Gatekeeper may still require right-click → Open the first time.

Developer ID + notarization is out of scope unless the user asks and has certificates.

## DMG layout

```
PS2 System Menu/
  PS2.app
  Applications -> /Applications
```

Users drag the app into Applications. Format: `UDZO` (compressed).

## Verifying linkage

```bash
otool -L PS2.app/Contents/MacOS/ps2
# expect @rpath/libSDL2-… not /opt/homebrew/...

ls PS2.app/Contents/Frameworks
# expect libSDL2-*.dylib and libSDL3.dylib
```
