#ifndef __LINKAGE_H__
#define __LINKAGE_H__

#define L1_CACHE_BYTES 32
/*
  Linux 2.4.0内核的traps.c文件使用asmlinkage宏声明各异常处理模块的入口标识符（如：nmi、page_fault等）
  是为了通知编译器在执行异常处理模块时不得使用寄存器传参方式；但在编译64位程序时，GCC编译器只能使用寄存器传参方式，因此该宏功能无效
  regparm(0)限制可使用寄存器传参的个数为0，该函数的所有参数都应该通过栈来传递，而不是通过寄存器
*/
#define asmlinkage __attribute__((regparm(0)))

#define ____cacheline_aligned __attribute__((__aligned__(L1_CACHE_BYTES)))

#define SYMBOL_NAME(X) X
//预处理操作符#：将其后的内容强制转换成字符串（例：interrupt.c中Build_IRQ宏的应用）
#define SYMBOL_NAME_STR(X) #X
/*
  宏函数：将标号名与:组合起来，当前宏展开后即可得到X: 完整标号语句
  参数：
  1.X：标号名称（没有带:号）

  符号粘接操作符##：用于连接两个宏值，在宏展开过程中会将操作符两边的内容连接起来，组成一个完整的内容
*/
#define SYMBOL_NAME_LABEL(X) X##:
/*
  宏函数：取代.global name以及name:语句
  参数：
  1.name：标号名称（没有带:号）
*/
#define ENTRY(name)            \
    .global SYMBOL_NAME(name); \
    SYMBOL_NAME_LABEL(name)

#endif