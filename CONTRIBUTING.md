# Contributing to kcov-wrapper-ptrace

Thank you for your interest in contributing! This document will help you get started.

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). Please read it before participating.

## Getting Started

### Prerequisites

- Linux system with kernel headers
- GCC or Clang (C11 support)
- For testing RISC-V: QEMU with RISC-V system emulation

### Setup

```bash
git clone git@6eanut:6eanut/kcov-wrapper-ptrace.git
cd kcov-wrapper-ptrace
make
```

### Build

```bash
make          # Build the tracer
make clean    # Remove build artifacts
```

## Development Workflow

1. **Find or create an issue** for the change you want to make
2. **Fork the repository** and create a feature branch
3. **Make your changes** following the coding style below
4. **Run linting:** `make lint`
5. **Add tests** for new functionality (see Testing section)
6. **Run tests:** `make test`
7. **Commit** with a descriptive message following [Conventional Commits](https://www.conventionalcommits.org/)
8. **Sign off** your commit: `git commit -s` (Developer Certificate of Origin)
9. **Push** and open a pull request

## Coding Style

### C Code

This project follows the **Linux kernel coding style**. Key points:

- Use 4-space indentation (as configured in `.clang-format`)
- Follow LLVM-based style with Linux brace placement
- Keep lines under 80 columns where reasonable
- Functions should be small and focused
- Use `static` for file-local functions
- Error handling via `goto` cleanup labels is encouraged
- All source files must include the SPDX license header

Run `make format` to auto-format with clang-format.

### Python Code

- Follow [PEP 8](https://peps.python.org/pep-0008/)
- Use type hints
- Use `argparse` for CLI
- All strings in English

## Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add ARM64 syscall annotation support
fix: handle SIGKILL during trace loop cleanup
docs: add architecture overview diagram
```

All commits must include a `Signed-off-by` line (DCO):

```
Signed-off-by: Your Name <your.email@example.com>
```

Use `git commit -s` to add it automatically.

## Testing

### Running Tests

```bash
make test       # Run all tests
make cov        # Run with coverage (requires gcov)
```

### Writing Tests

- Unit tests use a minimal self-contained CHECK macro
- Place test files in `tests/`
- Test file naming: `test_<module>.c`
- Test function naming: descriptive of the behavior tested

### Integration Tests

Integration tests run in QEMU with a kernel that has `CONFIG_KCOV=y`.
See `tests/integration/run_tests.sh`.

## Pull Request Process

1. Ensure CI passes (build, lint, test)
2. Update documentation if needed
3. Add a changelog entry under `[Unreleased]`
4. Request review from a maintainer
5. Address review feedback
6. Once approved, a maintainer will merge

## Architecture

See [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) for detailed internal documentation.

## Questions?

Open a [GitHub Discussion](https://github.com/6eanut/kcov-wrapper-ptrace/discussions) or file an issue.
