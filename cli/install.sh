#!/bin/bash
set -e

# ── Rune CLI Installer ──────────────────────────────────
# curl -fsSL https://raw.githubusercontent.com/... | bash
# ────────────────────────────────────────────────────────

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}◈ Rune CLI Installer${NC}"
echo ""

TARGET_DIR="$HOME/.local/bin"
TARGET="$TARGET_DIR/rune"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Check dependencies ──────────────────────────────
echo -n "  Node.js ... "
if command -v node &>/dev/null; then
  echo -e "${GREEN}$(node --version)${NC}"
else
  echo -e "${RED}NOT FOUND${NC}"
  echo "  Install: https://nodejs.org"
  exit 1
fi

echo -n "  yarn ... "
if command -v yarn &>/dev/null; then
  echo -e "${GREEN}$(yarn --version)${NC}"
else
  echo -n "  installing via corepack... "
  corepack enable 2>/dev/null && echo -e "${GREEN}OK${NC}" || {
    echo -e "${RED}FAILED${NC}"
    echo "  Install: npm install -g yarn"
    exit 1
  }
fi

# ── Build CLI ───────────────────────────────────────
echo ""
echo "Building Rune CLI..."
cd "$SCRIPT_DIR"

yarn install --frozen-lockfile 2>/dev/null || yarn install
yarn build

# ── Install binary ──────────────────────────────────
mkdir -p "$TARGET_DIR"

if [ -f "$TARGET" ]; then
  echo -n "  Removing old install... "
  rm -f "$TARGET"
  echo "OK"
fi

if [ -f "dist/index.js" ]; then
  cp dist/index.js "$TARGET"
  chmod +x "$TARGET"
  echo -e "  Binary -> ${GREEN}$TARGET${NC}"
elif [ -f "src/index.ts" ]; then
  # Fallback: use tsx wrapper
  cat > "$TARGET" << 'WRAPPER'
#!/bin/bash
exec node --import tsx "$SCRIPT_DIR/src/index.ts" "$@"
WRAPPER
  chmod +x "$TARGET"
  echo -e "  Binary -> ${GREEN}$TARGET${NC} (dev mode — tsx)"
else
  echo -e "${RED}Error: No entry point found${NC}"
  exit 1
fi

# ── Check PATH ──────────────────────────────────────
if [[ ":$PATH:" != *":$TARGET_DIR:"* ]]; then
  echo ""
  echo -e "${CYAN}⚠  Add to PATH:${NC}"
  echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
  echo "  (add to ~/.bashrc or ~/.zshrc)"
fi

# ── Done ────────────────────────────────────────────
echo ""
echo -e "${GREEN}◈ Rune CLI installed!${NC}"
echo ""
echo "  rune init my-app    Create a new project"
echo "  rune dev            Start dev server"
echo "  rune doctor         Check environment"
echo ""
