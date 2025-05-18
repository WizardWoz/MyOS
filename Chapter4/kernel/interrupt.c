#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"

/*SAVE_ALL汇编宏模块：负责保存当前通用寄存器状态，与entry.S的error_code模块类似，中断处理程序的调用入口均指向同一中断处理函数do_IRQ，
  触发中断时不会压入错误码（内部错误/陷阱/终止才有），所以此处不能复用error_code模块

  将内联汇编代码定义为C宏，就形成了一个“汇编宏模块”，SAVE_ALL汇编宏模块将被应用在宏函数Build_IRQ中
*/
#define SAVE_ALL                \
    "cld; \n\t"                 \
    "pushq %rax; \n\t"          \
    "pushq %rax; \n\t"          \
    "movq %es,%rax; \n\t"       \
    "pushq %rax; \n\t"          \
    "movq %ds,%rax; \n\t"       \
    "pushq %rax; \n\t"          \
    "xorq %rax,%rax; \n\t"      \
    "pushq %rbp; \n\t"          \
    "pushq %rdi; \n\t"          \
    "pushq %rsi; \n\t"          \
    "pushq %rdx; \n\t"          \
    "pushq %rcx; \n\t"          \
    "pushq %rbx; \n\t"          \
    "pushq %r8; \n\t"           \
    "pushq %r9; \n\t"           \
    "pushq %r10; \n\t"          \
    "pushq %r11; \n\t"          \
    "pushq %r12; \n\t"          \
    "pushq %r13; \n\t"          \
    "pushq %r14; \n\t"          \
    "pushq %r15; \n\t"          \
    "movq $0x10,%rdx; \n\t"     \
    "movq %rdx,%ds; \n\t"       \
    "movq %rdx,%es; \n\t"       

/*符号粘接操作符##：用于连接两个宏值，在宏展开过程中会将操作符两边的内容连接起来，组成一个完整的内容
  例如：Build_IRQ(0x20)宏的void IRQ_NAME(nr)部分逐渐展开：IRQ_NAME(0x20)=>IRQ_NAME2(IRQ0x20)=>IRQ0x20_interrupt(void)

  预处理操作符#：将其后的内容强制转换成字符串
  例如：SYMBOL_NAME_STR(IRQ)#nr"_interrupt: => IRQ0x20_interrupt:
*/

#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##NR)

/*宏函数：定义中断处理程序的入口部分，类似entry.S的ENTRY(异常处理函数名)
  参数：
  1.nr：十六进制整数类型的中断向量号，例：nr=0x20

  指令部分：（__asm__内嵌汇编指令无__volatile__修饰，所以指令部分可能会被编译器优化）
  SYMBOL_NAME_STR(IRQ)#nr"_interrupt://宏展开后变为IRQ0x20_interrupt:，相当于生成了外部中断处理程序的入口标号（地址）
  pushq $0x00       //因为外部中断不会产生错误码，但为确保所有中断处理程序的寄存器压栈顺序一致，便向栈中压入0占位
  SAVE_ALL          //汇编宏模块，负责保存当前通用寄存器状态
  //汇编语言调用C语言，按cdecl调用约定调用do_IRQ(unsigned long regs,unsigned long nr)，将参数从左向右存入各寄存器
  movq %rsp,%rdi    //RDI=RSP=栈指针
  leaq ret_from_intr(%rip),%rax     //RAX=基地址RIP+偏移地址ret_from_intr（位于entry.S）=中断处理程序完成后的返回地址
  pushq %rax        //将ret_from_intr函数中断处理程序完成后的返回地址压入中断处理程序栈（具体声明及定义位于trap.h和trap.c），RAX作为
  movq $"#nr",%rsi  //RSI=中断向量号
  jmp do_IRQ        //跳转至do_IRQ
*/
#define Build_IRQ(nr)           \
void IRQ_NAME(nr);              \
__asm__ (   SYMBOL_NAME_STR(IRQ)#nr"_interrupt: \n\t"  \
            "pushq $0x00 \n\t"                          \
            SAVE_ALL                                    \
            "movq %rsp,%rdi \n\t"                       \
            "leaq ret_from_intr(%rip),%rax \n\t"        \
            "pushq %rax \n\t"                           \
            "movq $"#nr",%rsi \n\t"                     \
            "jmp do_IRQ \n\t"                           \
);

