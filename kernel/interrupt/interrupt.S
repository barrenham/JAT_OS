[bits 32]
%define     ERROR_CODE      nop

%define     ZERO            push    0

extern      idt_table
extern      put_string

section     .data
    intr_str    db      "interrupt occur!", 0xa,    0

global      intr_entry_table
intr_entry_table:

%macro     VECTOR       2
section     .text
intr%1entry:
    %2
    push    ds
    push    es
    push    fs
    push    gs
    pushad

    mov     al,             0x20
    out     0xa0,           al
    out     0x20,           al

    push    %1
    call    [idt_table + %1*4]

    jmp     intr_exit

section     .data
    dd      intr%1entry
%endmacro

section     .text
global  intr_exit
intr_exit:
    add     esp,            4
    popad
    pop     gs
    pop     fs
    pop     es
    pop     ds
    add     esp,            4

    iret

VECTOR 0x0 ,ZERO
VECTOR 0X1 ,ZERO
VECTOR 0X2 ,ZERO
VECTOR 0x3 ,ZERO
VECTOR 0X4 ,ZERO
VECTOR 0X5 ,ZERO
VECTOR 0x6 ,ZERO
VECTOR 0X7 ,ZERO
VECTOR 0X8 ,ERROR_CODE
VECTOR 0x9 ,ZERO
VECTOR 0XA ,ERROR_CODE
VECTOR 0XB ,ERROR_CODE
VECTOR 0XC ,ERROR_CODE
VECTOR 0XD ,ERROR_CODE
VECTOR 0XE ,ERROR_CODE
VECTOR 0XF ,ZERO
VECTOR 0X10 ,ZERO
VECTOR 0X11 ,ERROR_CODE
VECTOR 0x12 ,ZERO
VECTOR 0X13 ,ZERO
VECTOR 0X14 ,ZERO
VECTOR 0x15 ,ZERO
VECTOR 0X16 ,ZERO
VECTOR 0X17 ,ZERO
VECTOR 0X18 ,ZERO
VECTOR 0X19 ,ZERO
VECTOR 0X1A ,ZERO
VECTOR 0X1B ,ZERO
VECTOR 0X1C ,ZERO
VECTOR 0X1D ,ZERO
VECTOR 0X1E ,ERROR_CODE                               ;处理器自动推错误码
VECTOR 0X1F ,ZERO
VECTOR 0X20 ,ZERO
VECTOR 0x21 ,ZERO
VECTOR 0x22 ,ZERO
VECTOR 0x23 ,ZERO
VECTOR 0x24 ,ZERO
VECTOR 0x25 ,ZERO
VECTOR 0x26 ,ZERO
VECTOR 0x27 ,ZERO
VECTOR 0x28 ,ZERO
VECTOR 0x29 ,ZERO
VECTOR 0x2a ,ZERO
VECTOR 0x2b ,ZERO
VECTOR 0x2c ,ZERO
VECTOR 0x2d ,ZERO
VECTOR 0x2e ,ZERO
VECTOR 0x2f ,ZERO



[bits 32]
extern syscall_table
section .text
global  syscall_handler
syscall_handler:
    push    0
    push    ds
    push    es
    push    fs
    push    gs
    pushad

    push    0x80

    push    edx
    push    ecx
    push    ebx

    call    [syscall_table+eax*4]
    add     esp,    12

    mov     [esp+8*4],  eax
    jmp     intr_exit
    