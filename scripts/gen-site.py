#!/usr/bin/env python3
"""Build the bx.sdisk.us site into site/.

Everything is generated from sources that already exist in the repo, so the
pages cannot drift from the code:

  docs.html      box-docs-redone.md   (the user's manual)
  packages.html  boxpkg/registry.txt  (with deps read from the package files)

index.html and install.html have their copy here because it is prose about the
project rather than a mirror of a source file. Only the stdlib is used, so
there is nothing to install before building the site.
"""

import html
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "site")
VERSION = open(os.path.join(ROOT, "VERSION")).read().strip()
REPO = "https://github.com/mc20000-01/boxed"

PAGES = {
    "index.html": "Home",
    "docs.html": "Docs",
    "packages.html": "Packages",
    "install.html": "Install",
}


def esc(s):
    return html.escape(s, quote=False)


def layout(page, title, body, desc):
    nav = "\n".join(
        '        <a class="nav%s" href="%s">%s</a>'
        % (" active" if href == page else "", href, label)
        for href, label in PAGES.items()
    )
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)}</title>
<meta name="description" content="{esc(desc)}">
<link rel="icon" href="favicon.svg" type="image/svg+xml">
<link rel="stylesheet" href="style.css">
</head>
<body>
<nav class="top">
    <div class="inner">
        <a class="brand" href="index.html"><span class="sq">BX</span>BoxedLANG</a>
        {nav}
        <span class="spacer"></span>
        <a class="nav" href="{REPO}" rel="noopener">GitHub</a>
    </div>
</nav>
<main class="wrap{'' if page == 'docs.html' else ''}">
{body}
</main>
<footer>
    BoxedLANG v{VERSION} &middot; <a href="{REPO}">source</a> &middot;
    <a href="https://github.com/mc20000-01/BoxPkg">BoxPkg registry</a>
