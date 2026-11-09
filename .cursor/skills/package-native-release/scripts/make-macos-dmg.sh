#!/usr/bin/env bash
# Build PS2.app with bundled SDL dylibs and wrap it in a redistributable DMG.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"

APP_BUNDLE="${APP_BUNDLE:-PS2.app}"
DMG_VOLUME_NAME="${DMG_VOLUME_NAME:-PS2 System Menu}"
DIST_DIR="${DIST_DIR:-dist}"
SKIP_BUILD="${SKIP_BUILD:-0}"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "error: macOS DMG packaging must run on macOS" >&2
  exit 1
fi

if [[ "$SKIP_BUILD" != "1" ]]; then
  make clean
  make bundle
fi

if [[ ! -x "$APP_BUNDLE/Contents/MacOS/ps2" ]]; then
  echo "error: missing $APP_BUNDLE/Contents/MacOS/ps2 — run make bundle first" >&2
  exit 1
fi

VERSION="${VERSION:-}"
if [[ -z "$VERSION" && -f "$APP_BUNDLE/Contents/Info.plist" ]]; then
  VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP_BUNDLE/Contents/Info.plist" 2>/dev/null || true)"
fi
VERSION="${VERSION:-2.0}"
ARCH="$(uname -m)"
DMG_NAME="PS2-System-Menu-${VERSION}-macos-${ARCH}.dmg"

FRAMEWORKS="$APP_BUNDLE/Contents/Frameworks"
BINARY="$APP_BUNDLE/Contents/MacOS/ps2"
mkdir -p "$FRAMEWORKS"

# Resolve Homebrew SDL2 (+ SDL3, required by sdl2-compat at runtime).
# Binary may already be rewritten to @rpath from a prior packaging pass.
SDL2_LINK="$(otool -L "$BINARY" | awk '/libSDL2.*\.dylib/ {print $1; exit}')"
SDL2_SRC=""
if [[ -n "$SDL2_LINK" && -f "$SDL2_LINK" ]]; then
  SDL2_SRC="$SDL2_LINK"
else
  SDL2_PREFIX="$(brew --prefix sdl2-compat 2>/dev/null || brew --prefix sdl2 2>/dev/null || true)"
  for c in \
    "${SDL2_PREFIX:+$SDL2_PREFIX/lib/libSDL2-2.0.0.dylib}" \
    "${SDL2_PREFIX:+$SDL2_PREFIX/lib/libSDL2.dylib}" \
    "/opt/homebrew/opt/sdl2-compat/lib/libSDL2-2.0.0.dylib" \
    "/opt/homebrew/opt/sdl2/lib/libSDL2-2.0.0.dylib"
  do
    [[ -z "$c" ]] && continue
    if [[ -f "$c" ]]; then
      SDL2_SRC="$c"
      break
    fi
  done
fi

if [[ -z "$SDL2_SRC" || ! -f "$SDL2_SRC" ]]; then
  echo "error: could not resolve libSDL2 (brew install sdl2)" >&2
  exit 1
fi

SDL2_NAME="$(basename "$SDL2_SRC")"
# Prefer the versioned soname the linker typically records.
if [[ "$SDL2_NAME" == "libSDL2.dylib" && -f "$(dirname "$SDL2_SRC")/libSDL2-2.0.0.dylib" ]]; then
  SDL2_SRC="$(dirname "$SDL2_SRC")/libSDL2-2.0.0.dylib"
  SDL2_NAME="libSDL2-2.0.0.dylib"
fi
cp -f "$SDL2_SRC" "$FRAMEWORKS/$SDL2_NAME"
chmod u+w "$FRAMEWORKS/$SDL2_NAME"

SDL3_PREFIX="$(brew --prefix sdl3 2>/dev/null || true)"
SDL3_CANDIDATES=(
  "${SDL3_PREFIX:+$SDL3_PREFIX/lib/libSDL3.dylib}"
  "${SDL3_PREFIX:+$SDL3_PREFIX/lib/libSDL3.0.dylib}"
  "/opt/homebrew/opt/sdl3/lib/libSDL3.dylib"
  "/usr/local/opt/sdl3/lib/libSDL3.dylib"
)
SDL3_SRC=""
for c in "${SDL3_CANDIDATES[@]}"; do
  [[ -z "$c" ]] && continue
  if [[ -f "$c" ]]; then
    SDL3_SRC="$c"
    break
  fi
done

if [[ -z "$SDL3_SRC" || ! -f "$SDL3_SRC" ]]; then
  echo "error: libSDL3.dylib not found (brew install sdl3) — required by sdl2-compat" >&2
  exit 1
fi

cp -f "$SDL3_SRC" "$FRAMEWORKS/libSDL3.dylib"
chmod u+w "$FRAMEWORKS/libSDL3.dylib"
SDL3_DIR="$(dirname "$SDL3_SRC")"
if [[ -f "$SDL3_DIR/libSDL3.0.dylib" ]]; then
  cp -f "$SDL3_DIR/libSDL3.0.dylib" "$FRAMEWORKS/libSDL3.0.dylib"
  chmod u+w "$FRAMEWORKS/libSDL3.0.dylib"
fi

# Point the app binary at bundled Frameworks via @rpath (skip if already rewritten).
CURRENT_SDL2="$(otool -L "$BINARY" | awk '/libSDL2/ {print $1; exit}')"
if [[ -n "$CURRENT_SDL2" && "$CURRENT_SDL2" != "@rpath/$SDL2_NAME" ]]; then
  install_name_tool -change "$CURRENT_SDL2" "@rpath/$SDL2_NAME" "$BINARY"
fi

# Ensure @rpath includes Frameworks (idempotent-ish: ignore duplicate errors).
install_name_tool -add_rpath "@executable_path/../Frameworks" "$BINARY" 2>/dev/null || true

# Normalize dylib ids so dyld resolves cleanly inside the bundle.
install_name_tool -id "@rpath/$SDL2_NAME" "$FRAMEWORKS/$SDL2_NAME"
install_name_tool -id "@rpath/libSDL3.dylib" "$FRAMEWORKS/libSDL3.dylib" 2>/dev/null || true

# Strip Finder/xattr detritus that breaks codesign, then ad-hoc sign
# (required on Apple Silicon after install_name_tool rewrites).
xattr -cr "$APP_BUNDLE" || true
codesign --force --deep --sign - "$APP_BUNDLE"

mkdir -p "$DIST_DIR"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

cp -R "$APP_BUNDLE" "$STAGE/"
ln -s /Applications "$STAGE/Applications"

# Optional drag-to-Applications hint for Finder DMG window.
DMG_PATH="$DIST_DIR/$DMG_NAME"
rm -f "$DMG_PATH"

hdiutil create \
  -volname "$DMG_VOLUME_NAME" \
  -srcfolder "$STAGE" \
  -ov -format UDZO \
  "$DMG_PATH"

echo "Created $DMG_PATH"
ls -lh "$DMG_PATH"
