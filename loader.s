%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_ADDR
    LOADER_STACK_TOP    equ     LOADER_BASE_ADDR

    jmp loader_start

    GDT_BASE:           dd      0x00000000
                        dd      0x00000000
    CODE_DESC:          dd      0x0000FFFF
                        dd      DESC_CODE_HIGH4
    DATA_STACK_DESC     dd      0x0000FFFF
                        dd      DESC_DATA_HIGH4
    VIDEO_DESC          dd      0x80000007
                        dd      DESC_VIDEO_HIGH4
    times 60            dq      0
    GDT_SIZE            equ     $-GDT_BASE
    GDT_LIMIT           equ     GDT_SIZE-1
    SELECTOR_CODE       equ     (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA       equ     (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO      equ     (0x0003<<3) + TI_GDT + RPL0

    gdt_ptr             dw      GDT_LIMIT
                        dd      GDT_BASE

    total_mem_bytes     dd      0   ;0x809

    ards_buf times 244  db      0
    ards_nr             dw      0

loader_start:
    ;mov ah,     06h
    ;mov al,     0
    ;mov ch,     0x00                ; 清除屏幕的左上角行号（通常是0）  
    ;mov cl,     0x00                ; 清除屏幕的左上角列号（通常是0）  
    ;mov dh,     24                  ; 清除屏幕的右下角行号（通常是24可能根据视频模式而变化）  
    ;mov dl,     79                  ; 清除屏幕的右下角列号（通常是79可能根据视频模式而变化）
    ;int 10h
    ;清屏

    ;打印一个字符串,用来说明loader加载成功
    ;mov bp,     Congratulation
    ;mov ah,     13h
    ;mov al,     1
    ;mov bh,     0
    ;mov bl,     84h
    ;mov cx,     18
    ;mov dh,     12
    ;mov dl,     40
    ;int 10h

    mov ah,     06h
    mov al,     0
    mov ch,     0x00                ; 清除屏幕的左上角行号（通常是0）  
    mov cl,     0x00                ; 清除屏幕的左上角列号（通常是0）  
    mov dh,     24                  ; 清除屏幕的右下角行号（通常是24可能根据视频模式而变化）  
    mov dl,     79                  ; 清除屏幕的右下角列号（通常是79可能根据视频模式而变化）
    int 10h
    ;清屏

    mov ecx,    0xFFFFFFFF
lp: loop        lp

    xor ebx,    ebx
    mov edx,    0x534d4150
    mov di,     ards_buf
.e820_mem_get:
.e820_mem_get_loop:
    mov eax,    0x0000e820
    mov ecx,    20
    int 0x15
    jc  .e820_failed_so_try_e801
    add di,     cx
    inc word    [ards_nr]
    cmp ebx,    0
    jne .e820_mem_get_loop

;找出ards中（base_add_low + length_low) 的最大值
    mov cx,     [ards_nr]
    mov ebx,    ards_buf
    xor edx,    edx
;该程序暂时只能支持4GB内存容量读取，如果需要扩展，请改变下面的运算过程
;tip:未用到ards结构中的所有内容，对于mem>4GB可能会存在问题
.find_max_mem_area:
    mov eax,    [ebx]
    add eax,    [ebx+8]
    add ebx,    20
    cmp edx,    eax
    jge .next_ards
    mov edx,    eax
.next_ards:
    loop .find_max_mem_area
    jmp  .find_mem_ok
.e820_failed_so_try_e801:
.e801_mem_get:
    mov ax,     0xe801
    int 0x15
    jc  .e801_failed_so_try88
    mov cx,     0x400
    mul cx
    shl edx,    16
    and eax,    0x0000FFFF
    or  edx,    eax
    add edx,    0x100000
    mov esi,    edx

    xor eax,    eax
    mov ax,     bx
    mov ecx,    0x10000
    mul ecx

    add esi,    eax
    mov edx,    esi
    jmp .find_mem_ok
.e801_failed_so_try88:
.e88_mem_get:
    mov ah,     0x88
    int 0x15
    jc  .error_hlt
    and eax,    0x0000FFFF
    
    mov cx,     0x400
    mul cx
    shl edx,    16
    or  edx,    eax
    add edx,    0x100000
.find_mem_ok:
    mov [total_mem_bytes],  edx
;待填充,(同时需要对上方增加跳转指令,避免进入出错处理)
.error_hlt:


;进入保护模式
;1.打开A20
;2.加载GDT
;3.将cr0的pe位置1

    ;step 1
    in  al,     0x92
    or  al,     0000_0010b
    out 0x92,   al

    ;step 2
    lgdt        [gdt_ptr]

    ;step 3
    mov eax,    cr0
    or  eax,    0x00000001
    mov cr0,    eax


    jmp dword   SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax,     SELECTOR_DATA
    mov ds,     ax
    mov es,     ax
    mov ss,     ax
    mov esp,    LOADER_STACK_TOP
    mov ax,     SELECTOR_VIDEO
    mov gs,     ax
    mov byte [gs:160], 'P'
    mov byte [gs:161], 0x84

    mov eax,    KERNEL_START_SECTOR
    mov ebx,    KERNEL_BIN_BASE_ADDR
    mov ecx,    255
    call        rd_disk_mem

    call        setup_page

    sgdt        [gdt_ptr]
    mov ebx,    [gdt_ptr+2]
    or  dword   [ebx+0x18+4],           0xc0000000

    add dword   [gdt_ptr+2],            0xc0000000
    add esp,    0xc0000000

    mov eax,    PAGE_DIR_TABLE_POS
    mov cr3,    eax

    ;打开cr0的pg位(第31位)
    mov eax,    cr0
    or  eax,    0x80000000
    mov cr0,    eax

    lgdt        [gdt_ptr]

    mov byte    gs:[160],               'V'

    jmp SELECTOR_CODE:enter_kernel

enter_kernel:
    call    kernel_init
    mov esp,    0xc009f000
    jmp KERNEL_ENTRY_POINT

    jmp $
;kernel_init
;input:
;output:
;impact:    解析elf并放置于新的位置
;tip:       
kernel_init:
    xor eax,    eax
    xor ebx,    ebx     ;ebx记录程序头表地址
    xor ecx,    ecx     ;cx记录程序头表中的program header数量
    xor edx,    edx     ;dx记录program header尺寸,即e_phentaise

    mov dx,     [ELF_PH_SIZE_ADDR]
    mov ebx,    [ELF_PH_OFFSET_ADDR]

    add ebx,    KERNEL_BIN_BASE_ADDR
    mov cx,     [ELF_PH_COUNT_ADDR]

.each_segment:
    cmp byte    [ebx + 0],  ELF_PH_PT_NULL
    je  .PTNULL
    ;cmp byte    [ebx + 24], ELF_PH_PF_RX
    ;jne .PFNRX
    ;memcpy{dst,src,size}
    push dword  [ebx + 16]  ;压入第三个参数size

    mov eax,    [ebx + 4]   ;每个program_header里的offset,代表该program在elf中的位置
    add eax,    KERNEL_BIN_BASE_ADDR

    push    eax             ;压入src
    push dword  [ebx + 8]   ;压入dst

    call    mem_cpy
    add esp,    12
.PFNRX:
.PTNULL:
    add ebx,    edx

    loop    .each_segment
    ret
;mem_cpy
;input: 
;dst:       复制到的内存起始的地址
;src:       被复制的内存起始的地址
;size:      复制的内存的大小
;output:    NONE
;impact:    内存操作，将内存从一个地方复制到另一个地方
;tip:
mem_cpy:
    cld
    push    ebp
    mov ebp,    esp
    push    ecx

    mov edi,    [ebp + 8]
    mov esi,    [ebp + 12]
    mov ecx,    [ebp + 16]
    rep movsb

    pop     ecx
    pop     ebp
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

    or  eax,    eax
    mov ax,     di
    mov dx,     512
    mul dx
    shl edx,    16
    or  eax,    edx
    mov ecx,    eax
    mov dx,     1F0h
.read_loop:
    in  ax,     dx
    mov [ebx],  ax
    add ebx,    2
    loop .read_loop

    mov eax,    esi
    mov cx,     di
    ret

setup_page:
    mov ecx,    4096
    mov esi,    0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

.create_pde:
    mov eax,    PAGE_DIR_TABLE_POS
    add eax,    0x1000
    mov ebx,    eax

    or  eax,    PG_US_U|PG_RW_W|PG_P
    mov [PAGE_DIR_TABLE_POS+0x0],   eax
    mov [PAGE_DIR_TABLE_POS+0xc00], eax
    ;用于将0xc0000000和0x00000000映射至同一页表中
    sub eax,    0x1000
    mov [PAGE_DIR_TABLE_POS+4092],  eax
    ;将页表目录最后一项设为本身，用于找到它管理的页表
    ;接下来开始创建页表项，将前1MB的内存存在不同的页内
    mov ecx,    256     ;256 * 4KB = 1MB
    mov esi,    0
    mov edx,    PG_US_U|PG_RW_W|PG_P
.create_pte:
    mov [ebx+esi*4],                edx
    add edx,    4096
    inc esi
    loop    .create_pte

;创建内核其他页表的PDE
    mov eax,    PAGE_DIR_TABLE_POS
    add eax,    0x2000
    or  eax,    PG_US_U|PG_RW_W|PG_P
    mov ebx,    PAGE_DIR_TABLE_POS
    mov ecx,    254
    mov esi,    769
.create_kernel_pde:
    mov [ebx+esi*4],                eax
    inc esi
    add eax,    0x1000
    loop    .create_kernel_pde
    ret
Congratulation:
    db  "Congratulation!!!"
times 2048-($-$$) db 0