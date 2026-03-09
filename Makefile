# kcov-wrapper-ptrace — Linux KCOV coverage collector with syscall segmentation
# SPDX-License-Identifier: GPL-2.0-only

CC       ?= gcc
CFLAGS   ?= -O2 -Wall -Wextra -Werror -std=gnu11
LDFLAGS  ?=
TARGET   := kcov-wrapper-ptrace

# ── Source files ─────────────────────────────────────────────────────

SRC_DIR  := src
INC_DIR  := include
ARCH_DIR := $(SRC_DIR)/arch

SRCS_C   := $(SRC_DIR)/main.c \
            $(SRC_DIR)/kcov.c \
            $(SRC_DIR)/dedup.c \
            $(SRC_DIR)/output.c \
            $(SRC_DIR)/compaction.c \
            $(SRC_DIR)/ptrace.c \
            $(SRC_DIR)/postprocess.c \
            $(ARCH_DIR)/detect.c \
            $(ARCH_DIR)/riscv64.c \
            $(ARCH_DIR)/x86_64.c \
            $(ARCH_DIR)/arm64.c \
            $(ARCH_DIR)/generic.c

OBJS     := $(SRCS_C:.c=.o)
DEPS     := $(wildcard $(INC_DIR)/*.h $(ARCH_DIR)/*.h)

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
MANDIR   ?= $(PREFIX)/share/man/man1

VERSION  ?= 0.1.0

# ── Build ────────────────────────────────────────────────────────────

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile src/*.c with -I for the include directory
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I. -c -o $@ $<

$(ARCH_DIR)/%.o: $(ARCH_DIR)/%.c $(ARCH_DIR)/arch.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -I. -c -o $@ $<

# ── Install ──────────────────────────────────────────────────────────

.PHONY: install
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 src/kcov-sym.py $(DESTDIR)$(BINDIR)/kcov-sym

.PHONY: install-man
install-man:
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 doc/kcov-wrapper-ptrace.1 $(DESTDIR)$(MANDIR)/

# ── Clean ────────────────────────────────────────────────────────────

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f pcs.txt pcs_detailed.txt files.txt

# ── Lint ─────────────────────────────────────────────────────────────

.PHONY: lint
lint: cppcheck checkpatch

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all --inconclusive --std=c11 \
		--suppress=missingIncludeSystem \
		-I$(INC_DIR) $(SRCS_C); \
	cppcheck_exit=$$?; \
	if [ $$cppcheck_exit -ne 0 ]; then \
		echo "cppcheck found issues (exit $$cppcheck_exit)"; \
	fi 2>&1 || true

.PHONY: checkpatch
checkpatch:
	@if command -v checkpatch.pl >/dev/null 2>&1; then \
		git ls-files '*.c' '*.h' 2>/dev/null | xargs -r checkpatch.pl --no-tree -f; \
	else \
		echo "checkpatch.pl not found (install linux-tools-common)"; \
	fi

.PHONY: format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(SRCS_C) $(wildcard $(INC_DIR)/*.h $(ARCH_DIR)/*.h); \
	else \
		echo "clang-format not found"; \
	fi

# ── Test ─────────────────────────────────────────────────────────────

TEST_SRCS := tests/test_dedup.c tests/test_parse_line.c tests/test_output.c
TEST_BINS := $(patsubst tests/%.c,/tmp/kcov_%,$(TEST_SRCS))
TEST_DEPS := src/dedup.c src/compaction.c src/output.c

.PHONY: test
test: $(TEST_BINS)
	@pass=0; fail=0; \
	for t in $(TEST_BINS); do \
		if $$t > /dev/null 2>&1; then \
			pass=$$((pass + 1)); \
		else \
			fail=$$((fail + 1)); \
		fi; \
	done; \
	echo "==========================================="; \
	echo "Test results: $$((pass + fail)) run, $$pass passed, $$fail failed"; \
	echo "==========================================="; \
	[ $$fail -eq 0 ]

/tmp/kcov_test_dedup: tests/test_dedup.c src/dedup.c include/kcov-wrapper-ptrace.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< src/dedup.c

/tmp/kcov_test_parse_line: tests/test_parse_line.c src/compaction.c include/kcov-wrapper-ptrace.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< src/compaction.c

/tmp/kcov_test_output: tests/test_output.c src/output.c include/kcov-wrapper-ptrace.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< src/output.c

# ── Dist ─────────────────────────────────────────────────────────────

.PHONY: dist
dist: clean
	mkdir -p kcov-wrapper-ptrace-$(VERSION)
	cp -r *.py *.md LICENSE Makefile .gitignore \
		doc/ examples/ .github/ include/ src/ tests/ \
		kcov-wrapper-ptrace-$(VERSION)/ 2>/dev/null || true
	tar czf kcov-wrapper-ptrace-$(VERSION).tar.gz kcov-wrapper-ptrace-$(VERSION)
	rm -rf kcov-wrapper-ptrace-$(VERSION)

# ── Help ─────────────────────────────────────────────────────────────

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all        Build $(TARGET)"
	@echo "  install    Install to $(PREFIX)/bin"
	@echo "  clean      Remove build artifacts"
	@echo "  lint       Run cppcheck and checkpatch"
	@echo "  format     Format source with clang-format"
	@echo "  test       Run tests"
	@echo "  dist       Create release tarball"
