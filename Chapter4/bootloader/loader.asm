org 10000h
    jmp Label_Start
;跟C语言引用头文件的作用相同，fat12.inc是从Boot引导程序中提取出的FAT12文件系统结构
;通过关键字include将文件fat12.inc的内容包含进loader.asm文件
%include "fat12.inc"
;=======内核程序真正的起始物理地址为0x100000（1MB），因为1MB以下并不全是可用地址空间
;======所以让内核程序跳过纷繁复杂的0x100000以下的内存地址空间，从平坦的1MB地址处开始
BaseOfKernelFile equ 0x00
OffsetOfKernelFile equ 0x100000
;=======内核程序临时转存的物理地址空间，因为要通过BIOS中断服务程序int 13h实现内核读取操作
;=======并且BIOS在16bit实模式下只支持上限为1MB的物理地址空间寻址，所以先读入临时转存空间，再通过特殊方式搬运到1MB以上空间
BaseTmpOfKernelAddr equ 0x00
OffsetTmpOfKernelFile equ 0x7E00
;=======内核程序转存至最终物理地址后，临时转存空间可作他用，此处改为内存结构数据的存储空间
MemoryStructBufferAddr equ 0x7E00

;=======SECTION伪指令追加定义一个名为gdt的段，实际上创建了一个32位保护模式的临时GDT表
;=======为避免保护模式段结构的复杂性，此处将代码段和数据段的段基地址设置在0x00000000，段限长为0xFFFFFFFF，可索引4GB内存地址空间
[SECTION gdt]
LABEL_GDT: dd 0,0
;LABEL_DESC_CODE32、LABEL_DESC_DATA32在Chapter4 kernel的head.S文件中还会出现
LABEL_DESC_CODE32: dd 0x0000FFFF,0x00CF9A00     ;前面4B为段限长，后面4B为段基地址
LABEL_DESC_DATA32: dd 0x0000FFFF,0x00CF9200
GdtLen equ $-LABEL_GDT
;dw会向dd对齐（因为已经进入32位模式），所以临时GDT表一共占用32Bytes
GdtPtr dw GdtLen-1          ;存放32位临时GDT表的长度
       dd LABEL_GDT         ;存放32位临时GDT表的起始地址
;SelectorCode32、SelectorData32是两个LDT段选择子，是程序局部段描述符在GDT表中的索引号
SelectorCode32 equ LABEL_DESC_CODE32-LABEL_GDT
SelectorData32 equ LABEL_DESC_DATA32-LABEL_GDT
;=======SECTION伪指令追加定义一个名为gdt64的段，实际上创建了一个IA-32e 64位长模式的临时GDT表
;=======简化了保护模式的段结构，删减掉冗余的段基地址和段限长，使段直接覆盖整个线性空间，变为平坦地址空间
[SECTION gdt64]
;以下三个描述符在Chapter4 kernel的head.S文件中还会出现
LABEL_GDT64: dq	0x0000000000000000
LABEL_DESC_CODE64: dq 0x0020980000000000
LABEL_DESC_DATA64: dq 0x0000920000000000
GdtLen64 equ $-LABEL_GDT64
GdtPtr64 dw GdtLen64-1
         dd	LABEL_GDT64
SelectorCode64 equ LABEL_DESC_CODE64-LABEL_GDT64
SelectorData64 equ LABEL_DESC_DATA64-LABEL_GDT64

[SECTION .s16]      ;SECTION伪指令追加定义一个名为.s16的段
[BITS 16]           ;BITS伪指令通知NASM编译器应为16位宽的处理器生成代码
;当NASM编译器处于16位宽状态下，使用32位宽数据指令需要加上指令前缀0x66；使用32位宽地址指令需要加上指令前缀0x67
Label_Start:
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ax,0x00
    mov ss,ax
    mov sp,0x7c00
;=======显示字符串：Start Loader......
;=======int 10h,AH=13h：显示字符串
	;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    mov bx,000fh        ;BH=页码，BL=字符属性
    mov dx,0200h        ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,12           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax           ;(ES:BP)=要显示的字符串的内存地址
    pop ax
    mov bp,StartLoaderMessage
    int 10h
    ;jmp $               ;调试用，屏幕显示P54 图3-7 Boot跳转至Loader
