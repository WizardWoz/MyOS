#include "linkage.h"
/*
  异常（内部中断）的分类：
  1.错误：可被修正，处理器将程序/任务的运行环境还原至异常发生前（栈中保存CS、EIP），并重新执行产生异常的指令（返回地址）
  2.陷阱：无需修正，处理器跳过产生异常的指令，继续执行之后的程序/任务（返回地址）
  3.终止：不可修正，无法准确提供产生异常的位置，不允许程序/任务继续执行（无返回地址）

  系统异常处理：
  1.处理器捕获到异常/中断时，会根据异常/中断向量号从中断描述符表IDT索引出对应的门描述符，读取门描述符保存的段选择子
  ，随后从GDT或LDT描述符表中检索出处理程序所在代码段，再根据门描述符记录的段内偏移量来确定中断/异常程序的入口地址；
  2.处理器在执行中断/异常处理程序时，会检测所在代码段的特权级，并与代码段寄存器的特权级进行比较
  (1)中断/异常处理程序的特权级>代码段寄存器的特权级，则会在中断/异常处理程序执行前切换栈空间，以下为栈空间切换过程
  处理器从TSS任务状态段中取出对应特权级的栈段选择子和栈指针，作为中断/异常处理程序的栈空间进行切换，
  自动将被中断程序的SS、ESP、EFLAGS、CS、EIP、错误码（如果有则在EIP之后入栈）压入中断/异常处理程序栈
  (2)中断/异常处理程序的特权级=代码段寄存器的特权级，则被中断程序与中断/异常处理程序共用同一个内存栈空间
  自动将被中断程序的EFLAGS、CS、EIP、错误码（如果有则在EIP之后入栈）压入中断/异常处理程序栈
  3.处理器必须借助iret指令才能从中断/异常处理程序返回，iret与ret极其相似，不过iret会还原之前保存的EFLAGS寄存器值，并且EFLAGS寄存器
  IOPL标志位只有在CPL=0时才可被还原；IF标志位只有在CPL<=IOPL时才能改变
  4.处理器通过中断门执行异常/中断处理程序，会在EFLAGS入栈后复位TF、VM、RF、NT标志位（IF=0即关闭单步调试）
  处理器通过陷阱门执行异常/中断处理程序，会在EFLAGS入栈后复位VM、RF、NT标志位（处理器没有关闭单步调试功能）
*/

//符号常量定义了各寄存器（程序执行现场）在栈中的保存顺序（基于栈顶指针的偏移值），以下一直到ERRCODE都需要手动保存
//异常/中断处理程序起始处必须保存被中断程序的执行现场，因为栈向下生长，由高地址——>低地址，则当前栈指针RSP+符号常量可取得程序执行现场的寄存器值
    R15=0x00
    R14=0x08
    R13=0x10
    R12=0x18
    R11=0x20
    R10=0x28
    R9=0x30
    R8=0x38
    RBX=0x40
    RCX=0x48
    RDX=0x50
    RSI=0x58
    RDI=0x60
    RBP=0x68
    DS=0x70
    ES=0x78
    RAX=0x80
    FUNC=0x88
    ERRCODE=0x90
    //以下是处理器自动保存的信息，无需手动保存；无中断特权级切换时只保存RFLAGS、CS、RIP到相同栈；
    //有中断特权级切换时，先切换至中断/异常程序栈空间，再保存SS、RSP、RFLAGS、CS、RIP
    RIP=0x98
    CS=0xa0
    RFLAGS=0xa8
    //有中断特权级切换时，切换不同程序的栈空间
    OLDRSP=0xb0
    OLDSS=0xb8
//异常/中断处理程序的返回模块，负责还原被中断程序的执行现场
RESTORE_ALL:
    popq %r15;      //若多条汇编语句在同一行，则 ; 是必要的
    popq %r14;
    popq %r13;
    popq %r12;
    popq %r11;
    popq %r10;
    popq %r9;
    popq %r8;
    popq %rbx;
    popq %rcx;
    popq %rdx;
    popq %rsi;
    popq %rdi;
    popq %rbp;
    //因为push CS/ES/DS/SS和pop DS/ES/SS是无效指令，所以使用popq %rax;movq %rax,%ds
    popq %rax;
    movq %rax,%ds;
    popq %rax;
    movq %rax,%es;
    popq %rax;          //此语句才是真正弹出RAX
    //将栈指针向上移动16B，目的是弹出栈中的FUNC和ERRCODE，因为iretq指令不会在异常返回过程中弹出FUNC和ERRCODE
    addq $0x10,%rsp;
    //iretq指令可自行弹出栈中存放的RIP~OLDSS
    iretq;      //可自行判断还原过程是否涉及特权级切换，如果是则将OLDSS、OLDRSP弹出栈

