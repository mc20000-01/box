// name: kernel-i386
// version: 1.0.0
// author: BoxedLANG
// description: Freestanding Multiboot1 kernel, 32-bit, custom linker script
// cc: x86_64-linux-gnu-gcc
// cflags: -m32 -std=c99 -ffreestanding -nostdlib -nostartfiles -fno-pie -O2 -Wall -Wextra -I. -fno-stack-protector -fno-builtin
// ld: ld
// ldflags: -m elf_i386 -T src/kernel.ld -nostdlib -z max-page-size=0x1000
// objcopy: objcopy
// suffix: elf
//
//   bx compile program.bx --ruleset kernel-i386 -o kernel.elf
//
// The header lines above are the ruleset. Everything from here down is a
// comment, so the file still reads as documentation.
