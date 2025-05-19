#ifndef __TASK_H__
#define __TASK_H__

#include "lib.h"
#include "memory.h"

struct mm_struct
{
    /* data */
};

struct thread_struct
{
    /* data */
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

//程序进程控制结构体（进程控制块PCB）
struct task_struct
{
    struct List list;       //双向链表，连接各进程控制结构体（进程控制块PCB）
    volatile long state;    //进程状态：运行态
    unsigned long flags;
    struct mm_struct *mm;
    struct thread_struct *thread;
    unsigned long addr_limit;//
    long pid;
    long counter;
    long signal;
    long priority;
};

#endif