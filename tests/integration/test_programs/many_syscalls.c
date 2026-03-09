/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Diverse syscall test program — exercises multiple syscall types
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 *
 * Exercises: read, write, openat, close, mmap, munmap, getpid, exit
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void) {
    /* File I/O */
    int fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) {
        write(fd, "test\n", 5);
        close(fd);
    }

    /* Memory mapping */
    size_t sz = 4096;
    void *p = mmap(NULL, sz, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p != MAP_FAILED) {
        *(volatile char *)p = 42;   /* force a page fault */
        munmap(p, sz);
    }

    /* Misc */
    getpid();
    getuid();

    printf("many_syscalls: done\n");
    return 0;
}
