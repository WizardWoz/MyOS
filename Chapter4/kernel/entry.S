#include "linkage.h"

//符号常量定义了各寄存器（程序执行现场）在栈中的保存顺序（基于栈顶指针的偏移值）
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
    //无中断特权级切换时也可使用
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
    iretq;      //可自行判断还原过程是否涉及特权级切换，如果是则将OLDSS、OLDRSP弹出栈
//目前只负责还原被中断程序的执行现场，异常的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ret_from_exception:
    //GET_CURRENT(%ebx)需要重写
ENTRY(ret_from_intr)
    jmp RESTORE_ALL
//进入异常/中断处理程序，首先要保存各段寄存器和通用寄存器；然后保存错误码
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
//向量号为0，#DE错误，无错误码 div或idiv指令触发除法错误的异常处理函数入口
ENTRY(divide_error)
//程序执行现场的保存过程
    pushq $0        //因为#DE异常不会产生错误码，但为确保所有异常处理程序的寄存器压栈顺序一致，便向栈中压入0占位
    pushq %rax      
    leaq do_divide_error(%rip),%rax     //RAX=基地址RIP+偏移地址do_divide_error=do_divide_error函数的线性地址
    //xchgq指令将其两个操作数（源操作数和目的操作数）的内容进行互换
    xchgq %rax,(%rsp)   //do_divide_error函数的线性地址入栈，恢复RAX的值
    jmp error_code
//向量号为1，#DB错误/陷阱，无错误码 Intel处理器调试异常的异常处理函数入口
ENTRY(debug)
	pushq $0
	pushq %rax
	leaq do_debug(%rip),%rax
	xchgq %rax,(%rsp)
	jmp	error_code
//向量号为2，#NMI内部中断，无错误码 不可屏蔽中断的中断处理函数入口
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
//向量号为3，#BP陷阱，无错误码 int3指令触发断点异常的异常处理函数入口
ENTRY(int3)
	pushq	$0
	pushq	%rax
	leaq	do_int3(%rip),%rax
	xchgq	%rax,(%rsp)
	jmp	error_code
//向量号为4，#OF陷阱，无错误码 into指令触发的溢出异常的异常处理函数入口
ENTRY(overflow)
	pushq	$0
	pushq	%rax
	leaq	do_overflow(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为5，#BR错误，无错误码 bound指令触发的越界异常的异常处理函数入口
ENTRY(bounds)
	pushq	$0
	pushq	%rax
	leaq	do_bounds(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为6，#UD错误，无错误码 ud2指令或保留的机器码触发的无效/未定义机器码的异常处理函数入口
ENTRY(undefined_opcode)
	pushq	$0
	pushq	%rax
	leaq	do_undefined_opcode(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为7，#NM错误，无错误码 浮点指令wait/fwait触发的设备异常（FPU不存在）的异常处理函数入口
ENTRY(dev_not_available)	/* need rewrite */
	pushq	$0
	pushq	%rax
	leaq	do_dev_not_available(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为8，#DF终止，有错误码0 任何异常、NMI内部中断、INTR外部中断触发的双重错误的异常处理函数入口
ENTRY(double_fault)
	pushq	%rax
	leaq	do_double_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为9，#-错误（保留），无错误码 浮点指令触发的协处理器段越界的异常处理函数入口
ENTRY(coprocessor_segment_overrun)
	pushq	$0
	pushq	%rax
	leaq	do_coprocessor_segment_overrun(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为10，#TS错误，有错误码 访问TSS段或任务切换触发的无效的TSS段异常的异常处理函数入口
ENTRY(invalid_TSS)
    pushq %rax
    leaq do_invalid_TSS(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code
//向量号为11，#NP错误，有错误码 加载段寄存器或访问系统段触发的段不存在的异常处理函数入口
ENTRY(segment_not_present)
	pushq	%rax
	leaq	do_segment_not_present(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为12，#SS错误，有错误码 加载栈段寄存器SS或栈操作触发的SS段错误的异常处理函数入口
ENTRY(stack_segment_fault)
	pushq	%rax
	leaq	do_stack_segment_fault(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为13，#GP错误，有错误码 任何内存引用和保护检测触发的通用保护性异常的异常处理函数入口
ENTRY(general_protection)
	pushq	%rax
	leaq	do_general_protection(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为14，#PF错误，有错误码 任何内存引用触发的页错误的异常处理函数入口
ENTRY(page_fault)
    pushq %rax
    leaq do_page_fault(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code
//向量号为15，#-错误（Intel保留，请勿使用）无错误码

//向量号为16，#MF错误，无错误码   x87 FPU浮点指令或wait/fwait指令触发的x87 FPU计算错误的异常处理函数入口
ENTRY(x87_FPU_error)
	pushq	$0
	pushq	%rax
	leaq	do_x87_FPU_error(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为17，#AC错误，有错误码0   引用内存中的任意数据触发的对齐检测错误的异常处理函数入口
ENTRY(alignment_check)
	pushq	%rax
	leaq	do_alignment_check(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为18，#MC终止，无错误码（如果有错误码则和CPU类型有关）   机器检测的异常处理函数入口
ENTRY(machine_check)
	pushq	$0
	pushq	%rax
	leaq	do_machine_check(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为19，#XM错误，无错误码   SSE/SSE2/SSE3浮点指令触发的SIMD浮点异常的异常处理函数入口
ENTRY(SIMD_exception)
	pushq	$0
	pushq	%rax
	leaq	do_SIMD_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code
//向量号为20，#VE错误，无错误码   违反EPT触发的虚拟化异常的异常处理函数入口
ENTRY(virtualization_exception)
	pushq	$0
	pushq	%rax
	leaq	do_virtualization_exception(%rip),	%rax
	xchgq	%rax,	(%rsp)
	jmp	error_code