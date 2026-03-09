/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Compaction — merge and deduplicate output file when it exceeds threshold
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/kcov-wrapper-ptrace.h"

static int compare_pc_entry(const void *a, const void *b) {
    const PCEntry *ea = a, *eb = b;
    if (ea->pc != eb->pc)
        return (ea->pc < eb->pc) ? -1 : 1;
    if (ea->syscall_num != eb->syscall_num)
        return (ea->syscall_num < eb->syscall_num) ? -1 : 1;
    return 0;
}

size_t parse_line(const char *line, PCEntry *out, size_t out_cap) {
    uint64_t pc;
    if (sscanf(line, "0x%lx", &pc) != 1)
        return 0;

    const char *tag = strstr(line, "syscall=");
    if (!tag) {
        if (out_cap < 1) return 0;
        out[0].pc = pc;
        out[0].syscall_num = NO_SYSCALL;
        return 1;
    }

    const char *p = tag + strlen("syscall=");
    size_t n = 0;
    while (*p && *p != '\n' && *p != '\r') {
        if (n >= out_cap) break;
        char *end;
        uint64_t sn = strtoull(p, &end, 10);
        if (end == p) break;
        out[n].pc = pc;
        out[n].syscall_num = sn;
        n++;
        p = end;
        if (*p == ',') p++;
    }
    return n;
}

void compact_detailed_file(FILE **fp_ptr, const char *output_path) {
    /* Check current file size via fstat (works on append-mode files) */
    struct stat st;
    if (fstat(fileno(*fp_ptr), &st) == -1) return;
    if ((uint64_t)st.st_size < MAX_DETAIL_FILE_SIZE) return;

    fprintf(stderr,
            "\n[compact] %s reached %.2f GB, starting compaction...\n",
            output_path, (double)st.st_size / (1024.0 * 1024.0 * 1024.0));

    fflush(*fp_ptr);
    fclose(*fp_ptr);
    *fp_ptr = NULL;

    /* Read all (pc, syscall) pairs */
    FILE *fp_read = fopen(output_path, "r");
    if (!fp_read) {
        perror("[compact] Cannot open file for reading");
        goto reopen_append;
    }

    size_t capacity = 8 * 1024 * 1024;
    PCEntry *entries = malloc(capacity * sizeof(PCEntry));
    if (!entries) {
        perror("[compact] malloc failed");
        fclose(fp_read);
        goto reopen_append;
    }

    size_t count = 0;
    char line[512];
    PCEntry tmp[256];

    while (fgets(line, sizeof(line), fp_read)) {
        size_t n = parse_line(line, tmp, 256);
        for (size_t i = 0; i < n; i++) {
            if (count >= capacity) {
                capacity *= 2;
                PCEntry *t = realloc(entries, capacity * sizeof(PCEntry));
                if (!t) {
                    perror("[compact] realloc failed, aborting compaction");
                    free(entries);
                    fclose(fp_read);
                    goto reopen_append;
                }
                entries = t;
            }
            entries[count++] = tmp[i];
        }
    }
    fclose(fp_read);

    fprintf(stderr, "[compact] Read %zu (pc, syscall) pairs, sorting...\n", count);

    /* Sort and rewrite */
    qsort(entries, count, sizeof(PCEntry), compare_pc_entry);

    FILE *fp_write = fopen(output_path, "w");
    if (!fp_write) {
        perror("[compact] Cannot open file for writing");
        free(entries);
        goto reopen_append;
    }

    uint64_t unique_pc_count = 0;
    size_t i = 0;
    while (i < count) {
        uint64_t cur_pc = entries[i].pc;
        fprintf(fp_write, "0x%lx", cur_pc);

        int has_syscall = 0;
        size_t j = i;
        while (j < count && entries[j].pc == cur_pc) {
            uint64_t sn = entries[j].syscall_num;
            if (sn != NO_SYSCALL) {
                int dup = (j > i) && (entries[j-1].pc == cur_pc)
                               && (entries[j-1].syscall_num == sn);
                if (!dup) {
                    fprintf(fp_write, has_syscall ? ",%lu" : " syscall=%lu", sn);
                    has_syscall = 1;
                }
            }
            j++;
        }

        fprintf(fp_write, "\n");
        unique_pc_count++;
        i = j;
    }

    fclose(fp_write);
    free(entries);

    fprintf(stderr, "[compact] Done: %lu unique PCs written\n", unique_pc_count);

reopen_append:
    *fp_ptr = fopen(output_path, "a");
    if (!*fp_ptr)
        perror("[compact] FATAL: cannot reopen file for appending");
}
