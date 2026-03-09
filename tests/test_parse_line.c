/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Unit tests for parse_line()
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 *
 * Build:  gcc -std=gnu11 -I../include -o test_parse_line test_parse_line.c ../src/compaction.c
 * Run:    ./test_parse_line
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

/* ── Tests ─────────────────────────────────────────────────────────── */

static void test_simple_pc(void) {
    PCEntry out[4];
    size_t n = parse_line("0xdeadbeef\n", out, 4);
    CHECK(n == 1, "simple PC returns 1 entry");
    CHECK(out[0].pc == 0xdeadbeef, "pc value correct");
    CHECK(out[0].syscall_num == NO_SYSCALL, "no syscall annotation");
}

static void test_pc_with_single_syscall(void) {
    PCEntry out[4];
    size_t n = parse_line("0x1234 syscall=42\n", out, 4);
    CHECK(n == 1, "one syscall annotation = 1 entry");
    CHECK(out[0].pc == 0x1234, "pc correct");
    CHECK(out[0].syscall_num == 42, "syscall number correct");
}

static void test_pc_with_multiple_syscalls(void) {
    PCEntry out[4];
    size_t n = parse_line("0xabcd syscall=1,2,3\n", out, 4);
    CHECK(n == 3, "three syscall annotations");
    CHECK(out[0].syscall_num == 1, "first syscall");
    CHECK(out[1].syscall_num == 2, "second syscall");
    CHECK(out[2].syscall_num == 3, "third syscall");
    CHECK(out[0].pc == 0xabcd, "pc same for all");
    CHECK(out[1].pc == 0xabcd, "pc same for all");
    CHECK(out[2].pc == 0xabcd, "pc same for all");
}

static void test_output_capacity_limit(void) {
    PCEntry out[2];
    size_t n = parse_line("0xff syscall=10,20,30\n", out, 2);
    CHECK(n == 2, "truncated at capacity");
    CHECK(out[0].syscall_num == 10, "first stored");
    CHECK(out[1].syscall_num == 20, "second stored");
}

static void test_empty_string(void) {
    PCEntry out[4];
    size_t n = parse_line("", out, 4);
    CHECK(n == 0, "empty string = 0 entries");
}

static void test_garbage_input(void) {
    PCEntry out[4];
    size_t n = parse_line("not a pc address\n", out, 4);
    CHECK(n == 0, "garbage = 0 entries");
}

static void test_missing_syscall_value(void) {
    /* "syscall=" with nothing after is tolerated */
    PCEntry out[4];
    size_t n = parse_line("0xbeef syscall=\n", out, 4);
    CHECK(n == 0, "empty syscall value = 0 entries");
}

static void test_trailing_newline_handling(void) {
    PCEntry out[4];
    size_t n = parse_line("0xcafe syscall=7\r\n", out, 4);
    CHECK(n == 1, "CRLF handled");
    CHECK(out[0].syscall_num == 7, "value correct");
}

/* ── Runner ────────────────────────────────────────────────────────── */

int main(void) {
    test_simple_pc();
    test_pc_with_single_syscall();
    test_pc_with_multiple_syscalls();
    test_output_capacity_limit();
    test_empty_string();
    test_garbage_input();
    test_missing_syscall_value();
    test_trailing_newline_handling();

    printf("parse_line: %d run, %d failed\n", tests_run, tests_fail);
    return tests_fail ? 1 : 0;
}
