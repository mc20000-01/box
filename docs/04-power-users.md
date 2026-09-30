# Power Users

This page is the one worth reading twice. Almost everything awkward about
BoxedLANG comes from one place: **every argument is interpolated before the
command sees it.** Once that is in your head, the rest of the language is
straightforward.

## Contents

- [How interpolation actually works](#how-interpolation-actually-works)
- [The six forms](#the-six-forms)
- [Names and where they stop](#names-and-where-they-stop)
- [Indirection: tables by name](#indirection-tables-by-name)
- [Recommended way to structure a script](#recommended-way-to-structure-a-script)
- [Command reference](#command-reference)
- [Short aliases](#short-aliases)
- [Nested conditionals](#nested-conditionals)
- [Libraries](#libraries)
- [Errors and exit codes](#errors-and-exit-codes)

## How interpolation actually works

When a line runs, each of its arguments goes through one substitution pass
before anything else happens. During that pass, these are recognised, anywhere
in the text, and replaced:

| you write | it means | result |
|---|---|---|
| `$n` | value of box `n` | `n` is replaced by its value |
| `\(n)` | value of box `n`, stops at `)` | same, but an explicit end |
| `$$n` | value of the box **named by** box `n` | one level of indirection |
| `:$n` | `n`'s value, with the colon dropped | `C:$path` becomes `C:file.txt` |
| `\/:` | a literal colon | when the colon is data, not your escape |
| `$$` | a literal `$` | for writing a dollar sign |

Two details make this more forgiving than it first looks.

**An unset box is the empty string, not an error.** This is deliberate. A
script can reference `$typo` and get nothing rather than stopping, which is
what you want in a `say`, and what you do not want anywhere else. It is the
main reason a BoxedLANG script can fail quietly.

**A bare `$` followed by something that is not a name character expands to
nothing.** So text containing a dollar sign loses it unless you write `$$`.

**Arguments are split on `|` first, and an empty box still passes as an empty
argument.** So an interpolated separator works:

```bx
box sp|
str r|replace|hello world| |$sp
say [$r]
end
```

Output is `[helloworld]`.

The one place this is genuinely awkward is when the empty value is the *last*
argument, because a command cannot tell an empty string from a missing one. If
you need a real empty string in the final slot, the practical workaround is to
put a placeholder in the box and `str replace` it back out afterwards.

Here is all six forms, running:

```bx
box n|world
box holder|n
box greeting|hello $n
box d_\(n\)_0|first value
box drive|C:$n
box dollars|I want a $$ sign

say 1: $greeting
say 2: $$holder
say 3: $d_\(n\)_0
say 4: $drive
say 5: $dollars
end
```

Output:

```text
1: hello world
2: world
3: world_0
4: Cworld
5: I want a  sign
```

Line 2 is the one that needs a second look. `holder` contains the *text*
`n`, and `$$holder` looks up whatever box that text names, which is `n`, whose
value is `world`.

Line 4 shows why `:$` exists: the colon is dropped so that `C:` plus a value
does not become `C::file.txt`.

## Names and where they stop

A box name is letters, digits, `_`, `-`, `?` and `#`, and it ends at the first
character that is not one of those. This is worth internalising because the
`-` is a trap:

```bx
box a|1
box b-c|2
say [$a-$b-c]
end
```

Output is `[1-2-c]`. The parser read `b` and stopped at the `-`.

If you need a name in the middle of a word, use `\(name)`, which ends at the
closing parenthesis:

```bx
box i|1
box d_\(i\)_0|first value
say $d_\(i\)_0
end
```

Output is `first value`.

### Recommended way to name things

Stick to letters, digits, and underscore. `-`, `?` and `#` are accepted, but a
name containing them will surprise someone the first time they interpolate it in
the middle of an expression, and that is not a surprise worth having.

Keep names short and specific. `count`, `line`, `path`. Not `t`, `x1`, `tmp2`.

## Indirection: tables by name

`$$` is how you build a lookup table in a language with no arrays. A box holds
a *name*, and `$$` follows that name to the box it names.

```bx
box k|width
box width|200
say [$k]
say [$$k]
end
```

Output:

```text
[width]
[200]
```

`$k` gives you the text `width`, which is not what you want. `$$k` follows it to
the box named `width` and gives you `200`.

### Recommended way to build a table

One box per entry, named after the key, plus one box holding the key itself.
It costs one line per entry and needs no parsing at all, which in a language
without arrays is a good trade.

Where the key comes from data, split the line first. Two constraints to know:

- `str split` returns its fields joined with `|`, and `|` is also the argument
  separator, so **you cannot split on `|`**.
- A space separator leaves the leading space on every field after the first.

The reliable idiom is to replace your delimiter with `|`, split on that, then
`take` or `left` the fields you want. Write the separator literally: an
interpolated empty box is dropped from the argument list, which costs you the
`to` argument and makes `replace` print a usage line instead of working.

```bx
box line|alpha beta gamma
str bars|replace|$line| |
str parts|split|$bars|
str c1|left|$parts|5
str rest|right|$parts|6
str bars2|replace|$rest| |
str c2|left|$bars2|4
say first=[$c1] second=[$c2]
end
```

Output is `first=alpha second=agam`.

That is more work than `split` should be, and it is the main reason to prefer
one box per entry over parsing a table at runtime.

## Recommended way to structure a script

The pattern this codebase uses everywhere, and the one worth copying, is a
checker with a shared failure path and a jump table.

```bx
box failures|0
jump first_check|m

premark check
jumpif $got|!==|$want|check_fail|m
say PASS $name
jump $next|m

premark check_fail
say FAIL $name got [$got] want [$want]
math failures|$failures|1|+
jump $next|m

premark first_check
box name|first
box got|1
box want|1
box next|second_check
jump check|m

premark second_check
box name|second
box got|2
box want|2
box next|all_done
jump check|m

premark all_done
say done, failures: $failures
end
```

Three things about this are load-bearing:

**`jump` at the top, before the first check.** Execution starts at line one, so
without it the program falls into the `check` block with nothing set up. This
is why every test file in this repo begins `box failures|0` and `jump`.

**`premark` does not execute.** It records a name against the current line
number while the program loads. The line after it is what runs when something
jumps there. That is why the `check` block is placed immediately after its
`premark` and ends with an explicit `jump $next`.

**The name `m` is a marker flag, not a name.** `jump foo|m` goes to the line
named `foo`. `jump 42` goes to line 42 instead, which is useful for generated
code but is not something you should write by hand.

### When a script gets long

BoxedLANG has no `include`, so splitting a script across files means either
generating the file with `printf` or writing the parts in C and linking them.
For a script under a few hundred lines, keeping it in one file and using
`premark` to organise it is less work than either. See
[Compiling](06-compiling.md) for the C route.

## Command reference

These are the builtins. Everything else lives in a library and is documented in
[Libraries](07-libraries.md).

### `box NAME|VALUE`

Store a value. Overwrites without complaint. Alias: `b`.

```bx
box count|1
box count|2
say $count
end
```

### `say TEXT`

Print a line. Alias: `s`. A `box` with an empty value prints an empty line.

```bx
say hello
say
end
```

### `math DEST|A|B|OP`

Integer arithmetic into `DEST`. Alias: `m`.

Operators are `+`, `-`, `*`, `/`, `%`, and `x` also multiplies. Division and
modulo by zero give `0`.

```bx
math sum|17|25|+
math half|9|2|/
say sum=$sum half=$half
end
```

Output is `sum=42 half=4`. The division is integer division; 9/2 is 4, not 4.5.

### `test DEST|LEFT|OP|RIGHT|YES|NO`

Evaluate a condition and store `YES` or `NO`. Leave off the pair and you get
`1` or `0`. Alias: `t`.

Operators: `===`, `!==`, `<`, `>`, `<=`, `>=`. Comparison is by string unless
both sides look like numbers, in which case it is numeric.

```bx
test a|10|>|5|YES|NO
test b|10|>|50|YES|NO
test c|5|===|5
say $a $b $c
end
```

Output is `YES NO 1`.

The order is `LEFT|OP|RIGHT`, which is the same as `jumpif`. An earlier build
had these three transposed and every result came back false; the six cases in
`tests/features.bx` exist to keep that from coming back.

### `jump TARGET|m`

Jump to a `premark` name, or to a line number if you omit the `m`. Alias: `j`.

### `jumpif LEFT|OP|RIGHT|TARGET|m`

Jump if the condition holds. There is no else-form; for that, jump past the
block on the false case. Alias: `ji`.

```bx
jumpif $n|>|0|positive|m
say n was not positive
end

premark positive
say n is positive
end
```

### `premark NAME`

Register a name for the current line. Aliases: `mark`, `mk`. Executes nothing.

### `del NAME`

Delete a box. Alias: `d`. Reading it afterwards gives the empty string.

### `clear`

Clear the screen. Alias: `cls`.

### `end`

Stop. Alias: `e`. Optional; a script that runs off the end stops anyway.

### `ask PROMPT|NAME`

Print a prompt and read one line from stdin into a box. Alias: `a`. The prompt
is everything before the last space; the name is the last word.

```bx skip
ask What is your name|name
say hello $name
end
```

### `file DEST|OP|...`

File access. **The destination comes first and the operation second**, which
reads backwards from every other command in the language.

| op | arguments | result |
|---|---|---|
| `open` | `PATH MODE` | a handle, into `DEST` |
| `append` | `PATH MODE` | a handle, into `DEST` |
| `write` | `HANDLE TEXT` | bytes written |
| `writeline` | `HANDLE TEXT` | bytes written, plus a newline |
| `line` | `HANDLE` | next line, newline stripped |
| `read` | `HANDLE BYTES` | that many bytes |
| `close` | `HANDLE` | nothing |
| `size` | `PATH` | size in bytes, or `-1` |
| `exists` | `PATH` | `1` or `0` |
| `remove` | `PATH` | `1` or `0` |

```bx
file|h|open|.build/note.txt|w
file|n|writeline|$h|Hello
file|$h|close
file|sz|size|.build/note.txt
file|ex|exists|.build/note.txt
say wrote $n, size $sz, exists $ex
end
```

Output is `wrote 6, size 6, exists=1`.

Handles are ordinary strings that look like `#fh0`. Keep them in a box and pass
`$h`, but note that `open` is the one operation whose first argument is the
destination: `file|h|open|...` fills `h` with the handle.

### `str DEST|OP|...`

String manipulation.

| op | form | result |
|---|---|---|
| `len` | `TEXT` | length |
| `upper` / `lower` | `TEXT` | cased |
| `find` | `TEXT SUBSTR` | offset, or `-1` |
| `take` | `TEXT N` | first N characters |
| `left` / `right` | `TEXT N` | the side |
| `repeat` | `TEXT N` | repeated |
| `pad` / `pad_head` | `TEXT N` | padded to width |
| `replace` | `TEXT OLD NEW` | substituted |
| `wordcount` | `TEXT` | words |
| `reverse` | `TEXT` | reversed |
| `contains` | `TEXT SUB` | `1` or `0` |
| `startswith` / `endswith` | `TEXT SUB` | `1` or `0` |
| `count` | `TEXT CHARS` | occurrences |
| `join` | `TEXT ... SEP` | joined |

```bx
str a|len|hello world
str b|upper|hello
str c|find|hello world|world
str d|contains|hello|ell
say len=$a upper=$b find=$c has=$d
end
```

Output is `len=11 upper=HELLO find=6 has=1`.

### `if LEFT|OP|RIGHT|COMMAND`

Run one nested command if the condition holds. This is the only builtin that
takes a command as an argument, and it is rarely the clearest way to write
something. Prefer `jumpif` unless the body is genuinely one line.

```bx
box n|5
if ===|n|5|say five
if ===|n|6|say six
end
```

Output is `five`.

## Short aliases

Every alias is one character. They are not deprecated; the test suites use them
because they are shorter.

| alias | command | alias | command |
|---|---|---|---|
| `b` | `box` | `s` | `say` |
| `m` | `math` | `a` | `ask` |
| `t` | `test` | `i` | `if` |
| `j` | `jump` | `ji` | `jumpif` |
| `d` | `del` | `e` | `end` |
| `mk` | `premark` | `cls` | `clear` |

### Recommended way to write

Full names in scripts you will read again, aliases in tests and in throwaway
code. `math` says more than `m` when you find the line in a stack of
generated code at 2am.

## Nested conditionals

`if` runs its last argument as a command line. The nested command is joined
back together with bars, which means a nested `if` inside an `if` works but
gets hard to read fast:

```bx
box a|1
box b|1
if ===|a|1|if ===|b|1|say both are one
end
```

Output is `both are one`.

Two levels of nesting is about the limit of what stays readable. Beyond that,
use `jumpif` and real blocks.

## Libraries

`lib load|NAME` turns on one of four built-in libraries. There is no
file-include mechanism; if you need more than one program, generate the file or
use the C compiler to link.

```bx
lib load|gfx
lib load|math
lib list
end
```

`lib list` prints what is currently active, which is the fastest way to find out
why a command is not recognised:

```bx
lib load|math
lib list
end
```

Output:

```text
lib: math loaded
  math
```

The four names are `gfx`, `math`, `snd` and `wifi`. Anything else is an error
saying the name is not compiled into this build.

The ordering rule matters: a `lib load` on a line after the first `jump` never
runs, and the failure is silent, which is the worst combination available. Load
libraries first, then jump.

Library commands are documented in [Libraries](07-libraries.md).

## Errors and exit codes

There are no exceptions and no `try`. A bad argument prints to stderr and the
script continues to the next line. Division by zero gives `0` rather than
stopping. An unset box is empty rather than an error.

This means **a failing BoxedLANG script is a script that ran to completion and
printed something wrong to stderr**, which is why every test target greps for
`^FAIL` and also watches the exit status.

The one thing that does stop a script is a parse error: an unknown top-level
command is reported with its line number and the program exits non-zero.

```bx no-stderr
this is not a command
end
```

Output:

```text
line 1: unknown command 'this'
```

Commands beginning `high.`, `ui.` and `low.` are accepted by the parser without
being looked at, so a typo inside a library command is still silent. Check the
command list in [Libraries](07-libraries.md) when a library command seems to do
nothing.