//内部异常处理完成，目前只负责还原被中断程序的执行现场，异常的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ret_from_exception:
    //GET_CURRENT(%ebx)需要重写
//外部中断处理完成，目前只负责还原被中断程序的执行现场，中断的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ENTRY(ret_from_intr)
    jmp RESTORE_ALL
/*
  调用程序借助call指令访问被调用程序的调用门，处理器将按照以下步骤为被调用程序准备执行环境
  1.处理器检查被调用程序代码段的特权级
  2.处理器内部保存SS（更高特权级程序）、ESP（更高特权级程序）、CS、RIP的副本，以备调用返回时使用
  3.依据目标代码段特权级，处理器从TSS结构提取相应的栈段选择子和栈基地址并加载到SS和ESP（更高特权级程序）
  4.将步骤2临时保存的SS、ESP副本存入目标程序栈空间（更高特权级程序）
  5.根据调用门描述符记录的参数个数，从调用者栈空间复制参数到目标程序栈
  6.将步骤2临时保存的CS、EIP副本存入目标程序栈空间
  7.将调用门描述符记录的目标代码段选择子和程序的起始地址加载到CS和EIP
  8.处理器在目标代码段特权级下执行程序

  被调用程序借助ret指令返回至调用程序，处理器将按照以下步骤还原调用者的执行环境
  1.处理器检查调用程序代码段的特权级
  2.还原被调用者栈空间的CS、EIP至调用者CS、EIP寄存器
  3.ret n：栈指针向上移动n个字节释放被调用者栈空间；若访问来自调用门，则同时释放调用者、被调用者栈空间（参数列表）
  4.还原被调用者栈空间的SS、ESP至调用者SS、ESP寄存器，使得栈空间切换回调用者；若访问来自调用门，此时已完成还原
  5.如果没有访问调用门，ret n使得栈指针向上移动n个字节释放调用者栈空间
  6.处理器继续执行调用者程序

  老版本Linux内核采用ret/iret指令从内核层跳转至应用层，因为该类指令执行速度慢，消耗处理器时钟周期数过多，Intel推出新的指令
  sysenter/sysexit实现快速系统调用
  1.整个调用过程不会执行数据压栈/出栈（如RIP、RSP寄存器值），免去了访问内存的时间消耗
  2.跨特权级跳转时不会对段描述符进行检测（只能是0特权级和3特权级之间相互转换），并且使用的段描述符皆位于同一描述符表
  并且各个描述符是相邻的。只有这样才能使处理器根据IA32_SYSENTER_CS_MSR寄存器值索引到段选择子
  3.只能为应用程序提供系统调用，无法在内核层执行系统调用；中断型系统调用可在任意权限下执行
*/
/*
  sysenter指令：从应用层（3特权级）快速进入内核层（0特权级），执行该指令前处理器必须为其提供0特权级的衔接程序和0特权级的栈空间（数据保存在MSR寄存器、通用寄存器）
  由操作系统确保各个段描述符的正确性，将使RFLAGS.IF标志位=0
  1.IA32_SYSENTER_CS（MSR寄存器组0x174地址）：是一个32位寄存器（值不能为NULL），用于索引0特权级下的代码段选择子和栈段选择子
  0特权级下的代码段选择子=IA32_SYSENTER_CS[15:0]；0特权级下的栈段选择子=IA32_SYSENTER_CS[15:0]+8
  2.IA32_SYSENTER_ESP（MSR寄存器组0x175地址）：保存Canonical型地址，将会被载入到RSP寄存器（保护模式下只载入低32位）
  3.IA32_SYSENTER_EIP（MSR寄存器组0x176地址）：保存Canonical型地址，将会被载入到RIP寄存器（保护模式下只载入低32位）
*/
//开始进行系统调用，利用sysenter从应用层进入内核层；
ENTRY(system_call)
    sti
    subq $0x38,%rsp
    cld
    pushq %rax
    movq %es,%rax
    pushq %rax
    movq %ds,%rax
    pushq %rax
    xorq %rax,%rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq $0x10,%rdx
    movq %rdx,%ds
    movq %rdx,%es
    movq %rsp,%rdi      //将当前栈指针作为参数传递给C语言system_call_function函数，指向struct pt_regs结构体首地址
    callq system_call_function