;=======访问A20快速门，开启A20地址线（最初的处理器只有20根地址线，使得处理器只能寻址1MB内的地址空间，超过1MB的也只有低20位有效）
;=======为保证硬件平台的向下兼容性，出现了一个控制开启或关闭1MB以上地址的开关
;=======当时的8042键盘控制器恰好有空闲的端口引脚（输出端口P2，引脚P21），从而使用此引脚为功能控制开关，即A20功能
;=======若A20引脚=0为低电平，则只有低20位地址有效，其他位均为0
;=======有多种开启A20功能的方法：1.操作键盘控制器，速度慢；2.使用I/O端口0x92（A20快速门），该端口有可能被其他设备占用；
;=======3.BIOS int15h的AX=2400子功能禁用A20、AX=2401开启A20、AX=2403查询A20状态；4.读0xEE端口开启A20、写0xEE端口禁止A20
    push ax
    in al,92h           ;从端口0x92读一个字节到AL寄存器
    or al,00000010b     ;AL=AL | 00000010，AL寄存器的第1位被置为1
    out 92h,al          ;将AL寄存器内容输出到0x92端口（0x92的A20引脚为高电平，可寻址1MB以上地址空间）
    pop ax
    cli                 ;关闭外部中断
    db 0x66             ;当NASM编译器处于16位宽状态下，使用32位宽数据指令需要加上指令前缀0x66
    lgdt [GdtPtr]       ;通过LGDT加载保护模式结构数据
    mov eax,cr0         
    or eax,1            
    mov cr0,eax         ;CR0寄存器的第0位被置为1，开启32bit保护模式
    mov ax,SelectorData32
    mov fs,ax           ;FS=AX=SelectorData32=LABEL_DESC_DATA32-LABEL_GDT
    ;目的是使FS段寄存器在实模式下的寻址能力超过1MB（即传说中的Big Real Mode模式），为之后的内核转存至高地址做准备
    mov eax,cr0
    and al,11111110b    ;CR0寄存器的第0位被重新置为0，关闭保护模式
    mov cr0,eax
    sti
    ;jmp $，调试用，在Bochs终端按下Ctrl+C进入DBG调试命令行，输入sreg查看当前段状态信息，验证FS段寄存器进入Big Real Mode
;=======从FAT12文件系统搜索引导加载程序kernel.bin
    mov word [SectorNo],SectorNumOfRootDirStart ;根目录的起始扇区号存放在DS:SectorNo处=19
;=======在根目录搜索与LoaderFileName标号相同的目录项
Label_Search_In_Root_Dir_Begin:
    ;初始时(DS:RootDirSizeForLoop)=RootDirSectors=根目录占用的扇区数14
    cmp word [RootDirSizeForLoop],0     ;判断当前是否已经到达根目录最后一个扇区
    jz Label_No_KernelBin               ;若为0则ZF标志位=1表示找不到目录项，跳转到Label_No_KernelBin作处理
    dec word [RootDirSizeForLoop]       ;根目录扇区数大小减少2B
    ;调用Func_ReadOneSector之前设置参数
    mov ax,00h                  
    mov es,ax                   ;ES=AX=0000H，设置内存缓冲区
    mov bx,8000h                ;ES:BX=08000H，读取数据存放在内存缓冲区的起始地址
    mov ax,[SectorNo]           ;AX=(DS:SectorNo)=待读取的磁盘根目录的起始扇区号
    mov cl,1                    ;CL=读入的扇区数量
    call Func_ReadOneSector     ;读入属于根目录的第一个扇区；call指令等同于push IP；jmp Func_ReadOneSector
    mov si,KernelFileName       ;将kernel.bin文件名标号所在地址放入si，源数据地址为DS:SI=KernelFileName标号地址
    mov di,8000h                ;要比对的FAT目录项名称数据地址为ES:DI=08000H
    cld                         ;因为后面用到lodsb指令（与DF标志位有关），所以清除DF标志位
    mov dx,10h                  ;DX=每个扇区可容纳FAT12目录项个数=512/32=16=0x10
;=======在第一个扇区内依据文件名“KERNEL  BIN”搜索每个FAT12目录项
Label_Search_For_KernelBin:
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
    mov si,KernelFileName
    jmp Label_Search_For_KernelBin
;=======前往根目录的下一个扇区继续寻找相应目录项
Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo],1
    jmp Label_Search_In_Root_Dir_Begin
;=======根目录下所有扇区都不包含KernelFileName标号的目录项，则查找失败，打印对应信息
Label_No_KernelBin:
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
    mov bx,008ch
    mov dx,0300h       ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,21          ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,NoKernelMessage
    int 10h
    jmp $              ;屏幕显示P49 图3-6 Boot错误效果图