</footer>
</body>
</html>
"""


# ---------------------------------------------------------------- docs

def inline(s):
    """Markdown spans that appear inside a paragraph or heading."""
    # Code spans are pulled out first and put back at the end. Converting them
    # in place would let the emphasis rules eat characters that belong to a
    # command name, turning `high.gfx.*` into italics.
    spans = []

    def stash(m):
        spans.append("<code>%s</code>" % esc(m.group(1)))
        return "\x00%d\x00" % (len(spans) - 1)

    s = re.sub(r"`([^`]+)`", stash, s)
    s = esc(s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"(?<!\*)\*([^*\n]+)\*(?!\*)", r"<em>\1</em>", s)
    s = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', s)
    return re.sub(r"\x00(\d+)\x00", lambda m: spans[int(m.group(1))], s)


def slug(t):
    return re.sub(r"[^a-z0-9]+", "-", t.lower()).strip("-")


def md_to_html(md):
    out = []
    i = 0
    lines = md.split("\n")
    n = len(lines)
    in_code = False
    code_lang = ""
    code_buf = []
    list_open = None  # "ul" | "ol"

    def close_list():
        nonlocal list_open
        if list_open:
            out.append("</%s>" % list_open)
            list_open = None

    while i < n:
        line = lines[i]
        stripped = line.strip()

        # fenced code
        if stripped.startswith("```"):
            if in_code:
                cls = ' class="language-%s"' % esc(code_lang) if code_lang else ""
                out.append("<pre><code%s>%s</code></pre>" % (cls, esc("\n".join(code_buf))))
                in_code = False
                code_buf = []
            else:
                close_list()
                in_code = True
                code_lang = stripped[3:].strip()
            i += 1
            continue
        if in_code:
            code_buf.append(line)
            i += 1
            continue

        if not stripped:
            close_list()
            i += 1
            continue

        # heading
        m = re.match(r"^(#{1,6})\s+(.*)$", stripped)
        if m:
            close_list()
            lvl = len(m.group(1))
            text = m.group(2).strip()
            anchor = slug(text)
            out.append('<h%d id="%s">%s</h%d>' % (lvl, anchor, inline(text), lvl))
            i += 1
            continue

        if re.match(r"^(---|\*\*\*|___)$", stripped):
            close_list()
            out.append("<hr>")
            i += 1
            continue

        # lists
        m = re.match(r"^[-*]\s+(.*)$", stripped)
        if m:
            if list_open != "ul":
                close_list()
                out.append("<ul>")
                list_open = "ul"
            out.append("<li>%s</li>" % inline(m.group(1)))
            i += 1
            continue
        m = re.match(r"^\d+\.\s+(.*)$", stripped)
        if m:
            if list_open != "ol":
                close_list()
                out.append("<ol>")
                list_open = "ol"
            out.append("<li>%s</li>" % inline(m.group(1)))
            i += 1
            continue

        close_list()
        # paragraph: gather until a blank line or a block starter
        para = [stripped]
        j = i + 1
        while j < n:
            s2 = lines[j].strip()
            if (
                not s2
                or s2.startswith("```")
                or re.match(r"^#{1,6}\s+", s2)
                or re.match(r"^[-*]\s+", s2)
                or re.match(r"^\d+\.\s+", s2)
            ):
                break
            para.append(s2)
            j += 1
        out.append("<p>%s</p>" % inline(" ".join(para)))
        i = j

    close_list()
    if in_code:
        out.append("<pre><code>%s</code></pre>" % esc("\n".join(code_buf)))
    return "\n".join(out)


def toc_from(md):
    """Chapter links for the top of the docs page."""
    items = []
    for m in re.finditer(r"^(#{2,3})\s+(.*)$", md, re.M):
        lvl = len(m.group(1))
        text = m.group(2).strip()
        if lvl == 3:
            continue
        items.append((lvl, text, slug(text)))
    return items


def build_docs():
    src = os.path.join(ROOT, "box-docs-redone.md")
    md = open(src, encoding="utf-8").read()
    body = md_to_html(md)
    items = toc_from(md)
    toc = ["<details class=\"toc\" open>", "<summary>Contents</summary>", "<ol>"]
    for lvl, text, anchor in items:
        toc.append('<li><a href="#%s">%s</a></li>' % (anchor, inline(text)))
    toc.append("</ol>")
    toc.append("</details>")
    intro = (
        "<div class=\"note\"><strong>This is the manual, rendered.</strong> "
        "It is generated from <code>box-docs-redone.md</code> in the repository, "
        "so the site and the source never disagree.</div>"
    )
    body = intro + "\n" + "\n".join(toc) + "\n" + body
    return layout(
        "docs.html",
        "Docs - BoxedLANG",
        body,
        "The complete BoxedLANG user's manual: boxes, jumps, loops, packages, "
        "libraries, and compile targets.",
    )


# ---------------------------------------------------------------- packages

def read_registry():
    path = os.path.join(ROOT, "boxpkg", "registry.txt")
    pkgs = []
    if not os.path.exists(path):
        return pkgs
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        f = line.split("|")
        if len(f) < 5:
            continue
        pkgs.append(
            {"name": f[0], "url": f[1], "version": f[2], "desc": f[3], "author": f[4]}
        )
    return pkgs


def read_deps(name):
    path = os.path.join(ROOT, "boxpkg", "packages", "%s.bx" % name)
    if not os.path.exists(path):
        return "none"
    try:
        head = open(path, encoding="utf-8").read(4096)
    except OSError:
        return "none"
    m = re.search(r"^//\s*deps:\s*(.+)$", head, re.M)
    if not m:
        return "none"
    return m.group(1).strip()


def build_packages():
    pkgs = read_registry()
    out = [
        "<h1>Packages</h1>",
        "<p class=\"lead\">Libraries published to the "
        "<a href=\"https://github.com/mc20000-01/BoxPkg\">BoxPkg registry</a>. "
        "<code>umload require|&lt;name&gt;</code> installs a package if it is not "
        "cached, resolves its dependencies, and runs it.</p>",
        "<div class=\"note\"><strong>Generated from the registry.</strong> "
        "This page reads <code>boxpkg/registry.txt</code>, so it always matches "
        "what <code>umload</code> will actually find.</div>",
    ]
    if not pkgs:
        out.append("<p>No packages are published yet.</p>")
    for p in sorted(pkgs, key=lambda x: x["name"]):
        deps = read_deps(p["name"])
        dep_html = (
            ""
            if deps in ("none", "")
            else ' &middot; deps: <code>%s</code>' % esc(deps)
        )
        out.append(
            '<div class="pkg">\n'
            '  <div class="head"><span class="name">%s</span>'
            '<span class="ver">v%s</span></div>\n'
            "  <p>%s</p>\n"
            '  <div class="meta">by %s%s</div>\n'
            '  <div class="use"><pre><code>umload require|%s</code></pre></div>\n'
            "</div>"
            % (
                esc(p["name"]),
                esc(p["version"]),
                inline(p["desc"]),
                esc(p["author"]),
                dep_html,
                esc(p["name"]),
            )
        )
    out.append(
        "<h2>Publishing your own</h2>"
        "<pre><code>umload publish|&lt;name&gt;|&lt;version&gt;</code></pre>"
        "<p>The package needs <code>// name:</code>, <code>// version:</code>, "
        "<code>// author:</code>, <code>// description:</code> and "
        "<code>// deps:</code> comments at the top of the file. Publish copies it "
        "into the registry checkout, adds the registry line, commits and pushes.</p>"
    )
    return layout(
        "packages.html",
        "Packages - BoxedLANG",
        "\n".join(out),
        "Published BoxedLANG packages: fuzzy matching, math helpers, 3D vectors, "
        "sound, string transforms.",
    )


# ---------------------------------------------------------------- index

def build_index():
    body = f"""
