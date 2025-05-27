;当前物理内存的分布图
;|----------------------|
;|	    100000 ~ END	|
;|	      KERNEL	    |
;|----------------------|
;|		E0000 ~ 100000	|
;| Extended System BIOS |
;|----------------------|
;|		C0000 ~ Dffff	|
;|     Expansion Area   |
;|----------------------|
;|		A0000 ~ bffff	|
;|   Legacy Video Area  |
;|----------------------|
;|		9f000 ~ A0000	|
;|	 	BIOS reserve	|
;|----------------------|
;|		90000 ~ 9f000	|
;|	 	kernel tmpbuf	|
;|----------------------|
;|		10000 ~ 90000	|
;|	   		LOADER		|
;|----------------------|
;|		8000 ~ 10000	|
;|	  	VBE info		|
;|----------------------|
;|		7e00 ~ 8000		|
;|	  	mem info		|
;|----------------------|
;|		7c00 ~ 7e00		|
;|	 	MBR (BOOT)		|
;|----------------------|
;|		0000 ~ 7c00		|
;|	 	BIOS Code		|
;|----------------------|

org 0x7c00              ;org是original的缩写，用于指定程序的起始地址；若没有使用org伪指令，则编译器把0x0000作为起始地址
                        ;主要影响绝对地址寻址指令，不同的起始地址会编译成不同的绝对地址
;=======程序起始物理地址：BaseOfLoader<<4+OffsetOfLoader=0x10000
BaseOfStack equ 0x7c00
BaseOfLoader equ 0x1000
OffsetOfLoader equ 0x00

jmp short Label_Start       ;跳转至主程序启动函数
nop                         ;空指令nop汇编后占1B，jmp指令占2B
RootDirSectors equ 14   ;根目录占用的扇区数14=(根目录容纳目录项数224*每个目录项大小32B+每个扇区字节数-1)/每个扇区字节数512
SectorNumOfRootDirStart equ 19  ;根目录起始扇区号19=保留扇区数1+FAT表扇区数9*FAT表份数2
SectorNumOfFAT1Start equ 1  ;FAT1表的起始扇区号为1（因为FAT表1前只有一个保留扇区（引导扇区）且扇区号为0）
SectorBalance equ 17    ;因为FAT[0]与FAT[1]簇为无效簇，所以将根目录起始扇区号-2，则间接将数据区起始扇区号-2
;=======FAT12文件系统不仅包含Boot程序，还有FAT12文件系统的组成结构信息；相当于EXT类文件系统的superblock结构
BS_OEMName db 'MINEboot'    ;软盘生产厂家名称
BPB_BytesPerSec dw 512      ;每个扇区占用字节数，可能是512B，1024B，2048B，4096B
BPB_SecPerClus db 1         ;每个簇占用扇区数，取值必须是2的幂次（从2的0次方开始）
BPB_RsvdSecCnt dw 1         ;保留扇区数，FAT12此字段值必为1，因为引导扇区需要占用一个保留扇区
BPB_NumFATs db 2            ;FAT表份数，FAT12多数保留2份表，FAT表1为主表，FAT表2为备份表
BPB_RootEntCnt dw 224       ;根目录可容纳的FAT目录项数
BPB_TotSec16 dw 2880        ;FAT12文件系统总扇区数，BPB_TotSec16与BPB_TotSec32必有一个不为0
BPB_Media db 0xF0           ;介质描述符，不可移动介质：0xF8、可移动介质：0xF0，且应该写入FAT[0]表项低字节
BPB_FATSz16 dw 9            ;FAT表占用扇区数
BPB_SecPerTrk dw 18         ;每个磁道扇区数
BPB_NumHeads dw 2           ;软盘磁头数，因为1.44M软盘只有一张盘片，所以只需要上下两个磁头
BPB_HiddSec dd 0            ;隐藏扇区数
BPB_TotSec32 dd 0           ;FAT12文件系统总扇区数，BPB_TotSec16与BPB_TotSec32必有一个不为0
BS_DrvNum db 0              ;int 13h的驱动器号
BS_Reserved1 db 0           ;未使用，作为保留字段
BS_BootSig db 29H           ;扩展引导标志，必须是0x29
BS_VolID dd 0               ;卷序列号
BS_VOlLab db 'boot loader'  ;卷标，即Windows和Linux中的磁盘名
BS_FileSysType db 'FAT12   '   ;文件系统类型，字段可自定义任意值，操作系统不依靠该字段识别文件系统

