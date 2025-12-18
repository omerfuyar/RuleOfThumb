.syntax unified
.cpu cortex-m0plus
.thumb
.thumb_func

.global main
    .equ RESETS_BASE, 0x4000c000
    .equ RESETS_CLEAR, 0x3000

    .equ RESET_IO_BANK0, (1<<5)
    .equ RESET_PADS, (1<<8)
    .equ RESET_SIO, (1<<3)

    .equ IO_BANK0_BASE, 0x40014000
    .equ PADS_BANK0_BASE, 0x4001c000

    .equ GPIO17_STATUS, 0x088
    .equ GPIO17_CTRL, 0x08c
    .equ GPIO17_BIT, (1<<17)
    .equ GPIO17_PAD, 0x48

    .equ GPIO18_STATUS, 0x090
    .equ GPIO18_CTRL, 0x094
    .equ GPIO18_BIT, (1<<18)
    .equ GPIO18_PAD, 0x4c

    .equ GPIO25_STATUS, 0x0c8
    .equ GPIO25_CTRL, 0x0cc
    .equ GPIO25_BIT, (1<<25)
    .equ GPIO25_PAD, 0x68

    .equ SIO_BASE, 0xD0000000 // single-cycle IO bank
    .equ GPIO_OE_SET, 0x20 // output enable
    .equ GPIO_OUT_SET, 0x14
    .equ GPIO_OUT_CLR, 0x18

    .equ BLINK_VALUE, 0x00100000

.section .text
.thumb_func
main:
    // initiate reset for IO_BANK0, PADS and SIO
    ldr r0, =RESETS_BASE // start location of resets
    ldr r1, =RESETS_CLEAR // offset of clear register
    ldr r2, =(RESET_IO_BANK0| RESET_PADS| RESET_SIO)// configuration of the reset

    str r2, [r0, r1]// apply the configured reset by writing to offsetted base

reset_wait:
    // keep checking if the value in address 'RESETS_BASE + 8' is the configuration we want
    ldr r1, [r0, #8]
    ands r1, r2 // masking out the unnecessary bits
    cmp r1, r2

    bne reset_wait // go back if not match

    // write value '5 (0b101)' to address 'IO_BANK0_BASE + GPIO25_CTRL' to set function to SIO
    ldr r0, =(IO_BANK0_BASE+ GPIO25_CTRL)
    movs r1, #5
    str r1, [r0]

    /*
    ldr r0, =(IO_BANK0_BASE+ GPIO17_CTRL)
    movs r1, #5
    str r1, [r0]
    // configure pads (input enable)
    ldr r0, =(PADS_BANK0_BASE+ GPIO18_PAD)
    movs r1, #0b1000000
    str r1, [r0]
    */

    // apply bit mask to the address 'SIO_BASE + GPIO_OE_SET' to configure pins as output (output enable)
    ldr r0, =SIO_BASE
    ldr r1, =(GPIO25_BIT| GPIO17_BIT)
    str r1, [r0, #GPIO_OE_SET]

blink_loop:
    str r1, [r0, #GPIO_OUT_SET]// write GPIO25_BIT to the address 'SIO_BASE + GPIO_OUT_SET'
    bl delay

    str r1, [r0, #GPIO_OUT_CLR]
    bl delay

    b blink_loop

.thumb_func
delay:
    ldr r2, =BLINK_VALUE
delay_loop:
    subs r2, r2, #1
    bne delay_loop
    bx lr
