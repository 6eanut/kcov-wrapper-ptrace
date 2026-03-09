/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Generic (no syscall annotation) fallback
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include "arch.h"
#include <stdint.h>

uint64_t get_syscall_number_generic(int pid) {
    (void)pid;
    return UINT64_MAX;
}
