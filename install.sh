#!/bin/bash
# BoxedLANG Installer - Cross-platform with TUI
# Usage: ./install.sh [--user|--system|--windows|--tui] [--auto-detect] [--watch] [--jit]

set -euo pipefail

VERSION="0.2.0"
INSTALL_MODE="user"
AUTO_DETECT=0
ENABLE_WATCH=0
ENABLE_JIT=0
FORCE_TUI=0

# Colors for TUI
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

detect_platform() {
    case "$(uname -s)" in
        Linux*)     PLATFORM="linux";;
        Darwin*)    PLATFORM="macos";;
        CYGWIN*|MINGW*|MSYS*) PLATFORM="windows";;
        *)          PLATFORM="unknown";;
    esac
}

show_banner() {
    echo -e "${CYAN}${BOLD}"
    cat << 'EOF'
╔═══════════════════════════════════════════╗
║        BoxedLANG Installer v0.1.0        ║
║      Cross-platform TUI Installer        ║
╚═══════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

tui_main_menu() {
    while true; do
        clear
        show_banner
        echo -e "${BOLD}Installation Mode:${NC}"
        echo -e "  ${GREEN}1)${NC} User install (~/.local/bin)"
        echo -e "  ${GREEN}2)${NC} System install (/usr/local/bin) ${RED}[requires sudo]${NC}"
        echo -e "  ${GREEN}3)${NC} Portable (./bx in current dir)"
        echo -e "  ${GREEN}4)${NC} Windows installer (MSYS2/MinGW)"
        echo ""
        echo -e "${BOLD}Features:${NC}"
        echo -e "  ${GREEN}5)${NC} Auto-detect box projects: ${YELLOW}$([ $AUTO_DETECT -eq 1 ] && echo "ON" || echo "OFF")${NC}"
        echo -e "  ${GREEN}6)${NC} Watch mode (auto-rebuild): ${YELLOW}$([ $ENABLE_WATCH -eq 1 ] && echo "ON" || echo "OFF")${NC}"
        echo -e "  ${GREEN}7)${NC} JIT runner (AST cache): ${YELLOW}$([ $ENABLE_JIT -eq 1 ] && echo "ON" || echo "OFF")${NC}"
        echo ""
        echo -e "  ${GREEN}I)${NC} ${BOLD}INSTALL NOW${NC}"
        echo -e "  ${RED}Q)${NC} Quit"
        echo ""
        read -p "Choice: " choice
        case "$choice" in
            1) INSTALL_MODE="user";;
            2) INSTALL_MODE="system";;
            3) INSTALL_MODE="portable";;
            4) INSTALL_MODE="windows";;
            5) AUTO_DETECT=$((1 - AUTO_DETECT));;
            6) ENABLE_WATCH=$((1 - ENABLE_WATCH));;
            7) ENABLE_JIT=$((1 - ENABLE_JIT));;
            [Ii]) return 0;;
            [Qq]) exit 0;;
        esac
    done
}

check_dependencies() {
    local missing=0
    for cmd in cc make git; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            echo -e "${RED}Missing: $cmd${NC}"
            missing=1
        fi
    done
    if [ $missing -eq 1 ]; then
        echo -e "${YELLOW}Install dependencies first.${NC}"
        case "$PLATFORM" in
            linux) echo "  sudo apt install build-essential git";;
            macos) echo "  xcode-select --install && brew install git";;
            windows) echo "  pacman -S mingw-w64-x86_64-gcc make git";;
        esac
        return 1
    fi
    return 0
}

build_bx() {
    echo -e "${BLUE}Building BoxedLANG...${NC}"
    make clean
    if ! make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4); then
        echo -e "${RED}Build failed!${NC}"
        return 1
    fi
    echo -e "${GREEN}Build successful!${NC}"
    return 0
}

install_user() {
    mkdir -p "$HOME/.local/bin"
    cp bx "$HOME/.local/bin/"
    echo -e "${GREEN}Installed to ~/.local/bin/bx${NC}"
    if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
        echo -e "${YELLOW}Add to PATH: export PATH=\"\$HOME/.local/bin:\$PATH\"${NC}"
    fi
}

install_system() {
    sudo cp bx /usr/local/bin/
    echo -e "${GREEN}Installed to /usr/local/bin/bx${NC}"
}

install_portable() {
    echo -e "${GREEN}Portable binary: ./bx${NC}"
}

install_windows() {
    if [ "$PLATFORM" != "windows" ]; then
        echo -e "${RED}Windows install must run on Windows (MSYS2/MinGW)${NC}"
        return 1
    fi
    mkdir -p "/c/BoxedLANG/bin"
    cp bx.exe "/c/BoxedLANG/bin/bx.exe" 2>/dev/null || cp bx "/c/BoxedLANG/bin/bx.exe"
    echo -e "${GREEN}Installed to C:\\BoxedLANG\\bin\\bx.exe${NC}"
    echo -e "${YELLOW}Add C:\\BoxedLANG\\bin to your PATH${NC}"
}

setup_auto_detect() {
    if [ $AUTO_DETECT -eq 1 ]; then
        local shell_rc=""
        case "$(basename "$SHELL")" in
            bash) shell_rc="$HOME/.bashrc";;
            zsh) shell_rc="$HOME/.zshrc";;
            fish) shell_rc="$HOME/.config/fish/config.fish";;
        esac
        if [ -n "$shell_rc" ]; then
            cat >> "$shell_rc" << 'EOF'

