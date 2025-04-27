/*在很多操作系统开发场景中，C语言无法完全代替汇编语言，例如：操作某些特殊寄存器,某些IO端口,或对性能要求极为苛刻的场景
  此时我们必须要在C语言中内嵌汇编语言满足上述要求
*/
#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

#define container_of(ptr,type,member)							\
({											\
	typeof(((type *)0)->member) * p = (ptr);					\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));		\
})

/*C语言使用关键字__asm__和__volatile__修饰汇编语句，符合ANSI C标准
  __asm__是关键字asm的宏定义，声明该行代码是一个内嵌汇编表达式，任何内嵌汇编语言表达式均以此为开头，必不可少
  __volatile__告诉编译器此行代码不能被编译器优化，因为经过优化后汇编语句很可能被修改从而无法达到预期效果
  GNU C语言内嵌汇编表达式由四部分构成：指令部分:输出部分:输入部分:损坏部分（冒号:必须存在）
  1.指令部分（必填项）：即汇编代码本身，指令本身格式与AT&T汇编语言格式基本相同"指令1    \n\t""指令2    \n\t"；当指令表达式存在多条汇编指令可全部写在一对双引号中
  如果全部指令写在同一对双引号内，则相邻两指令必须使用分号;或者换行符+制表符\n\t分隔，为了与序号占位符作区分，引用寄存器时必须使用两个%%；例如：movl $0x10,%%eax
  2.输出部分（可选项）：记录指令的输出信息，格式为："输出操作约束"(输出表达式),"输出操作约束"(输出表达式),......
  输出操作约束/输出约束必须使用=或+修饰，=意味着输出表达式是一个纯粹的输出操作；+意味着输出表达式即可输出也可输入（可读写）
  输出表达式保存指令的执行结果，通常情况下输出表达式是一个寄存器或C语言变量，例：int var "=d0"(var)
  3.输入部分（可选项）：记录指令的输入信息，格式为："输入操作约束"(输入表达式),"输入操作约束"(输入表达式),......
  输入操作约束/输入约束不允许使用=或+修饰，所以输入部分是只读的
  输入表达式保存指令的输入数据，通常情况下输入表达式是一个寄存器或C语言变量，例：int var "=d0"(var)
  4.损坏部分（可选项）：记录并未在输出/输入部分出现过的被修改的寄存器和内存空间，格式："损坏描述","损坏描述",......

  1.寄存器约束：表达式载体是一个寄存器，可以明确指派，亦可模糊指派由编译器自行分配；可使用寄存器全名，也可以使用寄存器的缩写名称（编译器根据指令汇编代码来确定位宽）
  r：任何输入/输出寄存器；q：EAX/EBX/ECX/EDX；g：寄存器/内存空间；a：RAX/EAX/AX/AL；b：RBX/EBX/BX/BL；c：RCX/ECX/CX/CL；d：RDX/EDX/DX/DL；
  D：RDI/EDI/DI；S：RSI/ESI/SI；f：浮点寄存器
  2.内存约束：表达式载体是一个内存空间，使用约束名m表示   m：内存空间
  3.立即数约束：表达式的载体是一个数值    i：整数类型立即数；F：浮点类型立即数
  4.修饰符：只用于输出操作约束，=意味着输出表达式是一个纯粹的输出操作；+意味着输出表达式即可输出也可输入（可读写）
  &：只能写在输出操作约束的第二个符号位置（在=或+之后），意味着编译器不得为任何输入表达式分配该寄存器（只有输入操作约束使用了q、r、g等模糊指派才有意义）
  5.占位符：是输出/输入操作约束的数值映射，每个内嵌汇编表达式输出约束和输入约束最多有10条，按书写顺序被映射为序号0～9
  指令部分引用序号占位符，必须使用%前缀修饰例：%0对应第1个操作约束；%1对应第2个操作约束......；编译时会将每个占位符代表的表达式替换到相应的寄存器或内存
  指令可根据需要指定操作位宽是字节还是字，也可以指定操作的字节位置

  1.寄存器修改通知：寄存器出现于指令部分，又不是输出/输入操作表达式指定的寄存器，也不是编译器为r、g、q约束选择的寄存器
  若寄存器被指令部分所修改，则应该在损坏部分加以描述
  例：__asm__ __volatile__ ("mov %0,%%ecx"::"a"(__tmp):"cx")
  2.内存修改通知：若指令部分修改了内存数据，或者在内嵌汇编表达式出现处内存数据发生改变，并且被修改的内存未使用m约束
  此时应该在损坏部分使用memory字符串，向编译器声明内存会发生改变，编译器会保证执行内嵌汇编表达式之后重新向寄存器装载引用过的内存空间
*/

