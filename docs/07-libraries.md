# Libraries

The boxes, the jumps, and the `math` command are the language itself. Everything else is a library: a C module that adds a new family of commands to BX, reachable through a prefix so a program can say `high.gfx.line` and mean a line on a framebuffer.

Libraries are not loaded automatically. A program asks for the ones it wants, so a script that draws nothing does not pay for the drawing code.

## LOADING A LIBRARY

```bx
lib load|gfx
say $lib_active_gfx

```

`lib list` prints what is compiled in and which is active:

```text
compiled-in libraries: wifi, gfx, snd, math
active:
  gfx

```

`lib unload|name` turns one back off. Each family has its own active flag, so loading `gfx` does not turn on `snd`.

Every library command is native-only. That is, they work in the reference `bx` interpreter but not in a transpiled program, because a transpiled program embeds a copy of the interpreter runtime and does not link the library sources. The commands still exist in that case; they explain that the library is not available rather than failing mysteriously.

## THE FAMILIES

Five families ship in the box. Each one uses the dotted spelling, `prefix.action|args...`, because the dot is what tells a BX command apart from an ordinary one.

| Family | What it does |
| --- | --- |
| `high.gfx.*` | framebuffer, themes, and the element tree |
| `high.snd.*` | tones, melodies, and WAV files |
| `high.math.*` | trigonometry, number theory, and constants |
| `high.m3d.*` | vectors, quaternions, matrices, and transforms |
| `high.wifi.*` | scanning, connecting, and status |

There is also a `low.*` family, which reports the environment the runtime found: `low.env` answers `native`, `web`, or `baremetal`, which is how a program adapts instead of guessing.

## GRAPHICS

`high.gfx.*` owns a framebuffer and a themed element tree.

```bx
lib load|gfx
high.gfx.fbsize|800|600
high.gfx.theme|dark
high.gfx.rect|10|10|400|300|#4fd6c4
high.gfx.render
end
```

Colors are written the way a designer writes them: `#rgb`, `#rgba`, `#rrggbb`, and `#rrggbbaa`. The shape command takes the color as its **last** argument, with no mode word in front of it — `rect|10|10|400|300|#4fd6c4`, not `rect|...|fill|#4fd6c4`. Passing `fill` makes the command report a bad color, because that is what it is being read as.

Shapes cover the usual ground: `rect`, `circle`, `ring`, `line`, `tri`, `poly`, `triline`, `plot`, and `pixel`. Gradients come in two directions, `gradh` and `gradv`, and `alpha` sets transparency for whatever is drawn next. `fbinfo`, `fbsize`, and `fbclear` report and reset the surface.

A theme is exactly six ordered tags with `^^` caret inheritance, so a theme can say "title bar text is dim" without repeating the color, and `theme` / `styles` / `style` push and pop theme history.

The element tree is the part that is meant to outlive a one-off drawing command: `new` makes an element by id, `get` looks one up, `set` changes a field, `draw` renders a subtree, and `clip` and `translate` set up a coordinate space for it. `render` puts the framebuffer on screen.

`make gfx-tests` runs the checks.

## SOUND

`high.snd.*` covers both synthesis and playback.

```bx
lib load|snd
high.snd.init|44100|1
high.snd.tone|440|0.25
high.snd.melody|C4|D4|E4|G4
high.snd.render

```

`tone` plays one frequency for a duration. `melody` plays a list of note names, and `note` plays a single named note. `midi`, `mvel`, and `voices` cover instruments and polyphony, `freq` converts a note to hertz, and `samples` and `wav` read or write sample data. `env` reads the envelope and `rate` sets the sample rate.

`make snd-tests` runs the checks.

## MATH

`high.math.*` is the arithmetic the built-in `math` command does not do. The built-in command is integer-only and always will be, because integer math is fast and predictable; this family is where the rest lives.

```bx
lib load|math
high.math.sqrt|2|root
high.math.fib|20|fib
say $root
say $fib

```

`sqrt`, `log`, `exp`, `pow`, and the trigonometry (`sin`, `cos`, `tan`, and their hyperbolic and degree variants) all take an operation and an output box. `deg2rad` and `rad2deg` convert angles without going through a library call.

The number theory is the interesting half: `isprime`, `nthprime`, `fib`, `fact`, `ncr`, `npr`, `gcd`, `lcm`, `isqrt`, `trunc`, and the digit sums `digitsum`, `divsum`, and `divcount`. `factor` returns a prime factorisation, `phi` and `totient` count coprimes, `collatz` runs the sequence, and `droot` takes an nth root.

## 3D MATH

`high.m3d.*` works on vectors, quaternions, and matrices.

```bx
lib load|math
high.m3d.vadd|a|b|c
high.m3d.vnorm|a|unit
high.m3d.qmul|q|r|out

```

The vector commands are `vadd`, `vsub`, `vscale`, `vdot`, `vcross`, and `vlen`. Quaternions get `qmul`, `qconj`, `qnorm`, `qrot`, and `qaxis`. Matrices get `mmul`, `mrot`, `mrotx`, `mroty`, `mrotz`, `mtrans`, and `mscale`. `mvec` transforms a vector by a matrix, `mident` makes an identity matrix, and `vnorm` normalises in place.

Rotations are quaternions rather than Euler angles on purpose: `mrotx`, `mroty`, and `mrotz` exist for composing a rotation from parts, but gimbal lock lives in the Euler representation, not in this one.

## WIFI

`high.wifi.*` has three implementations behind the same commands: native, web/WASM, and bare metal. `init` and `status` report what is available, `connect` and `disconnect` manage a link.

This is the family most likely to be unavailable: the commands report the reason instead of pretending, so a program that asks on a machine with no radio gets an answer rather than a hang.

## NAMING AND STYLE

A library never invents its own way to talk to the program. Results go into boxes, exactly like `math`, so `$` works the same everywhere:

```bx
high.math.fib|20|n
say $n

```

That consistency is the point. Once you know that a box holds a value and `$name` reads it, no library needs its own syntax to teach you.
