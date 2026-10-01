# Libraries

Boxes, jumps and `math` are the language itself. Everything else is a library:
a C module that adds a family of commands, reachable through a prefix so a
program can say `high.gfx.line` and mean a line on a framebuffer.

Libraries are not loaded automatically. A program asks for the ones it wants,
so a script that draws nothing does not pay for the drawing code.

## Contents

- [Loading](#loading)
- [The families](#the-families)
- [Graphics](#graphics)
- [The element tree](#the-element-tree)
- [Math](#math)
- [3D math](#3d-math)
- [Sound](#sound)
- [Wi-Fi](#wi-fi)
- [The environment](#the-environment)
- [Finding out what a command wants](#finding-out-what-a-command-wants)

## Loading

```bx
lib load|gfx
say $lib_active_gfx
end
```

`lib list` prints what is compiled in and which is active:

```bx
lib load|math
lib load|gfx
lib list
end
```

Output:

```text
lib: math loaded
lib: gfx loaded
compiled-in libraries: wifi, gfx, snd, math
active:
  gfx
  math
```

The four names are `gfx`, `math`, `snd` and `wifi`. `lib load` with anything
else reports that it is not compiled into this build — there is no file
include, so this is the only way to reach another file of code.

Load libraries **before the first `jump`**. A `lib load` on a line after it
never runs, and the failure is silent, which is the worst combination available.

Library commands are native-only. They work in the reference `bx` interpreter
but not in a transpiled program, which embeds a copy of the runtime without
linking the library sources. There they still exist, and they say the library is
not available rather than failing mysteriously.

## The families

| Family | Load | What it does |
|---|---|---|
| `high.gfx.*` | `gfx` | framebuffer, themes, element tree |
| `high.snd.*` | `snd` | tones, melodies, WAV files |
| `high.math.*` | `math` | arithmetic, trigonometry, number theory |
| `high.m3d.*` | `math` | vectors, quaternions, matrices |
| `high.wifi.*` | `wifi` | scan, connect, status |

`high.m3d` needs the `math` library. There is no separate one.

Every library command follows the same shape: **`prefix.action|BOX|args`**, and
the first argument names a box that receives the result. Commands that only
draw take no box.

## Graphics

### The surface

```bx
lib load|gfx
high.gfx.fbsize|800|600
high.gfx.fbclear|#101820
high.gfx.rect|20|20|300|200|#4fd6c4
high.gfx.ring|170|120|60|#e9f5f0
high.gfx.line|20|400|780|400|#e76f51
end
```

`fbsize` sets the surface, `fbclear` fills it, `fbinfo` reports it, and `ppm`
writes a crop out as an image.

Colors are written the way a designer writes them: `#rgb`, `#rgba`, `#rrggbb`
and `#rrggbbaa`. **The color is always the last argument, with no mode word in
front of it.** `rect|10|10|400|300|#4fd6c4`, not `rect|...|fill|#4fd6c4` — the
`fill` is read as the color, and the command reports a bad one.

Shapes: `rect` and `frame` take `x|y|w|h`, `circle` and `ring` take
`cx|cy|r`, `line` takes `x1|y1|x2|y2`, and `tri` takes six coordinates.
`triline` is the outline version, `poly` a filled polygon, `plot` draws from the
previous point, and `pixel` sets one.

`gradh` and `gradv` are gradients, `alpha|0-100` sets transparency for whatever
is drawn next.

### Transforms

```bx
lib load|gfx
high.gfx.fbsize|400|300
high.gfx.identity
high.gfx.translate|10|20|on
high.gfx.rect|0|0|50|50|#ffffff
high.gfx.translate|10|20|off
high.gfx.identity
end
```

`identity` resets everything at once, which is the easiest way to be sure a
transform from earlier in the script is not still in effect. `clip`, `scale`
and `rotate` work the same way.

### Colors as numbers

```bx
lib load|gfx
high.gfx.color|a|#4fd6c4
high.gfx.colorhex|b|$a
high.gfx.named|c|teal
high.gfx.colors|d
say a=$a b=$b c=$c
end
```

`color` parses a `#` color into a number, `colorhex` formats a number back, and
`named` looks up a CSS color name. The distinction worth knowing: `color` gives
decimal, `named` gives `0x` hexadecimal, and `rgb|x|y` reads a pixel back as
four space-separated components.

## The element tree

Immediate-mode shapes draw and are gone. The tree is the part meant to outlive
one drawing call: elements have numeric ids, fields and parents.

There are two kinds, and mixing them up is the main thing to get right here.

**`shape` makes drawable geometry.** This is what `render` draws.

```bx
lib load|gfx
high.gfx.fbsize|800|600
high.gfx.shape|1||rect|20|20|300|180|0|0|#4fd6c4
high.gfx.shape|2|1|tri|360|20|460|200|300|200|#e76f51
high.gfx.render
end
```

`shape|BOX|PARENT|KIND|x0|y0|x1|y1|x2|y2|COLOR` takes six coordinates because a
polygon needs them. `rect` uses the first four, `tri` uses all six. The color
may be `c0,c1` for a gradient.

**`new` makes widget elements.** These carry text and geometry but are not
drawn by `render`.

```bx
lib load|gfx
high.gfx.fbsize|800|600
high.gfx.new|10||panel|40|40
high.gfx.set|10|w|320
high.gfx.set|10|h|180
high.gfx.new|11|10|label|56|56
high.gfx.set|11|text|hello
high.gfx.count|n
high.gfx.get|11|text|t
say count=$n text=$t
end
```

Output is `count=2 text=hello`.

`new|id|parent|type|x|y` takes optional `w`, `h`, `theme`, `box` and `text`.
The types are `button`, `text`, `slider`, `box`, `textbox`, `label`, `image`
and `panel`. An id of `0` auto-assigns, and an empty parent means top level.

Ids are **numbers**, not names. `high.gfx.set|card|w|320` parses `card` as
`atoi` and gets 0, so it reports an element that does not exist.

`set|id|field|value` changes `x`, `y`, `w`, `h`, `text`, `theme` or `box`.
There is no `fill` field on a widget; color belongs to the `shape` that
underlaps it.

`count`, `list`, `types` and `clear` inspect and empty the tree. `push` and
`pop` are a stack, for scripts that build a tree by depth.

## Math

`high.math` has 50 commands. `high.math` on its own prints them grouped:

```bx
lib load|math
high.math
end
```

Output:

```text
high.math commands:
  gcd|lcm|fact|fib|isprime|nthprime|ncr|npr|isqrt
  abs|sign|floor|ceil|round|trunc|clamp|lerp|min|max
  sqrt|root|pow|exp|log|log2|log10|hypot
  sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh
  sind|cosd|tand|deg2rad|rad2deg
  totient|divcount|divsum|digitsum|droot|collatz|factor|frac
```

### Arithmetic

```bx
lib load|math
high.math.abs|a|-7
high.math.sign|b|-7
high.math.floor|c|2.7
high.math.ceil|d|2.1
high.math.round|e|2.5
high.math.trunc|f|-2.7
high.math.clamp|g|15|0|10
high.math.lerp|h|0|10|0.25
high.math.min|i|3|9
high.math.max|j|3|9
high.math.hypot|k|3|4
say abs=$a sign=$b floor=$c ceil=$d round=$e trunc=$f
say clamp=$g lerp=$h min=$i max=$j hypot=$k
end
```

Output:

```text
abs=7 sign=-1 floor=2 ceil=3 round=3 trunc=-2
clamp=10 lerp=2.5 min=3 max=9 hypot=5
```

`sign` is `-1`, `0` or `1`. `clamp|value|lo|hi` pins a value to a range, and
`lerp|from|to|t` interpolates, with `t` from 0 to 1.

### Roots and powers

```bx
lib load|math
high.math.sqrt|a|9
high.math.root|b|27|3
high.math.pow|c|2|10
high.math.exp|d|1
high.math.log|e|100
high.math.log2|f|1024
high.math.log10|g|1000
high.math.isqrt|h|17
say sqrt=$a root=$b pow=$c exp=$d log=$e log2=$f log10=$g isqrt=$h
end
```

Output:

```text
sqrt=3 root=3 pow=1024 exp=2.71828 log=4.60517 log2=10 log10=3 isqrt=4
```

Results are printed with `%g`, so a whole number comes back as `3` and not
`3.0`. `log` is natural, `log2` and `log10` are the obvious ones, and `isqrt` is
the integer root rather than the floating one.

### Trigonometry

The trig functions take **radians**. The `d` variants take degrees, which is
usually what you want when a number came from a layout.

```bx
lib load|math
high.math.sin|a|0.5
high.math.cosd|b|60
high.math.tand|c|45
high.math.deg2rad|d|180
high.math.rad2deg|e|$d
high.math.atan2|f|1|1
say sin=$a cosd=$b tand=$c deg2rad=$d rad2deg=$e atan2=$f
end
```

Output:

```text
sin=0.479426 cosd=0.5 tand=1 deg2rad=3.14159 rad2deg=180 atan2=0.785398
```

`sin(0.5)` is 0.479, not 28.6, which is the mistake this family invites. Use
`cosd` for degrees, or convert with `deg2rad` first.

### Number theory

```bx
lib load|math
high.math.gcd|a|12|8
high.math.lcm|b|4|6
high.math.fact|c|10
high.math.fib|d|10
high.math.isprime|e|97
high.math.nthprime|f|10
high.math.ncr|g|5|2
high.math.npr|h|5|2
high.math.totient|i|36
high.math.divcount|j|36
high.math.divsum|k|28
high.math.digitsum|l|987
high.math.collatz|m|27
say gcd=$a lcm=$b fact=$c fib=$d isprime=$e
say nthprime=$f ncr=$g npr=$h totient=$i
say divcount=$j divsum=$k digitsum=$l collatz=$m
end
```

Output:

```text
gcd=4 lcm=12 fact=3628800 fib=55 isprime=1
nthprime=29 ncr=10 npr=20 totient=12
divcount=9 divsum=28 digitsum=24 collatz=111
```

`factor` returns a space-separated list of prime factors, `factor|60` giving
`2 2 3 5`, and `frac` returns a reduced fraction as two numbers, `frac|2.5`
giving `5 2`.

```bx
lib load|math
high.math.factor|a|60
high.math.frac|b|2.5
high.math.droot|c|625
say factors=[$a] fraction=[$b] droot=[$c]
end
```

Output is `factors=[2 2 3 5] fraction=[5 2] droot=[4]`.

**Recommended way to read a list result:** `str split` on a `|` will not work,
because `|` is the argument separator. Use `str find` on a space, then
`str take` and `str right`. See [Power Users](04-power-users.md#indirection-tables-by-name)
for the full idiom.

## 3D math

`high.m3d` covers vectors, quaternions and matrices. Vectors are written
`x,y,z` and matrices as a bracketed list.

```bx
lib load|math
high.m3d.vadd|a|1,0,0|0,1,0
high.m3d.vlen|b|3,4,0
high.m3d.vdot|c|1,2,3|4,5,6
high.m3d.vcross|d|1,0,0|0,1,0
high.m3d.vsub|e|1,2,3|1,1,1
high.m3d.vscale|f|1,2,3|2
high.m3d.vnorm|g|3,4,0
say vadd=$a vlen=$b vdot=$c vcross=$d vsub=$e vscale=$f vnorm=$g
end
```

Output:

```text
vadd=1,1,0 vlen=5 vdot=32 vcross=0,0,1 vsub=0,1,2 vscale=2,4,6 vnorm=0.6,0.8,0
```

Matrices:

```bx
lib load|math
high.m3d.mmul|a|[1,0,0,0,1,0,0,0,1]|[1,1,0,0,1,1,0,0,1]
high.m3d.mrotx|b|1,0,0|90
high.m3d.mroty|c|1,0,0|90
high.m3d.mrotz|d|1,0,0|90
high.m3d.mrot|e|1,0,0|0,1,0|0,0,1
high.m3d.mscale|f|1,2,3|2
high.m3d.mtrans|g|1,2,3|1,0,0|0,1,0|0,0,1|0,0,0
say mmul=[$a]
say mrotx=[$b] mroty=[$c] mrotz=[$d]
say mrot=[$e] mscale=[$f] mtrans=[$g]
end
```

Quaternions:

```bx
lib load|math
high.m3d.qmul|a|1,0,0,0|0,1,0,0
high.m3d.qnorm|b|1,2,3,4
high.m3d.qconj|c|1,2,3,4
high.m3d.qrot|d|1,0,0|0,0,0,1|90
high.m3d.qaxis|e|0,0,0,1
say qmul=[$a] qnorm=[$b] qconj=[$c] qrot=[$d] qaxis=[$e]
end
```

A quaternion is `x,y,z,w`, with `w` last, which is the opposite order to a
vector and is the single most common way to get an `m3d` result wrong.

Also present: `arch` for an arc, `env` for the environment name, `mident` for
an identity matrix, `pid` for a proportional-integral-derivative term, and
`tick` for a fixed-step clock. Run `high.m3d` to see the full list.

## Sound

`high.snd` covers synthesis and playback. It initialises on a machine with a
sound device; where there is none, the commands still parse and produce
silence.

```bx
lib load|snd
high.snd.init
high.snd.rate|r
high.snd.samples|s
high.snd.voices|v
high.snd.freq|f|69
high.snd.midi|m|a4
high.snd.env|0.005|0.05|0.6|0.05
high.snd.note|n|60|0.5
high.snd.tone|t|440|0.5
high.snd.melody|1,2,3|0,2,4|0.25
high.snd.wav|out.wav
say rate=$r samples=$s voices=$v a4=$m
end
```

`freq` takes a MIDI note number and gives hertz, which is `note_freq(69)` =
440 for A4 at the standard tuning. `midi` goes the other way, from a note name
to a number.

`env|attack|decay|sustain|release` sets the envelope once for the session;
the defaults are the ones shown above. `note|number|seconds` plays one note,
`tone|hertz|seconds` plays a frequency, and `melody|notes|offsets|duration`
plays a list.

## Wi-Fi

```bx
lib load|wifi
high.wifi.init
high.wifi.status|s
high.wifi.env|e
say status=$s env=$e
end
```

`connect|ssid` and `connect|ssid|security|pass` join a network, where security
is `0` for open, `1` for WPA and `2` for WPA2. `disconnect` leaves, and `info`
reports the interface.

This one is platform-dependent: it works where the host exposes a wireless
interface and reports a failure where it does not, rather than pretending.

## The environment

`low.env` answers `native`, `web` or `baremetal`. This is how a program adapts
instead of guessing, and it is the reason `lib load` failing on a transpiled
target is survivable:

```bx
low.env|e
say running on $e
end
```

## Finding out what a command wants

The best source is the interpreter itself. `high.gfx` prints all 45 graphics
commands, and `high.math` prints all 50 maths ones, grouped and with the
argument shapes.

```bx
lib load|gfx
high.gfx
end
```

When a library command does nothing at all, the usual cause is one of these,
in order of likelihood:

1. The library is not loaded. `lib list` answers that.
2. The line is after the first `jump`, so it never ran.
3. The command is a dotted family member, and the parser accepts anything under
   `high.` without checking it, so a typo is silent.
4. The command wants a result box in its first argument and you passed the
   value there instead.
