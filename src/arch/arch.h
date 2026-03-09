/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Architecture abstraction layer
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

/* Return the system call number from the traced process, or NO_SYSCALL */
typedef uint64_t (*syscall_reader_fn)(int pid);

/* Arch-specific implementations */
uint64_t get_syscall_number_riscv64(int pid);
uint64_t get_syscall_number_x86_64(int pid);
uint64_t get_syscall_number_arm64(int pid);
uint64_t get_syscall_number_generic(int pid);

/* Detect architecture at compile time and return the appropriate reader */
syscall_reader_fn arch_detect_syscall_reader(void);

#endif /* ARCH_H */
