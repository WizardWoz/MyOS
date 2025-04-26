org 100000h    
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ax,0x00
    mov ss,ax
    mov sp,0x7c00
;=======显示字符串：Start Kernel......
;=======int 10h,AH=13h：显示字符串
	;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    mov bx,000fh        ;BH=页码，BL=字符属性
    mov dx,0300h        ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,12           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax           ;(ES:BP)=要显示的字符串的内存地址
    pop ax
    mov bp,StartKernelMessage
    int 10h
    jmp $
;=======需要显示的字符串
StartKernelMessage db "Start Kernel"