;=======在根目录FAT表项中找到kernel.bin对应的目录项
Label_FileName_Found:
    mov ax,RootDirSectors       ;AX=RootDirSectors=根目录占用的扇区数14
    and di,0ffe0h               ;0000 1111 1111 1110 0000 & DI，保留DI的高11位
    add di,01ah                 ;DI=DI+0000 0001 1010=1111 1111 1111 1010
    mov cx,word [es:di]         ;CX=(ES:DI)=0
    push cx                     ;将CX=(ES:DI)保存进栈，下文即将使用CX
    add cx,ax                   ;CX=CX+AX（根目录占用的扇区数14）=14
    add cx,SectorBalance        ;CX=CX+SectorBalance（数据区起始扇区号17）=14+17=31
    mov eax,BaseTmpOfKernelAddr
    mov es,eax                  ;ES=EAX=BaseTmpOfKernelAddr 0x00
    mov bx,OffsetTmpOfKernelFile;BX=OffsetTmpOfKernelFile 0x7E00；ES:BX即为kernel.bin程序在内存中临时存储的起始地址
    mov ax,cx                   ;AX=CX（ES:DI）+SectorBalance（数据区起始扇区号17）
;=======将kernel.bin程序对应的全部磁盘簇读入内存
;=======int 10h,AH=0EH功能：在屏幕上显示一个字符
Label_Go_On_Loading_File:
    push ax                     
    push bx                     
    mov ah,0eh                  ;int 10h中断的主功能号0EH
    mov al,'.'                  ;AL=要显示的字符'.'
    mov bl,0fh                  ;BL=前景色设置（字体颜色设置）
    int 10h                     ;每读入一个磁盘簇（一个扇区），显示一个'.'
    pop bx
    pop ax
    mov cl,1                    ;以单个扇区方式读取loader.bin文件
    call Func_ReadOneSector     
    pop ax
    push cx
    push eax
    push fs
    push edi
    push ds
    push esi
    mov cx,200h                 ;CX=0x200，设置Label_Move_Kernel内核转存过程的循环次数
    mov ax,BaseOfKernelFile
    mov fs,ax                   ;FS=AX=BaseOfKernelFile 0x00=内核真正的起始物理地址
    mov edi,dword [OffsetOfKernelFileCount]     ;EDI=(DS:OffsetOfKernelFileCount)=起始时内核转存的目标地址空间偏移值
    mov ax,BaseTmpOfKernelAddr
    mov ds,ax                   ;DS=AX=BaseTmpOfKernelAddr 0x00=内核临时存储的物理地址
    mov esi,OffsetTmpOfKernelFile   ;ESI=OffsetTmpOfKernelFile 0x7E00
;=======内核转存过程，为避免发生错误，使用单字节复制当前读入的新磁盘簇
Label_Move_Kernel:
    ;因为mov指令的操作数不能同时为内存地址，所以要借助AL寄存器
    mov al,byte [ds:esi]        ;AL=(DS:ESI)，将存储在临时物理地址的kernel.bin文件的单个字节内容存放在AL
    ;(FS:EDI)=AL，将AL的单个字节存储到kernel.bin的真正物理地址，因为之前已经将FS设置成Big Real Mode
    mov byte [fs:edi],al        ;所以FS可以寻址0x10000之上的物理内存位置
    inc esi
    inc edi
    loop Label_Move_Kernel      ;继续循环，直到CX=0
    mov eax,0x1000  
    mov ds,eax                  ;DS=EAX=0x1000
    mov dword [OffsetOfKernelFileCount],edi;(DS:OffsetOfKernelFileCount)=EDI=结束时内核转存的目标地址空间偏移值
    pop esi
    pop ds
    pop edi
    pop fs
    pop eax
    pop cx
    call Fun_GetFATEntry        ;每读入一个扇区的数据就通过Fun_GetFATEntry取得下一个FAT表项
    cmp ax,0fffh                ;直到Fun_GetFATEntry返回的FAT表项值为0FFFH为止
    jz Label_File_Loaded        ;若AX=0FFFH则跳转至Label_File_Loaded标号处往下执行
    push ax
    mov dx,RootDirSectors       ;DX=RootDirSectors根目录占用的扇区数14
    add ax,dx                   ;AX=AX+DX
    add ax,SectorBalance        ;AX=AX+SectorBalance
    ;add bx,[BPB_BytesPerSec]    ;BX=BX+(DS:BPB_BytesPerSec)每个扇区占用字节数512B
    jmp Label_Go_On_Loading_File;继续读kernel.bin的下一个磁盘簇（当前FAT12文件系统的一个磁盘簇只包含一个扇区）
