#ifndef __TASK_H__
#define __TASK_H__

#include "lib.h"
#include "memory.h"
#include "cpu.h"
#include "ptrace.h"

#define KERNEL_CS 0x08			//内核程序段选择子
#define KERNEL_DS 0x10			//内核数据段选择子
#define CLONE_FS (1<<0)			//标志位：克隆
#define CLONE_FILES (1<<1)		//标志位：克隆
#define CLONE_SIGNAL (1<<2)		//标志位：克隆信号量=4
#define STACK_SIZE 32768            //表示进程的内核栈空间和struct task_struct结构体占用存储空间总量为32768B（32KB）

#define TASK_RUNNING (1<<0)         //进程运行态=1
#define TASK_INTERRUPTIBLE (1<<1)   //进程阻塞态=2
#define TASK_UNINTERRUPTIBLE (1<<2) //进程就绪态=4
#define TASK_ZOMBIE (1<<3)          //进程挂起态=8
#define TASK_STOPPED (1<<4)         //进程终止态=16

#define PF_KTHREAD (1<<0)           //进程标志：内核级线程=1

extern char _text;		// Kernel.lds链接脚本将_text放在线性地址0xFFFF800000100000，使得_text位于内核程序的代码段起始地址
extern char _etext;		//进程代码段结束地址
extern char _data;		//进程数据段起始地址
extern char _edata;		//进程数据段结束地址
extern char _rodata;	//进程只读数据段起始地址
extern char _erodata;	//进程只读数据段结束地址
extern char _bss;		//进程.bss段起始地址
extern char _ebss;		//进程.bss段结束地址
extern char _end;		//进程程序结束地址
extern unsigned long _stack_start;	//进程栈起始地址，在head.S中作为汇编语句的标号入口Entry(_stack_start)
extern void ret_from_intr();	//从外部中断返回，在entry.S中作为汇编语句的标号入口Entry(ret_from_intr)

/*
  内存空间分布结构体：描述进程的页表结构和各程序段信息（页目录基地址、代码段、数据段、只读数据段、应用层栈地址等）
*/
struct mm_struct
{
    pml4t_t *pgd;//pml4t_t是typedef unsigned long pml4t; 保存CR3控制寄存器值（页目录基地址和页表属性的组合值）
    unsigned long start_code,end_code;      //代码段空间
    unsigned long start_data,end_data;      //数据段空间
    unsigned long start_rodata,end_rodata;  //只读数据段空间
    unsigned long start_brk,end_brk;        //动态内存分配区（堆区域）
    unsigned long start_stack;              //应用层栈基地址
};

/*
  当进程发生调度切换时，保存执行现场的寄存器值，以备再次执行时使用
*/
struct thread_struct
{
    unsigned long rsp0;         //内核栈基地址（位于任务状态段TSS）
    unsigned long rip;          //内核层代码指针
    unsigned long rsp;          //内核层当前栈指针
    unsigned long fs;           //FS段寄存器
    unsigned long gs;           //GS段寄存器
    unsigned long cr2;          //CR2控制寄存器
    unsigned long trap_nr;      //产生异常的异常号
    unsigned long error_code;   //异常的错误码
};

/*
  一、C语言volatile类型限定符（volatile：易变的）
  1.告诉编译器不要对变量的访问进行某些优化，例：阻止缓存变量值于寄存器、阻止指令重排、阻止移除看似无用的代码
  2.确保直接内存访问：每次对volatile变量的引用都会产生一次实际的内存访问（读或写）
  3.当变量的值可能被当前代码范围之外的因素改变时，就需要使用volatile，例：
  内存映射的硬件寄存器，硬件寄存器的值可以由硬件自身改变，或者写入硬件寄存器会触发硬件行为；
  由中断服务程序（ISR）修改的全局变量，全局变量在主程序中被读取，并且在中断服务程序中被修改；
  当多个线程共享一个全局变量时，如果一个线程修改了它，其他线程应该能看到这个修改
  4.局限性：volatile 本身并不能保证原子性或线程间的同步。它只处理编译器优化和内存可见性问题（确保从内存读写）对于复杂的操作（如 i++，它至少包含读-改-写三个步骤），
  或者需要互斥访问的场景，仍然需要使用互斥锁（mutexes）、信号量（semaphores）或其他原子操作（如 C11中的 _Atomic 类型或特定平台的原子API）

  二、如何使用volatile：volatile 关键字可以用于任何数据类型，包括基本类型、指针和结构体
  1.volatile int * volatile p：p是一个volatile指针，它指向一个volatile整型变量
  2.一个变量可以同时是const和volatile，例：volatile const int *mmio_status_reg
  const: 程序不应该试图通过这个指针修改这个内存位置的值（例如，它是一个只读的状态寄存器）
  volatile: 这个内存位置的值可能在程序控制之外被改变（例如，由硬件更新），所以每次读取都必须从内存进行
*/

