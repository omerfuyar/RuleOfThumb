ENTRY(_start)

MEMORY {
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 256K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .vectors : {
        KEEP(*(.vectors))
    } > RAM

    .text : {
        *(.text*)
    } > RAM
}
