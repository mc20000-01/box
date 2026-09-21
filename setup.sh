#!/usr/bin/env sh
set -eu

# BoxedLANG setup helper.
# Usage:
#   ./setup.sh                 build only, install deps if missing promptlessly where possible
#   ./setup.sh --install        build and install bx to PREFIX/bin, default /usr/local/bin
#   ./setup.sh --user           install to ~/.local/bin
#   ./setup.sh --cross          try to install common cross compilers too
#   PREFIX=/opt/boxed ./setup.sh --install

INSTALL=0
USER_INSTALL=0
WITH_CROSS=0
PREFIX_VALUE="${PREFIX:-/usr/local}"

for arg in "$@"; do
    case "$arg" in
        --install) INSTALL=1 ;;
        --user) INSTALL=1; USER_INSTALL=1; PREFIX_VALUE="$HOME/.local" ;;
        --cross) WITH_CROSS=1 ;;
        --help|-h)
            sed -n '4,10p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

need_cmd() { command -v "$1" >/dev/null 2>&1; }

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    if need_cmd sudo; then SUDO=sudo; fi
fi

install_deps() {
    if need_cmd apt-get; then
        $SUDO apt-get update
        $SUDO apt-get install -y build-essential make binutils
        if [ "$WITH_CROSS" -eq 1 ]; then
            $SUDO apt-get install -y \
                gcc-x86-64-linux-gnu gcc-i686-linux-gnu \
                gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi gcc-arm-linux-gnueabihf \
                gcc-riscv64-linux-gnu gcc-mips-linux-gnu gcc-mipsel-linux-gnu \
                gcc-powerpc-linux-gnu gcc-powerpc64-linux-gnu gcc-powerpc64le-linux-gnu \
                gcc-s390x-linux-gnu gcc-sparc64-linux-gnu || true
        fi
    elif need_cmd dnf; then
        $SUDO dnf install -y gcc make binutils
        if [ "$WITH_CROSS" -eq 1 ]; then
            echo "dnf cross compiler package names vary. Install target gcc packages manually if needed."
        fi
    elif need_cmd pacman; then
        $SUDO pacman -Sy --needed --noconfirm base-devel binutils
        if [ "$WITH_CROSS" -eq 1 ]; then
            echo "pacman cross compilers are usually AUR or target-specific. Install them manually if needed."
        fi
    elif need_cmd zypper; then
        $SUDO zypper --non-interactive install gcc make binutils
    elif need_cmd apk; then
        $SUDO apk add build-base binutils
    else
        echo "No supported package manager found. Install a C compiler, make, and binutils manually." >&2
    fi
}

if ! need_cmd cc || ! need_cmd make || ! need_cmd objcopy; then
    echo "Installing required native build tools..."
    install_deps
fi

echo "Building bx..."
make

echo "Running smoke test..."
make smoke

if [ "$INSTALL" -eq 1 ]; then
    echo "Installing bx to $PREFIX_VALUE/bin..."
    if [ "$USER_INSTALL" -eq 1 ] || [ -w "$PREFIX_VALUE" ] || { [ -d "$PREFIX_VALUE/bin" ] && [ -w "$PREFIX_VALUE/bin" ]; }; then
        make install PREFIX="$PREFIX_VALUE"
    elif [ -n "$SUDO" ]; then
        $SUDO make install PREFIX="$PREFIX_VALUE"
    else
        echo "Cannot write to $PREFIX_VALUE/bin and sudo is not available." >&2
        echo "Try: ./setup.sh --user" >&2
        exit 1
    fi
    echo "Installed: $PREFIX_VALUE/bin/bx"
    if [ "$USER_INSTALL" -eq 1 ]; then
        case ":$PATH:" in
            *":$HOME/.local/bin:"*) ;;
            *) echo "Note: add ~/.local/bin to PATH if bx is not found." ;;
        esac
    fi
else
    echo "Done. Run ./bx, or install with: ./setup.sh --install"
fi
