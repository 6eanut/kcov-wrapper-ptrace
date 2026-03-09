<!-- SPDX-License-Identifier: GPL-2.0-only -->
# Architecture

How `kcov-wrapper-ptrace` achieves unlimited-depth kernel coverage with per-syscall attribution.

**Two outcomes from one mechanism:**
1. **Unlimited depth** — by draining and resetting the KCOV buffer at every syscall exit, the buffer never overflows. You can trace any program regardless of how many syscalls it makes.
2. **Per-syscall attribution** — because we snapshot at syscall boundaries, every PC address is naturally tagged with the syscall number that triggered it.

## Overview

```
┌──────────────┐    ptrace     ┌──────────────────┐
│  Child Proc  │◄──────────────│  Tracer (parent) │
│  (tracee)    │  syscall-stop │                  │
│              │──────────────►│  trace_loop()    │
└──────┬───────┘               └────────┬─────────┘
       │ KCOV                           │
       ▼                                ▼
┌──────────────┐               ┌──────────────────┐
│  KCOV Buffer │  mmap         │  Output Pipeline │
│  (shared)    │◄──────────────│  dedup → save →  │
│              │               │  compact         │
└──────────────┘               └──────────────────┘
```

## Trace Loop State Machine

The core of the tool is a two-state machine driven by `PTRACE_SYSCALL` stops:

```
           PTRACE_SYSCALL
  ┌──────────┐            ┌──────────┐
  │  USER    │───────────►│  KERNEL  │
  │  SPACE   │◄───────────│  SPACE   │
  └──────────┘  syscall   └──────────┘
                     exit
                         │
                         ▼
                  ┌────────────────┐
                  │ on exit:       │
                  │ 1. dedup()     │
                  │ 2. save_pcs()  │
                  │ 3. reset buf() │
                  │ 4. compact()   │
                  └────────────────┘
```

- **Syscall entry** (`in_syscall = 0 → 1`): The kernel has entered the syscall handler. We do nothing — KCOV is already recording.
- **Syscall exit** (`in_syscall = 1 → 0`): The kernel is about to return to userspace. We:
  1. Read the current syscall number from registers (architecture-specific)
  2. Deduplicate the KCOV buffer in-place
  3. Flush unique PCs to `pcs_detailed.txt` annotated with the syscall number
  4. Reset `cover[0] = 0` — the buffer is ready for the next syscall
  5. Check if compaction is needed

## Unlimited-Depth Coverage: Why the Buffer Never Overflows

Raw KCOV uses a fixed-size ring buffer (default 1M entries). When a program executes many syscalls, the buffer wraps and earlier entries are lost — you cannot fully trace a real workload.

This tool resets `cover[0]` to 0 after every syscall exit. The kernel interprets `cover[0] == 0` as "buffer is empty" and starts filling from `cover[1]` again. By draining and resetting at every syscall boundary, each syscall gets a fresh buffer — the total number of syscalls across the run becomes irrelevant.

**The only remaining risk**: a single syscall that generates more PC entries than `cover_size`. This is unusual for normal syscalls (most generate < 1000 entries), but possible for long-running ioctl-heavy code paths. Increase `-b/--buffer-size` if needed.

## KCOV Buffer Data Flow

```
                     KCOV Device
                          │
                          ▼
┌─────────────────────────────────────────┐
│ cover[0]    = number of entries (N)     │  ← atomic counter
│ cover[1]    = PC address 1              │
│ cover[2]    = PC address 2              │
│ ...                                     │
│ cover[N]    = PC address N              │
│ cover[N+1]  = garbage / next entry      │
│ cover[2^20] = end of buffer             │
└─────────────────────────────────────────┘
        │
        ▼ dedup_cover()
        │  - qsort cover[1..N]
        │  - compact duplicates
        │  - atomic_store new size
        │
        ▼ save_pcs()
           - iterate cover[1..N]
           - fprintf "0x%lx syscall=%lu\n"
           - atomic_store cover[0] = 0
```

## Compaction Algorithm

When `pcs_detailed.txt` exceeds `MAX_DETAIL_FILE_SIZE` (1 GB default):

1. Flush and close the file
2. Reopen for reading
3. Parse every line via `parse_line()` into `PCEntry[]` array
4. `qsort` by (PC, syscall_num)
5. Rewrite to the same file, collapsing duplicates within each PC
6. Reopen for appending

The compaction is **lossy with respect to duplicate syscall annotations** for the same (PC, syscall) pair but preserves the PC coverage set exactly.

## Architecture Abstraction

```
arch_detect_syscall_reader()
    │
    ├── #ifdef __riscv    → get_syscall_number_riscv64()  [regs.a7]
    ├── #ifdef __x86_64__ → get_syscall_number_x86_64()   [orig_rax]
    ├── #ifdef __aarch64__→ get_syscall_number_arm64()    [regs.regs[8]]
    └── #else             → get_syscall_number_generic()  [UINT64_MAX]
```

Each arch file is guarded with `#ifdef` so the entire build tree compiles on any host. The dispatch is compile-time (zero runtime overhead).

## Post-Processing: Unique PC Generation

After the trace loop ends:

1. Read `pcs_detailed.txt` line by line
2. Extract the PC address from each line (ignore syscall annotations)
3. `qsort` all PCs
4. Write unique PCs to `pcs.txt` (adjacent comparison, single pass)

## Signal Handling

Non-syscall-stop signals (e.g., `SIGCHLD`, `SIGALRM`) are forwarded to the child process via `PTRACE_SYSCALL` with the signal number. This preserves normal signal semantics for the traced program.

## Performance Considerations

- **Buffer size**: Default 1M entries (8 MB). Larger values reduce syscall overhead but increase sort time.
- **Compaction**: O(N log N) on the file size. The 1 GB default keeps this rare.
- **Atomic operations**: `__atomic_load_n`/`__atomic_store_n` with appropriate memory orderings ensure correct visibility across the fork boundary.
