/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * RISC-V 64-bit syscall number extraction
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include "arch.h"
#include <stdint.h>

#ifdef __riscv

#include <linux/elf.h>
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <unistd.h>

#ifdef __has_include
    #if __has_include(<asm/ptrace.h>)
        #include <asm/ptrace.h>
        #define HAVE_USER_REGS_STRUCT
    #elif __has_include(<linux/ptrace.h>)
        #include <linux/ptrace.h>
        #define HAVE_USER_REGS_STRUCT
    #endif
#else
    #include <asm/ptrace.h>
    #define HAVE_USER_REGS_STRUCT
#endif

#ifndef HAVE_USER_REGS_STRUCT
struct user_regs_struct {
    unsigned long pc;
    unsigned long ra, sp, gp, tp;
    unsigned long t0, t1, t2;
    unsigned long s0, s1;
    unsigned long a0, a1, a2, a3, a4, a5, a6;
    unsigned long a7;   /* syscall number */
    unsigned long s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    unsigned long t3, t4, t5, t6;
};
#endif

uint64_t get_syscall_number_riscv64(int pid) {
    struct user_regs_struct regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) == 0)
        return regs.a7;
    return UINT64_MAX;
}

#else /* !__riscv */

uint64_t get_syscall_number_riscv64(int pid) {
    (void)pid;
    return UINT64_MAX;
}

#endif /* __riscv */
