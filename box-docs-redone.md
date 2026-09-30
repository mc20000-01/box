# THE BOXEDLANG USER'S MANUAL!

### A Programmer's Guide

Welcome to the exciting world of programming, You are about to unlock the full potential of your computer using BoxedLANG, or **BX** for short. BX is a wonderfully simple but powerful command-oriented programming language built entirely around a fun and easy-to-understand concept: **boxes**.

Whether you want to build a calculator, write a text adventure, or just make your screen light up with text, BX gives you the tools to make it happen. Let's dive right in and start communicating with your machine!

---

## CHAPTER 1: MEET YOUR NEW BEST FRIEND, THE BOX!

Imagine a little mailbox inside your computer's memory. In BX, a box stores a value under a specific name.

To create your very first box, you simply use the `box` command followed by the name you want to give it, the special `|` symbol (which BX uses to separate arguments), and the value you want to put inside.

For example:

```bx
box name|mc20000

```

This creates a box named `name` containing the text `mc20000`.

Whenever you want to pull that information back out and use it, you just place a `$` symbol right in front of the box's name. If you want your computer to speak to you, you can combine this with the `say` command:

```bx
say Hello $name

```

Your computer will proudly output:

```text
Hello mc20000

```

.

You can change what is inside a box at any time! Assigning to an existing box simply replaces its previous value.

```bx
box score|10
box score|20
say $score

```

This will print `20` because the old score was replaced.

Box names are not as strict as you might think. A name can contain letters, digits, and the characters `_`, `-`, `?`, and `#`. That is why BX programs can have boxes called `file-usr`, `boot?`, or `#1`.

Boxes can even contain references to *other* boxes. Values are resolved when the `box` command runs, so if `name` already holds `BX`, a box whose value is `Hello \)name` stores the text `Hello BX`. This is what lets a box be a little template.

---

## CHAPTER 2: WRITING YOUR FIRST PROGRAM

Are you ready to write a real program? Create a file on your disk named `hello.bx`.

Type these exact lines into your file:

```bx
box name|BX
say Hello from $name!
end

```

.

When you run this, your program will perform three magic steps:

1. It creates a box called `name`.


2. It prints a message containing the contents of your box.


3. It uses the `end` command to tell the computer the program is finished.



Output:

```text
Hello from BX!

```

.

### The Golden Rules of BX Syntax

* **The Separator:** The vertical bar `|` is BX's normal argument separator. You will use it to split up the different pieces of your commands.


* **Whitespace is Preserved:** If you put spaces inside a value, BX respects them exactly as written. For example, `box message|Hello     world` keeps all those spaces perfectly intact because the `|` character is the only thing that officially ends a value.


* **Case Doesn't Matter:** Commands are not normally case-sensitive. Typing `SAY Hello` works exactly the same as `say Hello`.


* **Comments:** If you ever want to leave a note for yourself in your code, just use `//`. Everything after the `//` is treated as a comment and ignored by the computer.


* **Unknown commands do nothing:** BX never crashes on a command it does not know. The line is simply skipped, so a typo costs you one step and nothing else.



---

## CHAPTER 3: TALKING AND LISTENING

### Printing with SAY

The `say` command prints text to your output screen.

```bx
say Hello

```

.

If you want to add a dramatic pause, you can add a time delay as an optional second argument.

```bx
say Hello|1

```

This prints "Hello" and then waits for the specified amount of time before moving to the next instruction. If you want absolutely no delay, you can explicitly type `|0`.

### Getting Input with ASK

What if you want the user to type something in? Use the `ask` command! `ask` reads input from the user and safely tucks it away into a box.

Here is the most important rule for `ask`: **The last space-separated word is always the target box**. Everything before that final word is printed as the prompt for the user.

```bx
ask What is your name username

```

In this example, the prompt is "What is your name" and the user's answer gets stored in the box `$username`.

If you just type `ask username`, there is no prompt message, and the computer just waits for the user to type something to put into `$username`.

Because box names may contain digits and `#`, `ask #1` and `ask op` are perfectly legal. The built-in boxed-OS uses this heavily: `ask #1`, `ask op`, `ask #2` collects a calculator expression with no prompt at all.

---

## CHAPTER 4: COMPUTER MATH

Your computer is a giant calculator, and BX lets you tap into that power using the `math` command! It performs integer arithmetic.

The syntax looks like this: `math TARGET|LEFT|RIGHT|OP`.

