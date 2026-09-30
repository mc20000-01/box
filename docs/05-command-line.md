# Driving bx From Your Shell

## DRIVING BX FROM YOUR SHELL

Everything above is the language. The `bx` program itself is the tool that runs, transpiles, and compiles it.

```text
usage: bx run FILE | bx transpile FILE -o OUT.c | bx emit-c FILE | bx compile FILE -o OUT [--target T] | bx asm FILE -o OUT.s [--target T] | bx raw FILE -o OUT.bin [--target T] | bx targets | bx version | add -t/--time for timing, options may appear anywhere

```

### The commands

* **`bx run FILE`** interprets `FILE` and prints the output of your program right now. Nothing is written to disk.


* **`bx transpile FILE -o OUT.c`** writes a self-contained C program to `OUT.c`. The BX source is embedded inside the C file along with the runtime, so compiling `OUT.c` later gives you a program that behaves exactly like `bx run` did.


* **`bx emit-c FILE`** is `transpile` without the file: it prints the generated C to your terminal so you can pipe it somewhere.


* **`bx compile FILE -o OUT`** builds a native executable for your machine.


* **`bx asm FILE -o OUT.s`** stops one step earlier and writes assembly instead of a binary.


* **`bx raw FILE -o OUT.bin`** goes all the way to a flat binary with no ELF wrapper, ready to be loaded by a bootloader or written into a disk image.


* **`bx targets`** lists every known compile target with the compiler it uses.


* **`bx version`** prints the version of `bx` and compares it with the `VERSION` file in the project, so you can tell at a glance whether you are up to date.

### The flags

* **`-o OUT`** names the output file. It is required by `transpile`, `compile`, `asm`, and `raw`.


* **`--target NAME`** picks the compiler and flags for `compile`, `asm`, and `raw`. Without it you get `native`.


* **`--keep-c`** leaves the intermediate generated C file in `/tmp` instead of deleting it, which is handy when you want to see exactly what your BX turned into.


* **`-t` / `--time`** prints how long a run or compile took to standard error as `bx: MODE FILE took X.XXXXXX s`. Options can appear anywhere on the command line, before or after the mode and file, so `./bx -t run file.bx` and `./bx --time compile file.bx -o out` both work.


* **`--keep-comments`, `-D NAME`, `-U NAME`** are accepted so scripts and build files have a stable command line, but the transpiler does not act on them yet.

### A taste of the target list

`bx targets` lists the compilers and cross compilers that `compile`, `asm`, and `raw` know how to drive. Every one of them produces a normal userspace program for a host CPU:

* **`native`**, which is just `cc` for the machine you are on.

* **Cross compilers**, where you get a userspace program for another CPU: `x86_64`, `i386`, `i686`, `aarch64`, `armv7`, `arm`, `riscv64`, `riscv32`, `mips`, `mipsel`, `mips64`, `powerpc`, `ppc64`, `ppc64le`, `s390x`, `sparc64`, and `loongarch64`. These need the matching cross compiler installed, which is what `setup.sh --cross` tries to arrange for you.

There are currently no freestanding, Multiboot, or UEFI compile targets. The kernel under `src/` is a hand-written ELF32 Multiboot1 image built by `make kernel` and linked with `src/kernel.ld`; it is not produced by `bx compile`. Adding those target families would mean teaching the transpiler to emit freestanding code and giving the runtime its own box storage, since the current runtime leans on the C standard library.

### If you would rather use make

The project ships a `Makefile` that wraps all of this:

```text
make            # build ./bx
make smoke      # run, transpile, compile, asm, and raw a small test program
make features   # string and file builtin tests
make gfx-tests  # gfx library tests
make packagetests  # package manager tests, using an isolated cache
make run-example            # run examples/hello.bx
make compile-example        # compile it with $(TARGET), default native
make targets                # list compile targets
make os                    # compile os/os.bx into .build/boxed-os
make run-os                # build and run boxed-os
make kernel                # build .build/kernel.elf, the Multiboot1 kernel
make verify-kernel         # boot that kernel under QEMU and check for BOXEDLANG-OK
make install               # install to /usr/local/bin, PREFIX= overrides it
make clean

```

`setup.sh` is the one-shot helper around all of it. Run `./setup.sh` to build and smoke test, `./setup.sh --install` to install system wide, `./setup.sh --user` to install into `~/.local/bin`, and `./setup.sh --cross` to also try installing cross compilers.

---
