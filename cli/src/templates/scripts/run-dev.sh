#!/bin/bash
set -e

# ── Rune Dev Launcher ───────────────────────────────────
# Launched by `rune dev`. Starts the Rune C++ runtime in
# dev mode with hot-reload support.
# ────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

# Find or build the Rune core binary
RUNE_CORE=""

# Check common locations
for path in \
  "$PROJECT_DIR/build/src/rune-core" \
  "$PROJECT_DIR/../build/src/rune-core" \
  "$HOME/.rune/rune-core" \
  "/usr/local/bin/rune-core"; do
  if [ -x "$path" ]; then
    RUNE_CORE="$path"
    break
  fi
done

if [ -z "$RUNE_CORE" ]; then
  echo "  Building Rune core..."
  cd "$PROJECT_DIR"
  mkdir -p build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Debug
  make -j$(nproc)
  RUNE_CORE="$PROJECT_DIR/build/src/rune-core"
  cd "$PROJECT_DIR"
fi

echo "  Runtime: $RUNE_CORE"
echo "  App dir: $PROJECT_DIR"
echo ""

# Launch with X11 backend for stability
exec env GDK_BACKEND=x11 "$RUNE_CORE" --app-dir "$PROJECT_DIR" "$@"