;======准备跳转至kernel.bin程序处执行
Label_File_Loaded:
    mov ax,0B800H
    mov gs,ax                   ;GS=AX=0B800H，B8000H～B8FFFH是16bit实模式下的显存空间
    mov ah,0FH                  ;AH=字符颜色属性；0000：黑底、1111：白字
    mov al,'G'                  ;AL=要显示的字符'G'
    mov [gs:((80*0+39)*2)],ax   ;在屏幕第0行，第39列显示
    ;jmp $                       ;调试用，屏幕显示P61 图3-8 在屏幕上显示字符'G'
;=======Loader加载完Kernel后软盘驱动器不再使用，以下子程序关闭软驱马达
KillMotor:
    push dx
    mov dx,03F2H    ;通过向I/O端口0x3F2写入控制命令实现
    ;第0、1位：00～11选择软盘驱动器A～D；第2位：0复位软盘驱动器，1允许软盘驱动器发送控制信息；第3位：0禁止DMA和中断请求，1允许DMA和中断请求
    ;第4～7位：控制软驱A～D的马达，1为启动，0为关闭
    mov al,0
    out dx,al       ;out指令的源操作数根据端口位宽选用AL，AX，EAX；目的操作数可以是立即数或DX寄存器
    pop dx
;=======int 10h子功能AH=13h：显示一行字符串
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
    mov bx,000fh
    mov dx,0400h       ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,24          ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax          ;ES:BP=要显示的字符串的内存地址
    pop ax
    mov bp,StartGetMemStructMessage ;BP=StartGetMemStructMessage
    int 10h
    mov ebx,0          ;EBX=0，调用int 15h前BX=0
    mov ax,0x00
    mov es,ax          ;ES=AX=0x0000
    mov di,MemoryStructBufferAddr   ;ES:DI=0x07E00
;=======获取物理地址空间信息（由一个结构体数组构成，包括可用物理内存地址空间、设备寄存器地址空间、内存空洞）
Label_Get_Mem_Struct:
    ;int 15h子功能号AH=E8H：获取物理地址空间信息，并将其保存在ES:DI=0x07E00地址处
    mov eax,0x0E820
    mov ecx,20
    mov edx,0x534D4150
    int 15h
    jc Label_Get_Mem_Struct_Fail    ;int 15h若使得标志寄存器EFLAGS的CF位=1，则获取失败
    add di,20
    inc dword [MemStructNumber]     ;(DS:MemStructNumber)++
    ;int 15h成功调用后，BX会被BIOS更新为一个非零值，作为下一次调用时获取下一个内存区域描述符的句柄
    ;当BX返回0时，表示已经遍历完所有的内存区域描述符
    cmp ebx,0
    jne Label_Get_Mem_Struct        ;EBX!=0则使得ZF!=0，代表获取物理地址信息失败，重新获取
    jmp Label_Get_Mem_Struct_OK     ;没有跳转到任何其他子过程，说明获取成功
;=======获取物理地址空间信息失败
Label_Get_Mem_Struct_Fail:
;=======int 10h子功能AH=13h：显示一行字符串
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov dword [MemStructNumber],0
    mov ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
    mov bx,008ch
    mov dx,0500h       ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,23          ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,GetMemStructErrMessage
    int 10h
    jmp $
;=======获取物理地址空间信息成功
Label_Get_Mem_Struct_OK:
;=======int 10h子功能AH=13h：显示一行字符串
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
    mov bx,000fh
    mov dx,0600h       ;DH=游标坐标列号，DL=游标坐标行号
    mov cx,29          ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
    push ax
    mov ax,ds
    mov es,ax
    pop ax
    mov bp,GetMemStructOKMessage
    int 10h
;=======获取SVGA VBE（VESA BIOS EXTENSION）显示模式
Label_Get_SVGA_VBE_Info:
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
	mov	ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
	mov	bx,000Fh
	mov	dx,0800h	    ;DH=游标坐标列号，DL=游标坐标行号
	mov	cx,23           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,StartGetSVGAVBEInfoMessage
	int	10h
    mov	ax,0x00
	mov	es,ax           ;ES=AX=0x0000，执行int 10h,AH=4FH子功能需要先设置ES：DI
	mov	di,0x8000       ;DI=0x8000；所以ES:DI=0x08000指向一个用于存放 VBE 控制器信息块的缓冲区（至少512字节）
    ;int 10h，AH=4FH，子功能指定要执行VESA BIOS EXTENSION
    ;AL=00h：获取VBE控制器信息；AL=01h：获取VBE模式信息；AL=02h：设置VBE视频模式；AL=03h：获取当前VBE视频模式。
    ;AL=04h：保存/恢复VBE状态；AL=05h：屏幕窗口控制；AL=06h：设置逻辑像素格式
	mov	ax,4F00h
	int	10h
    ;int 10h,AH=4FH输出：AX存放VBE状态（AH=00h表示成功，AH=01h表示失败），AL=4FH。
    ;ES:DI：缓冲区会被填充VBEControllerInfo结构体，包含显卡信息、支持的VBE模式列表等
	cmp	ax,004Fh
	jz	Label_Get_SVGA_VBE_Info_OK
