/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Unit tests for save_pcs()
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 *
 * Build:  gcc -std=gnu11 -I../include -o test_output test_output.c ../src/output.c
 * Run:    ./test_output
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static char *read_whole_file(const char *path, size_t *len_out) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    if (buf) {
        size_t n = fread(buf, 1, sz, f);
        buf[n] = '\0';
        *len_out = n;
    }
    fclose(f);
    return buf;
}

#define TEST_BUF_SZ 8

/* ── Tests ─────────────────────────────────────────────────────────── */

static void test_save_with_syscall(void) {
    const char *path = "/tmp/test_output_syscall.txt";
    uint64_t buf[TEST_BUF_SZ] = {2, 0xa, 0xb};
    uint64_t total = 0;

    FILE *fp = fopen(path, "w");
    CHECK(fp != NULL, "open temp file");
    save_pcs(buf, fp, &total, 42, TEST_BUF_SZ);
    fclose(fp);

    CHECK(total == 2, "total incremented by 2");
    CHECK(__atomic_load_n(&buf[0], __ATOMIC_ACQUIRE) == 0, "buffer cleared");

    size_t len;
    char *content = read_whole_file(path, &len);
    CHECK(content != NULL, "read back");
    CHECK(strstr(content, "0xa syscall=42") != NULL, "contains annotated line");
    CHECK(strstr(content, "0xb syscall=42") != NULL, "contains second line");
    free(content);
    remove(path);
}

static void test_save_without_syscall(void) {
    const char *path = "/tmp/test_output_nosys.txt";
    uint64_t buf[TEST_BUF_SZ] = {1, 0xdead};
    uint64_t total = 0;

    FILE *fp = fopen(path, "w");
    save_pcs(buf, fp, &total, NO_SYSCALL, TEST_BUF_SZ);
    fclose(fp);

    CHECK(total == 1, "total = 1");
    size_t len;
    char *content = read_whole_file(path, &len);
    CHECK(strstr(content, "0xdead") != NULL, "contains PC");
    CHECK(strstr(content, "syscall=") == NULL, "no syscall annotation");
    free(content);
    remove(path);
}

static void test_save_empty_buffer(void) {
    uint64_t buf[4] = {0};   /* n=0 */
    uint64_t total = 0;

    FILE *fp = fopen("/tmp/test_output_empty.txt", "w");
    save_pcs(buf, fp, &total, 1, 4);
    fclose(fp);

    CHECK(total == 0, "total unchanged");
    remove("/tmp/test_output_empty.txt");
}

/* ── Runner ────────────────────────────────────────────────────────── */

int main(void) {
    test_save_with_syscall();
    test_save_without_syscall();
    test_save_empty_buffer();

    printf("output: %d run, %d failed\n", tests_run, tests_fail);
    return tests_fail ? 1 : 0;
}
