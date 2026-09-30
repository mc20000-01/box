# Jumping Around: Flow Control

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
