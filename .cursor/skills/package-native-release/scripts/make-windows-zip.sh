#!/usr/bin/env bash
# Package an existing Windows build into a redistributable ZIP.
# Does not compile Windows binaries — fails clearly if none are present.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"

DIST_DIR="${DIST_DIR:-dist}"
VERSION="${VERSION:-2.0}"
WINDOWS_EXE="${WINDOWS_EXE:-}"
SDL2_DLL="${SDL2_DLL:-}"

if [[ -z "$WINDOWS_EXE" ]]; then
  for candidate in \
    "dist/windows/ps2.exe" \
    "build/windows/ps2.exe" \
    "ps2.exe" \
    "Release/ps2.exe"
  do
    if [[ -f "$candidate" ]]; then
      WINDOWS_EXE="$candidate"
      break
    fi
  done
fi

if [[ -z "$WINDOWS_EXE" || ! -f "$WINDOWS_EXE" ]]; then
  cat >&2 <<'EOF'
error: no Windows binary found.

This project does not currently produce a Windows build from make.
Provide a built exe, then re-run:

  WINDOWS_EXE=/path/to/ps2.exe SDL2_DLL=/path/to/SDL2.dll \
    bash .cursor/skills/package-native-release/scripts/make-windows-zip.sh

Do not invent a stub exe or claim Windows support without a real build.
EOF
  exit 1
fi

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
OUT_DIR="$STAGE/PS2-System-Menu"
mkdir -p "$OUT_DIR"
cp -f "$WINDOWS_EXE" "$OUT_DIR/ps2.exe"

# Locate SDL2.dll next to the exe or via env.
if [[ -z "$SDL2_DLL" ]]; then
  EXE_DIR="$(cd "$(dirname "$WINDOWS_EXE")" && pwd)"
  for candidate in "$EXE_DIR/SDL2.dll" "dist/windows/SDL2.dll" "SDL2.dll"; do
    if [[ -f "$candidate" ]]; then
      SDL2_DLL="$candidate"
      break
    fi
  done
fi

if [[ -z "$SDL2_DLL" || ! -f "$SDL2_DLL" ]]; then
  echo "error: SDL2.dll not found. Set SDL2_DLL=/path/to/SDL2.dll" >&2
  exit 1
fi
cp -f "$SDL2_DLL" "$OUT_DIR/SDL2.dll"

cat > "$OUT_DIR/README.txt" <<EOF
PS2 System Menu (unofficial)

1. Keep ps2.exe and SDL2.dll in the same folder.
2. Double-click ps2.exe to launch.
3. Unofficial fan project — not affiliated with Sony Interactive Entertainment.
EOF

mkdir -p "$DIST_DIR"
ZIP_NAME="PS2-System-Menu-${VERSION}-windows-x64.zip"
ZIP_PATH="$DIST_DIR/$ZIP_NAME"
rm -f "$ZIP_PATH"

if command -v zip >/dev/null 2>&1; then
  (cd "$STAGE" && zip -r "$ROOT/$ZIP_PATH" "PS2-System-Menu")
elif command -v powershell.exe >/dev/null 2>&1; then
  powershell.exe -NoProfile -Command \
    "Compress-Archive -Path '$OUT_DIR' -DestinationPath '$ROOT/$ZIP_PATH' -Force"
else
  echo "error: need zip or powershell.exe to create the archive" >&2
  exit 1
fi

echo "Created $ZIP_PATH"
ls -lh "$ZIP_PATH"
