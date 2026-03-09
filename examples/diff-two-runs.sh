#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Example: Compare two runs — find coverage differences
#
# Useful for: regression testing ("did this patch change coverage?"),
# fuzzer effectiveness ("did the new mutator reach new code?").

set -eu

RUN1="${1:?Usage: $0 <run1.pcs.txt> <run2.pcs.txt>}"
RUN2="${2:?Usage: $0 <run1.pcs.txt> <run2.pcs.txt>}"

echo "=== Coverage Diff ==="
echo "Run 1: $RUN1 ($(wc -l < "$RUN1") PCs)"
echo "Run 2: $RUN2 ($(wc -l < "$RUN2") PCs)"

# PCs only in run 1 (lost coverage)
LOST=$(comm -23 <(sort "$RUN1") <(sort "$RUN2") | wc -l)

# PCs only in run 2 (new coverage)
GAINED=$(comm -13 <(sort "$RUN1") <(sort "$RUN2") | wc -l)

# PCs in both
COMMON=$(comm -12 <(sort "$RUN1") <(sort "$RUN2") | wc -l)

echo ""
echo "Coverage lost:   $LOST"
echo "Coverage gained:  $GAINED"
echo "Coverage shared:  $COMMON"
echo ""

if [ "$GAINED" -gt 0 ]; then
    echo "New PCs in run 2:"
    comm -13 <(sort "$RUN1") <(sort "$RUN2") | head -20
    echo "  ... ($GAINED total)"
fi

if [ "$LOST" -gt 0 ]; then
    echo ""
    echo "WARNING: $LOST PCs present in run 1 but missing in run 2"
fi
