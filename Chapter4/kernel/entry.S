#include "linkage.h"
//符号常量定义了各寄存器（程序执行现场）在栈中的保存顺序（基于栈顶指针的偏移值）
//异常/中断处理程序起始处必须保存被中断程序的执行现场，因为栈向下生长，则当前栈指针RSP+符号常量可取得程序执行现场的寄存器值
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
    //将栈指针向上移动16B，目的是弹出栈中的FUNC和ERRCODE
    addq $0x10,%rsp;
    iretq;      //可自行判断还原过程是否涉及特权级切换，如果是则将OLDSS、OLDRSP弹出栈
//目前只负责还原被中断程序的执行现场，异常的返回过程还可进行进程调度、进程信号处理等工作，后续会逐步加入新功能
ret_from_exception:
    //GET_CURRENT(%ebx)需要重写
ENTRY(ret_from_intr)
    jmp RESTORE_ALL
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
    movq ERRCODE(%rsp),%rsi     //RSI=基地址RSP+错误码ERRCODE偏移地址
    movq FUNC(%rsp),%rdx        //RDX=基地址RSP+异常处理函数FUNC偏移地址
    movq $0x10,%rdi
    movq %rdi,%ds
    movq %rdi,%es               //DS=ES=内核层数据段0x10
    movq %rsp,%rdi              //RDI=RSP=栈指针
    callq *%rdx                  //GAS汇编语言call/jmp指令的操作数前缀含有*，则表示目标地址为绝对地址
    jmp ret_from_exception      //异常处理程序执行结束，跳转至ret_from_exception模块还原被中断程序的执行现场
//#DE（除法）异常处理模块，属于内部中断
ENTRY(divide_error)
//程序执行现场的保存过程
    pushq $0        //因为#DE异常不会产生错误码，但为确保所有异常处理程序的寄存器压栈顺序一致，便向栈中压入0占位
    pushq %rax      
    leaq do_divide_error(%rip),%rax     //RAX=基地址RIP+偏移地址do_divide_error=do_divide_error函数的线性地址
    //xchgq指令将其两个操作数（源操作数和目的操作数）的内容进行互换
    xchgq %rax,(%rsp)   //do_divide_error函数的线性地址入栈，恢复RAX的值
    jmp error_code
//#NMI不可屏蔽中断不是异常，属于外部中断，不会生成错误码，与#DE异常处理过程相似
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
//#TS无效的TSS段异常
ENTRY(invalid_TSS)
    pushq %rax
    leaq do_invalid_TSS(%rip),%rax
    xchgq %rax,(%rsp)
    jmp error_code