/*
  在很多操作系统开发场景中，C语言无法完全代替汇编语言，例如：操作某些特殊寄存器，某些IO端口，或对性能要求极为苛刻的场景
  此时我们必须要在C语言中内嵌汇编语言满足上述要求
*/
#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

/*
  函数：内存空间初始化函数
  参数：
  1.void* Address：内存块的起始地址
  2.unsigned char C：将连续的Count数量内存单元初始化为该字符
  3.long Count：以Address为首地址的内存单元数
  返回值：void*，即指向内存块的起始地址void* Address
*/
inline void * memset(void * Address,unsigned char C,long Count)
{
	int d0,d1;
	//因为unsigned char C取值范围是0x00~0xFF，所以经过tmp=C*0x0101010101010101UL操作后
	//tmp变量的每个字节都能保留完整的unsigned char C
	unsigned long tmp = C * 0x0101010101010101UL;
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
	//rep指令前缀修改其后跟随的字符串操作指令的重复次数（由CX/ECX/RCX决定），并重复执行字符串操作直至CX/ECX/RCX=0
					"rep	\n\t"
	//stosq：将RAX寄存器（存放tmp变量）中的1个四字（8B）存储到(ES:RDI)，RDI寄存器根据DF=0自动增加8
					"stosq	\n\t"
	//testb：执行立即数4和序号占位符为3的寄存器q（存放Count）的最低3字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为4的倍数
					"testb $4,%b3	\n\t"
	//je 1f：若ZF=1则跳转至标号1处执行，f表示汇编器会向当前je指令之后的代码查找最近的定义为1:的标签
					"je 1f	\n\t"
	//stosl：将EAX寄存器中的1个双字（4B）存储到(ES:RDI)，RDI寄存器根据DF=0自动增加4
					"stosl	\n\t"
	//testb：执行立即数2和序号占位符为3的寄存器q（存放Count）的最低3字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为2的倍数
					"1:\ttestb $2,%b3	\n\t"
	//je 2f：若ZF=1则跳转至标号2处执行，f表示汇编器会向当前je指令之后的代码查找最近的定义为2:的标签
					"je 2f\n\t"
	//stosw：将EAX寄存器中的1个字（2B）存储到(ES:RDI)，RDI寄存器根据DF=0自动增加2
					"stosw	\n\t"
	//testb：执行立即数1和序号占位符为3的寄存器q（存放Count）的最低3字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为1的倍数
					"2:\ttestb $1,%b3	\n\t"
	//je 3f：若ZF=1则跳转至标号3处执行，f表示汇编器会向当前je指令之后的代码查找最近的定义为3:的标签
					"je 3f	\n\t"
	//stosw：将EAX寄存器中的1个字节（1B）存储到(ES:RDI)，RDI寄存器根据DF=0自动增加1
					"stosb	\n\t"
					"3:	\n\t"
	//输出约束：执行完指令后，更新RCX并存入C语言变量d0；更新RDI并存入C语言变量d1
					:"=&c"(d0),"=&D"(d1)
	//输入约束：执行指令前，将tmp的值存入RAX；Count的值存入RBX/RCX/RDX；Count/8的值存入RCX作为循环次数，Address的值存入RDI作为原始目的内存块地址
					:"a"(tmp),"q"(Count),"0"(Count/8),"1"(Address)
	//损坏描述：因为(ES:RDI)存放内容有改变并且输出/输入约束未曾对其使用m约束，应使用memory声明
					:"memory"					
				);
	return Address;
}

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