# box

BoxedLANG: a small scripting language with a C standard library, a software
framebuffer, a UI toolkit, and vector documents.

```box
lib load|gfx
high.gfx.fbsize 320|200
high.gfx.fbclear #101820

ui init
ui.new|root|frame
ui.set|root|w|320
ui.set|root|h|200
ui.set|root|theme|[c#2a9d8f.h#e9f5f0.r4.t%90.x#000000.b#ffffff]

ui.new|hello|label|root
ui.set|hello|text|hello, world

ui.render
```

Two things to notice, because they are what trips everyone up. A theme is
bracketed, dot-separated, and **all six fields are required in order**
`c h r t x b`. And `ui.new` takes `ID|KIND [parent] [W H]`, where the width and
height only apply to a `frame` or a `pane`; a label takes its size from the
frame it is in.

## Build

```sh
make            # builds ./bx
```

Needs a C99 compiler and `-lm`. There is nothing else to install.

## Test

There is no aggregate target, because a suite that reports "ok" without saying
how much it checked is not worth much. Each one prints its own count:

```sh
make smoke           # 41 checks
make features        # 41 checks
make math-tests      # 61 checks
make friendly-tests  # 28 checks
make bxe-tests       # 12 checks
make ui-tests        # 50 checks
make gfx-tests       # 13 checks
make gfx2-tests      # 64 checks
make snd-tests       # 20 checks
make packagetests
make kernel
make os
```

330 checks in total. A failing suite prints the `FAIL` lines and exits
non-zero.

## Run

```sh
./bx run hello.bx
./bx                 # bare, prints the command list
./bx run script.bx   # relative to the file, or absolute
```

## Documentation

| | |
|---|---|
| [Getting started](docs/01-getting-started.md) | the whole language in one page |
| [Math and logic](docs/02-math-and-logic.md) | `high.math`, the 3D toolkit |
| [Flow control](docs/03-flow-control.md) | marks, jumps, conditions |
| [Power users](docs/04-power-users.md) | interpolation, modules, packages |
| [Command line](docs/05-command-line.md) | `bx` itself |
| [Compiling](docs/06-compiling.md) | rulesets and search paths |
| [Libraries](docs/07-libraries.md) | `gfx`, `ui`, `math`, `snd`, `wifi` |
| [UI](docs/08-ui.md) | the toolkit, element by element |

`make site` builds these into `site/`, and `make serve-site` looks at them
locally.

## Layout

```
src/bx.c         the interpreter and every command
src/bx_gfx.c     software rasterizer
src/bx_ui.c      UI toolkit
src/bx_bxvg.c    vector documents
src/bx_math.c    math and 3D
src/bx_snd.c     sound
src/bx_wifi.c    network
tests/           the suites behind the make targets
docs/            the documentation behind make site
```

## License

No licence has been chosen yet. Until one is, the default applies:
all rights reserved. Say the word and this becomes MIT.
