ENTRY(_start)

MEMORY {
    /* We are using RAM for everything now */
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 256K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    /* Put vectors at the start of RAM */
    .vectors : {
        KEEP(*(.vectors))
    } > RAM

    /* Put code in RAM */
    .text : {
        *(.text*)
    } > RAM
}
