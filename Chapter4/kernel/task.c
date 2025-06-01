#include "task.h"
#include "printk.h"
#include "lib.h"
#include "gate.h"
#include "memory.h"
#include "ptrace.h"
#include "linkage.h"

/*
  函数：init进程实体的功能
  参数：
  1.unsigned long arg：创建进程需要的参数
  返回值：unsigned long，目前定为1

  从编程角度看，进程是由一系列维护程序运行的信息和若干组程序片段构成；init函数与C语言的main函数一样，经过编译器的编译生成
  若干程序片段并记录程序入口地址，操作系统为程序创建进程控制结构体（进程控制块PCB），会取得程序入口地址并从入口地址处开始执行
*/
unsigned long init(unsigned long arg)
{
	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);
	return 1;
}

/*
  函数：释放进程控制结构体（进程控制块PCB），因为PCB释放过程较复杂，暂时不实现而是打印一条日志信息证明do_exit函数被执行
  参数：
  1.unsigned long code：init进程执行完毕的返回代码
  返回值：unsigned long，暂未使用（do_exit用while(1);死循环）
*/
unsigned long do_exit(unsigned long code)
{
	color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",code);
	while (1)
	{

	}
}

/*
  函数：创建目的进程控制结构体（进程控制块PCB），并完成目的进程运行前的初始化工作
  参数：
  1.struct pt_regs *regs：指向为新进程准备执行现场的结构体指针
  2.unsigned long clone_flags：创建新进程需要的标志位（task.h)
  3.unsigned long stack_start：新进程内存栈起始地址
  4.unsigned long stack_size：新进程内存栈大小
  返回值：unsigned long，暂定为0
*/
unsigned long do_fork(struct pt_regs *regs,unsigned long clone_flags,unsigned long stack_start,unsigned long stack_size)
{
	//由于内核未实现内存分配功能，所以内存空间的使用暂时只能以物理页为单位
	struct task_struct *tsk=NULL;		//目的进程的进程控制结构体（进程控制块PCB）
	struct thread_struct *thd=NULL;		//目的进程的执行现场结构体
	struct Page *p=NULL;	//目的进程分配到的物理页结构体指针
	//为检测alloc_pages函数的执行效果，分配物理页的前后分别打印出物理内存页的位图映射信息
	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);
	p=alloc_pages(ZONE_NORMAL,1,PG_PTable_Mapped|PG_Active|PG_Kernel);
	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);
	//将当前进程控制块PCB的数据复制到新分配物理页中（物理页的线性地址起始处），进一步初始化相关成员变量
	//初始化PCB时，未曾分配struct mm_struct的存储空间，依然沿用全局变量init_mm，既然init目的进程此时还运行在内核层
	//那么在实现内存分配功能前，暂且不创建新的页目录表和页表
	tsk=(struct task_struct *)Phy_To_Virt(p->PHY_address);
	color_printk(WHITE,BLACK,"struct task_struct address:%#018lx\n",(unsigned long)tsk);
	memset(tsk,0,sizeof(*tsk));
	*tsk=*current;
	list_init(&tsk->list);
	list_add_to_before(&init_task_union.task.list,&tsk->list);//将目的进程链接进入进程队列
	tsk->pid++;
	tsk->state=TASK_UNINTERRUPTIBLE;	//目的进程转为就绪态
	thd=(struct thread_struct *)(tsk+1);
	tsk->thread=thd;
	//将源进程的进程控制块的执行现场复制到目的进程的内核层栈顶处
	memcpy(regs,(void *)((unsigned long)tsk+STACK_SIZE-sizeof(struct pt_regs)),sizeof(struct pt_regs));
	thd->rsp0=(unsigned long)tsk+STACK_SIZE;//目的进程的执行现场结构体内核栈基地址RSP0=目的进程PCB的内核层栈基地址
	thd->rip=regs->rip;		//如果目的进程是内核级线程，那么执行现场结构体RIP=目的进程入口地址kernel_thread_func
	thd->rsp=(unsigned long)tsk+STACK_SIZE-sizeof(struct pt_regs);//目的进程的执行现场结构体RSP=目的进程PCB的内核层栈顶
	//判断目的进程的进程标志，确定运行在内核层空间还是应用层空间
	if (!(tsk->flags&PF_KTHREAD))
	{
		//如果目的进程不是内核级线程，则将进程执行入口地址设置在ret_from_intr
		thd->rip=regs->rip=(unsigned long)ret_from_intr;
	}
	tsk->state=TASK_RUNNING;	//将目标进程设置成运行态
	return 0;
}