;=======以上为FAT12文件系统引导扇区数据，接下来是主程序启动函数
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
    ; jmp $
    ; ;=======用0填充当前扇区剩余空间
	; times 510-($-$$) db 0   ;$表示当前行被编译后的地址；$$表示当前节（Section）：Label_Start的起始地址
	; dw 0xaa55

;=======有了软盘读取功能，可在其基础上实现目标文件搜索功能
;=======从FAT12文件系统搜索引导加载程序loader.bin
    mov word [SectorNo],SectorNumOfRootDirStart ;根目录的起始扇区号存放在DS:SectorNo处=19
;=======在根目录搜索与LoaderFileName标号相同的目录项
Label_Search_In_Root_Dir_Begin:
    ;初始时(DS:RootDirSizeForLoop)=RootDirSectors=根目录占用的扇区数14
    cmp word [RootDirSizeForLoop],0     ;判断当前是否已经到达根目录最后一个扇区
    jz Label_No_LoaderBin               ;若为0则ZF标志位=1表示找不到目录项，跳转到Label_No_LoaderBin作处理
    dec word [RootDirSizeForLoop]       ;根目录扇区数大小减少2B
    ;调用Func_ReadOneSector之前设置参数
    mov ax,00h                  
    mov es,ax                   ;ES=AX=0000H，设置内存缓冲区
    mov bx,8000h                ;ES:BX=08000H，读取数据存放在内存缓冲区的起始地址
    mov ax,[SectorNo]           ;AX=(DS:SectorNo)=待读取的磁盘根目录的起始扇区号
    mov cl,1                    ;CL=读入的扇区数量
    call Func_ReadOneSector     ;读入属于根目录的第一个扇区；call指令等同于push IP；jmp Func_ReadOneSector
    mov si,LoaderFileName       ;将loader.bin文件名标号所在地址放入si，源数据地址为DS:SI=LoaderFileName标号地址
    mov di,8000h                ;要比对的FAT目录项名称数据地址为ES:DI=08000H
    cld                         ;因为后面用到lodsb指令（与DF标志位有关），所以清除DF标志位
    mov dx,10h                  ;DX=每个扇区可容纳FAT12目录项个数=512/32=16=0x10
;=======在第一个扇区内依据文件名“LOADER BIN”搜索每个FAT12目录项
Label_Search_For_LoaderBin:
    cmp dx,0                    ;检查DX是否为0
    jz Label_Goto_Next_Sector_In_Root_Dir   ;如果是则检查属于根目录的下一个扇区
    dec dx                      ;每检查一个目录项，DX-1    
    mov cx,11                   ;CX=目录项文件名长度
;=======寻找与文件名相匹配的目录项
Label_Cmp_FileName:
    cmp cx,0                    ;检查CX是否为0
    jz Label_FileName_Found     ;如果是则匹配成功
    dec cx                      ;每检查一个字符CX-1
    lodsb                       ;从DS:SI指定的源数据内存地址读取数据到AL寄存器
    cmp al,byte [es:di]         ;检查AL（即DS:SI）与ES:DI目标数据内存地址里的内容是否匹配
    jz Label_Go_On              ;如果源字符与要比对的目的字符相同，ZF=0，则继续比对下一字符
    jmp Label_Different         ;源字符与目的字符不一致，跳转至不匹配处理子程序
;=======当前字符匹配成功后继续下一轮匹配
Label_Go_On:
    inc di
    jmp Label_Cmp_FileName
;=======当前字符匹配失败，则继续寻找下一目录项
Label_Different:
    and di,0ffe0h               ;0000 1111 1111 1110 0000 & DI，保留DI高11位
    add di,20h
    mov si,LoaderFileName
    jmp Label_Search_For_LoaderBin
;=======前往根目录的下一个扇区继续寻找相应目录项
Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo],1
    jmp Label_Search_In_Root_Dir_Begin
;=======根目录下所有扇区都不包含LoaderFileName标号的目录项，则查找失败，打印对应信息
Label_No_LoaderBin:
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
    mov bx,008ch
    mov dx,0100h       ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,21          ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,NoLoaderMessage
    int 10h
    jmp $
