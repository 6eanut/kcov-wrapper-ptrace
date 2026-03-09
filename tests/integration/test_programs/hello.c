/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal integration test program — makes a few simple syscalls
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 *
 * Used by the QEMU integration test suite to verify basic KCOV + ptrace
 * functionality: the tracing wrapper should capture PCs from write(),
 * getpid(), and getuid().
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {
    printf("Hello from integration test!\n");
    printf("PID: %d\n", getpid());
    printf("UID: %d\n", getuid());
    return 0;
}
