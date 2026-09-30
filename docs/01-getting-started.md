# Getting Started

## MEET YOUR NEW BEST FRIEND, THE BOX!

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

## WRITING YOUR FIRST PROGRAM

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

## TALKING AND LISTENING

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
