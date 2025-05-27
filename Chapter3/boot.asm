org 0x7c00              ;org是original的缩写，用于指定程序的起始地址；若没有使用org伪指令，则编译器把0x0000作为起始地址
                        ;主要影响绝对地址寻址指令，不同的起始地址会编译成不同的绝对地址
;=======程序起始物理地址：BaseOfLoader<<4+OffsetOfLoader=0x10000
BaseOfStack equ 0x7c00      ;equ相当于C语言#define的常量，在汇编时就已经处理，不会在最终的可执行文件中占用空间
;=======接下来是主程序启动函数
Label_Start:
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax			;CS=DS=ES=SS=0x0000	
	mov sp,BaseOfStack	;SP=0x7C00
;BIOS中断服务程序int 10h各种子功能的使用
;=======int 10h,AH=06h：按指定范围滚动窗口
	mov ax,0600h		;AL=滚动列数，若为0则实现清空屏幕（此时其他寄存器参数不起作用）
	mov bx,0700h		;BH=滚动后空出位置放入内容的属性
	mov cx,0			;CH=滚动范围左上角坐标列号，CL=滚动范围左上角坐标行号
	mov dx,0184fh		;DH=滚动范围右下角坐标列号，DL=滚动范围右下角坐标行号
	int 10h
;=======int 10h,AH=02h：设置屏幕光标位置（坐标原点(0,0)位于左上角）
	mov ax,0200h		;AH=02h，2号子功能
	mov bx,0000h		;BH=页码
	mov dx,0000h		;DH=游标坐标列号，DL=游标坐标行号
	int 10h
;=======int 10h,AH=13h：显示字符串
	;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
	mov ax,1301h
	mov bx,000fh		;BH=页码，BL=字符属性
	mov dx,0000h		;DH=游标坐标列号，DL=游标坐标行号
	mov cx,10			;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
	push ax
	mov ax,ds
	mov es,ax			;ES:BP=要显示的字符串的内存地址
	pop ax
	mov bp,StartBootMessage
	int 10h
;=======int 13h,AH=00h：重置磁盘驱动器
	xor ah,ah   ;等价于mov ah,0000h
	xor dl,dl	;DL为驱动器号，00h～7fh：软盘，80h～0ffh：硬盘
	int 13h
    jmp $
;=======在屏幕上显示的消息文本
StartBootMessage: db "Start Boot"	;可理解成C语言的一维字符串

;=======用0填充当前扇区剩余空间
	times 510-($-$$) db 0   ;$表示当前行被编译后的地址；$$表示当前节（Section）：Label_Start的起始地址
	dw 0xaa55