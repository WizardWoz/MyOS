/*
  在很多操作系统开发场景中，C语言无法完全代替汇编语言，例如：操作某些特殊寄存器，某些IO端口，或对性能要求极为苛刻的场景
  此时我们必须要在C语言中内嵌汇编语言满足上述要求
*/
#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

/*
  函数：求字符串长度
  参数：
  1.char* String：所指向字符串的首地址
  返回值：int，该字符串总长度
*/
inline int strlen(char * String)
{
	register int __res;
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
	//根据计数寄存器（RCX）和零标志位（ZF）的状态，当RCX!=0且ZF!=1时重复执行紧随其后的字符串指令scasb
	//目的是找到String字符串末尾'\0'
					"repne	\n\t"
	//将AL寄存器（AL=0）中的1个字节数据与(ES:RDI)（String字符串）的1个字节数据进行比较。实质是执行AL-String并更新标志位，不保存结果
	//影响的寄存器与cmp指令类似，ZF：如果累加器值与内存数据相等ZF=1，否则ZF=0；SF：如果比较结果为负SF=1，否则SF=0；CF：如果比较发生借位CF=1，否则CF=0
					"scasb	\n\t"
	//notl：对序号占位符0代表的ECX寄存器进行按位取反操作
					"notl %0	\n\t"
	//decl：对序号占位符0代表的ECX寄存器进行ECX-1操作（不计算末尾'\0'字符），得到真正的字符串长度
					"decl %0	\n\t"
	//输出约束：相关指令执行后，结果存放至ECX，再将ECX存放至int __res
					:"=c"(__res)
	//输入约束：所有指令执行前，将String指针值存放至RDX；将0存放至RAX，将0xFFFFFFFF存放至ECX（初始值为负数）
					:"D"(String),"a"(0),"0"(0xffffffff)
	//损坏描述：无
					:
				);
	return __res;
}

#endif