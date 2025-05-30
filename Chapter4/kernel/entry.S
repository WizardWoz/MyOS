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
    R15=0x00        //低地址处
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
    RIP=0x98
    CS=0xa0
    RFLAGS=0xa8
    //有中断特权级切换时，处理器先切换至中断/异常程序栈空间，再自动保存SS、RSP
    OLDRSP=0xb0
    OLDSS=0xb8      //高地址处
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
//内部异常处理完成，目前只负责还原被中断程序的执行现场，异常的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ret_from_exception:
    //GET_CURRENT(%ebx)需要重写
//外部中断处理完成，目前只负责还原被中断程序的执行现场，中断的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ENTRY(ret_from_intr)
    jmp RESTORE_ALL
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

//向量号为2，#NMI外部中断，无错误码 不可屏蔽中断的中断处理函数入口（具体声明及定义位于trap.h和trap.c）
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

//向量号为10，#TS错误，有错误码 访问TSS段或任务切换触发的无效的TSS段异常的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(invalid_TSS)
    pushq %rax
    leaq do_invalid_TSS(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code

//向量号为14，#PF错误，有错误码 任何内存引用触发的页错误的异常处理函数入口（具体声明及定义位于trap.h和trap.c）
ENTRY(page_fault)
    pushq %rax
    leaq do_page_fault(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code