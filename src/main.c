/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * kcov-wrapper-ptrace — main entry point
 * Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <signal.h>
#include "../include/kcov-wrapper-ptrace.h"
#include "arch/arch.h"

#define DEFAULT_KCOV_PATH       "/sys/kernel/debug/kcov"
#define DEFAULT_OUTPUT          "pcs_detailed.txt"
#define DEFAULT_UNIQUE_OUTPUT   "pcs.txt"
#define DEFAULT_BUFFER_SIZE     COVER_SIZE
#define DEFAULT_COMPACT_THRESH  1

static const char *help_text =
    "Usage: kcov-wrapper-ptrace [OPTIONS] <program> [args...]\n"
    "\n"
    "Collect Linux kernel KCOV coverage data segmented by system call boundaries.\n"
    "\n"
    "Options:\n"
    "  -o, --output FILE         Detailed output file (default: pcs_detailed.txt)\n"
    "  -u, --unique-output FILE  Unique PC output (default: pcs.txt)\n"
    "  -k, --kcov-path PATH      KCOV device path (default: /sys/kernel/debug/kcov)\n"
    "  -t, --timeout SEC         Kill traced process after SEC seconds (0 = no limit)\n"
    "  -b, --buffer-size N       KCOV buffer entries (default: 1048576)\n"
    "  -c, --compact-threshold N File compaction threshold in GB (default: 1)\n"
    "  -v, --verbose             Enable verbose output\n"
    "  -h, --help                Show this help\n"
    "  -V, --version             Show version\n"
    "\n"
    "Environment:\n"
    "  KCOV_PATH                 Override default KCOV device path\n"
    "\n"
    "Output files:\n"
    "  pcs_detailed.txt          PC addresses annotated with syscall numbers\n"
    "  pcs.txt                   Deduplicated unique PC addresses\n";

int main(int argc, char **argv) {
    const char *kcov_path       = NULL;
    const char *output_path     = DEFAULT_OUTPUT;
    const char *unique_path     = DEFAULT_UNIQUE_OUTPUT;
    unsigned int timeout_sec    = 0;
    size_t buffer_size          = DEFAULT_BUFFER_SIZE;
    unsigned int compact_thresh = DEFAULT_COMPACT_THRESH;
    int verbose                 = 0;

    /* ── Parse CLI ────────────────────────────────────────────────── */
    static struct option long_opts[] = {
        {"output",            required_argument, NULL, 'o'},
        {"unique-output",     required_argument, NULL, 'u'},
        {"kcov-path",         required_argument, NULL, 'k'},
        {"timeout",           required_argument, NULL, 't'},
        {"buffer-size",       required_argument, NULL, 'b'},
        {"compact-threshold", required_argument, NULL, 'c'},
        {"verbose",           no_argument,       NULL, 'v'},
        {"help",              no_argument,       NULL, 'h'},
        {"version",           no_argument,       NULL, 'V'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:u:k:t:b:c:vhV", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'o': output_path = optarg; break;
        case 'u': unique_path = optarg; break;
        case 'k': kcov_path   = optarg; break;
        case 't': timeout_sec = (unsigned int)strtoul(optarg, NULL, 10); break;
        case 'b':
            buffer_size = strtoull(optarg, NULL, 10);
            if (buffer_size < 2) {
                fprintf(stderr, "Error: buffer size must be at least 2.\n");
                return 1;
            }
            break;
        case 'c': compact_thresh = (unsigned int)strtoul(optarg, NULL, 10); break;
        case 'v': verbose = 1; break;
        case 'V':
            printf("kcov-wrapper-ptrace version 0.1.0\n");
            return 0;
        case 'h':
        default:
            fputs(help_text, stdout);
            return (opt == 'h') ? 0 : 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: no program specified.\n");
        fprintf(stderr, "Usage: %s [OPTIONS] <program> [args...]\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    /* Resolve KCOV path: CLI flag → env var → default */
    if (!kcov_path) {
        kcov_path = getenv("KCOV_PATH");
        if (!kcov_path)
            kcov_path = DEFAULT_KCOV_PATH;
    }

    if (verbose) {
        fprintf(stderr, "KCOV path:   %s\n", kcov_path);
        fprintf(stderr, "Output:      %s\n", output_path);
        fprintf(stderr, "Unique PCs:  %s\n", unique_path);
        fprintf(stderr, "Buffer size: %zu entries\n", buffer_size);
        fprintf(stderr, "Compact thresh: %u GB\n", compact_thresh);
    }

    /* ── Open KCOV device ──────────────────────────────────────────── */
    int fd = kcov_open(kcov_path);
    if (fd == -1) {
        perror("Failed to open KCOV device");
        fprintf(stderr, "Please check:\n"
                        "  1. CONFIG_KCOV is enabled in the kernel\n"
                        "  2. debugfs is mounted: mount -t debugfs none /sys/kernel/debug\n"
                        "  3. You have permission to access %s\n", kcov_path);
        return 1;
    }

    /* ── Init trace and mmap shared buffer ─────────────────────────── */
    if (kcov_init(fd, buffer_size)) {
        perror("ioctl(KCOV_INIT_TRACE) failed");
        kcov_cleanup(fd, NULL, buffer_size);
        return 1;
    }

    uint64_t *cover = kcov_mmap(fd, buffer_size);
    if (cover == MAP_FAILED) {
        perror("mmap failed");
        kcov_cleanup(fd, NULL, buffer_size);
        return 1;
    }

    /* ── Fork child (before opening output file — avoids fd leak) ─── */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        kcov_cleanup(fd, cover, buffer_size);
        return 1;
    }

    if (pid == 0) {
        /* Child */
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
            perror("ptrace(TRACEME) failed");
            exit(1);
        }
        if (kcov_enable_trace(fd)) {
            perror("ioctl(KCOV_ENABLE) failed");
            exit(1);
        }
        __atomic_store_n(&cover[0], 0, __ATOMIC_RELAXED);
        execvp(argv[optind], &argv[optind]);
        perror("execvp failed");
        exit(1);
    }

    /* ── Parent: open output file (after fork — child doesn't inherit) */
    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        perror("Cannot create output file");
        kcov_cleanup(fd, cover, buffer_size);
        kill(pid, SIGKILL);
        return 1;
    }

    /* ── Parent: trace loop ────────────────────────────────────────── */
    uint64_t total_pcs     = 0;
    uint64_t syscall_count = 0;
    syscall_reader_fn read_syscall = arch_detect_syscall_reader();

    if (timeout_sec > 0) {
        fprintf(stderr, "Timeout set to %u seconds (alarm not yet implemented)\n",
                timeout_sec);
    }

    trace_loop(pid, cover, buffer_size, &fp, output_path, read_syscall,
               &total_pcs, &syscall_count);

    if (fp) fclose(fp);

    /* ── Generate deduplicated pcs.txt ─────────────────────────────── */
    fprintf(stderr, "\nProcessing collected data...\n");

    uint64_t unique_count = 0;
    generate_unique_pcs(output_path, unique_path, &unique_count);

    /* ── Print summary ─────────────────────────────────────────────── */
    printf("\n======== KCOV collection completed ========\n");
    printf("Traced syscalls        : %lu\n", syscall_count);
    printf("Total PCs collected    : %lu\n", total_pcs);
    printf("Unique PCs (%s) : %lu\n", unique_path, unique_count);
    printf("Detailed data          : %s\n", output_path);
    printf("===========================================\n");

    kcov_cleanup(fd, cover, buffer_size);
    return 0;
}
