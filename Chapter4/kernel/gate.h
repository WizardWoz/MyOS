#ifndef __GATE_H__
#define __GATE_H__

/*段描述符结构体*/
struct desc_struct
{
	unsigned char x[8];   //每个段描述符占用8B
};

/*调用门描述符结构体*/
struct gate_struct
{
	unsigned char x[16];  //每个调用门描述符占用16B
};

/*
  extern是C语言中的一个存储类说明符（storage class specifier）它的主要作用是声明一个变量或函数，
  表明这个变量或函数是在其他地方定义的（可能是在同一个文件的后面，或者更常见的是在另一个C文件中）
*/

extern struct desc_struct GDT_Table[]; // 全局描述符表GDT结构，在内核执行头文件head.S中声明.globl GDT_Table
extern struct gate_struct IDT_Table[]; // 中断描述符表IDT结构，在内核执行头文件head.S中声明.globl IDT_Table
extern unsigned int TSS64_Table[26];   // 任务状态段TSS64结构，在内核执行头文件head.S中声明.globl TSS64_Table

/*do {...}while(0)（无需加;）的目的主要不是为了循环，而是为了利用do-while语句的一些特性，尤其是在宏定义和流程控制中
  1.将多条语句封装成一个“逻辑单条语句”（最常见和最重要的用途，尤其是在定义包含多条语句的宏时）
  为了让宏在任何需要单条语句的地方都能安全使用，并且允许用户在宏调用后像函数调用一样加上分号，避免语法错误和潜在的逻辑问题
  例子如下：
  有宏定义语句#define SWAP(a, b) int temp = a; a = b; b = temp; 如果在if-else语句中使用该宏定义会出现问题
  if (x > y)				if (x > y)
	SWAP(x, y);					int temp = x; // 注意：这里可能会引发语法错误，或者行为不符合预期
  else						a = y; // 这条语句不再受 if 控制
	printf("No swap\n");	b = temp; // 这条语句也不再受 if 控制
							else // else 找不到对应的 if，引发语法错误
								printf("No swap\n");
  如果使用{...}将宏体包裹起来#define SWAP(a, b) { int temp = a; a = b; b = temp; }
  if (x > y)				if (x > y)
	SWAP(x, y);					SWAP(x, y); // 展开为 if (x > y) { int temp = x; a = y; b = temp; };
  else						else // else 找不到对应的 if，引发语法错误
	printf("No swap\n");		printf("No swap\n");
  如果使用do{...}while(0)包裹宏体#define SWAP(a, b) do { int temp = a; a = b; b = temp; } while(0)
  if (x > y)				if (x > y)
	SWAP(x, y);					SWAP(x, y); // 展开为 if (x > y) do { int temp = a; a = b; b = temp; } while(0);
  else						else
	printf("No swap\n");		printf("No swap\n");
  2.作为替代goto的结构化流程控制（尤其用于错误处理和资源释放）在函数内部，有时需要在遇到错误时跳过剩余的代码，
  直接执行末尾的清理工作（如释放内存）。传统的做法是使用goto语句跳转到一个清理标签，但在某些编程规范中鼓励避免使用goto，
  因为它可能导致代码难以阅读和维护（"面条式代码"）；do{...}while(0); （创建了一个代码块，比 goto 更局部化，可读性更好）
  当遇到错误时，使用break语句可以立即跳出这个 do-while 块，程序流程会继续执行 while(0); 之后的代码，也就是清理部分。
   do { // 开始一个逻辑块
		res1 = acquire_resource1();
		if (!res1) {
			break; // 遇到错误，跳出 do-while 块
		}

		res2 = acquire_resource2();
		if (!res2) {
			break; // 遇到错误，跳出 do-while 块
		}

		// ... 处理数据 ...
		result = 0; // 成功

	} while (0); // do-while 块结束

	// 清理部分 - 无论是否发生错误，这部分都会执行
	if (res2) {
		release_resource2(res2);
	}
	if (res1) {
		release_resource1(res1);
	}
  3.局部作用域限制：在do{...}while(0); 块内部声明的变量，其作用域仅限于该块内部。有助于避免变量名冲突，并使得变量的生命周期更加清晰
  在宏定义中尤其有用，可以防止宏内部使用的临时变量与宏外部的同名变量发生冲突
  do {
	int temp_var = 10;
	// ... 使用 temp_var ...
  } while(0);
  // temp_var 在这里不可见
*/

