TI_GDT              equ     0
RPL0                equ     0
SELECTOR_VIDEO      equ     (0x0003<<3) + TI_GDT + RPL0

SECTION .data
put_int_buffer      dq      0
put_int_buffer_end  db      0

[bits 32]
SECTION .text
;put_int:
;input:     [ESP](栈一){int}
;output:
;impact:    在屏幕光标处打印一个int整型变量
;tip:       如果需要速度提升,put_int函数内调用的put_string可以自行展开，减少栈操作
global put_int
put_int:
    pushad
    mov     ebp,    esp
    mov     eax,    [ebp+4*9]
    mov     edx,    eax
    mov     edi,    7
    mov     ecx,    8
    mov     ebx,    put_int_buffer
.16based_4bits:
    and     edx,    0x0000000F
    cmp     edx,    9
    jg      .is_A2F
    add     edx,    '0'
    jmp     .store
.is_A2F:
    sub     edx,    10
    add     edx,    'A'
.store:
    mov     [ebx+edi],  dl
    dec     edi
    shr     eax,    4
    mov     edx,    eax
    loop    .16based_4bits
.ready_to_print:
    inc     edi
.skip_prefix_0:
    cmp     edi,    8
    je      .full0
.go_on_skip:
    mov     cl,     [ebx+edi]
    inc     edi
    cmp     cl,     '0'
    je      .skip_prefix_0
.full0:
    dec     edi
    add     ebx,    edi
    push    ebx
    call    put_string
    add     esp,    4
    popad
    ret
;put_string:
;input:     [ESP](栈一){char*}
;output:
;impact:    在屏幕光标处打印一个字符串
;tip:
global put_string
put_string:
    push    ebx
    push    ecx
    xor     ecx,    ecx ;使用ecx存储参数
    mov     ebx,    [esp + 12]
.goon:
    mov     cl,     [ebx]
    cmp     cl,     0
    jz      .str_over
    push    ecx
    call    put_char    ;call指令会进行push ebp ret指令会进行pop ebp
    add     esp,    4
    inc     ebx
    jmp     .goon
.str_over:
    pop     ecx
    pop     ebx
    ret
;put_char
;input:     [ESP](栈一){char}
;output:    
;impact:    在屏幕光标处打印一个字符
;tip:
global put_char
put_char:
    pushad          ;备份32位寄存器环境
    mov     ax,     SELECTOR_VIDEO
    mov     gs,     ax
    
    ;获取光标高8位
    mov     dx,     0x03d4
    mov     al,     0x0e
    out     dx,     al
    mov     dx,     0x03d5
    in      al,     dx
    mov     ah,     al
    ;获取光标低8位
    mov     dx,     0x03d4
    mov     al,     0x0f
    out     dx,     al
    mov     dx,     0x03d5
    in      al,     dx

    ;存放光标到bx
    mov     bx,     ax

    mov     ecx,    [esp + 36]  ;越过所有备份

    cmp     cl,     0xd         ;0xd    \r
    jz      .is_carriage_return
    cmp     cl,     0xa         ;0xa    \n
    jz      .is_line_feed

    cmp     cl,     0x8         ;0x8    backspace
    jz      .is_backspace
    jmp     .put_other

.is_backspace:
    dec     bx
    shl     bx,     1           ;bx * 2用于表示文本模式下的正确光标位置

    mov     byte    [gs:bx],    0x20    ;将待删除的字节变为0或空格
    inc     bx
    mov     byte    [gs:bx],    0x07
    shr     bx,     1
    jmp     .set_cursor

.put_other:
    shl     bx,     1
    mov     byte    [gs:bx],    cl
    inc     bx
    mov     byte    [gs:bx],    0x07
    shr     bx,     1
    inc     bx
    cmp     bx,     2000
    jl      .set_cursor
    jmp     .roll_screen
.is_line_feed:
.is_carriage_return:
    xor     dx,     dx
    mov     ax,     bx
    mov     si,     80

    div     si

    sub     bx,     dx
    cmp     cl,     0xd
    jz      .is_line_feed_end
.is_carriage_return_end:
    add     bx,     80
.is_line_feed_end:
    cmp     bx,     2000
    jl      .set_cursor

.roll_screen:
    cld
    mov     ecx,    960

    mov     esi,    0xc00b80a0
    mov     edi,    0xc00b8000
    rep     movsd

    mov     ebx,    3840
    mov     ecx,    80

.cls:
    mov     word    [gs:ebx],   0x0720
    add     ebx,    2
    loop    .cls
    mov     ebx,    1920

.set_cursor:
    mov     dx,     0x03d4
    mov     al,     0x0e
    out     dx,     al
    mov     dx,     0x03d5
    mov     al,     bh
    out     dx,     al

    mov     dx,     0x03d4
    mov     al,     0x0f
    out     dx,     al
    mov     dx,     0x03d5
    mov     al,     bl
    out     dx,     al
.put_char_done:
    popad
    ret
;待添加ebx判断
global set_cursor
set_cursor:
    pushad
    mov     ebx,    [esp+20]
    
    mov     dx,     0x03d4
    mov     al,     0x0e
    out     dx,     al
    mov     dx,     0x03d5
    mov     al,     bh
    out     dx,     al

    mov     dx,     0x03d4
    mov     al,     0x0f
    out     dx,     al
    mov     dx,     0x03d5
    mov     al,     bl
    out     dx,     al

    popad
    ret

global clean_screen
clean_screen:
    pushad
    mov     ebx,    0
    mov     ecx,    2000
loop_cls:
    mov     byte    [gs:ebx],   0
    inc     ebx
    mov     byte    [gs:ebx],   0x07
    inc     ebx
    loop    loop_cls
    popad
    ret