/*
  函数：负责还原进程执行现场、运行进程、退出进程
  参数：void，无
  返回值：void，无

  由于init进程运行在内核层空间，因此init进程在执行init函数前会先执行kernel_thread_func模块
*/
extern void kernel_thread_func(void);
//RSP正指向当前进程的内核层栈顶地址，此时栈顶位于栈基地址向下偏移struct pt_regs regs结构体处
__asm__ (
	"kernel_thread_func: \n\t"
	"popq %r15 \n\t"
	"popq %r14 \n\t"
	"popq %r13 \n\t"
	"popq %r12 \n\t"
	"popq %r11 \n\t"
	"popq %r10 \n\t"
	"popq %r9 \n\t"
	"popq %r8 \n\t"
	"popq %rbx \n\t"
	"popq %rcx \n\t"
	"popq %rdx \n\t"
	"popq %rsi \n\t"
	"popq %rdi \n\t"
	"popq %rbp \n\t"
	"popq %rax \n\t"
	"movq %rax,%ds \n\t"
	"popq %rax \n\t"
	"movq %rax,%es \n\t"
	"popq %rax \n\t"
	"addq $0x38,%rsp \n\t"
	"movq %rdx,%rdi \n\t"	//RDI=kernel_thread函数中设置RDX=进程创建者传入的参数unsigned long arg
	"callq *%rbx \n\t"		//kernel_thread函数中设置RBX=程序入口地址unsigned long (*fn)，启动init进程
	"movq %rax,%rdi \n\t"	//RDI=RAX=init进程退出的返回值（暂定为1）
	"callq do_exit \n\t"	//调用do_exit函数，证明init进程已退出
);

/*
  函数：创建struct pt_regs regs结构体，为最终调用do_fork()函数创建进程准备数据
  参数：
  1.unsigned long (*fn)(unsigned long)：指向目标进程的指针（函数指针）
  2.unsigned long arg：创建进程需要的参数
  3.unsigned long flags：创建进程需要的标志位（task.h)
  返回值：int，调用do_fork()函数的返回值
*/
int kernel_thread(unsigned long (*fn)(unsigned long),unsigned long arg,unsigned long flags)
{
    struct pt_regs regs;			//struct pt_regs regs结构体变量为新进程init准备执行现场的数据
    memset(&regs,0,sizeof(regs));	//将regs结构体变量空间全部置0
    regs.rbx=(unsigned long)fn;		//RBX=程序入口地址
    regs.rdx=(unsigned long)arg;	//RDX=进程创建者传入的参数
    regs.ds=KERNEL_DS;
    regs.es=KERNEL_DS;
    regs.cs=KERNEL_CS;
    regs.ss=KERNEL_DS;
    regs.rflags=(1<<9);
    regs.rip=(unsigned long)kernel_thread_func;//RIP=引导程序kernel_thread_func，会在目标程序init执行前运行
    return do_fork(&regs,flags,0,0);	//
}

