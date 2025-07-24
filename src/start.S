.section .text
.global _start
_start:
    ldr x5, =_stack_top
    mov sp, x5
    mov x1, x0
    bl rom_main
1:
    wfe
    b 1b

