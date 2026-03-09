#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Example: Basic KCOV coverage collection
#
# Simplest usage — trace a single command and get unique PCs.
# Run as root (or with CAP_SYS_PTRACE + kcov access).

set -eu

echo "=== KCOV Wrapper Ptrace: Basic Collection ==="

# Trace `ls /` — a program that makes many syscalls
kcov-wrapper-ptrace ls /

echo ""
echo "Output files:"
echo "  pcs.txt              — deduplicated unique PC addresses"
echo "  pcs_detailed.txt     — PC addresses with syscall annotations"
echo ""
echo "Next step: resolve symbols with kcov-sym"
echo "  kcov-sym /path/to/vmlinux pcs.txt"