* **TARGET:** The box where the answer will be saved.
* **LEFT & RIGHT:** The numbers you want to calculate.
* **OP:** The operator you want to use.

BX supports addition (`+`), subtraction (`-`), multiplication (`*` or `x`), integer division (`/`), and modulo (`%`).

```bx
box a|20
box b|7

math add|$a|$b|+
say The sum is $add

```

. Because BX uses integer arithmetic, dividing 10 by 3 (`math answer|10|3|/`) will give you `3`, not a decimal. Also, if you ever accidentally try to divide by zero, BX safely returns `0` to keep your program from crashing!

---

## CHAPTER 5: MAKING DECISIONS

A smart program can make choices! BX does this using **conditions** to compare two values. You can use the following comparison operators:

* `==` (Equal)


* `!=` (Not equal)


* `>` (Greater than)


* `<` (Less than)


* `>=` (Greater than or equal)


* `<=` (Less than or equal)



BX is very flexible. You can put the operator right in the middle, like `\(score|==|100`, or you can put it at the very end, like `\)score|100|==`. Both mean the exact same thing!

### The TEST Command

`test` evaluates a condition and stores one of two values depending on if it is true or false.

```bx
test result|10|>|5|YES|NO
say $result

```

Because 10 is greater than 5, the `$result` box will contain `YES`. If you leave off the true/false values, `test` uses `1` for true and `0` for false.

### The IF Command

If you want to execute an entire command *only* when a condition is true, use `if`.

```bx
box score|100
if $score|>|50|say|You passed!

```

This checks if the score is greater than 50. Because it is, it executes the nested command `say You passed!`.

The nested command can be *anything*, including `jump` or `mark`. That is how the built-in `os/os.bx` does its login gate in a single line:

```bx
if $loggedin|==|2|jump|loggedin|m

```

If the condition is false, `if` does nothing at all and the program moves on to the next line.

---

## CHAPTER 6: JUMPING AROUND!

Programs don't just have to run straight down from top to bottom. You can jump around!

### Planting Flags: premark and mark

A mark gives a name to a location in your program, like planting a flag. BX has **two** ways to plant one, and the difference is *when* they happen.

**`premark NAME` is a pre-run mark.** Before a single line of your BX code executes, `bx` reads your whole program and collects every `premark` into the marks list. That means every `premark` in the file already exists on line one, so you are free to jump forward to a `premark` that appears further down your program.

```bx
premark loop

```

**`mark NAME` is a runtime mark.** `mark` is an ordinary command. It only adds `NAME` to the marks list when execution actually reaches that line, and the name it adds can be built out of boxes at that moment.

```bx
box i|1
mark retry-$i

```

Compare the two:

| | `premark NAME` | `mark NAME` |
| --- | --- | --- |
| When it runs | Before any BX code runs, while the program loads | When your program reaches the line |
| Name | Literal text, taken from the source | Resolved first, so `$boxes` and `\)boxes` work |
| Jumping to it before it exists | Always fine | Not found, the jump is ignored and the program carries on |
| Good for | Fixed labels known ahead of time | Labels created later, or built from user input |

Both kinds end up in the *same* marks list, so `jump NAME|m` and `jumpif ... NAME|m` treat them identically. There is no extra syntax to remember. A `mark` that runs a second time simply re-points its name to the new spot, so a mark inside a loop never fills the list up with duplicates.

A `premark` points at its own line (jumping to it just falls through to the next line), while a `mark` points at the line *after* it, which is usually what you want when a loop jumps back.

### JUMP and JUMPIF

The `jump` command changes the current program position.

* To jump to a specific line number, just use the number: `jump 22`. Line numbers are physical lines counted from 1, including blank and comment lines.


* To jump to a mark you planted, you **must** add `|m` to the end so BX knows you are looking for a mark name, not a line number: `jump loop|m`. If the name is not in the marks list, or the line number is out of range, the jump is ignored and execution moves on to the next line.



You can combine jumps with conditions using `jumpif`! This is how you build amazing things like loops and game menus.

```bx
box i|0

premark loop

say $i
math i|$i|1|+
jumpif $i|<|10|loop|m

say Finished.
end

```

This creates a counting loop! It prints the number, adds 1 to it, and jumps back to the `loop` mark as long as `$i` is less than 10.

### Loops built with mark

Because `mark` waits to be reached, it can sit inside a loop and plant a new mark on every pass. Here `retry` only becomes a real destination the first time line 5 runs, and jumping back to it lands on `say attempt`, never on the `mark` line itself.

