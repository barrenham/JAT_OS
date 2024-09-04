%include "boot.inc"
SECTION MBR vstart=07c00h
    mov ax,     cs
    mov ds,     ax
    mov es,     ax
    mov ss,     ax
    mov fs,     ax

    ;mov dx,     0h
    ;call displayHelloWorld

    mov eax,    LOADER_START_SECTOR
    mov bx,     LOADER_BASE_ADDR
    mov cx,     4
    call rd_disk_mem   

    ;mov dx,     00100h
    ;call displayHelloWorld

    ;mov dx,     00200h
    ;call displayLoader

    jmp LOADER_BASE_ADDR

    jmp $
displayHelloWorld:
    mov ax,     Message
    mov bp,     ax
    mov cx,     29
    mov ax,     01301h
    mov bx,     000ch
    int 10h
    ret
displayLoader:
    mov ax,     LOADER_BASE_ADDR
    mov bp,     ax
    mov cx,     512
    mov ax,     01301h
    mov bx,     000ch
    int 10h
    ret
;rd_disk_mem  
;input:     eax(LBA地址) bx(导入至内存的位置) cx(读取的扇区数) 
;output:    none
;impact:    通过LBA地址将特定扇区数的数据从主硬盘传入内存指定位置中
;tip:       cx 需要小于0x007F
rd_disk_mem:
    mov esi,    eax
    mov di,     cx      ;保存传入的参数值
    
    mov dx,     1F2h    ;向磁盘控制器传递需要的扇区个数
    mov al,     cl
    out dx,     al

    mov eax,    esi

    mov cl,     8

    mov dx,     1F3h    ;(1F5 1F4 1F3):LBA地址寄存器(个人理解)
    out dx,     al
    
    mov dx,     1F4h
    shr eax,    cl
    out dx,     al

    mov dx,     1F5h
    shr eax,    cl
    out dx,     al

    mov dx,     1F6h
    shr eax,    cl
    and al,     0fh
    or  al,     0E0h
    out dx,     al
    ;此时LBA地址已导入,等待发出命令

    mov dx,     1F7h
    mov al,     20h
    out dx,     al
    ;向磁盘控制器发出读硬盘指令

    ;接下来应该确保读取已经完成
.read_ready:
    nop
    in  al,     dx
    and al,     08h
    cmp al,     08h
    jne .read_ready
    ;此时磁盘已经准备好输出了

    mov ax,     di
    mov dx,     512
    mul dx
    mov cx,     ax
    mov dx,     1F0h
.read_loop:
    in  ax,     dx
    mov [bx],   ax
    add bx,     2
    loop .read_loop

    mov eax,    esi
    mov cx,     di
    ret

Message:
    db "hello world!!! --author: msc"
times 510-($-$$) db 0
    dw 0aa55h

