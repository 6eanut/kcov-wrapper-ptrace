/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KCOV device operations
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include "../include/kcov-wrapper-ptrace.h"

int kcov_open(const char *path) {
    return open(path, O_RDWR | O_CLOEXEC);
}

int kcov_init(int fd, size_t buffer_entries) {
    return ioctl(fd, KCOV_INIT_TRACE, buffer_entries);
}

int kcov_enable_trace(int fd) {
    return ioctl(fd, KCOV_ENABLE, KCOV_TRACE_PC);
}

int kcov_disable_trace(int fd) {
    return ioctl(fd, KCOV_DISABLE, 0);
}

uint64_t *kcov_mmap(int fd, size_t buffer_entries) {
    return (uint64_t *)mmap(NULL, buffer_entries * sizeof(uint64_t),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void kcov_cleanup(int fd, uint64_t *cover, size_t buffer_entries) {
    if (cover && cover != MAP_FAILED)
        munmap(cover, buffer_entries * sizeof(uint64_t));
    if (fd >= 0)
        close(fd);
}
