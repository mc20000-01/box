CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O3 -march=native -flto -pthread
LDFLAGS ?= -lpthread -ldl -lm
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
TARGET ?= native
EXAMPLE ?= examples/hello.bx

OS_SRC ?= os/os.bx
OS_OUT ?= .build/boxed-os

# Kernel build
KERNEL_CC = x86_64-linux-gnu-gcc
KERNEL_CFLAGS = -m32 -std=c99 -ffreestanding -nostdlib -nostartfiles -fno-pie -O2 -Wall -Wextra -I. -fno-stack-protector -fno-builtin
KERNEL_LD = ld
KERNEL_LDFLAGS = -m elf_i386 -T src/kernel.ld -nostdlib -z max-page-size=0x1000

# QEMU
QEMU = qemu-system-x86_64
QEMU_FLAGS = -m 256M -serial mon:stdio -display gtk,gl=on -kernel

.PHONY: all features gfx-tests gfx2-tests snd-tests math-tests friendly-tests bxe-tests packagetests install install-user uninstall clean smoke targets run-example asm-example raw-example compile-example os run-os run-os-vm run-os-gfx run-kernel verify-kernel clean-kernel distclean

all: bx

bx: src/bx.c src/bx_gfx.c src/bx_wifi.c src/bx_thread_gpu.c src/bx_snd.c src/bx_math.c
	$(CC) $(CFLAGS) -o bx src/bx.c src/bx_gfx.c src/bx_wifi.c src/bx_thread_gpu.c src/bx_snd.c src/bx_math.c $(LDFLAGS)

# Kernel build
kernel: .build/kernel.elf

.build/kernel.elf: src/kernel.S src/kernel.c src/x86_64-graphics.c src/kernel.ld
	@mkdir -p .build
	$(KERNEL_CC) $(KERNEL_CFLAGS) -c src/kernel.S -o .build/kernel.S.o
	$(KERNEL_CC) $(KERNEL_CFLAGS) -c src/kernel.c -o .build/kernel.c.o
	$(KERNEL_CC) $(KERNEL_CFLAGS) -c src/x86_64-graphics.c -o .build/x86_64-graphics.o
	$(KERNEL_LD) $(KERNEL_LDFLAGS) -o .build/kernel.elf .build/kernel.S.o .build/kernel.c.o .build/x86_64-graphics.o

# Run kernel in QEMU with graphics
run-kernel: kernel
	$(QEMU) $(QEMU_FLAGS) .build/kernel.elf

# Boot kernel headless and confirm the COM1 marker
verify-kernel: kernel
	@timeout 25 $(QEMU) -m 256M -no-reboot -display none -monitor none -serial stdio -kernel .build/kernel.elf 2>/dev/null | grep -m1 BOXEDLANG-OK && echo "kernel boot verified (BOXEDLANG-OK)"

# OS build
os: bx $(OS_SRC)
	@mkdir -p .build
	./bx compile $(OS_SRC) -o $(OS_OUT)

run-os: os
	$(OS_OUT)

run-os-vm: os
	$(QEMU) -m 256M -kernel $(OS_OUT)

# New: Run OS with graphics
run-os-gfx: os
	$(QEMU) -m 256M -display gtk,gl=on -kernel $(OS_OUT)

install: bx
	install -Dm755 bx $(DESTDIR)$(BINDIR)/bx
	install -Dm644 boxpkg/registry.txt $(DESTDIR)$(PREFIX)/share/boxedlang/registry.txt

install-user: PREFIX := $(HOME)/.local
install-user: install

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/bx
	rm -f $(DESTDIR)$(PREFIX)/share/boxedlang/registry.txt

smoke: bx features
	@mkdir -p .build
	@printf 'box name|BoxedLANG\nsay Hello $$name\nbox i|0\npremark loop\nsay $$i\nmath i|$$i|1|+\njumpif $$i|<|2|loop|m\nbox j|0\nmark again\nsay again $$j\nmath j|$$j|1|+\njumpif $$j|<|2|again|m\nend\n' > .build/smoke.bx
	./bx run .build/smoke.bx
	./bx transpile .build/smoke.bx -o .build/smoke.c
	./bx compile .build/smoke.bx -o .build/smoke
	./bx asm .build/smoke.bx -o .build/smoke.s
	./bx raw .build/smoke.bx -o .build/smoke.bin
	.build/smoke
	@test -s .build/smoke.c
	@test -s .build/smoke.s
	@test -s .build/smoke.bin
	@echo 'smoke test passed'

features: bx
	@mkdir -p .build
	@cp tests/features.bx .build/features.bx
	@./bx run .build/features.bx > .build/features.out
	@grep -q '^FAIL' .build/features.out && { echo 'feature tests failed:'; grep '^FAIL' .build/features.out; exit 1; } || true
	@echo "feature tests passed ($$(grep -c '^PASS' .build/features.out) checks)"

