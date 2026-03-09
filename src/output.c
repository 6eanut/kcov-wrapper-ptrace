/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Output — flush deduplicated KCOV buffer to file
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdint.h>
#include <stdio.h>
#include "../include/kcov-wrapper-ptrace.h"

void save_pcs(uint64_t *cover, FILE *fp,
              uint64_t *total_pcs, uint64_t syscall_num,
              size_t buffer_entries) {
    uint64_t n = __atomic_load_n(&cover[0], __ATOMIC_ACQUIRE);
    if (n == 0) return;

    for (uint64_t i = 1; i <= n && i < buffer_entries; i++) {
        fprintf(fp, "0x%lx", cover[i]);
        if (syscall_num != NO_SYSCALL)
            fprintf(fp, " syscall=%lu", syscall_num);
        fprintf(fp, "\n");
    }

    *total_pcs += n;
    __atomic_store_n(&cover[0], 0, __ATOMIC_RELAXED);
}
