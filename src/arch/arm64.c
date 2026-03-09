/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ARM64 (AArch64) syscall number extraction
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include "arch.h"
#include <stdint.h>

#ifdef __aarch64__

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <linux/elf.h>
#include <stddef.h>

uint64_t get_syscall_number_arm64(int pid) {
    struct user_regs_struct regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) == 0)
        return regs.regs[8];  /* x8 holds syscall number on ARM64 */
    return UINT64_MAX;
}

#else /* !__aarch64__ */

uint64_t get_syscall_number_arm64(int pid) {
    (void)pid;
    return UINT64_MAX;
}

#endif /* __aarch64__ */
