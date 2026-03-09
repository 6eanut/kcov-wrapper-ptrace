#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# kcov-wrapper-ptrace integration test runner
# Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
#
# Runs inside a QEMU-emulated RISC-V (or other arch) Linux VM with:
#   - CONFIG_KCOV=y
#   - debugfs mounted at /sys/kernel/debug
#   - kcov-wrapper-ptrace binary available in $PATH
#
# Usage (inside VM):
#   ./run_tests.sh
#
# Usage (orchestrated from host):
#   Tests are driven by the CI workflow which boots a QEMU VM and
#   executes this script as the init process.
#
# NOTE: This is a scaffold. Full QEMU-based integration testing requires:
#   1. A minimal kernel with KCOV enabled
#   2. A rootfs with the test binaries
#   3. QEMU invocation wiring in CI

set -eu

WRAPPER="${KCOV_WRAPPER_BIN:-kcov-wrapper-ptrace}"
TESTS_DIR="$(dirname "$0")/test_programs"
TMPDIR="${TMPDIR:-/tmp}"
PASS=0
FAIL=0

say()  { printf "\n  [TEST] %s\n" "$*"; }
ok()   { PASS=$((PASS + 1)); printf "    PASS\n"; }
fail() { FAIL=$((FAIL + 1)); printf "    FAIL: %s\n" "$*"; }

# ── Build test programs if not pre-built ─────────────────────────────

for src in "$TESTS_DIR"/*.c; do
    name="$(basename "$src" .c)"
    bin="$TMPDIR/$name"
    if [ ! -x "$bin" ] || [ "$src" -nt "$bin" ]; then
        gcc -static -o "$bin" "$src" || {
            echo "ERROR: Cannot compile $src — check that gcc is available in VM"
            exit 1
        }
    fi
done

# ── Test 1: Basic hello trace ────────────────────────────────────────

say "hello program"
out="$TMPDIR/out_hello.txt"
uniq="$TMPDIR/uniq_hello.txt"
if "$WRAPPER" -o "$out" -u "$uniq" "$TMPDIR/hello" >/dev/null 2>&1; then
    if [ -s "$uniq" ]; then
        ok
    else
        fail "unique output is empty"
    fi
else
    fail "wrapper exited non-zero"
fi

# ── Test 2: many_syscalls trace ──────────────────────────────────────

say "many_syscalls program"
out2="$TMPDIR/out_many.txt"
uniq2="$TMPDIR/uniq_many.txt"
if "$WRAPPER" -o "$out2" -u "$uniq2" "$TMPDIR/many_syscalls" >/dev/null 2>&1; then
    if [ -s "$uniq2" ]; then
        ok
    else
        fail "unique output is empty"
    fi
else
    fail "wrapper exited non-zero"
fi

# ── Test 3: Verify output file format ────────────────────────────────

say "output file format"
if grep -qE '^0x[0-9a-f]+' "$out"; then
    ok
else
    fail "detailed output missing hex PC addresses"
fi

# ── Test 4: Compiled test binaries exist ─────────────────────────────

for src in "$TESTS_DIR"/*.c; do
    name="$(basename "$src" .c)"
    say "binary compile: $name"
    if [ -x "$TMPDIR/$name" ]; then
        ok
    else
        fail "missing binary $name"
    fi
done

# ── Summary ──────────────────────────────────────────────────────────

echo ""
echo "==========================================="
echo "Integration tests: $((PASS + FAIL)) run"
echo "  PASS: $PASS"
echo "  FAIL: $FAIL"
echo "==========================================="

[ "$FAIL" -eq 0 ] || exit 1