;=======在根目录FAT表项中找到loader.bin对应的目录项
Label_FileName_Found:
    mov ax,RootDirSectors       ;AX=RootDirSectors=根目录占用的扇区数14
    and di,0ffe0h               ;0000 1111 1111 1110 0000 & DI，保留DI的高11位
    add di,01ah                 ;DI=DI+0000 0001 1010=1111 1111 1111 1010
    mov cx,word [es:di]         ;CX=(ES:DI)=0
    push cx                     ;将CX=(ES:DI)保存进栈，下文即将使用CX
    add cx,ax                   ;CX=CX+AX（根目录占用的扇区数14）=14
    add cx,SectorBalance        ;CX=CX+SectorBalance（数据区起始扇区号17）=14+17=31
    mov ax,BaseOfLoader
    mov es,ax                   ;ES=AX=BaseOfLoader 0x1000
    mov bx,OffsetOfLoader       ;BX=OffsetOfLoader 0x00；ES:BX即为loader.bin程序在内存中的起始地址
    mov ax,cx                   ;AX=CX（ES:DI）+SectorBalance（数据区起始扇区号17）
;=======将loader.bin程序对应的磁盘簇读入内存
;=======int 10h,AH=0EH功能：在屏幕上显示一个字符
Label_Go_On_Loading_File:
    push ax                     
    push bx                     
    mov ah,0eh                  ;int 10h中断的主功能号0EH
    mov al,'.'                  ;AL=要显示的字符'.'
    mov bl,0fh                  ;BL=前景色设置（字体颜色设置）
    int 10h
    pop bx
    pop ax
    mov cl,1                    ;读取loader.bin文件的第1个扇区
    call Func_ReadOneSector     
    pop ax                      
    call Fun_GetFATEntry        ;每读入一个扇区的数据就通过Fun_GetFATEntry取得下一个FAT表项
    cmp ax,0fffh                ;直到Fun_GetFATEntry返回的FAT表项值为0FFFH为止
    jz Label_File_Loaded        ;若AX=0FFFH则跳转至Label_File_Loaded标号处往下执行
    push ax
    mov dx,RootDirSectors       ;DX=RootDirSectors 根目录占用的扇区数14
    add ax,dx                   ;AX=AX+DX
    add ax,SectorBalance        ;AX=AX+SectorBalance
    add bx,[BPB_BytesPerSec]    ;BX=BX+(DS:BPB_BytesPerSec) 每个扇区占用字节数512B
    jmp Label_Go_On_Loading_File;继续读loader.bin的下一个磁盘簇
;======准备跳转至loader.bin程序处执行
Label_File_Loaded:
    jmp BaseOfLoader:OffsetOfLoader     ;段间地址跳转（长跳转指令），必须明确指定跳转的目标段和段内偏移地址
                                        ;(CS)=0x1000，实模式下BaseOfLoader段基地址=0x1000<<4=0x10000

;=======int 13h,AH=02h：读取磁盘扇区，执行成功CF=0
;=======AL=读入的扇区数（必须非0）；CH=磁道号（柱面号）低8位
;=======CL=扇区号1～63（bit 0～5）磁道号（柱面号）的高2位（bit 6～7，只对硬盘有效）
;=======DH=磁头号；DL=驱动器号（如果操作的是硬盘驱动器，bit 7必须被置位）
;=======ES:BX=读取数据存放在内存缓冲区的起始地址
Func_ReadOneSector:         ;设置好int 13h,AH=02h：读取磁盘扇区功能的各寄存器参数
    push bp                 ;先保存栈帧寄存器和栈寄存器的值
    mov bp,sp               ;ESP,EBP:32bit  SP,BP:16bit
    sub esp,2               ;从栈中开辟2B的存储空间（栈指针向下移动2B）
    mov byte [bp-2],cl      ;bp-2与esp指向同一内存地址，CL保存在刚开辟的栈空间
    push bx                 ;即将使用BX寄存器，应入栈保存
    mov bl,[BPB_SecPerTrk]  ;BL=(DS:BPB_SecPerTrk) 每磁道扇区数
    div bl                  ;8位无符号数除法指令；AX=待读取磁盘LBA起始扇区号；BL=每磁道扇区数；AX/BL=AL......AH
    inc ah                  ;余数AH=目标磁道内起始扇区号；因为起始扇区号从1开始计数，所以AH+1
    mov cl,ah               ;CL=最终读取的扇区号
    mov dh,al               ;商AL=DH=目标磁道号，仍需对AL、DH操作得到最终读取的柱面号和磁头号
    shr al,1                
    mov ch,al               ;CH=AL>>1，最终读取的柱面号
    and dh,1                ;DH=DH&1，最终读取的磁头号
    pop bx                  ;BX使用完毕，恢复BX  
    mov dl,[BS_DrvNum]      ;DL=(DS:BS_DrvNum) 驱动器号（如果操作的是硬盘驱动器，bit 7必须被置位）