/*
  sysexit指令：从内核层（0特权级）快速返回应用层（3特权级）执行前处理器必须为其提供3特权级的衔接程序以及3特权级的栈空间（数据保存在MSR寄存器和通用寄存器中）
  由操作系统确保各个段描述符的正确性
  1.IA32_SYSENTER_CS（MSR寄存器组0x174地址）：是一个32位寄存器，用于索引3特权级下的代码段选择子和栈段选择子
  在IA-32e的64位模式下代码段选择子地址为IA32_SYSENTER_CS[15:0]+32，否则为IA32_SYSENTER_CS[15:0]+16；栈段选择子地址=代码段选择子地址+8
  2.RDX寄存器：保存着Canonical型地址（64位特有地址结构），执行指令时会载入RIP寄存器（用户程序的第一条指令地址）
  如果返回到非64位模式，只有低32位被装载到RIP寄存器中
  3.RCX寄存器：保存着Canonical型地址（64位特有地址结构），执行指令时会载入RSP寄存器（3特权级下的栈指针）
  如果返回到非64位模式，只有低32位被装载到RSP寄存器中
  4.sysexit指令不会从描述符表（GDT/LDT）中加载段描述符到CS和SS寄存器，取而代之的是向寄存器写入固定值；
  虽然执行速度快但不能保证段描述符正确性，必须由操作系统负责确保段描述符的正确性
*/
//系统调用处理完成，利用sysexit从内核层返回应用层；目前只负责恢复调用程序的执行现场
ENTRY(ret_system_call)
    movq %rax,0x80(%rsp)    //将系统调用的返回值更新至保存程序运行环境RAX寄存器的存储空间中
    //恢复应用程序的执行环境，进入内核层前应该对RDX和RCX进行特殊处理（在应用程序中实现）
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rax
    movq %rax,%ds
    popq %rax
    movq %rax,%es
    popq %rax       //真正弹出RAX
    //将栈指针向上移动56B，目的是弹出栈中的FUNC~OLDSS，因为sysexit指令不会在系统调用的返回过程中进行相关栈操作
    addq $0x38,%rsp
    //因为sysexit指令在64位模式下的默认操作数不是64位，如果要返回到64位模式下的应用层，必须加上0x48指令前缀表示使用64位操作数
    .byte 0x48
    sysexit

//进入异常/中断处理程序之前的公共部分：首先要保存各段寄存器和通用寄存器，然后保存错误码
error_code:
    pushq %rax
    movq %es,%rax
    pushq %rax
    movq %ds,%rax
    pushq %rax
    xorq %rax,%rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    cld
    movq ERRCODE(%rsp),%rsi     //RSI=基地址RSP+错误码ERRCODE偏移地址=error_code
    movq FUNC(%rsp),%rdx        //RDX=基地址RSP+异常处理函数FUNC偏移地址=C语言异常处理函数do_xxx入口地址
    movq $0x10,%rdi
    movq %rdi,%ds
    movq %rdi,%es               //DS=ES=内核层数据段0x10
    movq %rsp,%rdi              //RDI=RSP=栈指针
    //汇编语言调用C语言，遵守cdecl调用约定：参数从左向右顺序是RDI、RSI、RDX、RCX、R8、R9，剩余参数使用内存传递
    //GAS汇编语言call/jmp指令的操作数前缀含有*，则表示目标地址为绝对地址
    callq *%rdx                 //跳转至相应的异常处理函数继续执行
    jmp ret_from_exception      //异常处理程序执行结束，跳转至ret_from_exception模块还原被中断程序的执行现场
//向量号为0，#DE错误，无错误码 div或idiv指令触发除法错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(divide_error)
//程序执行现场的保存过程
    pushq $0        //因为#DE异常不会产生错误码，但为确保所有异常处理程序的寄存器压栈顺序一致，便向栈中压入0占位
    pushq %rax      
    leaq do_divide_error(%rip),%rax     //RAX=基地址RIP+偏移地址do_divide_error=do_divide_error函数的线性地址
    //xchgq指令将其两个操作数（源操作数和目的操作数）的内容进行互换
    xchgq %rax,(%rsp)   //do_divide_error函数的线性地址入栈，恢复RAX的值
    jmp error_code
