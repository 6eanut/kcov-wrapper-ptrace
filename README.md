# KCOV Wrapper Ptrace

[![CI](https://github.com/6eanut/kcov-wrapper-ptrace/actions/workflows/ci.yml/badge.svg)](https://github.com/6eanut/kcov-wrapper-ptrace/actions/workflows/ci.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-green.svg)](CHANGELOG.md)
[![Architectures](https://img.shields.io/badge/arch-riscv64%20%7C%20x86__64%20%7C%20arm64-lightgrey.svg)](#architecture-support)

**KCOV + ptrace: unlimited-depth kernel coverage, segmented by system call.**

Two things that make this different:

1. **Tracks any program, not just toy workloads.** Raw KCOV has a fixed-size ring buffer — when it wraps, you lose data. This tool snapshots and resets the buffer at every syscall exit, so it **never overflows**. Trace `ls`, trace a database server, trace your fuzzer — the coverage is complete.

2. **Every PC address is tagged with its syscall number.** Because we collect per-syscall, the output tells you _which syscall_ triggered each kernel code path. Not just "this code was reached" but "this code was reached by `read()`, `write()`, `ioctl()`..."

## Quick Start

```bash
# Build
make

# Trace a program (requires root / CAP_SYS_PTRACE)
sudo ./kcov-wrapper-ptrace ls /

# Resolve PC addresses to file:line symbols
kcov-sym /boot/vmlinux pcs.txt
```

## What Makes This Different

| Tool | Unlimited depth (tracks any program) | Per-syscall attribution | Works without recompile |
|------|--------------------------------------|------------------------|------------------------|
| **kcov-wrapper-ptrace** | **Yes** — resets buffer per syscall | **Yes** — every PC tagged with syscall number | Yes (just KCOV in kernel) |
| Raw KCOV | No — ring buffer wraps and loses data | No | Yes |
| `gcov` / `gcov-kernel` | N/A | No | No (needs GCOV kernel) |
| `perf record` | Yes (sampling) | No — cannot attribute to syscall | Yes |

**The key insight:** ptrace does double duty here — it both **manages the buffer** (snapshot + reset before overflow) and **annotates the data** (we know the current syscall number at each stop). Raw KCOV gives you a firehose of PC addresses and hopes you catch it all. This tool turns that firehose into discrete, labeled frames.

## How It Works

```
┌──────────┐  fork/exec   ┌──────────────┐  ptrace   ┌────────────────┐
│  Parent  │──────────────│  Child Proc  │◄──────────│  Tracer Loop   │
│  (main)  │              │  (tracee)    │────stop──►│  (trace_loop)  │
└──────────┘              └──────┬───────┘           └───────┬────────┘
                                 │ KCOV                      │
                                 ▼                           ▼
                          ┌─────────────┐            ┌───────────────┐
                          │ KCOV Buffer │───mmap─────│  dedup_cover  │
                          │  (shared)   │            │  save_pcs     │
                          └─────────────┘            │  compact      │
                                                     └───────┬───────┘
                                                             │
                                                             ▼
                                                    ┌───────────────┐
                                                    │ pcs_detailed  │
                                                    │ pcs.txt       │
                                                    └───────────────┘
```

## CLI Reference

```
Usage: kcov-wrapper-ptrace [OPTIONS] <program> [args...]

Options:
  -o, --output FILE         Detailed output (default: pcs_detailed.txt)
  -u, --unique-output FILE  Unique PC output (default: pcs.txt)
  -k, --kcov-path PATH      KCOV device (default: /sys/kernel/debug/kcov)
  -t, --timeout SEC         Kill traced process after SEC seconds
  -b, --buffer-size N       KCOV buffer entries (default: 1048576)
  -c, --compact-threshold N Compaction threshold in GB (default: 1)
  -v, --verbose             Enable verbose output
  -h, --help                Show this help
  -V, --version             Show version
```

## Output Format

**`pcs_detailed.txt`** — PC addresses with syscall annotations:
```
0xffffffff80123456 syscall=1
0xffffffff80234567 syscall=1,3,15
0xffffffff80345678
```

**`pcs.txt`** — Sorted unique PC addresses:
```
0xffffffff80123456
0xffffffff80234567
0xffffffff80345678
```

## Requirements

- Linux kernel with `CONFIG_KCOV=y`
- `debugfs` mounted at `/sys/kernel/debug`
- Root or `CAP_SYS_PTRACE` for ptrace
- GCC or Clang (C11 with GNU extensions)

## Architecture Support

| Architecture | Status | Syscall register |
|-------------|--------|-----------------|
| RISC-V (riscv64) | Stable | `a7` |
| x86_64 | Stable | `orig_rax` |
| ARM64 (aarch64) | Experimental | `x8` |

## Building from Source

```bash
# Native build
make CC=gcc

# Cross-compile for RISC-V
make CC=riscv64-linux-gnu-gcc

# Cross-compile for ARM64
make CC=aarch64-linux-gnu-gcc

# Install
make install PREFIX=/usr/local
```

## Companion: `kcov-sym`

Resolve raw PC addresses to `file:line` using `addr2line`:

```bash
kcov-sym --help
kcov-sym /boot/vmlinux pcs.txt -j 16 -o files.txt
```

Supports parallel resolution (`-j`) and configurable output (`-o`).

## Examples

The `examples/` directory contains:

| Script | Purpose |
|--------|---------|
| [`basic-collection.sh`](examples/basic-collection.sh) | Simplest usage |
| [`kernel-fuzzer.sh`](examples/kernel-fuzzer.sh) | Feed PCs to syzkaller |
| [`diff-two-runs.sh`](examples/diff-two-runs.sh) | Compare two coverage runs |
| [`symbol-resolution.sh`](examples/symbol-resolution.sh) | End-to-end pipeline |

## Documentation

- [ARCHITECTURE.md](doc/ARCHITECTURE.md) — Trace loop state machine, KCOV buffer flow, compaction algorithm
- [CONTRIBUTING.md](CONTRIBUTING.md) — Setup, build, test, PR process
- [CHANGELOG.md](CHANGELOG.md) — Version history
- `man kcov-wrapper-ptrace` — Man page (after `make install-man`)

## Limitations

- Single-process tracing only (no thread group support yet)
- KCOV buffer can still overflow within a **single** syscall if that syscall generates more entries than the buffer size (unusual, but possible for certain ioctl-heavy workloads)
- Ptrace overhead: each syscall incurs two context switches (entry/exit stop), which slows the traced program significantly
- Timeout flag (`-t`) is accepted but alarm-based kill is not yet wired
- ARM64 syscall register access is untested on hardware

## License

GPL-2.0-only. See [LICENSE](LICENSE).

Linux KCOV is a GPL-2.0 kernel subsystem. This userspace companion follows the same license, consistent with tools like `perf`, `bpftrace`, and `trace-cmd`.

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for the full process. All commits must include a `Signed-off-by:` line (Developer Certificate of Origin).
