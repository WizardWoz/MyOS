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
LABEL_DESC_CODE32: dd 0x0000FFFF,0x00CF9A00
LABEL_DESC_DATA32: dd 0x0000FFFF,0x00CF9200
GdtLen equ $-LABEL_GDT
GdtPtr dw GdtLen-1          ;dw会向dd对齐（因为已经进入32位模式）
       dd LABEL_GDT         ;所以临时GDT表一共占用32Bytes
;SelectorCode32、SelectorData32是两个LDT段选择子，是程序局部段描述符在GDT表中的索引号
SelectorCode32 equ LABEL_DESC_CODE32-LABEL_GDT
SelectorData32 equ LABEL_DESC_DATA32-LABEL_GDT
    
[SECTION .s16]      ;SECTION伪指令追加定义一个名为.s16的段
[BITS 16]           ;BITS伪指令通知NASM编译器应为16位宽的处理器生成代码
;当NASM编译器处于16位宽状态下，使用32位宽数据指令需要加上指令前缀0x66；使用32位宽地址指令需要加上指令前缀0x67
Label_Start:
    mov ax,cs
    mov ds,ax
    mov es,ax       ;AX=DS=ES=CS=0x10000
    mov ax,0x00
    mov ss,ax       ;SS=AX=0x0000
    mov sp,0x7c00   ;SP=0x7C00
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
    mov fs,ax           ;FS=AX=SS:SelectorData32
    mov eax,cr0         ;目的是使FS段寄存器在实模式下的寻址能力超过1MB，即传说中的Big Real Mode模式
    and al,11111110b    ;CR0寄存器的第0位被重新置为0，关闭保护模式
    mov cr0,eax
    sti
    ;jmp $，调试用，在Bochs终端按下Ctrl+C进入DBG调试命令行，输入sreg查看当前段状态信息，验证FS段寄存器进入Big Real Mode
;=======从FAT12文件系统搜索引导加载程序kernel.bin
    ;x86汇编语言中，[]内没有指定段寄存器。默认使用DS；SS是push、pop、call、ret等指令使用的内存栈寄存器
    mov word [SectorNo],SectorNumOfRootDirStart ;根目录的起始扇区号存放在DS:SectorNo处=19
;=======在根目录搜索与LoaderFileName标号相同的目录项
Label_Search_In_Root_Dir_Begin:
    ;初始时(DS:RootDirSizeForLoop)=(DS:RootDirSectors)=根目录占用的扇区数14
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
    mov dx,0100h       ;DH=游标坐标列号，DL=游标坐标行号
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
    mov cl,1                    ;读取loader.bin文件的第1个扇区
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
    mov edi,dword [OffsetOfKernelFileCount];EDI=(DS:OffsetOfKernelFileCount)=起始时内核转存的目标地址空间偏移值
    mov ax,BaseTmpOfKernelAddr
    mov ds,ax                   ;DS=AX=BaseTmpOfKernelAddr 0x00=内核临时存储的物理地址
    mov esi,OffsetTmpOfKernelFile   ;ESI=OffsetTmpOfKernelFile 0x7E00
;=======内核转存过程，为避免发生错误，使用单字节复制当前读入的新磁盘簇
Label_Move_Kernel:
    ;因为mov指令的操作数不能同时为内存地址，所以要借助AL寄存器
    mov al,byte [ds:esi]        ;AL=(DS:ESI)，将存储在临时物理地址的kernel.bin文件的单个字节内容存放在AL
    mov byte [fs:edi],al        ;(FS:EDI)=AL，将AL的单个字节存储到kernel.bin的真正物理地址
    inc esi
    inc edi
    loop Label_Move_Kernel      ;继续循环，直到CX=0
    mov eax,0x1000  
    mov ds,eax                  ;DS=EAX=0x1000
    mov dword [OffsetOfKernelFileCount],edi     ;(DS:OffsetOfKernelFileCount)=EDI=结束时内核转存的目标地址空间偏移值
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
    mov dx,RootDirSectors       ;DX=RootDirSectors 根目录占用的扇区数14
    add ax,dx                   ;AX=AX+DX
    add ax,SectorBalance        ;AX=AX+SectorBalance
    add bx,[BPB_BytesPerSec]    ;BX=BX+(DS:BPB_BytesPerSec) 每个扇区占用字节数512B
    jmp Label_Go_On_Loading_File;继续读kernel.bin的下一个磁盘簇（当前FAT12文件系统的一个磁盘簇只包含一个扇区）
;======准备跳转至kernel.bin程序处执行
Label_File_Loaded:
    mov ax,0B800H
    mov gs,ax                   ;GS=AX=0B800H，B8000H～B8FFFH是16bit实模式下的显存空间
    mov ah,0FH                  ;AH=字符颜色属性；0000：黑底、1111：白字
    mov al,'G'                  ;AL=要显示的字符'G'
    mov [gs:((80*0+39)*2)],ax   ;在屏幕第0行，第39列显示
    jmp $                       ;调试用，屏幕显示P61 图3-8 在屏幕上显示字符'G'

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
    mov dl,[BS_DrvNum]      ;DL=驱动器号（如果操作的是硬盘驱动器，bit 7必须被置位）
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
    mov byte [Odd],0    ;将奇偶标志变量(DS:Odd)置为0
    mov bx,3            ;因为每个FAT表项占12 bit，1.5Byte，3Byte存放2个FAT表项，所以目录项号具有奇偶性
    mul bx              ;16位无符号数乘法指令，AX=FAT表项号，BX=3；AX*BX=DX:AX，高16位在DX，低16位在AX
    mov bx,2            ;所以将FAT表乘3除2（扩大1.5倍），来判断余数奇偶性并保存在DS:Odd中，奇数为1，偶数为0
    div bx              ;16位无符号数除法指令，DX:AX/BX=AX......DX
    cmp dx,0            ;判断余数是否为0
    jz Lable_Even       ;余数为0，ZF=1，跳转到Lable_Even
    mov byte [Odd],1    ;将奇偶标志变量(DS:Odd)置为1
;=======FAT表项号为偶数
Lable_Even:
    xor dx,dx                   ;将DX寄存器清零
    mov bx,[BPB_BytesPerSec]    ;BX=(DS:BPB_BytesPerSec)每个扇区字节数
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

RootDirSizeForLoop dw RootDirSectors
SectorNo dw 0
Odd db 0
OffsetOfKernelFileCount dd OffsetOfKernelFile
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