# BoxedLANG auto-detect
bx_auto() {
    if [ -f ".boxinit" ] || [ -f "bx.lock" ] || [ -d ".bx" ]; then
        echo "BoxedLANG project detected - starting watch mode..."
        make watch &
    fi
}
cd() { builtin cd "$@" && bx_auto; }
EOF
            echo -e "${GREEN}Auto-detect added to $shell_rc${NC}"
        fi
    fi
}

setup_watch_service() {
    if [ $ENABLE_WATCH -eq 1 ]; then
        if command -v entr >/dev/null 2>&1; then
            echo -e "${GREEN}Watch mode: entr available${NC}"
        elif command -v inotifywait >/dev/null 2>&1; then
            echo -e "${GREEN}Watch mode: inotifywait available${NC}"
        else
            echo -e "${YELLOW}Watch mode: install entr or inotify-tools for best experience${NC}"
        fi
    fi
}

setup_jit() {
    if [ $ENABLE_JIT -eq 1 ]; then
        echo -e "${BLUE}Building JIT runner...${NC}"
        make .build/bx_jit
        echo -e "${GREEN}JIT runner ready: .build/bx_jit${NC}"
    fi
}

setup_editors() {
    local did=0
    # nano
    if command -v nano >/dev/null 2>&1; then
        local nanocfg="$HOME/.config/nano"
        mkdir -p "$nanocfg"
        cp editors/nano/boxedlang.nanorc "$nanocfg/boxedlang.nanorc"
        mkdir -p "$nanocfg/extra"
        mkdir -p "$HOME/.local/share/nano" 2>/dev/null || true
        cp editors/nano/boxedlang.nanorc "$HOME/.local/share/nano/boxedlang.nanorc" 2>/dev/null || true
        grep -q 'boxedlang.nanorc' "$HOME/.nanorc" 2>/dev/null || echo 'include "~/.config/nano/boxedlang.nanorc"' >> "$HOME/.nanorc"
        echo -e "${GREEN}nano: BoxedLANG syntax installed${NC}"
        did=1
    fi
    # micro
    if command -v micro >/dev/null 2>&1; then
        mkdir -p "$HOME/.config/micro/syntax"
        cp editors/micro/boxedlang.yaml "$HOME/.config/micro/syntax/boxedlang.yaml"
        echo -e "${GREEN}micro: BoxedLANG syntax installed${NC}"
        did=1
    fi
    # VSCode / Code OSS
    local codebin=""
    if command -v code >/dev/null 2>&1; then codebin=$(command -v code)
    elif command -v code-oss >/dev/null 2>&1; then codebin=$(command -v code-oss); fi
    if [ -n "$codebin" ] && [ -d "editors/vscode" ]; then
        local extdir
        extdir=$(grep -oP 'extensions=dir\K.*' "$codebin" 2>/dev/null | cut -d: -f1)
        mkdir -p editors/vscode/node_modules 2>/dev/null || true
        cp -r editors/vscode "$HOME/.local/share/${codebin##*/}-boxedlang" 2>/dev/null || true
        echo -e "${GREEN}vscode: syntax highlighting files at editors/vscode/ (load via Extensions > Install from VSIX/Extensions folder)${NC}"
        did=1
    fi
    # language server
    mkdir -p "$HOME/.local/bin"
    cp lsp/boxedlang_lsp.py "$HOME/.local/bin/boxedlang-lsp"
    echo -e "${GREEN}LSP: installed to ~/.local/bin/boxedlang-lsp${NC}"
    did=1
    [ $did -eq 1 ] && echo -e "${GREEN}Editor tooling installed.${NC}"
}

create_boxinit() {
    if [ $AUTO_DETECT -eq 1 ]; then
        touch .boxinit
        cat > .boxinit << 'EOF'
# BoxedLANG Project Marker
# This file enables auto-detection of BoxedLANG projects
# Features enabled:
# - Auto AST caching
# - Watch mode (make watch)
# - JIT runner (.build/bx_jit)
EOF
        echo -e "${GREEN}Created .boxinit project marker${NC}"
    fi
}

main() {
    detect_platform
    echo -e "${BLUE}Platform: $PLATFORM${NC}"

    # Parse args
    for arg in "$@"; do
        case "$arg" in
            --user) INSTALL_MODE="user";;
            --system) INSTALL_MODE="system";;
            --portable) INSTALL_MODE="portable";;
            --windows) INSTALL_MODE="windows";;
            --tui) FORCE_TUI=1;;
            --auto-detect) AUTO_DETECT=1;;
            --watch) ENABLE_WATCH=1;;
            --jit) ENABLE_JIT=1;;
            --help)
                echo "Usage: $0 [--user|--system|--portable|--windows] [--tui] [--auto-detect] [--watch] [--jit]"
                exit 0;;
        esac
    done

    if [ $FORCE_TUI -eq 1 ] || [ -t 0 ]; then
        tui_main_menu
    fi

    check_dependencies || exit 1
    build_bx || exit 1

    case "$INSTALL_MODE" in
        user) install_user;;
        system) install_system;;
        portable) install_portable;;
        windows) install_windows;;
    esac

    setup_auto_detect
    setup_watch_service
    setup_jit
    setup_editors
    create_boxinit

    echo -e "\n${GREEN}${BOLD}Installation complete!${NC}"
    echo -e "Run ${CYAN}bx --help${NC} to get started."
}

main "$@"