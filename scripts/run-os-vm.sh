#!/usr/bin/env sh
set -eu

OUT=${OUT:-.build/boxed-os}
KERNEL=${KERNEL:-}
INITRD=${INITRD:-}

mkdir -p .build
make bx
./bx compile os/os.bx -o "$OUT"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 is not installed." >&2
    echo "Install it first, for example:" >&2
    echo "  sudo apt install qemu-system-x86" >&2
    echo "  sudo dnf install qemu-system-x86" >&2
    echo "  sudo pacman -S qemu-system-x86" >&2
    exit 1
fi

cat >&2 <<MSG
$OUT is a normal Linux ELF program, not a bootable kernel image.
To run it in a full VM, boot a Linux guest and run/copy $OUT inside it.

If you set KERNEL and INITRD, this script will boot that Linux kernel/initrd
in QEMU serial mode. It will not automatically inject boxed-os yet.
MSG

if [ -n "$KERNEL" ] && [ -n "$INITRD" ]; then
    exec qemu-system-x86_64 \
        -m 256M \
        -serial mon:stdio \
        -display none \
        -kernel "$KERNEL" \
        -initrd "$INITRD" \
        -append "console=ttyS0"
fi

exit 1
