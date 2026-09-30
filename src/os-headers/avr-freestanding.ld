/* AVR linker script for Arduino (ATmega328P) */
MEMORY
{
    flash (rx)  : ORIGIN = 0x0000, LENGTH = 32768
    data  (rw!x): ORIGIN = 0x800100, LENGTH = 2048
    eeprom (rw!x): ORIGIN = 0x810000, LENGTH = 1024
}

SECTIONS
{
    .text :
    {
        *(.vectors)
        *(.text*)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
    } > flash

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(2);
        __data_end = .;
    } > data

    .bss :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(2);
        __bss_end = .;
    } > data

    .eeprom :
    {
        *(.eeprom*)
    } > eeprom
}
