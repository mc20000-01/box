# Math and Logic

## COMPUTER MATH

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

## MAKING DECISIONS

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
