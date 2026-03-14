#!/bin/bash
# fundamental-cli smoke test (Linux)

echo "========================================"
echo "fundamental-cli smoke test (Linux)"
echo "========================================"
echo

ORIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SMOKE_DIR="$ORIG_DIR/../fundamental-cli-smoke"
FUNDAMENTAL_DIR="$ORIG_DIR/../fundamental"
PASS=0
FAIL=0

pass() { echo "[PASS] $1"; PASS=$((PASS + 1)); }
fail() { echo "[FAIL] $1"; FAIL=$((FAIL + 1)); exit 1; }

check_file() {
    if [ -f "$1" ]; then
        pass "$1 exists"
    else
        fail "$1 missing"
    fi
}

# -----------------------------------------------------------------------
echo "[SETUP] Clean and create smoke directory"
rm -rf "$SMOKE_DIR"
mkdir -p "$SMOKE_DIR"
pass "smoke dir created"

# -----------------------------------------------------------------------
echo
echo "[BUILD] Build fundamental-cli"
cd "$ORIG_DIR"
./build-linux-amd64.sh || fail "build-linux-amd64.sh failed"
pass "fun built"

# -----------------------------------------------------------------------
echo
echo "[SETUP] Copy fun to smoke dir"
cp ./fun "$SMOKE_DIR/fun" || fail "copy fun failed"
chmod +x "$SMOKE_DIR/fun"
pass "fun copied and marked executable"

# -----------------------------------------------------------------------
cd "$SMOKE_DIR"

echo
echo "[INIT] Run: fun init"
./fun init || fail "fun init returned non-zero"
pass "fun init exited 0"

# -----------------------------------------------------------------------
echo
echo "[CHECK] Files created by fun init"
check_file "src/main.c"
check_file "src/cli.c"
check_file "src/cli.h"
check_file "src/commands/cmd_version.c"
check_file "src/commands/cmd_version.h"
check_file "src/commands/cmd_help.c"
check_file "src/commands/cmd_help.h"
check_file "build-linux-amd64.sh"
check_file "fun.ini"
check_file "README.md"

# -----------------------------------------------------------------------
echo
echo "[VENDOR] Copy fundamental into vendor/fundamental"
cp -r "$FUNDAMENTAL_DIR" vendor/fundamental || fail "cp fundamental failed"
pass "fundamental vendored"

check_file "vendor/fundamental/include/string/string.h"
pass "vendor sanity check passed"

# -----------------------------------------------------------------------
echo
echo "[BUILD] Run: fun build"
./fun build || fail "fun build returned non-zero"
pass "fun build exited 0"

# -----------------------------------------------------------------------
echo
echo "[CHECK] Build output"
check_file "app"

if [ ! -x "app" ]; then
    fail "app is not executable"
fi
pass "app is executable"

# -----------------------------------------------------------------------
echo
echo "[RUN] Execute app"
./app || fail "app exited non-zero"
pass "app ran successfully"

# -----------------------------------------------------------------------
echo
echo "========================================"
echo "Smoke test complete: $PASS passed, $FAIL failed"
echo "========================================"
