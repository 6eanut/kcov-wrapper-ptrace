# Security Policy

## Supported Versions

| Version | Supported          |
|---------|--------------------|
| 0.1.x   | :white_check_mark: |

## Reporting a Vulnerability

Please do **not** report security vulnerabilities through public GitHub issues.

Instead, report them via email to the project maintainers at
[kcov-wrapper-ptrace-security@googlegroups.com](mailto:kcov-wrapper-ptrace-security@googlegroups.com).

You should receive a response within 48 hours. If the issue is confirmed,
we will release a patch as soon as possible.

## Security Considerations for This Tool

`kcov-wrapper-ptrace` operates with elevated privileges (requires access to
`/sys/kernel/debug/kcov` and uses `ptrace`). Please be aware:

- This tool should only be used in trusted environments
- The KCOV interface exposes kernel memory addresses — output files may
  contain sensitive information about kernel layout
- Running `ptrace` on a process gives full visibility into that process's
  execution state

When reporting vulnerabilities, please include:

- A description of the vulnerability
- Steps to reproduce
- Affected versions
- Any potential mitigations you've identified
