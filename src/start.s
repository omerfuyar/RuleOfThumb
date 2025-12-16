.syntax unified
.cpu cortex-m0plus
.thumb
.thumb_func

.global _start
.global _stack_top

.section .vectors, "a"
.word _stack_top
.word _start

.section .text
.thumb_func
_start:
    ldr r0, =_stack_top
    mov sp, r0
    bl main
hang:
    b hang