;=======获取SVGA VBE（VESA BIOS EXTENSION）显示模式失败
Label_Get_SVGA_VBE_Info_Fail:
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov	ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
	mov	bx,008Ch
	mov	dx,0900h		;DH=游标坐标列号，DL=游标坐标行号
	mov	cx,23           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,GetSVGAVBEInfoErrMessage
	int	10h
	jmp	$
;=======获取SVGA VBE（VESA BIOS EXTENSION）显示模式成功
Label_Get_SVGA_VBE_Info_OK:
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
    mov	ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
	mov	bx,000Fh
	mov	dx,0A00h		;DH=游标坐标列号，DL=游标坐标行号
	mov	cx,29           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,GetSVGAVBEInfoOKMessage
	int	10h
;=======提示准备开始获取SVGA VBE Mode属性
    ;AL控制写入模式；AL=00h字符串属性由BL提供，光标位置不变；AL=01h字符串属性由BL提供，光标移动到字符串末端；
    ;AL=02h字符串属性由每个字符后的单个字节提供，光标位置不变
	mov	ax,1301h
    ;BH=页码，BL=字符属性，显存物理空间为0xB8000~0xBFFFF
    ;bit0～2为字体颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit3为字体亮度：0：字体正常亮度、1：字体高亮度
    ;bit4～6为背景颜色：0：黑、1：蓝、2：绿、3：青、4：红、5：紫、6：棕、7：白
    ;bit7为字体闪烁：0：不闪烁、1：字体闪烁
	mov	bx,000Fh        
	mov	dx,0C00h		;DH=游标坐标列号，DL=游标坐标行号
	mov	cx,24           ;CX=字符串长度；若AL=00h则长度以Byte为单位，若AL=02h则长度以Word为单位
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,StartGetSVGAModeInfoMessage
	int	10h
	mov	ax,0x00
	mov	es,ax
	mov	si,0x800e           ;ES:SI=0x0800E
	mov	esi,dword [es:si]   ;ESI=(ES:SI)=(0x0800E)
	mov	edi,0x8200          ;EDI=0x8200
;=======开始获取SVGA Mode Info
Label_Get_SVGA_Mode_Info:
	mov	cx,word [es:esi]
    push ax
	mov	ax,00h
	mov	al,ch
	call Label_DispAL
	mov	ax,00h
	mov	al,cl
	call Label_DispAL
	pop	ax
	cmp	cx,0FFFFh
	jz Label_Get_SVGA_Mode_Info_OK
	mov	ax,4F01h
	int	10h
	cmp	ax,004Fh
	jnz	Label_Get_SVGA_Mode_Info_FAIL
    inc	dword [SVGAModeCounter]
	add	esi,2
	add	edi,0x100
	jmp	Label_Get_SVGA_Mode_Info
;=======获取SVGA Mode Info失败
Label_Get_SVGA_Mode_Info_FAIL:
	mov	ax,1301h
	mov	bx,008Ch
	mov	dx,0D00h		
	mov	cx,24
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,GetSVGAModeInfoErrMessage
	int	10h
;=======获取SVGA Mode Info成功
Label_Get_SVGA_Mode_Info_OK:
	mov	ax,1301h
	mov	bx,000Fh
	mov	dx,0E00h		
	mov	cx,30
	push ax
	mov	ax,ds
	mov	es,ax
	pop	ax
	mov	bp,GetSVGAModeInfoOKMessage
	int	10h
    ;jmp $       ;调试用，屏幕显示P65 图3-9 Loader执行效果图
;=======开始设置VBE显示模式
Label_SET_SVGA_Mode_VESA_VBE:
    ;int 10h，AH=4FH，子功能指定要执行VESA BIOS EXTENSION
    ;AL=00h：获取VBE控制器信息；AL=01h：获取VBE模式信息；AL=02h：设置VBE视频模式；AL=03h：获取当前VBE视频模式。
    ;AL=04h：保存/恢复VBE状态；AL=05h：屏幕窗口控制；AL=06h：设置逻辑像素格式
    mov	ax,4F02h
    ;BX=想要设置的VBE视频模式的编号
	mov	bx,4180h	;有效的VBE显示模式代码0x180 or 0x143
	int 10h
    ;int 10h,AH=4FH输出：AX存放VBE状态（AH=00h表示成功，AH=01h表示失败），AL=4FH（AL!=4FH也是失败）
    ;ES:DI：缓冲区会被填充VBEControllerInfo结构体，包含显卡信息、支持的VBE模式列表等
	cmp	ax,004Fh
	jnz	Label_SET_SVGA_Mode_VESA_VBE_FAIL
