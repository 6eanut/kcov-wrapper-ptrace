#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Example: Feed unique PCs to a fuzzer's coverage-guided corpus
#
# Kernel fuzzers like syzkaller can consume PC traces to guide
# input mutation. This script collects coverage from a target
# and outputs a sorted unique PC list suitable for the fuzzer.

set -eu

FUZZER_INPUT_DIR="${1:?Usage: $0 <fuzzer-input-dir>}"
TARGET_PROGRAM="${2:?Usage: $0 <fuzzer-input-dir> <program> [args...]}"
shift 2

OUTDIR="$(mktemp -d)"
trap "rm -rf $OUTDIR" EXIT

echo "[*] Collecting KCOV coverage from: $TARGET_PROGRAM $@"

kcov-wrapper-ptrace \
    -o "$OUTDIR/detailed.txt" \
    -u "$OUTDIR/pcs.txt" \
    "$TARGET_PROGRAM" "$@"

UNIQUE=$(wc -l < "$OUTDIR/pcs.txt")
echo "[+] Collected $UNIQUE unique PCs"

# Copy to fuzzer corpus
cp "$OUTDIR/pcs.txt" "$FUZZER_INPUT_DIR/kcov-pcs-$(date +%s).txt"
echo "[+] PCs saved to $FUZZER_INPUT_DIR/"
