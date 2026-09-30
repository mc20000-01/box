# Power User Tricks

## ADVANCED TRICKS FOR POWER USERS

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

The full family list is in [Libraries](07-libraries.md), and the window system built on top of it is in [The UI Layer](08-ui.md).

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