;=======初始化GDT、IDT；从16bit实模式切换到32bit保护模式
Label_16bit_Switch_To_32bit:
;=======为保证代码在不同代Intel处理器上的兼容情况，建议遵循以下步骤进行模式切换
    cli                 ;1.cli禁止可屏蔽硬件中断，不可屏蔽中断NMI只能借助外部电路禁止
    db 0x66             ;当NASM编译器处于16位宽状态下，使用32位宽数据指令需要加上指令前缀0x66
    lgdt [GdtPtr]       ;2.实模式下lgdt将GDT的基地址和长度加载到GDTR
    ;db 0x66
    ;lidt [IDT_POINTER] ;3.实模式下lidt将IDT的基地址和长度加载到IDTR
    mov eax,cr0
    or eax,1
    mov cr0,eax         ;4.将CR0控制寄存器的PE标志位置为1即进入保护模式，可同时将PG标志位置为1开启分页模式
    ;5.PE标志位置为1后，因为代码段寄存器CS不能直接用赋值方法改变
    ;必须紧接着执行jmp far或者call far，可改变处理器执行流水线，使处理器加载保护模式代码段
    ;6.若开启分页机制（PG位=1），则mov cr0指令和jmp/call指令必须位于同一地址映射后的页面内
    ;jmp/call指令的目标地址无需进行同一性地址映射（线性地址与物理地址重合）
    jmp dword SelectorCode32:GO_TO_TMP_Protect
    ;7.如需使用LDT（多段式操作系统的局部描述符表），则需借助LLDT指令将GDT内的LDT段选择子加载到LDTR
    ;8.如需使用多任务机制或允许改变特权级，则必须在首次执行任务切换前，创建至少一个任务状态段TSS结构体和附加的TSS段描述符
    ;并使用LTR指令将一个TSS段描述符的段选择子加载至TR任务寄存器；处理器对TSS段结构体无特殊要求，可写的内存空间即可
    ;9.进入保护模式后，数据段寄存器仍保留实模式的数据，必须重新加载数据段选择子或使用jmp/call指令（第5步）
    ;10.保护模式下LIDT指令将IDT表基地址和长度加载到IDTR（选择在切换到保护模式后才加载是因为可以采用任务门）
    ;11.STI指令使能可屏蔽硬件中断，并执行必要的硬件操作使能NMI不可屏蔽中断
;=======设置VBE显示模式失败
Label_SET_SVGA_Mode_VESA_VBE_FAIL:
	jmp	$

[SECTION .s32]  ;SECTION伪指令追加定义一个名为.s32的段，说明此时代码在32位保护模式下运行
[BITS 32]       ;BITS伪指令通知NASM编译器应为32位宽的处理器生成代码
;=======暂时进入32位保护模式下进行操作
GO_TO_TMP_Protect:
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	ss,ax       ;DS=ES=FS=SS=0x10
	mov	esp,7E00h   ;ESP=0x00007E00
    ;jmp $   ;调试用（已成功），处理器暂停在此，Bochs虚拟机中输入Ctrl+C，然后再输入sreg查看当前段寄存器
	call support_long_mode  ;测试处理器是否支持64位长模式
    ;TEST指令执行op1和op2之间的按位逻辑AND运算。运算结果本身会被丢弃，但以下标志位会根据运算结果进行设置
    ;ZF：EAX=0则为1，EAX!=0则为0；SF：与结果最高位相同；PF：运算结果低8位包含偶数个1则为1，否则为0；CF、OF：为0；AF：未定义
	test eax,eax
	jz not_support_long_mode    ;若EAX=0，ZF=1则处理器不支持IA-32e
;=======1.为IA-32e 64bit长模式配置临时页目录项和页表项；页目录首地址是0x90000
	mov	dword [0x90000],0x91007     ;配置各级页表项：由页表起始地址和页属性组成
    mov dword [0x90004],0x00000
	mov	dword [0x90800],0x91007
    mov dword [0x90804],0x00000
	mov	dword [0x91000],0x92007
    mov dword [0x91004],0x00000
	mov	dword [0x92000],0x000083
    mov dword [0x92004],0x000000
	mov	dword [0x92008],0x200083
    mov dword [0x9200C],0x000000
	mov	dword [0x92010],0x400083
    mov dword [0x92014],0x000000
	mov	dword [0x92018],0x600083
    mov dword [0x9201C],0x000000
	mov	dword [0x92020],0x800083
    mov dword [0x92024],0x000000
	mov	dword [0x92028],0xa00083
    mov dword [0x9202C],0x000000
