/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ptrace trace loop — main syscall-boundary tracing engine
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "arch/arch.h"
#include "../include/kcov-wrapper-ptrace.h"

void trace_loop(int pid, uint64_t *cover, size_t buffer_entries,
                FILE **fp_ptr, const char *output_path,
                syscall_reader_fn read_syscall,
                uint64_t *total_pcs, uint64_t *syscall_count) {
    int status;
    int in_syscall = 0;

    /* Wait for initial stop (before execvp) */
    waitpid(pid, &status, 0);

    if (ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD) == -1) {
        perror("ptrace(SETOPTIONS) failed");
        kill(pid, SIGKILL);
        return;
    }

    fprintf(stderr, "Starting to trace process %d...\n", pid);

    while (1) {
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
            perror("ptrace(SYSCALL) failed");
            break;
        }

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            break;
        }

        if (WIFEXITED(status)) {
            fprintf(stderr, "Process exited with code: %d\n", WEXITSTATUS(status));
            break;
        }

        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Process terminated by signal: %d\n", WTERMSIG(status));
            break;
        }

        if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80)) {
            if (!in_syscall) {
                in_syscall = 1;
            } else {
                in_syscall = 0;
                (*syscall_count)++;

                uint64_t syscall_num = read_syscall(pid);

                dedup_cover(cover, buffer_entries);
                save_pcs(cover, *fp_ptr, total_pcs, syscall_num,
                         buffer_entries);
                compact_detailed_file(fp_ptr, output_path);

                if (!*fp_ptr) {
                    fprintf(stderr, "FATAL: lost output file handle, aborting\n");
                    kill(pid, SIGKILL);
                    break;
                }
            }
        } else if (WIFSTOPPED(status)) {
            /* Forward other signals to child */
            int sig = WSTOPSIG(status);
            if (ptrace(PTRACE_SYSCALL, pid, 0, sig) == -1) {
                perror("ptrace signal delivery failed");
                break;
            }
            continue;
        }
    }

    /* Flush remaining PCs after child exits */
    if (*fp_ptr)
        save_pcs(cover, *fp_ptr, total_pcs, NO_SYSCALL,
                 buffer_entries);
}