//向量号为1，#DB错误/陷阱，无错误码 Intel处理器调试异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(debug)
	pushq $0
	pushq %rax
	leaq do_debug(%rip),%rax
	xchgq %rax,(%rsp)
	jmp	error_code
//向量号为2，#NMI内部中断，无错误码 不可屏蔽中断的中断处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(nmi)
    pushq %rax
    cld
    pushq %rax
    pushq %rax
    movq %es,%rax
    pushq %rax
    movq %ds,%rax
    pushq %rax
    xorq %rax,%rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq $0x10,%rdx
    movq %rdx,%ds
    movq %rdx,%es           //DS=ES=内核层数据段0x10
    //汇编语言调用C语言
	//cdecl调用约定：参数从左向右顺序是RDI、RSI、RDX、RCX、R8、R9，剩余参数使用内存传递
    movq $0,%rsi            
    movq %rsp,%rdi
    callq do_nmi
    jmp RESTORE_ALL
//向量号为3，#BP陷阱，无错误码 int3指令触发断点异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(int3)
	pushq	$0
	pushq	%rax
	leaq	do_int3(%rip),%rax
	xchgq	%rax,(%rsp)
	jmp	error_code
//向量号为4，#OF陷阱，无错误码 into指令触发的溢出异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(overflow)
	pushq	$0
	pushq	%rax
	leaq	do_overflow(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为5，#BR错误，无错误码 bound指令触发的越界异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(bounds)
	pushq	$0
	pushq	%rax
	leaq	do_bounds(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为6，#UD错误，无错误码 ud2指令或保留的机器码触发的无效/未定义机器码的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(undefined_opcode)
	pushq	$0
	pushq	%rax
	leaq	do_undefined_opcode(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为7，#NM错误，无错误码 浮点指令wait/fwait触发的设备异常（FPU不存在）的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(dev_not_available)	/* need rewrite */
	pushq	$0
	pushq	%rax
	leaq	do_dev_not_available(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为8，#DF终止，有错误码0 任何异常、NMI内部中断、INTR外部中断触发的双重错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(double_fault)
	pushq	%rax
	leaq	do_double_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为9，#-错误（保留），无错误码 浮点指令触发的协处理器段越界的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(coprocessor_segment_overrun)
	pushq	$0
	pushq	%rax
	leaq	do_coprocessor_segment_overrun(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为10，#TS错误，有错误码 访问TSS段或任务切换触发的无效的TSS段异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(invalid_TSS)
    pushq %rax
    leaq do_invalid_TSS(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code
//向量号为11，#NP错误，有错误码 加载段寄存器或访问系统段触发的段不存在的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(segment_not_present)
	pushq	%rax
	leaq	do_segment_not_present(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为12，#SS错误，有错误码 加载栈段寄存器SS或栈操作触发的SS段错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(stack_segment_fault)
	pushq	%rax
	leaq	do_stack_segment_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为13，#GP错误，有错误码 任何内存引用和保护检测触发的通用保护性异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(general_protection)
	pushq	%rax
	leaq	do_general_protection(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为14，#PF错误，有错误码 任何内存引用触发的页错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(page_fault)
    pushq %rax
    leaq do_page_fault(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code
//向量号为15，#-错误（Intel保留，请勿使用）无错误码

//向量号为16，#MF错误，无错误码   x87 FPU浮点指令或wait/fwait指令触发的x87 FPU计算错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(x87_FPU_error)
	pushq	$0
	pushq	%rax
	leaq	do_x87_FPU_error(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为17，#AC错误，有错误码0，引用内存中的任意数据触发的对齐检测错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(alignment_check)
	pushq	%rax
	leaq	do_alignment_check(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为18，#MC终止，无错误码（如果有错误码则和CPU类型有关）机器检测的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(machine_check)
	pushq	$0
	pushq	%rax
	leaq	do_machine_check(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为19，#XM错误，无错误码   SSE/SSE2/SSE3浮点指令触发的SIMD浮点异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(SIMD_exception)
	pushq	$0
	pushq	%rax
	leaq	do_SIMD_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为20，#VE错误，无错误码   违反EPT触发的虚拟化异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(virtualization_exception)
	pushq	$0
	pushq	%rax
	leaq	do_virtualization_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code