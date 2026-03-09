/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * generate_unique_pcs — read detailed file, deduplicate, write unique PCs
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/kcov-wrapper-ptrace.h"

/* compare_uint64 is declared in kcov-wrapper-ptrace.h */

void generate_unique_pcs(const char *input_path, const char *output_path,
                         uint64_t *unique_count) {
    FILE *fp_in = fopen(input_path, "r");
    if (!fp_in) {
        perror("Cannot reopen detailed file for unique generation");
        return;
    }

    size_t cap = 1024 * 1024;
    uint64_t *all_pcs = malloc(cap * sizeof(uint64_t));
    if (!all_pcs) {
        perror("malloc for unique PCs failed");
        fclose(fp_in);
        return;
    }

    size_t cnt = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp_in)) {
        uint64_t pc;
        if (sscanf(line, "0x%lx", &pc) == 1) {
            if (cnt >= cap) {
                cap *= 2;
                uint64_t *t = realloc(all_pcs, cap * sizeof(uint64_t));
                if (!t) {
                    perror("realloc for unique PCs failed");
                    free(all_pcs);
                    fclose(fp_in);
                    return;
                }
                all_pcs = t;
            }
            all_pcs[cnt++] = pc;
        }
    }
    fclose(fp_in);

    qsort(all_pcs, cnt, sizeof(uint64_t), compare_uint64);

    FILE *fp_out = fopen(output_path, "w");
    if (!fp_out) {
        perror("Cannot create unique output file");
        free(all_pcs);
        return;
    }

    *unique_count = 0;
    if (cnt > 0) {
        fprintf(fp_out, "0x%lx\n", all_pcs[0]);
        *unique_count = 1;
        for (size_t i = 1; i < cnt; i++) {
            if (all_pcs[i] != all_pcs[i-1]) {
                fprintf(fp_out, "0x%lx\n", all_pcs[i]);
                (*unique_count)++;
            }
        }
    }
    fclose(fp_out);
    free(all_pcs);
}
