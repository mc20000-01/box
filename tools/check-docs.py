#!/usr/bin/env python3
"""Run every ```box fence in docs/ and report the ones that do not work.

The point is not that a snippet compiles. It is that a snippet that
documents a wrong thing fails, because a snippet that quietly does the
wrong thing is worse than no snippet.

A fence may carry flags after the language tag:

    ```box            must run clean: no stderr, no FAIL
    ```box no-stderr  must run clean, but stderr is expected (error demos)
    ```box skip       not executed, for fragments with no single entry point

Usage: python3 tools/check-docs.py [file.md ...]
"""
import re
import subprocess
import sys
import os
import glob
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BX = os.path.join(ROOT, "bx")

FENCE = re.compile(r"^```(?:box|bx)(?P<flags>[^\n]*)\n(?P<body>.*?)^```", re.S | re.M)


def run(body):
    with tempfile.NamedTemporaryFile("w", suffix=".bx", delete=False,
                                     dir=os.path.join(ROOT, ".build")) as f:
        # Some snippets need a framebuffer and some need a math library.
        f.write("lib load|gfx\nlib load|math\n")
        f.write(body)
        if not re.search(r"^\s*end\s*$", body, re.M):
            f.write("\nend\n")
        path = f.name
    try:
        p = subprocess.run([BX, "run", path], capture_output=True,
                           text=True, timeout=30, cwd=ROOT)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return 99, "", "TIMEOUT"
    finally:
        os.unlink(path)


def main():
    files = sys.argv[1:] or sorted(glob.glob(os.path.join(ROOT, "docs", "*.md")))
    if not os.path.exists(BX):
        print("bx not built: run make first", file=sys.stderr)
        return 2

    total = skipped = bad = 0
    for path in files:
        src = open(path, encoding="utf-8").read()
        name = os.path.relpath(path, ROOT)
        for i, m in enumerate(FENCE.finditer(src), 1):
            flags = m.group("flags").strip()
            if "skip" in flags:
                skipped += 1
                continue
            total += 1
            line = src[:m.start()].count("\n") + 1
            rc, out, err = run(m.group("body"))
            problems = []
            if rc != 0:
                problems.append("exit %d" % rc)
            if "FAIL" in out:
                problems.append("printed FAIL")
            if err.strip() and "no-stderr" not in flags:
                first = err.strip().splitlines()[0]
                problems.append("stderr: " + first[:70])
            if problems:
                bad += 1
                print("%s:%d  %s" % (name, line, "; ".join(problems)))
                for l in out.strip().splitlines()[-3:]:
                    print("      out| " + l[:78])

    print("\n%d examples run, %d bad, %d skipped" % (total, bad, skipped))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