/*
  程序进程控制结构体（进程控制块PCB）：用于记录进程的资源使用情况（软件/硬件资源）和运行态信息等
*/
struct task_struct
{
    struct List list;       //双向链表，连接各进程控制结构体（进程控制块PCB）
    volatile long state;    //进程状态：就绪态、运行态、阻塞态、可中断态等等
    unsigned long flags;    //进程标志：进程、用户级线程、内核级线程
    struct mm_struct *mm;   //内存空间分布结构体，记录内存页表和程序段信息
    struct thread_struct *thread;   //进程切换时保留的状态信息（执行现场的寄存器值）
    //进程地址空间范围：0x0000 0000 0000 0000~0x0000 7FFFF FFFF FFFF是应用层；0xFFFF 8000 0000 0000~0xFFFF FFFF FFFF FFFF是内核层
    unsigned long addr_limit;
    long pid;               //进程ID号
    long counter;           //进程可用时间片
    long signal;            //进程拥有的信号量
    long priority;          //进程优先级
};

/*
  进程内核层栈空间实现：借鉴Linux内核的设计思想，把进程控制结构体struct_task_struct与进程的内核层栈空间融为一体
  低地址处存放struct task_struct进程控制块PCB，高地址处作为进程的内核层栈空间
  Intel i386处理器架构的Linux内核中，进程默认使用8KB内核层栈空间；由于64位处理器寄存器位宽扩大一倍，相应栈空间也必须扩大，
  不清楚应该扩大多少算合理，所以暂时将其设定为32KB，等到存储空间不足时再扩容

  此联合体（使用__attribute__属性修饰）共占用32KB存储空间，并将这段空间按8B进行对齐，实际上该联合体的起始地址必须按照32KB字节对齐
  __attribute__((aligned(N))) 用于变量或类型，指定其最小对齐字节数。N必须是2的幂。这对于需要特定内存对齐以提高性能或满足硬件要求的场景非常有用。
*/
union task_union
{
    struct task_struct task;        //进程控制结构体（进程控制块PCB）1KB
	//STACKSIZE=32768；sizeof(unsigned long)=8；进程内核层栈空间=32768/8=4096，又因为每个元素是unsigned long类型，所以总大小为32KB
    unsigned long stack[STACK_SIZE/sizeof(unsigned long)];
}__attribute__((aligned(8)));

struct mm_struct init_mm;	//全局变量，新进程内存空间分布结构体
struct thread_struct init_thread;	//全局变量，新进程执行现场的寄存器值

/*
  宏函数：INIT_TASK，将union task_union实例化成全局变量init_task_union，并将其作为操作系统的第一个进程
  参数：
  1.tsk：struct task_struct task，进程控制结构体（进程控制块PCB）

  INIT_TASK没有初始化所属的进程双向链表
  进程状态：.state(volatile long state)=TASK_UNINTERRUPTIBLE（进程就绪态=4）
  进程标志：.flags(unsigned long flags)=PF_KTHREAD（内核级线程=1）
  进程内存空间分布结构体：.mm=声明的全局结构体struct mm_struct init_mm（还未初始化）
  进程运行现场：.thread=声明的全局结构体struct thread_struct init_thread（还未初始化）
  进程地址空间范围：.addr_limit=0xFFFF800000000000，位于内核层
  进程ID号：.pid=0，第一个进程（内核级线程）
  进程可用时间片：.counter=1，初始时每次调度只能运行一个CPU时间片
  进程拥有的信号量：.signal=0，初始时只有0个信号量
  进程优先级：.priority=0，最高优先级
*/
#define INIT_TASK(tsk)              \
{                                   \
    .state=TASK_UNINTERRUPTIBLE,    \
    .flags=PF_KTHREAD,              \
    .mm=&init_mm,                   \
    .thread=&init_thread,           \
    .addr_limit=0xFFFF800000000000, \
    .pid=0,                         \
    .counter=1,                     \
    .signal=0,                      \
    .priority=0                     \
}