/*
  函数：完成进程切换的后续工作，设置CPU 0核上运行的目的任务（进程）的TSS结构体
  参数：
  1.struct task_struct *prev：指向当前执行的原进程的进程控制结构体（进程控制块PCB）的指针
  2.struct task_struct *next：指向准备切换的目的进程的进程控制结构体（进程控制块PCB）的指针
  返回值：void，无

  因为进入内核层时已将进程执行现场（所有通用寄存器值）保存，所以进程切换过程不涉及保存/还原通用寄存器，仅需对RIP、RSP进行设置
*/
inline void __switch_to(struct task_struct *prev,struct task_struct *next)
{
    //首先将next进程的内核层栈基地址设置到TSS
    init_tss[0].rsp0=next->thread->rsp0;
    set_tss64(init_tss[0].rsp0,init_tss[0].rsp1,init_tss[0].rsp2,init_tss[0].ist1,init_tss[0].ist2,
    init_tss[0].ist3,init_tss[0].ist4,init_tss[0].ist5,init_tss[0].ist6,init_tss[0].ist7);
    //保存prev进程的FS和GS段寄存器值
    __asm__ __volatile__("movq %%fs,%0 \n\t":"=a"(prev->thread->fs));
    __asm__ __volatile__("movq %%gs,%0 \n\t":"=a"(prev->thread->gs));
    //还原next进程的FS和GS段寄存器值
    __asm__ __volatile__("movq %0,%%fs \n\t"::"a"(next->thread->fs));
    __asm__ __volatile__("movq %0,%%gs \n\t"::"a"(next->thread->gs));
    //打印prev和next进程的内核层栈基地址
    color_printk(WHITE,BLACK,"prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);
    color_printk(WHITE,BLACK,"next->thread->rsp0:%#018lx\n",next->thread->rsp0);
}

/*
  函数：补充进程控制结构体（进程控制块PCB）中未赋值的成员变量，并为其设置内核层栈基地址（位于TSS结构体内）
  参数：无
  返回值：void，无
*/
void task_init()
{
    //此时处理器已经运行在第一个进程（main.c的Start_Kernel）中，但此前进程结构体尚未初始化完毕，所以应补完进程PCB未赋值的成员变量
    struct task_struct *p=NULL;		//目的进程的进程控制结构体（进程控制块PCB）指针
    init_mm.pgd=(pml4t_t *)Global_CR3;
    init_mm.start_code=memory_management_struct.start_code;
    init_mm.end_code=memory_management_struct.end_code;
    init_mm.start_data=(unsigned long)&_data;
    init_mm.end_data=memory_management_struct.end_data;
    init_mm.start_rodata=(unsigned long)&_rodata;
    init_mm.end_rodata=(unsigned long)&_erodata;
    init_mm.start_brk=0;
    init_mm.end_brk=memory_management_struct.end_brk;
	//系统第一个进程会协助操作系统完成一些初始化任务，进程执行完毕后进入就绪态，在系统没有可运行进程时休眠处理器
	//达到省电的目的，所以系统的第一个进程不存在应用层空间
    init_mm.start_stack=_stack_start;   //记录系统第一个进程的内核层栈基地址
    //初始化进程运行现场，初始化TSS结构体
    set_tss64(init_thread.rsp0,init_tss[0].rsp1,init_tss[0].rsp2,init_tss[0].ist1,init_tss[0].ist2,
    init_tss[0].ist3,init_tss[0].ist4,init_tss[0].ist5,init_tss[0].ist6,init_tss[0].ist7);
    init_tss[0].rsp0=init_thread.rsp0;
	//为系统创建第二个进程：init进程
    list_init(&init_task_union.task.list);
    kernel_thread(init,10,CLONE_FS|CLONE_FILES|CLONE_SIGNAL);	
	init_task_union.task.state=TASK_RUNNING;
	//取得init进程PCB之后，方可调用switch_to模块切换至init内核线程
	p=container_of(list_next(&current->list),struct task_struct,list);
	//操作系统切换至init进程，处理器开始执行init进程，init进程执行init函数前会先执行kernel_thread_func模块
	switch_to(current,p);
}