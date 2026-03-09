/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * x86_64 syscall number extraction
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include "arch.h"
#include <stdint.h>

#ifdef __x86_64__

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <linux/elf.h>
#include <stddef.h>

uint64_t get_syscall_number_x86_64(int pid) {
    struct user_regs_struct regs;
    struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
    if (ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) == 0)
        return regs.orig_rax;
    return UINT64_MAX;
}

#else /* !__x86_64__ */

uint64_t get_syscall_number_x86_64(int pid) {
    (void)pid;
    return UINT64_MAX;
}

#endif /* __x86_64__ */