;=======2.重新加载全局描述符表GDT，并初始化大部分段寄存器
	db 0x66
	lgdt [GdtPtr64]
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax       ;DS=ES=FS=GS=SS=AX=0x10
	mov	esp,7E00h   ;ESP=0x7E00
    ;jmp $   ;调试用（已成功），Bochs虚拟机中输入Ctrl+C使处理器暂停在此，然后再输入sreg查看当前段寄存器
;=======3.通过置位CR4控制寄存器的PAE标志位，打开物理地址扩展功能PAE
	mov	eax,cr4
	bts	eax,5           ;CR4寄存器的第5位是PAE功能的标志位
	mov	cr4,eax
;=======4.将临时页目录的首地址设置到CR3控制寄存器中
	mov	eax,0x90000
	mov	cr3,eax
;=======5.通过置位IA32_EFER寄存器的LME标志位激活IA-32e 64bit长模式
    ;IA32_EFER位于MSR寄存器组内，第8位是LME标志位
	mov	ecx,0C0000080h  ;访问MSR前必须向ECX寄存器（64bit模式下RCX寄存器的高32位被忽略）传入寄存器地址
	rdmsr   ;读取目标MSR寄存器，是由EDX:EAX组成的64位寄存器代表；EDX保存MSR寄存器的高32位，EAX保存低32位
	bts	eax,8   ;EAX=IA32_EFER，将其第8位LME标志位置为1
	wrmsr   ;往目标MSR寄存器写入修改好的EAX
;=======6.同时置位CR0寄存器的PE标志位和PG标志位，开启分页机制
	mov	eax,	cr0
	bts	eax,	0
	bts	eax,	31
	mov	cr0,	eax
    ;至此处理器进入IA-32e模式，但是处理器目前正在执行保护模式的程序，该状态称为兼容模式
;=======7.需要一条跨段jmp/call指令将CS段寄存器的值更新为IA-32e的代码段描述符，则处理器真正运行在IA-32e模式
	jmp	SelectorCode64:OffsetOfKernelFile
;=======测试当前平台是否支持64位长模式
support_long_mode:
    ;因为CPUID指令的扩展功能项0x80000001的第29位指示处理器是否支持IA-32e模式，所以本段程序先检测当前处理器对CPUID的支持情况
    ;只有当CPUID的扩展功能号大于等于0x80000001时，才有可能支持64bit长模式
    ;CPUID指令无显式操作数，EAX=基础功能号/叶；ECX=扩展功能号（可选）
	mov	eax,0x80000000      ;EAX=0x80000000获取最大扩展功能号
	cpuid                   ;EAX、EBX、ECX、EDX存放处理器鉴定信息和机能信息
    ;输出EAX=处理器支持的最大扩展功能号 (Extended Function CPUID leaf)
    ;例如：如果返回0x80000008，表示支持从0x80000000到0x80000008的扩展功能号
	cmp	eax,0x80000001      ;比较EAX与0x80000001的大小，若EAX>=0x80000001则CF标志位为0
    ;SETNB指令根据CF标志位的状态来设置其操作数（一个字节长度，可以是8bit寄存器或内存单元），与SETAE和SETNC等效
	setnb al                ;若CF=0则AL=1；若CF=1则AL=0
	jb support_long_mode_done   ;若CF=1（即AL=0）则跳转至support_long_mode_done
	mov	eax,0x80000001      ;EAX=0x80000001获取扩展处理器信息和特性位
	cpuid
    ;EAX：保留/扩展处理器签名；EBX：保留；ECX：扩展特性标志位；EDX：扩展特性标志位
    ;BT指令（Bit Test）用于测试指定操作数中某个位的值，并将该位的值复制到进位标志（CF）中
	bt edx,29               ;测试位来源：EDX寄存器；测试位索引（立即数或16bit寄存器或32bit寄存器）：29（从0开始计数）
    ;SETC指令检查EFLAGS寄存器中的进位标志（CF）。如果CF为1（表示发生了进位或借位），SETC会将其目标字节操作数设置为1
    ;如果CF为0（表示没有发生进位或借位），SETC会将目标字节操作数设置为0
	setc al
;=======支持64位长模式
support_long_mode_done:
    ;MOVZX指令用于将一个较小尺寸的源操作数（可以是寄存器或内存地址）移动到一个较大尺寸的寄存器目标操作数
    ;并在高位用零进行填充（零扩展）
	movzx eax,al
	ret         ;返回GO_TO_TMP_Protect
