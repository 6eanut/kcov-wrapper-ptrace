# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- GPL-2.0-only license
- Makefile with build, install, lint targets
- .gitignore for build artifacts and output files
- Community health files (CONTRIBUTING.md, CODE_OF_CONDUCT.md, SECURITY.md)
- Issue and PR templates

## [0.1.0] - 2026-04-30

### Added
- Initial release: KCOV coverage collection via ptrace
- PC collection at system call boundaries on RISC-V
- Output to `pcs_detailed.txt` (with syscall annotations) and `pcs.txt` (deduplicated)
- Automatic file compaction when detailed output exceeds 1 GB
- `kcov-sym.py` helper for address-to-symbol resolution via `addr2line`

[Unreleased]: https://github.com/6eanut/kcov-wrapper-ptrace/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/6eanut/kcov-wrapper-ptrace/releases/tag/v0.1.0
