# Getting Started

BoxedLANG is a small language with a C standard library underneath it. It has
no types, no compiler, and no build step. A script is a list of lines; each line
is a command, its name, and some arguments separated by `|`.

```bx
box name|mc20000
say Hello $name
end
```

That is a complete program. It prints `Hello mc20000`.

## Contents

- [Boxes](#boxes)
- [Interpolation](#interpolation)
- [Arithmetic](#arithmetic)
- [Flow control](#flow-control)
- [Comparison](#comparison)
- [Files](#files)
- [The shape of a command](#the-shape-of-a-command)
- [Comments and blank lines](#comments-and-blank-lines)
- [Recommended way to start](#recommended-way-to-start)
- [Where to go next](#where-to-go-next)

## Boxes

A box is a named string. That is the only data type in the language.

```bx
box name|mc20000
box count|3
box pi|3.14159
```

Read one back with `$` in front of the name:

```bx
box name|mc20000
say Hello $name
end
```

Write to it again the same way. `box` overwrites without complaint:

```bx
box count|1
box count|2
say $count
end
```

Output is `2`.

An unset box reads as the empty string. It is not an error, which is why
`$unset` in a `say` prints nothing rather than complaining:

```bx
box nothing_here|
say [$nothing_here]
end
```

Names may contain letters, digits, `_`, `-`, `?` and `#`. Keep it to
alphanumerics and underscore unless you have a reason not to; `-` and `?` in a
name make `$name-with-dashes` ambiguous about where the name stops.

### Recommended way to use boxes

Give a box one job and name it after what it holds, not after what it is for.
`count` beats `temp`; `line_count` beats `c`. When you catch yourself writing
`box t|`, you are one step from not being able to read your own script.

Boxes are strings. If you want arithmetic, say so explicitly with `math`
rather than storing a number and hoping. `box` will happily hold `3.14` and
`math` will happily parse it, but `say` will print whatever bytes are there.

## Interpolation

`$name` inside any argument is replaced by that box's value. This happens
before the command sees the argument, so it works everywhere a value can
appear.

```bx
box user|mc20000
box greeting|Hello $user
say $greeting
end
```

Output is `Hello mc20000`.

Because it happens first, interpolation composes with everything else. Here the
name of a box is itself built from a variable, which is how you generate names
in a loop:

```bx
box i|1
box d_\(i\)_0|first value
say $d_\(i\)_0
end
```

Output is `first value`. The `\(name)` form is the one to reach for when the
variable name ends in the middle of a word, because a bare `$i` would stop at
the `_`.

There are six forms. All of them are in [Power Users](04-power-users.md), and
that is the one page worth reading twice:

| you write | you get | use it for |
|---|---|---|
| `$n` | the value of `n` | almost everything |
| `\(n)` | the value of `n`, stops at `)` | a name in the middle of a word |
| `$$n` | the box *named by* the value of `n` | indirection, tables by name |
| `:$n` | `n`'s value, colon dropped | `C:$path` |
| `\/:` | a literal `:` | when the colon is not your own escape |
| `$$` | a literal `$` | writing a `$` in text |

## Arithmetic

`math` takes a destination, two operands, and an operator:

```bx
math sum|17|25|+
say $sum
end
```

Output is `42`.

Operators are `+` `-` `*` `/` `%`, and `x` also works for multiply. Division and
modulo by zero give `0` rather than stopping the program, so a loop counter
that has gone wrong does not take the rest of the script with it.

Operands are integers, and the result is an integer. There is a separate float
library for that; see [Math and Logic](02-math-and-logic.md).

```bx
math half|9|2|/
say $half
end
```

Output is `4`, not `4.5`.

### Recommended way to do arithmetic

Keep the destination name meaningful and never reuse it as a scratch value
inside a loop. `math total|0|0|+` at the top and accumulate into it is
readable; `math x|...|+` in five different places is not.

## Flow control

Execution is a straight line of numbered lines. A `premark` names one of them,
and `jump` goes there:

```bx
box i|1
jump top|m

premark top
say line $i
math i|$i|1|+
jumpif $i|===|4|done|m
jump top|m

premark done
say finished
end
```

`premark` does not run anything. It records a name against a line number. The
`jump top|m` above it is the one that executes, and the `premark` line itself is
skipped when execution flows past it. That is why every block here ends with an
explicit jump and never relies on falling through.

### Comparison and `jumpif`

`jumpif` takes a left value, an operator, a right value, a mark to go to when
true, and optionally a different mark to go to when false:

```bx
jumpif $a|===|$b|if_equal|if_not_equal|m
```

The operators are `===` (equal), `!==` (not equal), `<` `>` `<=` `>=`. Both
sides are resolved first, so either may be a `$box`.

Comparison is by string, not by number, because everything is a string. That is
almost never what you want arithmetically, and `10` sorts before `9`. To compare
numbers, use `math` to reduce the difference to something comparable, or use
the math library's own helpers.

## Files

`file` opens, writes, reads and closes. The shape is always
`file|$dest|$op|...`, and the answer from most operations lands in `$dest`.
Note the order: the destination box comes first, the **operation name second**.
It reads backwards from every other command in the language, so it is worth
checking twice.

Opening gives you a handle, which is an ordinary string that happens to look
like `#fh0`. Keep it in a box:

```bx
file|h|open|.build/hello.txt|w
file|n|writeline|$h|Hello from BoxedLANG
file|$h|close
say wrote $n bytes
end
```

Output is `wrote 21 bytes`.

`write` writes the bytes you give it, exactly, and returns how many it wrote.
`writeline` is the same plus a newline if you did not already end with one, so
it is almost always what you want. The count includes the newline.

Reading pulls one line at a time, stripping the line ending. The dest box comes
first, then the op, then the handle:

```bx
file|h|open|.build/hello.txt|r
file|l1|line|$h
file|l2|line|$h
file|$h|close
say first was $l1
say second was $l2
end
```

Output:

```text
first was Hello from BoxedLANG
second was 
```

`size`, `exists` and `remove` work on a path rather than a handle, so they need
no open file:

```bx
file|h|open|.build/size-demo.txt|w
file|n|writeline|$h|hello
file|$h|close
file|sz|size|.build/size-demo.txt
file|ex|exists|.build/size-demo.txt
say $sz bytes, exists=$ex
end
```

Output is `6 bytes, exists=1`.

Modes are `r`, `w` and `a`. There is no `readline`; the op is `line`, and `read`
is there if you want a byte count instead of a line.

## The shape of a command

Every command is one line: a name, whitespace, then arguments. Arguments are
separated by `|`, with no spaces around it. There are no parentheses and no
commas.

```bx
lib load|gfx
high.gfx.fbsize 320|200
high.gfx.fbclear #101820
end
```

Two spellings exist for the library commands, and they are equivalent:

```bx
ui.new|first|frame
ui|new|second|frame
```

They are the same command, not two commands. Use the dotted form, which reads
like `high.gfx.*` and like everything else in
the libraries. Reach for the bar form when the whole line is data, as in a
generated spec or a table-driven dispatch.

Watch out for this one, because it fails silently: the **dotted** form takes its
arguments space-separated, and the **bar** form is `name|arg|arg`. Mixing them,
as in `ui.vg add|ring|circle` with a space, passes no arguments at all and
prints the usage line.

## Comments and blank lines

`//` starts a comment. Blank lines are ignored. Neither needs to be avoided:

```bx
// A comment, at the start of a line.
box n|5      // and at the end of one.

say $n
end
```

## Recommended way to start

Write the checker first. Every script in this repository does, and it is the
single habit that separates a script you can change next week from one you are
afraid to touch.

The pattern is a mark that decides, a block per check, and one shared failure
handler:

```bx skip
box fails|0

premark check
jumpif $got|!==|$want|fail|m
say PASS $what
jump $next|m

premark fail
say FAIL $what got [$got] want [$want]
math fails|1|$fails|+
jump $next|m
```

Everything else in this documentation assumes you are writing programs this
way. `make smoke` is the shortest complete example there is.

Two habits that pay for themselves immediately:

- **Run everything.** `lib load|gfx` must come before the first `jump`, because
  that is where the program starts.
- **Make the clock explicit.** In anything that animates, step by a fixed delta
  rather than by wall time, so the result is the same on a fast machine and a
  slow one. `ui frame step 0.016` is a fixed step; `ui frame once` is not.

## Where to go next

| if you want to | read |
|---|---|
| numbers, roots, vectors, matrices, quaternions | [Math and Logic](02-math-and-logic.md) |
| loops, branches, the whole flow-control surface | [Flow Control](03-flow-control.md) |
| interpolation, modules, packages, `$name` in depth | [Power Users](04-power-users.md) |
| running `bx`, the flags, exit codes | [Command Line](05-command-line.md) |
| rulesets, search paths, include semantics | [Compiling](06-compiling.md) |
| graphics, sound, network, math libraries | [Libraries](07-libraries.md) |
| the UI toolkit, themes, 134 element kinds | [UI](08-ui.md) |