<div class="hero">
    <h1><span class="sq">BX</span>BoxedLANG</h1>
    <p class="lead">A small, fast, command-oriented language built around one
    idea: everything is a <strong>box</strong>. Write a box, read a box, change
    it. Run the same source as a script, or compile it to a bare-metal binary
    for 19 architectures.</p>
    <p>
        <span class="tag">v{VERSION}</span>
        <span class="tag">C runtime</span>
        <span class="tag">19 compile targets</span>
        <span class="tag">4 libraries</span>
        <span class="tag">packages</span>
    </p>
    <div class="btnrow">
        <a class="btn primary" href="install.html">Get BoxedLANG</a>
        <a class="btn" href="docs.html">Read the manual</a>
        <a class="btn" href="packages.html">Browse packages</a>
    </div>
</div>

<h2>Hello, world</h2>
<p>Three commands: make a box, print it, stop.</p>
<pre><code>box name|BX
say Hello from $name!
end</code></pre>
<pre><code>$ ./bx run hello.bx
Hello from BX!</code></pre>

<h2>What makes it different</h2>
<div class="cards">
    <div class="card">
        <h3>One data type</h3>
        <p>A box holds text. Numbers are text that math knows how to read, so
        there is nothing to declare and nothing to convert.</p>
    </div>
    <div class="card">
        <h3>Jumps, not just branches</h3>
        <p><span class="cmd">premark</span> and <span class="cmd">jumpif</span>
        give loops and menus. Mark names can be built at runtime.</p>
    </div>
    <div class="card">
        <h3>Same source, many forms</h3>
        <p><span class="cmd">run</span>, <span class="cmd">transpile</span>,
        <span class="cmd">compile</span>, <span class="cmd">asm</span>,
        <span class="cmd">raw</span> - one program, five outputs.</p>
    </div>
    <div class="card">
        <h3>Compiles to bare metal</h3>
        <p>Boot a BoxedLANG program with no OS underneath, through the
        Multiboot1 path or as a UEFI or firmware payload.</p>
    </div>
    <div class="card">
        <h3>Real libraries</h3>
        <p><span class="cmd">gfx</span> rasteriser,
        <span class="cmd">snd</span> synthesiser with WAV export,
        <span class="cmd">math</span> and 3D, <span class="cmd">wifi</span>.</p>
    </div>
    <div class="card">
        <h3>Persistent environments</h3>
        <p><span class="cmd">bxe</span> keeps a named box workspace alive
        across calls, so a program can be called like a function.</p>
    </div>
</div>

<h2>A longer look</h2>
<p>Loops are a mark and a conditional jump, with no nesting surprises:</p>
<pre><code>box i|0
premark loop
say line $i
math i|$i|1|+
jumpif $i|<|3|loop|m
end</code></pre>
<p>Values flow between a program, the packages it requires, and the boxed
environments it calls. A package is an ordinary BoxedLANG file that reads input
boxes and writes output boxes:</p>
<pre><code>box st_op|slugify
box st_in|The Quick Brown Fox
umload require|strutil|
say $st_out</code></pre>

<h2>Start here</h2>
<div class="cards">
    <div class="card">
        <h3><a href="install.html">Install</a></h3>
        <p>Linux, macOS and Windows, or build from source with
        <code>make</code>.</p>
    </div>
    <div class="card">
        <h3><a href="docs.html">The manual</a></h3>
        <p>Boxes, jumps, loops, files, packages, libraries, and compile
        targets.</p>
    </div>
    <div class="card">
        <h3><a href="packages.html">Packages</a></h3>
        <p>What is published, and how to add something of your own.</p>
    </div>
