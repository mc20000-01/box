CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O3 -march=native -flto
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
TARGET ?= native
EXAMPLE ?= examples/hello.bx

OS_SRC ?= os/os.bx
OS_OUT ?= .build/boxed-os

.PHONY: all install install-user uninstall clean smoke targets run-example asm-example raw-example compile-example os run-os run-os-vm distclean

all: bx

bx: src/bx.c
	$(CC) $(CFLAGS) -o bx src/bx.c

install: bx
	install -Dm755 bx $(DESTDIR)$(BINDIR)/bx

install-user: PREFIX := $(HOME)/.local
install-user: install

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/bx

smoke: bx
	@mkdir -p .build
	@printf 'box name|BoxedLANG\nsay Hello $$name\nbox i|0\npremark loop\nsay $$i\nmath i|$$i|1|+\njumpif $$i|<|2|loop|m\nend\n' > .build/smoke.bx
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

os: bx $(OS_SRC)
	mkdir -p .build
	./bx compile $(OS_SRC) -o $(OS_OUT)

run-os: os
	$(OS_OUT)

run-os-vm: os
	OUT=$(OS_OUT) scripts/run-os-vm.sh

clean:
	rm -f bx *.out *.c.tmp
	rm -rf .build

distclean: clean