#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")

/*内核数据结构：不带头结点的双向链表*/
struct List
{
	struct List * prev;
	struct List * next;
};
/*inline关键字：建议编译器将被 inline 修饰的函数调用，在调用处展开为函数体本身的指令，从而避免函数调用的开销（如参数压栈、跳转、返回地址保存、栈帧恢复等）
  与宏定义#define相比，inline函数有自己的作用域；遵循C语言类型检查规则；通常更容易调试
*/

/*不带头结点的双向链表初始化*/
inline void list_init(struct List * list)
{
	list->prev = list;
	list->next = list;
}

/*不带头结点的双向链表在原结点后插入一个新结点*/
inline void list_add_to_behind(struct List * entry,struct List * new)
{
	new->next = entry->next;
	new->prev = entry;
	new->next->prev = new;
	entry->next = new;
}

/*不带头结点的双向链表在原结点前插入一个新结点*/
inline void list_add_to_before(struct List * entry,struct List * new)	////add to entry behind
{
	new->next = entry;
	entry->prev->next = new;
	new->prev = entry->prev;
	entry->prev = new;
}

/*不带头结点的双向链表删除某结点*/
inline void list_del(struct List * entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

/*判断不带头结点的双向链表是否为空链表*/
inline long list_is_empty(struct List * entry)
{
	if(entry == entry->next && entry->prev == entry)
		return 1;
	else
		return 0;
}

/*求不带头结点的双向链表中某个结点的前驱结点*/
inline struct List * list_prev(struct List * entry)
{
	if(entry->prev != NULL)
		return entry->prev;
	else
		return NULL;
}

/*求不带头结点的双向链表中某个结点的后继结点*/
inline struct List * list_next(struct List * entry)
{
	if(entry->next != NULL)
		return entry->next;
	else
		return NULL;
}

/*内存拷贝函数*/
inline void * memcpy(void *From,void * To,long Num)
{
	int d0,d1,d2;
    //CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器（ESI, EDI或SI, DI）的值
    //即从低地址向高地址处理数据确保数据从源地址块的开头复制到目标地址块的开头（正向复制）
	__asm__ __volatile__	(	"cld	\n\t"
    //rep指令前缀修改其后跟随的字符串操作指令 (String Instruction) 的行为，使其重复执行，重复次数由CX/ECX/RCX决定
					"rep	\n\t"
    //movsb：重复将[R/ESI]指向的字节复制到[R/EDI]指向的位置，共(R/E)CX次。这是实现块内存复制的常用方法
					"movsq	\n\t"
    //testb：执行op1和op2之间的按位逻辑AND运算。运算结果本身会被丢弃，但以下标志位会根据运算结果进行设置
    //ZF：EAX=0则为1，EAX!=0则为0；SF：与结果最高位相同；PF：运算结果低8位包含偶数个1则为1，否则为0；CF、OF：为0；AF：未定义
    //将q约束确定的寄存器低4个字节与立即数4作比较
					"testb	$4,%b4	\n\t"
					"je	1f	\n\t"
					"movsl	\n\t"
					"1:\ttestb	$2,%b4	\n\t"
					"je	2f	\n\t"
					"movsw	\n\t"
					"2:\ttestb	$1,%b4	\n\t"
					"je	3f	\n\t"
					"movsb	\n\t"
					"3:	\n\t"
					:"=&c"(d0),"=&D"(d1),"=&S"(d2)
					:"0"(Num/8),"q"(Num),"1"(To),"2"(From)
					:"memory"
				);
	return To;      //返回目标地址
}

/*
		FirstPart = SecondPart		=>	 0
		FirstPart > SecondPart		=>	 1
		FirstPart < SecondPart		=>	-1
*/

inline int memcmp(void * FirstPart,void * SecondPart,long Count)
{
	register int __res;

	__asm__	__volatile__	(	"cld	\n\t"		//clean direct
					"repe	\n\t"		//repeat if equal
					"cmpsb	\n\t"
					"je	1f	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	1f	\n\t"
					"negl	%%eax	\n\t"
					"1:	\n\t"
					:"=a"(__res)
					:"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
					:
				);
	return __res;
}

/*
		set memory at Address with C ,number is Count
*/

inline void * memset(void * Address,unsigned char C,long Count)
{
	int d0,d1;
	unsigned long tmp = C * 0x0101010101010101UL;
	__asm__	__volatile__	(	"cld	\n\t"
					"rep	\n\t"
					"stosq	\n\t"
					"testb	$4, %b3	\n\t"
					"je	1f	\n\t"
					"stosl	\n\t"
					"1:\ttestb	$2, %b3	\n\t"
					"je	2f\n\t"
					"stosw	\n\t"
					"2:\ttestb	$1, %b3	\n\t"
					"je	3f	\n\t"
					"stosb	\n\t"
					"3:	\n\t"
					:"=&c"(d0),"=&D"(d1)
					:"a"(tmp),"q"(Count),"0"(Count/8),"1"(Address)	
					:"memory"					
				);
	return Address;
}

/*
		string copy
*/

inline char * strcpy(char * Dest,char * Src)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"stosb	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					:
					:"S"(Src),"D"(Dest)
					:
					
				);
	return 	Dest;
}

