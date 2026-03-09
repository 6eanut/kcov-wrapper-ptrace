/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Deduplication — in-place sort + dedup of KCOV buffer
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdlib.h>
#include <stdint.h>
#include "../include/kcov-wrapper-ptrace.h"

int compare_uint64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

void dedup_cover(uint64_t *cover, size_t buffer_entries) {
    uint64_t n = __atomic_load_n(&cover[0], __ATOMIC_ACQUIRE);
    if (n == 0) return;
    if (n >= buffer_entries) n = buffer_entries - 1;

    qsort(&cover[1], n, sizeof(uint64_t), compare_uint64);

    uint64_t w = 1;
    for (uint64_t i = 2; i <= n; i++) {
        if (cover[i] != cover[w])
            cover[++w] = cover[i];
    }

    __atomic_store_n(&cover[0], w, __ATOMIC_RELAXED);
}
