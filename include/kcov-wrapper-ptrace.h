/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * kcov-wrapper-ptrace — shared constants and types
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#ifndef KCOV_WRAPPER_PTRACE_H
#define KCOV_WRAPPER_PTRACE_H

#include <stdint.h>
#include <stdio.h>

/* ── KCOV ioctl commands ──────────────────────────────────────────── */
#define KCOV_INIT_TRACE   _IOR('c', 1, unsigned long)
#define KCOV_ENABLE       _IO('c', 100)
#define KCOV_DISABLE      _IO('c', 101)
#define KCOV_TRACE_PC     0

/* ── Buffer sizing ────────────────────────────────────────────────── */
#define COVER_SIZE        (1024 * 1024)           /* entries in shared buffer */
#define MAX_DETAIL_FILE_SIZE  (1ULL * 1024 * 1024 * 1024)   /* 1 GB */

/* ── Sentinels ────────────────────────────────────────────────────── */
#define NO_SYSCALL        UINT64_MAX

/* ── Data types ───────────────────────────────────────────────────── */
typedef struct {
    uint64_t pc;
    uint64_t syscall_num;   /* NO_SYSCALL → no syscall annotation */
} PCEntry;

/* ── KCOV device operations ───────────────────────────────────────── */
int  kcov_open(const char *path);
int  kcov_init(int fd, size_t buffer_entries);
int  kcov_enable_trace(int fd);
int  kcov_disable_trace(int fd);
uint64_t *kcov_mmap(int fd, size_t buffer_entries);
void kcov_cleanup(int fd, uint64_t *cover, size_t buffer_entries);

/* ── Deduplication ────────────────────────────────────────────────── */
void dedup_cover(uint64_t *cover, size_t buffer_entries);

/* ── Output ───────────────────────────────────────────────────────── */
void save_pcs(uint64_t *cover, FILE *fp,
              uint64_t *total_pcs, uint64_t syscall_num,
              size_t buffer_entries);

/* ── Shared comparison helpers ────────────────────────────────────── */
int compare_uint64(const void *a, const void *b);

/* ── Compaction ───────────────────────────────────────────────────── */
size_t parse_line(const char *line, PCEntry *out, size_t out_cap);
void   compact_detailed_file(FILE **fp_ptr, const char *output_path);

/* ── Ptrace trace loop ────────────────────────────────────────────── */
typedef uint64_t (*syscall_reader_fn)(int pid);

void trace_loop(int pid, uint64_t *cover, size_t buffer_entries,
                FILE **fp_ptr, const char *output_path,
                syscall_reader_fn read_syscall,
                uint64_t *total_pcs, uint64_t *syscall_count);

/* ── Architecture abstraction ─────────────────────────────────────── */
syscall_reader_fn arch_detect_syscall_reader(void);

/* ── Post-trace: generate unique PC list ──────────────────────────── */
void generate_unique_pcs(const char *input_path, const char *output_path,
                         uint64_t *unique_count);

#endif /* KCOV_WRAPPER_PTRACE_H */
