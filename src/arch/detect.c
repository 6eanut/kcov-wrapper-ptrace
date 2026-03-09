/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Compile-time architecture detection
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include "arch.h"

syscall_reader_fn arch_detect_syscall_reader(void) {
#if defined(__riscv)
    return get_syscall_number_riscv64;
#elif defined(__x86_64__)
    return get_syscall_number_x86_64;
#elif defined(__aarch64__)
    return get_syscall_number_arm64;
#else
    return get_syscall_number_generic;
#endif
}
