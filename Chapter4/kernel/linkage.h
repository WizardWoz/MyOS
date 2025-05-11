#ifndef __LINKAGE_H__
#define __LINKAGE_H__

#define L1_CACHE_BYTES 32
//Linux 2.4.0内核的traps.c文件使用asmlinkage宏声明各异常处理模块的入口标识符（如：nmi、page_fault等）
//是为了通知编译器在执行异常处理模块时不得使用寄存器传参方式；但在编译64位程序时，GCC编译器只能使用寄存器传参方式，因此该宏功能无效
//regparm(0)限制可使用寄存器传参的个数为0
#define asmlinkage __attribute__((regparm(0)))	

#define ____cacheline_aligned __attribute__((__aligned__(L1_CACHE_BYTES)))

#define SYMBOL_NAME(X)	X

#define SYMBOL_NAME_STR(X)	#X

#define SYMBOL_NAME_LABEL(X) X##:

#define ENTRY(name)		\
.global	SYMBOL_NAME(name);	\
SYMBOL_NAME_LABEL(name)

#endif