/*
		string copy number bytes
*/

inline char * strncpy(char * Dest,char * Src,long Count)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"decq	%2	\n\t"
					"js	2f	\n\t"
					"lodsb	\n\t"
					"stosb	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"rep	\n\t"
					"stosb	\n\t"
					"2:	\n\t"
					:
					:"S"(Src),"D"(Dest),"c"(Count)
					:					
				);
	return Dest;
}

/*
		string cat Dest + Src
*/

inline char * strcat(char * Dest,char * Src)
{
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"decq	%1	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"stosb	\n\r"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					:
					:"S"(Src),"D"(Dest),"a"(0),"c"(0xffffffff)
					:					
				);
	return Dest;
}

/*
		string compare FirstPart and SecondPart
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strcmp(char * FirstPart,char * SecondPart)
{
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"lodsb	\n\t"
					"scasb	\n\t"
					"jne	2f	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"xorl	%%eax,	%%eax	\n\t"
					"jmp	3f	\n\t"
					"2:	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	3f	\n\t"
					"negl	%%eax	\n\t"
					"3:	\n\t"
					:"=a"(__res)
					:"D"(FirstPart),"S"(SecondPart)
					:					
				);
	return __res;
}

/*
		string compare FirstPart and SecondPart with Count Bytes
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strncmp(char * FirstPart,char * SecondPart,long Count)
{	
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
					"decq	%3	\n\t"
					"js	2f	\n\t"
					"lodsb	\n\t"
					"scasb	\n\t"
					"jne	3f	\n\t"
					"testb	%%al,	%%al	\n\t"
					"jne	1b	\n\t"
					"2:	\n\t"
					"xorl	%%eax,	%%eax	\n\t"
					"jmp	4f	\n\t"
					"3:	\n\t"
					"movl	$1,	%%eax	\n\t"
					"jl	4f	\n\t"
					"negl	%%eax	\n\t"
					"4:	\n\t"
					:"=a"(__res)
					:"D"(FirstPart),"S"(SecondPart),"c"(Count)
					:
				);
	return __res;
}

/*

*/

inline int strlen(char * String)
{
	register int __res;
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"notl	%0	\n\t"
					"decl	%0	\n\t"
					:"=c"(__res)
					:"D"(String),"a"(0),"0"(0xffffffff)
					:
				);
	return __res;
}

/*

*/
inline unsigned long bit_set(unsigned long * addr,unsigned long nr)
{
	return *addr | (1UL << nr);
}

/*

*/

inline unsigned long bit_get(unsigned long * addr,unsigned long nr)
{
	return	*addr & (1UL << nr);
}

/*

*/

inline unsigned long bit_clean(unsigned long * addr,unsigned long nr)
{
	return	*addr & (~(1UL << nr));
}

/*

*/

inline unsigned char io_in8(unsigned short port)
{
	unsigned char ret = 0;
	__asm__ __volatile__(	"inb	%%dx,	%0	\n\t"
				"mfence			\n\t"
				:"=a"(ret)
				:"d"(port)
				:"memory");
	return ret;
}

/*

*/

inline unsigned int io_in32(unsigned short port)
{
	unsigned int ret = 0;
	__asm__ __volatile__(	"inl	%%dx,	%0	\n\t"
				"mfence			\n\t"
				:"=a"(ret)
				:"d"(port)
				:"memory");
	return ret;
}

/*

*/

inline void io_out8(unsigned short port,unsigned char value)
{
	__asm__ __volatile__(	"outb	%0,	%%dx	\n\t"
				"mfence			\n\t"
				:
				:"a"(value),"d"(port)
				:"memory");
}

/*

*/

inline void io_out32(unsigned short port,unsigned int value)
{
	__asm__ __volatile__(	"outl	%0,	%%dx	\n\t"
				"mfence			\n\t"
				:
				:"a"(value),"d"(port)
				:"memory");
}

/*

*/

#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

#endif