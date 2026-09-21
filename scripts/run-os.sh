#!/usr/bin/env sh
set -eu

BX_PROGRAM=${BX_PROGRAM:-os/os.bx}
OUT=${OUT:-.build/boxed-os}

mkdir -p .build
make bx
./bx compile "$BX_PROGRAM" -o "$OUT"
echo "Built $OUT"
echo "Login: admin / 1234"
exec "$OUT"