</div>
"""
    return layout(
        "index.html",
        "BoxedLANG - a language built out of boxes",
        body,
        "BoxedLANG is a small, fast command-oriented language built around "
        "boxes, with packages, libraries and compile targets for 19 "
        "architectures.",
    )


# ---------------------------------------------------------------- install

def build_install():
    body = """
<h1>Install</h1>
<p class="lead">BoxedLANG is a single C program. Build it from source, or use
the installer script for your platform.</p>

<h2>From source</h2>
<p>Requires a C99 compiler and GNU make. No other dependencies.</p>
<pre><code>git clone https://github.com/mc20000-01/boxed.git
cd boxed
make</code></pre>
<p>That produces <code>./bx</code> in the repository root. Check it:</p>
<pre><code>$ ./bx version
BoxedLANG version __VERSION__
Up to date.</code></pre>
<p>To install the runner and the package registry system-wide:</p>
<pre><code>sudo make install          # /usr/local/bin/bx
make install-user          # ~/.local/bin/bx, no root needed</code></pre>

<h2>Installer script</h2>
<p>On Linux and macOS:</p>
<pre><code>curl -fsSL https://raw.githubusercontent.com/mc20000-01/boxed/main/install.sh | bash</code></pre>
<p>On Windows, PowerShell:</p>
<pre><code>irm https://raw.githubusercontent.com/mc20000-01/boxed/main/install.ps1 | iex</code></pre>
<p>The script has a TUI. Run it with no arguments for the menu, or use the
flags directly:</p>
<pre><code>./install.sh --user        # install into ~/.local
./install.sh --system      # install into /usr/local
./install.sh --auto-detect # pick a mode and install
./install.sh --jit         # build with the JIT enabled</code></pre>

<h2>Cross compiling</h2>
<p>A BoxedLANG program compiles to a freestanding binary. Target toolchains are
named rather than configured:</p>
<pre><code>./bx compile hello.bx -o hello --target x86_64
./bx compile hello.bx -o hello --target riscv64
./bx compile hello.bx -o hello --target aarch64
./bx targets</code></pre>
<p>The runner never guesses a sysroot. If the named toolchain is not on your
<code>PATH</code>, install it first. Nothing is linked against libc, so the
result boots on bare metal.</p>

<h2>Running without an operating system</h2>
<pre><code>./bx compile os/os.bx -o boxed-os
qemu-system-x86_64 -kernel boxed-os</code></pre>
<p>The repository also ships a Multiboot1 kernel that runs a BoxedLANG program
with graphics output. <code>make run-kernel</code> builds and boots it.</p>

<h2>Test suite</h2>
<p>Every part of the project is covered by BoxedLANG test scripts:</p>
<pre><code>make smoke           # run, transpile, compile, asm, raw
make features         # language core
make gfx-tests        # rasteriser
make snd-tests        # synthesiser and WAV export
make math-tests       # math and 3D
make friendly-tests   # package wrappers
make bxe-tests        # environment marshalling
make packagetests     # package manager</code></pre>
"""
    body = body.replace("__VERSION__", VERSION)
    return layout(
        "install.html",
        "Install - BoxedLANG",
        body,
        "Install BoxedLANG from source, with the installer script, or cross "
        "compile it for 19 architectures.",
    )


# ---------------------------------------------------------------- favicon

FAVICON = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <rect width="64" height="64" rx="12" fill="#0b0f14"/>
  <rect x="14" y="14" width="36" height="36" rx="7" fill="none"
        stroke="#4fd6c4" stroke-width="5"/>
  <text x="32" y="41" font-family="monospace" font-size="18" font-weight="bold"
        fill="#4fd6c4" text-anchor="middle">BX</text>
</svg>
"""


def main():
    os.makedirs(OUT, exist_ok=True)
    files = {
        "index.html": build_index(),
        "docs.html": build_docs(),
        "packages.html": build_packages(),
        "install.html": build_install(),
        "favicon.svg": FAVICON,
    }
    for name, content in files.items():
        with open(os.path.join(OUT, name), "w", encoding="utf-8") as fh:
            fh.write(content)
        print("  site/%s (%d bytes)" % (name, len(content)))
    print("site built: %d pages" % len(files))


if __name__ == "__main__":
    sys.exit(main())