/*宏函数：设置中断门、调用门、任务门等
  函数参数：
  1.gate_selector_addr：门选择子地址
  2.attr：门描述符属性
  3.ist：
  4.code_addr：代码地址

  输出部分：相关语句执行后，将结果存放于内存并转入gate_selector_addr门选择子所在的16B内存；将结果存放于RAX并转入unsigned long __d0；将结果存放于RDX并转入unsigned long __d1
  输入部分：所有语句执行前，attr门描述符属性左移8位作为操作立即数；code_addr代码地址存放到RDX；0x8<<16=0x1000 0000存放到RAX；ist存放到CL
  损坏描述：指令部分涉及到内存操作，使用memory声明
  注意："i"(attr<<8)，理论上可用r、g、q等寄存器约束代替i约束，但编译时会出现错误：Error: suffix or oprands invalid for 'add'
  如果代码改为addl %4,%%ecx \n\t则可通过编译；原因可能是GNU C编译器无法为该指令匹配合适的寄存器

  movw	%%dx,%%ax		//AX=DX=addr
  andq	$0x7,%%rcx		//RCX=RCX & 0x0000 0111，只保留RCX的低3位
  addq	%4,%%rcx		//RCX=RCX+addr<<8
  shlq	$32,%%rcx		//RCX向左移32位
  addq	%%rcx,%%rax		//RAX=RAX+RCX
  xorq	%%rcx,%%rcx		//RCX=0
  movl	%%edx,%%ecx		//ECX=EDX
  shrq	$16,%%rcx		//RCX向右移16位
  shlq	$48,%%rcx		//RCX向左移48位，此时RCX高16位=EDX高16位
  addq	%%rcx,%%rax		//RAX=RAX+RCX
  movq	%%rax,%0		//*((unsigned long *)(gate_selector_addr))=RAX
  shrq	$32,%%rdx		//RDX向右移32位
  movq	%%rdx,%1		//*(1 + (unsigned long *)(gate_selector_addr))=RDX
*/
#define _set_gate(gate_selector_addr, attr, ist, code_addr)                                \
	do                                                                                     \
	{                                                                                      \
		unsigned long __d0, __d1;                                                          \
		__asm__ __volatile__(                                                              \
			"movw %%dx,%%ax		\n\t"                                                         \
			"andq $0x7,%%rcx	\n\t"                                                         \
			"addq %4,%%rcx		\n\t"                                                          \
			"shlq $32,%%rcx		\n\t"                                                         \
			"addq %%rcx,%%rax	\n\t"                                                        \
			"xorq %%rcx,%%rcx	\n\t"                                                        \
			"movl %%edx,%%ecx	\n\t"                                                        \
			"shrq $16,%%rcx		\n\t"                                                         \
			"shlq $48,%%rcx		\n\t"                                                         \
			"addq %%rcx,%%rax	\n\t"                                                        \
			"movq %%rax,%0		\n\t"                                                          \
			"shrq $32,%%rdx		\n\t"                                                         \
			"movq %%rdx,%1		\n\t"                                                          \
			: "=m"(*((unsigned long *)(gate_selector_addr))),                              \
			  "=m"(*(1 + (unsigned long *)(gate_selector_addr))), "=&a"(__d0), "=&d"(__d1) \
			: "i"(attr << 8), "3"((unsigned long *)(code_addr)), "2"(0x8 << 16), "c"(ist)  \
			: "memory");                                                                   \
	} while (0)

