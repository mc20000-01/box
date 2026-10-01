# Implementations and Compile Targets

## IMPLEMENTATIONS AND COMPILE TARGETS

BX is a language specification first. A BX program should mean the same thing no matter whether it is run by an interpreter, transpiled into another language, assembled, or compiled straight into a raw binary.

The reference implementation is written in C. In that form, the C program is responsible for reading BX source, parsing each command, managing boxes, resolving `$name` style substitutions, keeping the marks list, and executing control flow such as `jump`, `jumpif`, and `end`. Note the order of operations it must respect: the marks list is seeded with every `premark` while the program loads, and only then does execution begin, which is the whole reason `premark` can be jumped to before its line runs.

BX can also be transpiled or compiled to lower-level targets:

* **C output:** BX commands become C code. Boxes can be represented as a runtime map from names to string values, with helper functions for substitution, math, comparisons, input, output, and jumps.

* **Assembly output:** BX commands become assembly instructions plus a small runtime model for string storage, box lookup, printing, input, integer math, and branch labels.

* **Raw binary output:** BX can be compiled all the way into an executable binary for a chosen platform. This target must define the exact binary format, calling convention, system calls or runtime services, memory layout, and entry point.

A transpiler does not have to copy the interpreter internally. It only has to preserve the observable behavior of the BX program: printed output, accepted input, box values, math results, jumps, program termination, and error handling.

### Required target behavior

Every target should preserve these rules:

* Commands are parsed case-insensitively unless a future extension says otherwise.
* `|` separates command arguments.
* `//` begins a comment outside literal text.
* Unknown commands are skipped instead of raising an error.
* Boxes store values by name, and a name may contain letters, digits, `_`, `-`, `?`, and `#`.
* `$name` resolves to the current value of a box, `$$name` to the value of the box that `name` points at.
* Reassigning a box replaces the old value.
* Integer math uses BX behavior, including returning `0` for division by zero.
* `premark NAME` enters the marks list before any BX code runs, so forward jumps to a `premark` always work.
* `mark NAME` enters the marks list when execution reaches it, points at the line after the `mark`, and re-points its name if it runs again.
* A jump to a name that is not in the marks list, or to a line number out of range, is ignored and execution continues on the next line.
* `jump NAME|m` jumps to a mark, while `jump NUMBER` jumps to a 1-based physical line number.
* `end` terminates the program.

Targets may use different internal representations, but they should not change what a valid BX program does.

### Libraries

Three C libraries live in `src/`. The wifi and gfx ones are now reachable from BX programs through the `lib` / `high.*` / `low.*` commands described in [Libraries](07-libraries.md), exactly as [Getting Started](01-getting-started.md) describes: the same boxes, the same marks, the same jump rules.

* `src/bx_gfx.c` and `src/bx_gfx.h` implement the themed UI element tree behind the `high.gfx.*` family described in [Libraries](07-libraries.md): colors in `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa` form, themes of exactly six ordered tags with `^^` caret inheritance, element creation and lookup by id, and theme push/pop history. `make gfx-tests` covers it.
* `src/bx_wifi.c` and `src/bx_wifi.h` cover scanning, connecting, and status, with separate paths for native, web/WASM, and bare-metal environments. `lib load|wifi` switches on the `high.wifi.*` family; `low.env` reports which environment the runtime detected.

## COMPILER RULESETS

`bx compile` normally assumes a hosted C toolchain: a compiler that can link against libc and produce a program the operating system will load. That assumption is wrong for a kernel, for a bootloader, and for anything else that runs before libc exists.

A ruleset is a small file that replaces those defaults. It says which compiler to call, what flags to pass, how to link, and what the result should be called.

```text
ruleset.md
name: freestanding-i386
version: 1.0.0
description: Bare-metal i386 with an explicit link step
cc: i686-linux-gnu-gcc
cflags: -m32 -std=c99 -ffreestanding -nostdlib -nostartfiles -fno-pie -O2 -I.
ld: i686-linux-gnu-ld
ldflags: -m elf_i386 -T kernel.ld
objcopy: i686-linux-gnu-objcopy
suffix: elf

```

The fields are `cc`, `cflags`, `ld`, `ldflags`, `objcopy`, and `suffix`. `ld` and `ldflags` are optional: a ruleset with only `cc` and `cflags` compiles and links in one step, which is the common case for cross-compiling to another hosted system. `suffix` is the file extension of the output, so the same build can produce `.elf` for one target and `.bin` for another.

Use one with any of the backends:

```text
bx compile program.bx -o program.elf --ruleset freestanding-i386
bx asm program.bx -o program.s --ruleset kernel-i386
bx raw program.bx -o program.bin --ruleset kernel-i386

```

`bx rulesets` lists what can be found, and `bx ruleset NAME` prints one resolved, which is the fastest way to find out where a name came from:

```text
$ bx ruleset kernel-i386
ruleset kernel-i386 (./rulesets/kernel-i386.md)
  name         kernel-i386
  cflags       -m32 -ffreestanding -nostdlib -nostartfiles ...
  ldflags      -m elf_i386 -T src/kernel.ld -nostdlib -z max-page-size=0x1000
  suffix       elf

```

Two directories are searched, in order: `$BOXEDLANG_RULESETS` first, then `./rulesets`, then the current directory. The first wins, so an environment variable can shadow the checked-in ruleset without editing anything. That is how the kernel build overrides a path without touching the file.

Two rulesets ship in the repository. `kernel-i386` is the Multiboot1 kernel the `make kernel` target uses, and `freestanding-i386` is a template with an explicit link step for anyone writing their own bare-metal program.

Without `--ruleset`, nothing changes: the native target still uses `cc` and links the way it always did.

### Performance

Measured on this machine (`-O3 -march=native`, output to `/dev/null`). The interpreter pre-parses the program once at load into per-line op records, resolves `$name` without allocating in the hot case, stores short box values inline, caches the numeric value of numeric boxes, and only flushes stdout on a real terminal.

* **1,000,000-iteration counter loop** (`math` + `jumpif`): 1.18 s before optimisation, 0.30 s after ≈ **0.15 µs per command**. Moving the rest of the way to 0.05 µs/command would require compiling the loop (JIT) rather than interpreting it.
* **FizzBuzz to 9,999**: 0.043 s before, 0.010 s after.
* **Per-command 10,000-iteration loops** (`stress.sh`): `box` 13→4 ms, `math` 17→6 ms, `test` 18→4 ms, `jumpif` 22→4 ms, `jump` 15→3 ms, `del` 16→4 ms.

Run `./stress.sh` to reproduce these numbers on your own machine.

You are now equipped with everything you need to become a BoxedLANG master.
