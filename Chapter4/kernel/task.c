#include "task.h"
#include "printk.h"
#include "lib.h"
#include "gate.h"
#include "memory.h"
#include "ptrace.h"
#include "linkage.h"

extern void kernel_thread_func(void);
__asm__ (
	
);

/*
  函数：创建struct pt_regs regs结构体，为最终调用do_fork()函数创建进程准备数据
  参数：
  1.unsigned long (*fn)(unsigned long)：指向目标进程的指针
  2.unsigned long arg：创建进程需要的参数
  3.unsigned long flags：
  返回值：int，调用do_fork()函数的返回值
*/
int kernel_thread(unsigned long (*fn)(unsigned long),unsigned long arg,unsigned long flags)
{
    struct pt_regs regs;
    memset(&regs,0,sizeof(regs));
    regs.rbx=(unsigned long)fn;
    regs.rdx=(unsigned long)arg;
    regs.ds=KERNEL_DS;
    regs.es=KERNEL_DS;
    regs.cs=KERNEL_CS;
    regs.ss=KERNEL_DS;
    regs.rflags=(1<<9);
    regs.rip=(unsigned long)kernel_thread_func;
    return do_fork(&regs,flags,0,0);
}

/*
  函数：完成进程切换的后续工作，设置CPU 0核上运行的目的任务（进程）的TSS结构体
  参数：
  1.struct task_struct *prev：指向当前执行的原进程的进程控制结构体（进程控制块PCB）的指针
  2.struct task_struct *next：指向准备切换的目的进程的进程控制结构体（进程控制块PCB）的指针
  返回值：void，无
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
    //此时处理器已经运行在第一个进程中
    struct task_struct *p=NULL;
    init_mm.pgd=(pml4t_t *)Global_CR3;
    init_mm.start_code=memory_management_struct.start_code;
    init_mm.end_code=memory_management_struct.end_code;
    init_mm.start_data=(unsigned long)&_data;
    init_mm.end_data=memory_management_struct.end_data;
    init_mm.start_rodata=(unsigned long)&_rodata;
    init_mm.end_rodata=(unsigned long)&_erodata;
    init_mm.start_brk=0;
    init_mm.end_brk=memory_management_struct.end_brk;
    init_mm.start_stack=_stack_start;   //记录系统第一个进程的内核层栈基地址
    //初始化进程运行现场，初始化TSS结构体
    set_tss64(init_thread.rsp0,init_tss[0].rsp1,init_tss[0].rsp2,init_tss[0].ist1,init_tss[0].ist2,
    init_tss[0].ist3,init_tss[0].ist4,init_tss[0].ist5,init_tss[0].ist6,init_tss[0].ist7);
    init_tss[0].rsp0=init_thread.rsp0;

    list_init(&init_task_union.task.list);
    ker
}