gfx-tests: bx
	@mkdir -p .build
	@./bx run tests/gfx.bx > .build/gfx.out
	@grep -q '^FAIL' .build/gfx.out && { echo 'gfx tests failed:'; grep '^FAIL' .build/gfx.out; exit 1; } || true
	@grep -q 'failures: 0' .build/gfx.out || { echo 'gfx tests did not finish cleanly:'; tail -3 .build/gfx.out; exit 1; }
	@echo "gfx tests passed ($$(grep -c '^PASS' .build/gfx.out) checks)"

gfx2-tests: bx
	@mkdir -p .build
	@./bx run tests/gfx2.bx > .build/gfx2.out 2>&1
	@grep -q '^FAIL' .build/gfx2.out && { echo 'gfx2 tests failed:'; grep '^FAIL' .build/gfx2.out; exit 1; } || true
	@grep -q 'failures: 0' .build/gfx2.out || { echo 'gfx2 tests did not finish cleanly:'; tail -3 .build/gfx2.out; exit 1; }
	@echo "gfx2 tests passed ($$(grep -c '^PASS' .build/gfx2.out) checks)"

snd-tests: bx
	@mkdir -p .build
	@rm -f .build/boxed-test.wav
	@./bx run tests/snd.bx > .build/snd.out 2>&1
	@grep -q '^FAIL' .build/snd.out && { echo 'snd tests failed:'; grep '^FAIL' .build/snd.out; exit 1; } || true
	@grep -q 'failures: 0' .build/snd.out || { echo 'snd tests did not finish cleanly:'; tail -3 .build/snd.out; exit 1; }
	@echo "snd tests passed ($$(grep -c '^PASS' .build/snd.out) checks) (wav=$(shell test -s .build/boxed-test.wav && echo present || echo missing))"

math-tests: bx
	@mkdir -p .build
	@./bx run tests/math.bx > .build/math.out 2>&1
	@grep -q '^FAIL' .build/math.out && { echo 'math tests failed:'; grep '^FAIL' .build/math.out; exit 1; } || true
	@grep -q 'failures: 0' .build/math.out || { echo 'math tests did not finish cleanly:'; tail -3 .build/math.out; exit 1; }
	@echo "math tests passed ($$(grep -c '^PASS' .build/math.out) checks)"

friendly-tests: bx
	@mkdir -p .build
	@rm -f .build/friendly.wav
	@rm -rf .build/pkgcache
	@BOXEDLANG_CACHE=$(CURDIR)/.build/pkgcache ./bx run tests/friendly.bx > .build/friendly.out 2>&1
	@grep -q '^FAIL' .build/friendly.out && { echo 'friendly tests failed:'; grep '^FAIL' .build/friendly.out; exit 1; } || true
	@grep -q 'failures: 0' .build/friendly.out || { echo 'friendly tests did not finish cleanly:'; tail -3 .build/friendly.out; exit 1; }
	@echo "friendly tests passed ($$(grep -c '^PASS' .build/friendly.out) checks) (wav=$(shell test -s .build/friendly.wav && echo present || echo missing))"

bxe-tests: bx
	@mkdir -p .build
	@./bx run tests/bxe.bx > .build/bxe.out 2>&1
	@grep -q '^FAIL' .build/bxe.out && { echo 'bxe tests failed:'; grep '^FAIL' .build/bxe.out; exit 1; } || true
	@grep -q 'failures: 0' .build/bxe.out || { echo 'bxe tests did not finish cleanly:'; tail -3 .build/bxe.out; exit 1; }
	@echo "bxe tests passed ($$(grep -c '^PASS' .build/bxe.out) checks)"

packagetests: bx
	@mkdir -p .build
	@rm -rf .build/pkgcache
	@BOXEDLANG_CACHE=$(CURDIR)/.build/pkgcache ./bx run tests/packages.bx > .build/packages.out
	@grep -q '^FAIL' .build/packages.out && { echo 'package tests failed:'; grep '^FAIL' .build/packages.out; exit 1; } || true
	@grep -q 'failures: 0' .build/packages.out || { echo 'package tests did not finish cleanly:'; tail -3 .build/packages.out; exit 1; }
	@echo "package tests passed"

targets: bx
	./bx targets

run-example: bx $(EXAMPLE)
	./bx run $(EXAMPLE)

compile-example: bx $(EXAMPLE)
	mkdir -p .build
	./bx compile $(EXAMPLE) --target $(TARGET) -o .build/boxed-example

asm-example: bx $(EXAMPLE)
	mkdir -p .build
	./bx asm $(EXAMPLE) --target $(TARGET) -o .build/boxed-example.s

raw-example: bx $(EXAMPLE)
	mkdir -p .build
	./bx raw $(EXAMPLE) --target $(TARGET) -o .build/boxed-example.bin

clean:
	rm -f bx *.out *.c.tmp
	rm -rf .build

clean-kernel:
	rm -f .build/kernel.S.o .build/kernel.c.o .build/x86_64-graphics.o .build/kernel.elf

distclean: clean clean-kernel