/*
  全局变量union task_union init_task_union使用__attribute__((__section__(".data.init_task")))修饰
  将该全局变量连接到一个特殊的程序段内。在链接脚本Kernel.lds中为这个特别的程序段规划了地址空间
*/
union task_union init_task_union __attribute__((__section__(".data.init_task")))={
    INIT_TASK(init_task_union.task)
};

/*
  进程控制结构体（进程控制块PCB）*init_task指针数组是为各处理器创建的初始进程控制结构体，
  目前只有数组的第0个元素投入使用，剩余成员将在多核处理器初始化后予以创建
*/
struct task_struct *init_task[NR_CPUS]={&init_task_union.task,0};
struct mm_struct init_mm={0};		//全局struct mm_struct内存空间分布结构体（task.h中初始化为0）
//全局struct thread_struct保存进程运行现场结构体
struct thread_struct init_thread=
{
	//内核栈基地址（位于任务状态段TSS）=全局变量init_task_union的stack数组变量首地址+4096
	.rsp0=(unsigned long)(init_task_union.stack+STACK_SIZE/sizeof(unsigned long)),
	//内核层代码指针=全局变量init_task_union的stack数组变量首地址+4096
	.rsp=(unsigned long)(init_task_union.stack+STACK_SIZE/sizeof(unsigned long)),
	.fs=KERNEL_DS,
	.gs=KERNEL_DS,	//FS=GS=KERNEL_DS=0x10
	.cr2=0,			//CR2控制寄存器=0
	.trap_nr=0,		//产生异常的异常号=0
	.error_code=0	//异常的错误码=0
};

/*
  IA-32e（64位长模式）的TSS结构（在head.S中的内核数据段中有定义.globl TSS64_Table）
  __attribute__((packed)) 用于结构体或联合体，告诉编译器尽可能地压缩其成员，减少内存占用，不进行字节对齐填充。
*/
struct tss_struct
{
	unsigned int reserved0;			//第一个4B保留字段
	unsigned long rsp0;				//栈顶指针rsp0
	unsigned long rsp1;				//栈顶指针rsp1
	unsigned long rsp2;				//栈顶指针rsp2
	unsigned long reserved1;		//第二个8B保留字段
	unsigned long ist1;					
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;		//第三个8B保留字段
	unsigned short reserved3;		//第四个2B保留字段
	unsigned short iomapbaseaddr;	//I/O映射的地址字段
}__attribute__((packed));

/*
  宏函数：初始化任务状态段TSS结构
  
  四个保留字段均设置为0：.reserved0=0；.reserved1=0；.reserved2=0；.reserved3=0
  .rsp0、.rsp1、.rsp2内核栈基地址（位于任务状态段TSS）=全局变量init_task_union的stack数组变量首地址+4096
*/
#define INIT_TSS					\
{	.reserved0=0,					\
	.rsp0=(unsigned long)(init_task_union.stack+STACK_SIZE/sizeof(unsigned long)),	\
	.rsp1=(unsigned long)(init_task_union.stack+STACK_SIZE/sizeof(unsigned long)),	\
	.rsp2=(unsigned long)(init_task_union.stack+STACK_SIZE/sizeof(unsigned long)),	\
	.reserved1=0,					\
	.ist1=0xFFFF800000007C00,		\
	.ist2=0xFFFF800000007C00,		\
	.ist3=0xFFFF800000007C00,		\
	.ist4=0xFFFF800000007C00,		\
	.ist5=0xFFFF800000007C00,		\
	.ist6=0xFFFF800000007C00,		\
	.ist7=0xFFFF800000007C00,		\
	.reserved2=0,					\
	.reserved3=0,					\
	.iomapbaseaddr=0				\
}

/*
  使用了GNU C的一个扩展功能，称为“指定初始化范围”（Designated Initializer Range）或“范围指定符”（Range Designator）。
  [first ... last]=value这种语法表示将数组中从索引first到索引last（包含first和last）的所有元素都初始化为value。
  将struct tss_struct 结构体数组的每个TSS结构体成员都使用INIT_TSS宏函数初始化
*/
struct tss_struct init_tss[NR_CPUS]={[0 ... NR_CPUS-1] =INIT_TSS};