```bx
box tries|0

premark start

mark retry
say attempt $tries
math tries|$tries|1|+
ask try again ?
if $?|==|y|jump|retry|m

say gave up after $tries attempts
end

```

Feed it `y`, `y`, `n` and you get:

```text
attempt 0
try again attempt 1
try again attempt 2
try again gave up after 3 attempts

```

### Mark names can be built at runtime

Since `mark` resolves its name like any other value, the same loop can plant a *different* mark on every pass. This is the runtime counterpart of the `premark app-calc` style dispatch in `os/os.bx`, except the names here are generated instead of written out by hand.

```bx
box id|0

premark build

mark try-$id
say trying $id
math id|$id|1|+
jumpif $id|<|2|build|m

say build finished
ask which try try-1
jump $try-1|m
end

```

The build loop plants `try-0` and `try-1` on its way past. Then `ask which try try-1` prints its prompt and drops whatever the user types into the box `try-1`, and `jump $try-1|m` uses that box as the mark name. Jumping to `try-1` lands right after `mark try-1`, which is `say trying $id`.

---

## CHAPTER 7: ADVANCED TRICKS FOR POWER USERS

* **Deleting Boxes:** Keep your computer's memory tidy! Use `del NAME` to completely erase a box you no longer need. Asking for a box that was never created, or that you deleted, gives you an empty string instead of an error.


* **Clearing the Screen:** Use the `clear` (or `cls`) command to wipe your terminal screen completely clean, giving you a fresh canvas.


* **Double Indirection:** If you have a box that contains the *name* of another box, you can use two dollar signs `$$` to get the final value! For example, if `$name` holds `message`, and `$message` holds `Hello`, typing `say $$name` will output `Hello`.


* **The Colon Rule:** BX keeps your colons. A `:` inside a value is printed exactly as you typed it, so `box t|a: b` really does hold `a: b`. The one exception is a colon placed directly in front of a `$` box, which is dropped so you can glue names together: `box-:$name` becomes `box-bob` when `$name` is `bob`. If you want a colon to survive right in front of a box, escape it as `\/:`, exactly as you would escape it anywhere else.


* **Explicit References:** `\(` and `\)` both mean "read the box that follows", which is the explicit spelling of `$name`. A value of `Hello \)name` stores `Hello BX` when `$name` is `BX`. Reach for it when you want the reference to read clearly inside a longer piece of text.


* **Watch What Counts as a Name:** A box name is a run of letters, digits, `_`, `-`, `?`, and `#`, and `$` grabs that whole run. So `say $a-b` asks for the box named `a-b`, not the value of `a` followed by a minus sign. Any character outside that set ends the name, which is why `say $a -b` prints the value of `a` and then ` -b`.


* **Dynamic Box Names:** You can name a box using the contents of *another* box! `box note-$name|$text` creates a box called `note-bob` if `$name` is `bob`. The built-in `os/os.bx` leans on this hard with things like `box file-:$name` and `box note-:$name`, which is how its file and note apps store a different box per user entry.


* **Boxes as Jump Tables:** Because a box can hold a mark name, `ask which app` + `jump $which|m` turns user input straight into a jump. That is the whole trick behind the `app-$app` dispatch in `os/os.bx`.

### Short Commands Reference

Tired of typing long words? BX has built-in abbreviations!

* `box` = `b`

* `say` = `s`

* `ask` = `a`

* `math` = `m`

* `test` = `t`

* `if` = `i`

* `jump` = `j`

* `jumpif` = `ji`

* `del` = `d`

* `premark` = `pm`

* `mark` = `mk`

* `end` = `e`

* `clear` = `cls`


### Runtime facilities: packages, environments, and libraries

Beyond the core commands, BX ships three runtime facility groups. They follow the same rules as everything else: `|` separates arguments, boxes hold values, and the command token can stop at either a space *or* a `|`, so `low.arch|boxname` and `low.arch boxname` both work.