Label_Go_On_Reading:        ;循环读取
    mov ah,2                ;AH=int 13h的子功能号：读取磁盘扇区
    mov al,byte [bp-2]      ;AL=CL=(DS:BP-2)=要读取的扇区号（1～63）
    int 13h                 ;int 13h，AH=02h执行成功后CF=0
    jc Label_Go_On_Reading  ;如果读取未完成（CF=1）则尝试重复读取
    add esp,2               ;当数据读取成功后恢复调用现场
    pop bp
    ret                     ;因为没有涉及call指令调用，无需使用leave指令，ret指令弹出栈顶push IP
;=======根据LoaderFileName标号找到相应的目录项，解析FAT表项并把Loader.bin程序的扇区数据读入内存，索引出下一个FAT表项
Fun_GetFATEntry:
    push es
    push bx
    push ax
    mov ax,00           ;AX=FAT表项号
    mov es,ax           ;ES=AX=0000h
    pop ax              
    mov byte [Odd],0    ;将奇偶标志变量DS:Odd置为0
    mov bx,3            ;因为每个FAT表项占12 bit，1.5Byte，3Byte存放2个FAT表项，所以目录项号具有奇偶性
    mul bx              ;16位无符号数乘法指令，AX=FAT表项号，BX=3；AX*BX=DX:AX，高16位在DX，低16位在AX
    mov bx,2            ;所以将FAT表乘3除2（扩大1.5倍），来判断余数奇偶性并保存在DS:Odd中，奇数为1，偶数为0
    div bx              ;16位无符号数除法指令，DX:AX/BX=AX......DX
    cmp dx,0            ;判断余数是否为0
    jz Lable_Even       ;余数为0，ZF=1，跳转到Lable_Even
    mov byte [Odd],1    ;将奇偶标志变量DS:Odd置为1
;=======FAT表项号为偶数
Lable_Even:
    xor dx,dx                   ;将DX寄存器清零
    mov bx,[BPB_BytesPerSec]    ;BX=(DS:BPB_BytesPerSec) 每个扇区字节数
    div bx                      ;16位无符号数除法指令，DX:AX/BX=AX（FAT表项偏移扇区号）......DX（FAT表项偏移位置）
    push dx                     ;DX在Func_ReadOneSector子过程内被调用，先压栈保存
    mov bx,8000h
    add ax,SectorNumOfFAT1Start ;AX=AX+FAT表1的起始扇区号
    mov cl,2                    ;CL=2，读入两个扇区
    call Func_ReadOneSector     ;解决FAT表项横跨两个扇区的问题
    pop dx                      
    add bx,dx                   ;BX=BX（8000h）+DX（FAT表项偏移位置）
    mov ax,[es:bx]              ;AX=(ES:BX)
    cmp byte [Odd],1            ;奇偶标志变量DS:Odd与1对比
    jnz Lable_Even_2            ;FAT目录项号为偶数跳转至Lable_Even_2处理
    shr ax,4                    ;进一步处理奇偶项错位问题，FAT目录项为奇数向右移动4位
Lable_Even_2:
    and ax,0fffh                ;AX=AX & 0000 1111 1111 1111
    pop bx
    pop es
    ret

;=======程序运行时的临时数据
RootDirSizeForLoop dw RootDirSectors
SectorNo dw 0
Odd db 0
;=======在屏幕上显示的消息文本
StartBootMessage: db "Start Boot"	;可理解成C语言的一维字符串
NoLoaderMessage: db "ERROR:NO LOADER Found"
LoaderFileName: db "LOADER  BIN",0
;=======用0填充当前扇区剩余空间
	times 510-($-$$) db 0   ;$表示当前行被编译后的地址；$$表示当前节（Section）：Label_Start的起始地址
	dw 0xaa55
times 1474560-($-$$) db 0   ;为剩下的2879个扇区填充0，是使用cp loader.bin /media引入loader.bin的必要前提