/*
  函数：借鉴Linux源码，用于获得当前进程的struct task_struct结构体（借助Kernel.lds的32KB对齐技巧）
  参数：无
  返回值：struct task_struct *，指向当前进程的struct task_struct结构体的指针

  指令部分：
  andq %%rsp,%0	//将栈指针寄存器RSP与0xFFFF FFFF FFFF 8000进行逻辑与，结果是当前进程struct task_struct结构体的基地址
  输出部分：相关指令执行后，将结果写入任意通用寄存器，并转存至struct task_struct *current指针
  输入部分：所有指令执行前，指令需要使用的寄存器被初始化为(~32767UL)=0xFFFF FFFF FFFF 8000
  损坏描述：无
*/
inline struct task_struct *get_current()
{
	struct task_struct *current=NULL;		//指向当前struct task_struct结构体的*current指针
	__asm__ __volatile__ ("andq %%rsp,%0 \n\t":"=r"(current):"0"(~32767UL));
	return current;
}

#define current get_current()	//宏定义，current宏即调用get_current()函数

/*
  宏函数：借鉴Linux源码，用于获得当前struct task_struct结构体（借助Kernel.lds的32KB对齐技巧）与get_current()函数实现的功能相同
  
  指令部分：
  movq %rsp,%rbx	//RBX=栈指针寄存器RSP
  andq $-32768,%rbx	//$-32768=0xFFFF FFFF FFFF 8000（补码表示），RBX与0xFFFF FFFF FFFF 8000进行逻辑与，
  结果是当前进程struct task_struct结构体的基地址
*/
#define GET_CURRENT				\
	"movq %rsp,%rbx \n\t"		\
	"andq $-32768,%rbx \n\t"	\

/*
  宏函数：进程切换过程
  参数：
  1.prev：指向当前执行的原进程的进程控制结构体（进程控制块PCB）struct task_struct prev的指针
  2.next：指向准备切换的目的进程的进程控制结构体（进程控制块PCB）struct task_struct next的指针
  应用程序进入内核层时已经将进程的执行现场（所有通用寄存器值）保存，所以进程切换过程并不涉及保存/还原通用寄存器，
  只需保存/还原RIP、RSP寄存器

  指令部分：
  pushq %%rbp		//将当前进程的栈基地址RBP入栈
  pushq %%rax		//将RAX入栈，因为后续指令暂时会使用RAX
  movq %%rsp,%0		//prev指针指向的struct task_struct进程控制结构体的栈指针存放到当前进程栈空间%0=prev->thread->rsp
  movq %2,%%rsp		//栈指针RSP替换成准备切换到的目的进程的栈指针%2=next->thread->rsp
  leaq 1f(%%rip),%%rax	//跳转地址RAX=基地址RIP+偏移地址1f（向后寻找标号为1的指令）
  movq %%rax,%1		//跳转地址RAX存放到%1=prev->thread->rip
  pushq %3			//将目的进程的指令指针寄存器%3=next->thread->rip入栈
  //使用cdecl函数调用约定：RDI、RSI分别保存宏参数prev和next代表的进程控制结构体，跳转至C语言函数__switch_to处继续执行，完成进程切换后续工作
  jmp __switch_to
  1:
  popq %%rax		//从__switch_to返回，弹出进程切换前准备工作压入栈的RAX
  popq %%rbp		//从__switch_to返回，弹出进程切换前准备工作压入栈的RBP

  输出部分：相关指令执行后，将结果暂存至内存栈区，并转移到prev->thread->rsp、prev->thread->rip变量
  输入部分：所有指令执行前，使用next->thread->rsp、next->thread->rip初始化内存栈区；
  使用prev、next指针初始化RDI、RSI（遵循cdecl调用约定调用C语言函数__switch_to）
  损坏描述：指令执行可能影响到相关内存区域，使用memory声明
*/
#define switch_to(prev,next)        \
do									\
{									\
	__asm__ __volatile__(			\
		"pushq %%rbp \n\t"			\
		"pushq %%rax \n\t"			\
		"movq %%rsp,%0 \n\t"		\
		"movq %2,%%rsp \n\t"		\
		"leaq 1f(%%rip),%%rax \n\t"	\
		"movq %%rax,%1 \n\t"		\
		"pushq %3 \n\t"				\
		"jmp __switch_to \n\t"		\
		"1: \n\t"					\
		"popq %%rax \n\t"			\
		"popq %%rbp \n\t"			\
		:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)					\
		:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
		:"memory"					\
	);								\
} while (0)

void task_init();

#endif