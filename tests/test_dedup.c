/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Unit tests for dedup_cover()
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 *
 * Build:  gcc -std=gnu11 -I../include -o test_dedup test_dedup.c ../src/dedup.c
 * Run:    ./test_dedup
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/kcov-wrapper-ptrace.h"

static int tests_run   = 0;
static int tests_fail  = 0;

#define CHECK(cond, msg) do {                               \
    tests_run++;                                            \
    if (!(cond)) {                                          \
        fprintf(stderr, "FAIL [%s:%d] %s\n",                \
                __FILE__, __LINE__, msg);                   \
        tests_fail++;                                       \
        return;                                             \
    }                                                       \
} while (0)

/* ── Helpers ───────────────────────────────────────────────────────── */

static uint64_t cover_size(uint64_t *cover) {
    return __atomic_load_n(&cover[0], __ATOMIC_ACQUIRE);
}

static int is_sorted(uint64_t *cover) {
    uint64_t n = cover_size(cover);
    for (uint64_t i = 1; i < n; i++)
        if (cover[i] > cover[i+1]) return 0;
    return 1;
}

static int has_duplicates(uint64_t *cover) {
    uint64_t n = cover_size(cover);
    for (uint64_t i = 1; i < n; i++)
        if (cover[i] == cover[i+1]) return 1;
    return 0;
}

/* ── Tests ─────────────────────────────────────────────────────────── */

static void test_empty(void) {
    uint64_t buf[8] = {0};
    dedup_cover(buf, 8);
    CHECK(cover_size(buf) == 0, "empty buffer should stay empty");
}

static void test_single_entry(void) {
    uint64_t buf[8] = {1, 0xdeadbeef};
    dedup_cover(buf, 8);
    CHECK(cover_size(buf) == 1, "single entry should remain");
    CHECK(buf[1] == 0xdeadbeef, "single entry value preserved");
}

static void test_sorted_no_dups(void) {
    uint64_t buf[8] = {3, 0x100, 0x200, 0x300};
    dedup_cover(buf, 8);
    CHECK(cover_size(buf) == 3, "3 unique entries remain 3");
    CHECK(is_sorted(buf), "output should be sorted");
    CHECK(!has_duplicates(buf), "no duplicates in output");
}

static void test_unsorted_with_dups(void) {
    uint64_t buf[16] = {6, 0x300, 0x100, 0x200, 0x100, 0x300, 0x400};
    dedup_cover(buf, 16);
    CHECK(cover_size(buf) == 4, "4 unique entries");
    CHECK(is_sorted(buf), "output sorted");
    CHECK(!has_duplicates(buf), "duplicates removed");
}

static void test_all_duplicates(void) {
    uint64_t buf[8] = {5, 0x42, 0x42, 0x42, 0x42, 0x42};
    dedup_cover(buf, 8);
    CHECK(cover_size(buf) == 1, "all same becomes 1");
    CHECK(buf[1] == 0x42, "value preserved");
}

static void test_interleaved_dups(void) {
    uint64_t buf[16] = {8, 0xa, 0xb, 0xa, 0xc, 0xb, 0xd, 0xa, 0xe};
    dedup_cover(buf, 16);
    CHECK(cover_size(buf) == 5, "a,b,c,d,e = 5 unique");
    CHECK(is_sorted(buf), "sorted");
}

static void test_near_capacity(void) {
    /* n >= buffer_entries should clamp to buffer_entries-1.
     * After calloc, buf[1]=0x1, rest are 0 — dedup gives {0, 0x1} = 2. */
    size_t sz = 32;
    uint64_t *buf = calloc(sz, sizeof(uint64_t));
    buf[0] = sz + 100;
    buf[1] = 0x1;
    dedup_cover(buf, sz);
    CHECK(cover_size(buf) == 2, "clamped + dedup: zero + 0x1 = 2");
    free(buf);
}

/* ── Runner ────────────────────────────────────────────────────────── */

int main(void) {
    test_empty();
    test_single_entry();
    test_sorted_no_dups();
    test_unsorted_with_dups();
    test_all_duplicates();
    test_interleaved_dups();
    test_near_capacity();

    printf("dedup: %d run, %d failed\n", tests_run, tests_fail);
    return tests_fail ? 1 : 0;
}
