# Implementations and Compile Targets

## CHAPTER 9: IMPLEMENTATIONS AND COMPILE TARGETS

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

Three C libraries live in `src/`. The wifi and gfx ones are now reachable from BX programs through the `lib` / `high.*` / `low.*` commands described in Chapter 7, exactly as chapter 3 describes: the same boxes, the same marks, the same jump rules.

* `src/bx_gfx.c` and `src/bx_gfx.h` implement the themed UI element tree behind the `high.gfx.*` family described in Chapter 7: colors in `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa` form, themes of exactly six ordered tags with `^^` caret inheritance, element creation and lookup by id, and theme push/pop history. `make gfx-tests` covers it.
* `src/bx_wifi.c` and `src/bx_wifi.h` cover scanning, connecting, and status, with separate paths for native, web/WASM, and bare-metal environments. `lib load|wifi` switches on the `high.wifi.*` family; `low.env` reports which environment the runtime detected.

### Performance

Measured on this machine (`-O3 -march=native`, output to `/dev/null`). The interpreter pre-parses the program once at load into per-line op records, resolves `$name` without allocating in the hot case, stores short box values inline, caches the numeric value of numeric boxes, and only flushes stdout on a real terminal.

* **1,000,000-iteration counter loop** (`math` + `jumpif`): 1.18 s before optimisation, 0.30 s after ≈ **0.15 µs per command**. Moving the rest of the way to 0.05 µs/command would require compiling the loop (JIT) rather than interpreting it.
* **FizzBuzz to 9,999**: 0.043 s before, 0.010 s after.
* **Per-command 10,000-iteration loops** (`stress.sh`): `box` 13→4 ms, `math` 17→6 ms, `test` 18→4 ms, `jumpif` 22→4 ms, `jump` 15→3 ms, `del` 16→4 ms.

Run `./stress.sh` to reproduce these numbers on your own machine.

You are now equipped with everything you need to become a BoxedLANG master.
