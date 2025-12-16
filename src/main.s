.syntax unified
.cpu cortex-m0plus
.thumb
.thumb_func

.global main

/* ===== RESET REGISTERS ===== */
.equ RESETS_BASE,     0x4000C000
.equ RESETS_CLR,      0x3000

/* Reset bits */
.equ RESET_IO_BANK0,  (1 << 5)
.equ RESET_PADS,      (1 << 8)
.equ RESET_SIO,       (1 << 3)

/* ===== GPIO ===== */
.equ GPIO25_CTRL,     0x400140CC

/* ===== SIO ===== */
.equ SIO_BASE,        0xD0000000
.equ GPIO_OE_SET,     0x20
.equ GPIO_OUT_SET,    0x14
.equ GPIO_OUT_CLR,    0x18

.equ GPIO25_BIT,      (1 << 25)

.section .text
.thumb_func
main:
    /* --- Release peripheral resets --- */
    ldr r0, =RESETS_BASE
    ldr r1, =(RESET_IO_BANK0 | RESET_PADS | RESET_SIO)
    str r1, [r0, #RESETS_CLR]

    /* --- Configure GPIO25 function (SIO) --- */
    ldr r0, =GPIO25_CTRL
    movs r1, #5
    str r1, [r0]

    /* --- Enable GPIO25 output --- */
    ldr r0, =SIO_BASE
    ldr r1, =GPIO25_BIT
    str r1, [r0, #GPIO_OE_SET]

blink_loop:
    /* LED ON */
    str r1, [r0, #GPIO_OUT_SET]
    bl delay

    /* LED OFF */
    str r1, [r0, #GPIO_OUT_CLR]
    bl delay

    b blink_loop

.thumb_func
delay:
    ldr r2, =0x00100000
delay_loop:
    subs r2, r2, #1
    bne delay_loop
    bx lr