/*宏函数：将TSS段描述符的段选择子加载到TR寄存器
  参数：
  1.n：

  指令部分：ltr %%ax		//将AX的数据加载到TR寄存器
  输出部分：无
  输入部分：所有指令执行前，将n左移3位得到的数据存入AX
  损坏描述：可能对内存空间有影响，用memory声明
*/
#define load_TR(n)                         \
	do                                     \
	{                                      \
		__asm__ __volatile__("ltr	%%ax"    \
							 :             \
							 : "a"(n << 3) \
							 : "memory");  \
	} while (0)

/*函数：设置外部中断门
  函数参数：
  1.unsigned int n：中断向量号
  2.unsigned char ist：
  3.void *addr：entry.S文件、interrupt.c等文件中各中断处理函数的标号对应的ENTRY()入口线性地址
  函数返回值：void
*/
inline void set_intr_gate(unsigned int n, unsigned char ist, void *addr)
{
	_set_gate(IDT_Table + n, 0x8E, ist, addr); // P,DPL=0,TYPE=E
}

/*函数：设置陷阱门
  函数参数：
  1.unsigned int n：中断向量号
  2.unsigned char ist：
  3.void *addr：entry.S文件中各异常处理函数的标号对应的ENTRY()入口线性地址
  函数返回值：void
*/
inline void set_trap_gate(unsigned int n, unsigned char ist, void *addr)
{
	_set_gate(IDT_Table + n, 0x8F, ist, addr); // P,DPL=0,TYPE=F
}

/*函数：设置系统调用门
  函数参数：
  1.unsigned int n：中断向量号
  2.unsigned char ist：
  3.void *addr：entry.S文件中各异常处理函数的标号对应的ENTRY()入口线性地址
  函数返回值：void
*/
inline void set_system_gate(unsigned int n, unsigned char ist, void *addr)
{
	_set_gate(IDT_Table + n, 0xEF, ist, addr); // P,DPL=3,TYPE=F
}

/*函数：设置内部中断门
  函数参数：
  1.unsigned int n：中断向量号
  2.unsigned char ist：
  3.void *addr：entry.S文件中各异常处理函数的标号对应的ENTRY()入口线性地址
  函数返回值：void
*/
inline void set_system_intr_gate(unsigned int n, unsigned char ist, void *addr) // int3
{
	_set_gate(IDT_Table + n, 0xEE, ist, addr); // P,DPL=3,TYPE=E
}

/*函数：负责配置TSS段内的各个RSP和IST项
  参数：
  1.unsigned long rsp0：
  2.unsigned long rsp1：
  3.unsigned long rsp2：
  4.unsigned long ist1：
  5.unsigned long ist2：
  6.unsigned long ist3：
  7.unsigned long ist4：
  8.unsigned long ist5：
  9.unsigned long ist6：
  10.unsigned long ist7：
  返回值：void
*/
void set_tss64(unsigned long rsp0, unsigned long rsp1, unsigned long rsp2, unsigned long ist1, unsigned long ist2, unsigned long ist3,
			   unsigned long ist4, unsigned long ist5, unsigned long ist6, unsigned long ist7)
{
	//因为TSS64_Table是unsigned int类型，所以unsigned long类型的rsp、ist占用2个位置
	*(unsigned long *)(TSS64_Table + 1) = rsp0;
	*(unsigned long *)(TSS64_Table + 3) = rsp1;
	*(unsigned long *)(TSS64_Table + 5) = rsp2;

	*(unsigned long *)(TSS64_Table + 9) = ist1;
	*(unsigned long *)(TSS64_Table + 11) = ist2;
	*(unsigned long *)(TSS64_Table + 13) = ist3;
	*(unsigned long *)(TSS64_Table + 15) = ist4;
	*(unsigned long *)(TSS64_Table + 17) = ist5;
	*(unsigned long *)(TSS64_Table + 19) = ist6;
	*(unsigned long *)(TSS64_Table + 21) = ist7;
}

#endif