* **`umload` — the package manager.** `umload require|NAME|mark` runs `NAME` as a module and jumps back to the mark when it ends. The module sees a copy of your boxes and writes its results back, so the usual idiom is to set input boxes, require the package, then read the output box. Packages are installed from a url, the registry, or `./packages/NAME.bx`, and cached in `$BOXEDLANG_CACHE`, `~/.cache/boxedlang`, or `./.boxcache`.

  ```text
  umload require|NAME|mark     install if needed, resolve deps, run the package
  umload install|NAME|URL      install into the cache
  umload remove|NAME           delete a cached package
  umload list                  loaded and cached packages
  umload info|NAME             metadata for one package
  umload deps|NAME             dependency tree
  umload verify|NAME           install dependencies without running
  umload search|QUERY          search the registry
  umload cache|dir             print the cache directory
  umload cache|clear           empty the cache
  umload publish|NAME|VERSION  copy into ./boxpkg, update the registry, push
  ```

  A package is an ordinary BX file whose metadata lives in leading `//` comments:

  ```text
  // name: mypack
  // version: 1.0.0
  // author: you
  // description: what it does
  // deps: otherpack, thirdpack
  ```

  Dependencies are installed automatically and cycles are reported rather than looped. `require` marks the package as loaded in a `pkg_NAME` box. The registry is `$BOXEDLANG_REGISTRY`, `./boxpkg/registry.txt`, `./packages/registry.txt`, `./registry.txt`, or one installed next to the `bx` binary; each line is `name|raw url|version|description|author`. See `boxpkg/README.md` for the package format.

* **`bxe` — the environment manager.** Env objects keep their own private boxes and a command counter while a running program executes other work, then hand the result back. Actions: `list`, `create|name`, `run|name|cmd...`, `all|cmd...` (run the same command in every env), `cmds|name` (show the command counter), `reset|name`. A lazy eviction policy keeps at most 16 envs and resets the highest-use env when a program passes 2000 commands inside it.

* **`lib`, `high.*`, and `low.*` — the library layer.** `lib list` shows compiled-in vs. active libraries, `lib load|NAME` turns one on, `lib unload|NAME` turns one off. The `high.*` commands are gated on their library being active and refuse with an error otherwise:
  * `high.wifi.init`, `high.wifi.connect|ssid|pass|sec`, `high.wifi.status|box`, `high.wifi.info`, `high.wifi.env|box`, `high.wifi.disconnect`. On the native environment connect simulates a successful association (`ip=192.168.1.100`).
  * `low.*` is always available: `low.arch`, `low.env`, `low.tick|box` (monotonic nanoseconds), `low.pid|box`.

### The GFX library

`lib load|gfx` turns on a themed UI element tree. Elements are identified by a `uint32` id; pass `0` as the id to have one assigned, and the box you name is set to the real id. Note that the subcommand is part of the command token, so it is spelled `high.gfx.new`, not `high.gfx new`.

```text
high.gfx.types                                  element type names
high.gfx.color|BOX|#rgb                         parse a color to an integer
high.gfx.colorhex|BOX|INTEGER                   format an integer back to #rrggbb
high.gfx.theme|BOX|[c#fff.h#000.r$4.t%100.x#000.b#eee]   parse a theme
high.gfx.new|ID|PARENT|TYPE|X|Y [W|H|THEME|BOX|TEXT]     create an element
high.gfx.set|ID|FIELD|VALUE                     field: x y w h text theme box
high.gfx.get|ID [FIELD] [BOX]                   read an element back
high.gfx.count [BOX]                            number of elements
high.gfx.list                                   print every element
high.gfx.clear                                  drop every element
```

`TYPE` is one of `button`, `text`, `slider`, `box`, `textbox`, `label`, `image`, `panel`.

A theme string is a bracketed list of exactly six tags, **in this order**: `c` primary, `h` highlight, `r` rounding, `t` transparency, `x` text, `b` background. Colors are `#rgb`, `#rgba`, `#rrggbb` or `#rrggbbaa`. Rounding is `$50` for a percentage or `8` for pixels. Transparency is `0`–`100`. Anything else is a parse error, and `high.gfx.theme` leaves the box empty.

Every successfully parsed theme is pushed onto a four-deep history. A theme of `^^` carets inherits from the most recently pushed theme, `^^^` from the one before that, and so on up to `^^^^^`. Asking for more history than exists is a parse error, and inheriting does not itself push a new entry.

```text
lib load|gfx
high.gfx.theme|c|[c#f00.h#0f0.r$8.t%50.x#00f.b#111]
high.gfx.theme|same|[c#00f.h#0f0.r$8.t%50.x#00f.b#111]
high.gfx.theme|inherit|^^^^^
```

```text
lib load|gfx
high.gfx.new|1|0|button|10|20|100|30|[c#f00.h#0f0.r$4.t%90.x#00f.b#eee]|btn|OK
high.gfx.set|1|text|Cancel
high.gfx.get|1|text|out
say $out
```



---

## CHAPTER 8: DRIVING BX FROM YOUR SHELL

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

