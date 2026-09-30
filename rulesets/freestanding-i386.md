// name: freestanding-i386
// version: 1.0.0
// author: BoxedLANG
// description: Template for bare-metal i386 with an explicit ld step
// cc: x86_64-linux-gnu-gcc
// cflags: -m32 -std=c99 -ffreestanding -nostdlib -nostartfiles -fno-pie -fno-builtin -fno-stack-protector -O2 -Wall -I.
// ld: ld
// ldflags: -m elf_i386 -T src/kernel.ld -nostdlib -z max-page-size=0x1000
// objcopy: objcopy
// suffix: elf
//
//   bx compile program.bx --ruleset freestanding-i386 -o out.elf
//
// This is a TEMPLATE. bx compiles one .bx file to C and links it alone, so
// the result only links if your program supplies _start, its own memcpy and
// the rest of the no-libc surface. A kernel also needs an assembly stub and
// its other C files, which is why `make kernel` still drives the linker
// itself. Copy this file, edit the fields, and drop it in ./rulesets.