;=======不支持64位长模式
not_support_long_mode:
	jmp	$

[SECTION .s16lib]   ;SECTION伪指令追加定义一个名为.s16lib的段，说明是16位实模式下的函数库
[BITS 16]
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
    mov bx,[BPB_BytesPerSec]    ;BX=(DS:BPB_BytesPerSec)每个扇区字节数
    div bx                      ;16位无符号数除法指令，DX:AX/BX=AX（FAT表项偏移扇区号）......DX（FAT表项偏移位置）
    push dx                     ;DX在Func_ReadOneSector子过程内被调用，先压栈保存
    mov bx,8000h
    add ax,SectorNumOfFAT1Start ;AX=AX+SectorNumOfFAT1Start FAT表1的起始扇区号
    mov cl,2                    ;CL=2，读入两个扇区
    call Func_ReadOneSector     ;解决FAT表项横跨两个扇区的问题
    pop dx                      
    add bx,dx                   ;BX=BX（8000h）+DX（FAT表项偏移位置）
    mov ax,[es:bx]              ;AX=(ES:BX)
    cmp byte [Odd],1            ;奇偶标志变量(DS:Odd)与1对比
    jnz Lable_Even_2            ;FAT目录项号为偶数跳转至Lable_Even_2处理
    shr ax,4                    ;进一步处理奇偶项错位问题，FAT目录项为奇数向右移动4位
Lable_Even_2:
    and ax,0fffh                ;AX=AX & 0000 1111 1111 1111
    pop bx
    pop es
    ret
;=======显示保存在AL寄存器内的十六进制数值，为显示一些查询到的物理地址空间信息
Label_DispAL:
    push ecx
    push edx
    push edi
    mov edi,[DisplayPosition]   ;EDI=(DS:DisplayPosition)=字符游标索引值（屏幕偏移值）
    mov ah,0FH                  ;AH=字体颜色属性值
    mov dl,al                   ;为先显示AL寄存器的高四位数据，先把AL的低四位数据保存至DL
    shr al,4                    ;AL的高四位数据右移至低四位
    mov ecx,2                   ;ECX=2设置loop .begin指令的段循环次数为2（先显示AL高四位，再显示AL低四位）
.begin:
    and al,0FH      ;AL=AL&0000 1111，只保留低四位（也即原来数据的高四位）
    cmp al,9        ;将原高四位数据与9比较
    ja .1           ;CF=0（AL>9）且ZF=0（AL!=9），则跳转至.1段执行
    add al,'0'      ;否则直接将其与字符'0'相加
    jmp .2
.1:
    sub al,0AH      ;之前已经判断出AL>9，所以AL=AL-10
    add al,'A'      ;AL=AL+'A'
.2:
    mov [gs:edi],ax ;(GS:DisplayPosition)显示字符内存空间0xB8000H=AX
    add edi,2       ;显示一个字符需要2Bytes
    mov al,dl       ;AL=DL=AL源数据的低四位，继续显示
    loop .begin
    mov [DisplayPosition],edi   ;(DS:DisplayPosition)=EDI
    pop edi
    pop edx
    pop ecx
    ret

;=======为IDT开辟内存空间，因为切换至保护模式前Loader.bin已使用cli指令关闭中断，进而不必完整初始化IDT
;=======只须有相应的结构体即可；若能保证模式切换过程中不产生异常，没有IDT也可以
IDT:
    times 0x50 dq 0
IDT_END:
IDT_POINTER:            ;与GdtPtr相似，都是指向结构体的指针
    dw IDT_END-IDT-1
    dd IDT

RootDirSizeForLoop dw RootDirSectors
SectorNo dw 0
Odd db 0
OffsetOfKernelFileCount dd OffsetOfKernelFile
MemStructNumber	dd 0
SVGAModeCounter	dd 0
DisplayPosition dd 0
;=======在屏幕上显示的字符串
StartLoaderMessage: db "Start Loader"
NoKernelMessage: db "ERROR:No KERNEL Found"
KernelFileName: db "KERNEL  BIN",0
StartGetMemStructMessage: db "Start Get Memory Struct."
GetMemStructErrMessage:	db "Get Memory Struct ERROR"
GetMemStructOKMessage: db "Get Memory Struct SUCCESSFUL!"
StartGetSVGAVBEInfoMessage: db "Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage: db "Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage: db "Get SVGA VBE Info SUCCESSFUL!"
StartGetSVGAModeInfoMessage: db "Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage: db "Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage: db "Get SVGA Mode Info SUCCESSFUL!"