/*
  声明了24个中断处理程序的入口代码片段，类似entry.S中的ENTRY(异常处理函数名)
*/
Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)

//定义函数指针数组，每个元素都指向由宏函数Build_IRQ定义的一个中断处理函数入口
void (* interrupt[24])(void)=
{
    IRQ0x20_interrupt,IRQ0x21_interrupt,IRQ0x22_interrupt,IRQ0x23_interrupt,IRQ0x24_interrupt,IRQ0x25_interrupt,
    IRQ0x26_interrupt,IRQ0x27_interrupt,IRQ0x28_interrupt,IRQ0x29_interrupt,IRQ0x30_interrupt,IRQ0x31_interrupt,
    IRQ0x32_interrupt,IRQ0x33_interrupt,IRQ0x34_interrupt,IRQ0x35_interrupt,IRQ0x36_interrupt,IRQ0x37_interrupt,
};

/*
  8259A PIC可编程中断控制器（一般采用两片8259A芯片级联组成PC的中断处理硬件系统）：单核处理机时代的中断控制器，
  多核处理机时代使用APIC高级可编程中断控制器（8259A的内部结构请看408复习PPT）
  一个8259A包含两组寄存器：
  一、初始化命令字寄存器组ICW（8bit）：主芯片ICW1映射到0x20端口，ICW2、3、4映射到0x21端口；从芯片ICW1映射到0xA0端口，ICW2、3、4映射到0xA1端口
  1.ICW1：（主从一致，均初始化为0x11）
  第5~7位：对于PC机必须为0
  第4位：对于ICW必须为1
  第3位：触发模式，现已忽略，必须为0
  第2位：忽略，必须为0
  第1位：单片8259A=1；级联8259A=0
  第0位：使用ICW4=1；不使用ICW4=0
  2.ICW2：（主芯片初始化为0x20；从芯片初始化为0x28）
  第3~7位：中断向量号
  第0~2位：必须为0
  3.ICW3：（主、从芯片不一致，分开讨论）
  主芯片第0~7位：某个位为1代表该引脚级联从芯片，为0无从芯片
  从芯片第3~7位：必须为0；从芯片第0~2位（0~7号引脚）：表示连接到主芯片的引脚号
  4.ICW4：（主从一致，均初始化为0x01）
  第5~7位：必须为0
  第4位：SFNM模式=1，FNM模式=0
  第2~3位：无缓冲模式=00；从芯片缓冲模式=10；主芯片缓冲模式=11
  第1位：AEOI模式=1，EOI模式=0
  第0位：8086/8088模式=1，MCS 80/85模式=0
  二、操作控制字寄存器组OCW（8bit）：主芯片OCW1映射到0x21端口，OCW2、3映射到0x20端口；从芯片OCW1映射到0xA1端口，OCW2、3映射到0xA0端口
  1.OCW1：（主从芯片一致）
  第0~7位：屏蔽IRQ n号中断请求=1，允许IRQ n号请求=0
  2.OCW2：（主从芯片一致）
  第7位：优先级循环标志
  第6位：特殊设定标志
  第5位：非自动结束标志
  第3~4位：必须为0
  第0~2位：优先级设定
  3.OCW3：（主从芯片一致）
  第7位：必须为0
  第5~6位：开启特殊屏蔽=11，关闭特殊屏蔽=10
  第4位：必须为0
  第3位：必须为1
  第2位：有轮询=1，无轮询=0
  第0~1位：读IRR寄存器=10，读ISR寄存器=11
*/

void init_interrupt()
{
    int i;
    for ( i = 32; i < 56; i++)
    {
        set_intr_gate(i,2,interrupt[i-32]);
    }
    color_printk(RED,BLACK,"8259A init \n");

    io_out8(0x20,0x11);     //主8259A芯片的ICW1，固定初始化为0x11
    io_out8(0x21,0x20);     //主8259A芯片的ICW2，固定初始化为0x20
    io_out8(0x21,0x04);     //主8259A芯片的ICW3，固定初始化为0x20
}