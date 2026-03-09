#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026  Kcov Wrapper Ptrace Contributors
"""
kcov-sym — convert KCOV program counter addresses to file:line symbols.

Uses addr2line in parallel to resolve PC addresses from pcs.txt
against a vmlinux kernel image.
"""
import argparse
import shutil
import subprocess
import sys
import os
from multiprocessing import Pool, cpu_count
from typing import List, Tuple


def addr2line_worker(args: Tuple[str, List[str]]) -> List[str]:
    """Worker function: resolve a chunk of PC addresses via addr2line."""
    vmlinux_path, pcs = args
    cmd = ['addr2line', '-e', vmlinux_path, '-f']

    process = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=10**6
    )

    # Batch all addresses in one write to reduce syscall overhead
    input_data = '\n'.join(pcs)
    stdout, stderr = process.communicate(input=input_data)

    if stderr:
        print(f"Error: {stderr}", file=sys.stderr)

    return stdout.splitlines()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert KCOV PC addresses to file:line symbols using addr2line."
    )
    parser.add_argument(
        'vmlinux', metavar='VMLINUX',
        help='Path to the vmlinux kernel image')
    parser.add_argument(
        'pcs_file', metavar='PCS_FILE',
        help='Path to pcs.txt (one hex address per line)')
    parser.add_argument(
        '-j', '--jobs', type=int, default=cpu_count(),
        help=f'Number of parallel workers (default: {cpu_count()})')
    parser.add_argument(
        '-o', '--output', default='files.txt',
        help='Output file (default: files.txt)')
    parser.add_argument(
        '--version', action='version', version='kcov-sym 0.1.0')

    args = parser.parse_args()

    # Validate addr2line is available
    if not shutil.which('addr2line'):
        print("Error: addr2line not found. Please install binutils.", file=sys.stderr)
        sys.exit(1)

    # Validate vmlinux exists
    if not os.path.exists(args.vmlinux):
        print(f"Error: vmlinux file not found: {args.vmlinux}", file=sys.stderr)
        sys.exit(1)

    # Read all PC addresses
    with open(args.pcs_file, 'r') as f:
        all_pcs = [line.strip() for line in f if line.strip()]

    total_pcs = len(all_pcs)
    if total_pcs == 0:
        print("Warning: no PC addresses found in input file.", file=sys.stderr)
        sys.exit(0)

    # Split addresses into chunks for parallel processing
    chunk_size = (total_pcs + args.jobs - 1) // args.jobs
    chunks = [all_pcs[i:i + chunk_size] for i in range(0, total_pcs, chunk_size)]

    print(f"[*] Processing {total_pcs} addresses using {args.jobs} parallel workers...")

    # Parallel addr2line resolution
    with Pool(processes=args.jobs) as pool:
        worker_args = [(args.vmlinux, chunk) for chunk in chunks]
        results = pool.map(addr2line_worker, worker_args)

    # Write results
    print(f"[*] Writing results to {args.output}...")
    with open(args.output, 'w') as f:
        for lines in results:
            for line in lines:
                f.write(line + '\n')

    print("[+] Done!")


if __name__ == "__main__":
    main()
