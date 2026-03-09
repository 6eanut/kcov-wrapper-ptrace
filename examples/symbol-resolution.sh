#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Example: End-to-end pipeline — collect → resolve symbols → inspect
#
# Full workflow: trace a kernel build command with KCOV, then
# resolve all PCs to file:line using the kernel's debug info.

set -eu

VMLINUX="${1:?Usage: $0 <vmlinux>}"
shift

echo "=== KCOV Pipeline ==="
echo "VMLinux: $VMLINUX"
echo "Command: $@"
echo ""

# Step 1: Collect
echo "[1/3] Collecting coverage..."
kcov-wrapper-ptrace "$@"

# Step 2: Resolve
echo "[2/3] Resolving symbols..."
kcov-sym "$VMLINUX" pcs.txt

# Step 3: Show top 20 hit files with counts
echo "[3/3] Top 20 files by coverage:"
cut -d: -f1 files.txt | sort | uniq -c | sort -rn | head -20

echo ""
echo "Output files:"
echo "  pcs.txt      — unique raw PCs"
echo "  files.txt    — resolved file:line symbols"
