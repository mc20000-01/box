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

This will print `20` because the old score was replaced. Boxes can even contain references to *other* boxes. For example, if a box holds the text `Hello \(name`, BX will automatically figure out what `\)name` is when you try to print it.

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

math add|\(a|\)b|+
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

---

## CHAPTER 6: JUMPING AROUND!

Programs don't just have to run straight down from top to bottom. You can jump around!

### Creating Marks

A mark gives a permanent name to a location in your program, like planting a flag. You create one using `premark`.

```bx
premark loop

```

.

### JUMP and JUMPIF

The `jump` command changes the current program position.

* To jump to a specific line number, just use the number: `jump 22`.


* To jump to a mark you made, you **must** add `|m` to the end so BX knows you are looking for a mark name, not a line number: `jump loop|m`.



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

---

## CHAPTER 7: ADVANCED TRICKS FOR POWER USERS

* **Deleting Boxes:** Keep your computer's memory tidy! Use `del NAME` to completely erase a box you no longer need.


* **Clearing the Screen:** Use the `clear` (or `cls`) command to wipe your terminal screen completely clean, giving you a fresh canvas.


* **Double Indirection:** If you have a box that contains the *name* of another box, you can use two dollar signs `

$$` to get the final value[cite: 1]! For example, if `$name` holds `message`, and `$message` holds `Hello`, typing `say$$

name`will output`Hello`.

* **Protecting Colons:** BX normally removes colons `:` when resolving strings. If you want to print a literal colon on your screen, you must escape it by typing `\/:`.


* **Dynamic Box Names:** You can name a box using the contents of *another* box! `box note-:\(name|\)note` will dynamically create a new box name depending on what the user typed into `$name`.



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

* `premark` = `mark` or `mk`

* `end` = `e`

* `clear` = `cls`


You are now equipped with everything you need to become a BoxedLANG master.

---

## CHAPTER 8: IMPLEMENTATIONS AND COMPILE TARGETS

BX is a language specification first. A BX program should mean the same thing no matter whether it is run by an interpreter, transpiled into another language, assembled, or compiled straight into a raw binary.

The reference implementation may be written in C. In that form, the C program is responsible for reading BX source, parsing each command, managing boxes, resolving `$name` style substitutions, and executing control flow such as `jump`, `jumpif`, and `end`.

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
* Boxes store values by name.
* `$name` resolves to the current value of a box.
* Reassigning a box replaces the old value.
* Integer math uses BX behavior, including returning `0` for division by zero.
* `premark` defines named jump locations.
* `jump NAME|m` jumps to a mark, while `jump NUMBER` jumps to a line number.
* `end` terminates the program.

Targets may use different internal representations, but they should not change what a valid BX program does.

