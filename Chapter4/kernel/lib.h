/*
  在很多操作系统开发场景中，C语言无法完全代替汇编语言，例如：操作某些特殊寄存器，某些IO端口，或对性能要求极为苛刻的场景
  此时我们必须要在C语言中内嵌汇编语言满足上述要求
*/
#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

/*
  C语言typeof编译器拓展，并非标准C语言的一部分。它最先由GNU C编译器（GCC）引入，后来也被Clang等其他一些编译器支持
  使用 typeof 可能会降低代码的可移植性，因为不保证所有C编译器都支持它
  typeof 允许你在编译时获取一个变量或表达式的类型，而无需显式地写出这个类型。这在以下几种情况下特别有用：
  1.声明与现有变量类型相同的变量
  int x = 10;									double arr[5];
  typeof(x) y; // y 的类型与 x 相同，即 int		  typeof(arr[0]) element; // element 的类型是 double
  y = 20;
  2.创建类型安全的宏（最强大的用途）：在宏定义中，你可能不知道传递给宏的参数的具体类型，但又需要声明一个与参数类型相同的临时变量
  #define MAX(a, b) \					// 潜在问题：如果不用 typeof，宏可能会多次评估参数
   ({ typeof (a) _a = (a); \			// 例如 #define MAX_BAD(a,b) ((a) > (b) ? (a) : (b))
      typeof (b) _b = (b); \			// MAX_BAD(i++, j++) 就会导致 i 或 j 被多次自增
      _a > _b ? _a : _b; })
  3.简化复杂类型的声明：当处理嵌套结构体、指针或函数指针等复杂类型时，typeof 可以使代码更简洁
  struct MyStruct s1;				int (*func_ptr)(int, int);
  typeof(s1) s2 = s1;				typeof(func_ptr) another_func_ptr; // another_func_ptr 的类型是 int (*)(int, int)
*/

/*
  宏函数：根据结构体变量中某成员变量基地址，算出结构体变量基地址（即反推导出父层结构的起始地址）
  参数：
  1.ptr：结构体内某个成员变量的基地址
  2.type：成员变量所在结构体
  3.member：成员变量名称

  typeof(((type *)0)->member) * p = (ptr)：首先计算出成员变量member在type结构体内的偏移
  (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));：根据ptr参数计算出结构体变量的起始地址
*/
#define container_of(ptr,type,member)							\
({											\
	typeof(((type *)0)->member) * p = (ptr);					\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));		\
})

/*
  C语言使用关键字__asm__和__volatile__修饰汇编语句，符合ANSI C标准
  __asm__是关键字asm的宏定义，声明该行代码是一个内嵌汇编表达式，任何内嵌汇编语言表达式均以此为开头，必不可少
  __volatile__告诉编译器此行代码不能被编译器优化，因为经过优化后汇编语句很可能被修改从而无法达到预期效果
  GNU C语言内嵌汇编表达式由四部分构成：指令部分:输出部分:输入部分:损坏部分（冒号:必须存在）
  1.指令部分（必填项）：即汇编代码本身，指令本身格式与AT&T汇编语言格式基本相同"指令1    \n\t""指令2    \n\t"；当指令表达式存在多条汇编指令可全部写在一对双引号中
  如果全部指令写在同一对双引号内，则相邻两指令必须使用分号;或者换行符+制表符\n\t分隔，为了与序号占位符作区分，引用寄存器时必须使用两个%%；例如：movl $0x10,%%eax
  2.输出部分（可选项）：（针对全体指令）用于指定汇编代码的计算结果应该存储到哪个寄存器/内存地址中，并且该寄存器/内存地址又应存在哪个C语言变量。任意一条指令执行后会更新对应的值
  格式为："输出操作约束"(输出表达式),"输出操作约束"(输出表达式),......
  输出操作约束/输出约束必须使用=或+修饰，=意味着输出表达式是一个纯粹的输出操作，汇编器会忽略这个C变量在汇编指令执行前的原始值。汇编指令必须向这个C变量写入数据；
  +意味着输出表达式即可输出也可输入（可读写）
  输出表达式保存指令的执行结果，通常情况下输出表达式是一个寄存器或C语言变量，例：int var "=d0"(var)
  3.输入部分（可选项）：（针对全体指令）用于将C语言变量或常量的值传递给哪个寄存器/内存地址。任意一条指令执行前会读取对应的寄存器/内存地址
  格式为："输入操作约束"(输入表达式),"输入操作约束"(输入表达式),......
  输入操作约束/输入约束不允许使用=或+修饰，所以输入部分是只读的
  输入表达式保存指令的输入数据，通常情况下输入表达式是一个寄存器或C语言变量，例：int var "=d0"(var)
  4.损坏部分（可选项）：记录并未在输出/输入部分出现过的被修改的寄存器和内存空间，格式："损坏描述","损坏描述",......

  1.寄存器约束：表达式载体是一个寄存器，可以明确指派，亦可模糊指派由编译器自行分配；可使用寄存器全名，也可以使用寄存器的缩写名称（编译器根据指令汇编代码来确定位宽）
  r：任何通用寄存器；q：EAX/EBX/ECX/EDX；g：寄存器/内存空间；a：RAX/EAX/AX/AL；b：RBX/EBX/BX/BL；c：RCX/ECX/CX/CL；d：RDX/EDX/DX/DL；
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
  3.标志寄存器修改通知：指令部分包含影响RFLAGS或EFLAGS的指令时，必须在损坏部分使用cc向编译器声明
*/

#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")//sti可能影响内存且未使用m约束，所以在损坏部分使用memory
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")

/*
  内核数据结构：不带头结点的双向链表
  数据成员：
  1.struct List* prev：指向前驱结点的指针
  2.struct List* next：指向后继结点的指针
*/
struct List
{
	struct List * prev;
	struct List * next;
};
/*
  inline关键字：建议编译器将被 inline 修饰的函数调用，在调用处展开为函数体本身的指令，从而避免函数调用的开销（如参数压栈、跳转、返回地址保存、栈帧恢复等）
  与宏定义#define相比，inline函数有自己的作用域；遵循C语言类型检查规则；通常更容易调试
*/

/*
  函数：不带头结点的双向链表初始化
  参数：
  1.struct List* list：双向链表初始结点
  返回值：void，无
*/
inline void list_init(struct List * list)
{
	list->prev = list;
	list->next = list;
}

/*
  函数：不带头结点的双向链表在原结点后插入一个新结点
  参数：
  1.struct List* entry：原双向链表的某个结点
  2.struct List* new：需要插入的新结点
  返回值：void
*/
inline void list_add_to_behind(struct List * entry,struct List * new)
{
	new->next = entry->next;
	new->prev = entry;
	new->next->prev = new;
	entry->next = new;
}

/*
  函数：不带头结点的双向链表在原结点前插入一个新结点
  参数：
  1.struct List* entry：原双向链表的某个结点
  2.struct List* new：需要插入的新结点
  返回值：void
*/
inline void list_add_to_before(struct List * entry,struct List * new)
{
	new->next = entry;
	entry->prev->next = new;
	new->prev = entry->prev;
	entry->prev = new;
}

/*
  函数：不带头结点的双向链表删除某结点
  参数：
  1.struct List* entry：原双向链表需要删除的某个结点
  返回值：void
*/
inline void list_del(struct List * entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

/*
  函数：判断不带头结点的双向链表是否为空链表
  参数：
  1.struct List* entry：原双向链表的头结点
  返回值：long，返回1表示原双向链表为空链表，返回0表示非空链表
*/
inline long list_is_empty(struct List * entry)
{
	if(entry == entry->next && entry->prev == entry)
		return 1;
	else
		return 0;
}

/*
  函数：求不带头结点的双向链表中某个结点的前驱结点
  参数：
  1.struct List* entry：原双向链表的某个结点
  返回值：struct List*，指向前驱结点
*/
inline struct List * list_prev(struct List * entry)
{
	if(entry->prev != NULL)
		return entry->prev;
	else
		return NULL;
}

/*
  函数：求不带头结点的双向链表中某个结点的后继结点
  参数：
  1.struct List* entry：原双向链表的某个结点
  返回值：struct List*，指向后继结点
*/
inline struct List * list_next(struct List * entry)
{
	if(entry->next != NULL)
		return entry->next;
	else
		return NULL;
}

/*
  void*指针：void* 指针指向一个void类型。表示没有特定类型。意味着void*指针可以指向内存中的任何数据，但它不知道所指向的数据具体是什么类型
  1.可以将任何对象的指针（指向数据类型的指针，而不是函数指针，尽管有些编译器允许函数指针但这不符合C语言标准）隐式地赋值给一个void* 指针，反之亦然
  将void*赋值给其他对象指针类型时，通常推荐进行显式类型转换以提高代码可读性和安全性，尽管C语言标准允许隐式转换）
  2.不能直接解引用，例：不能直接通过*pv来访问void*所指向的数据。因为编译器不知道数据类型，也就不知道要读取多少个字节
  3.不能直接进行加减整数的操作，例：pv+1。因为指针算术依赖于知道数据类型的大小（例如，int*加1表示跳过一个int的字节数），而void*不知道它指向的数据大小
  但GNU GCC等编译器作为扩展允许对void*进行指针算术，并将其视为char*来进行计算（即每次加减1个字节）。强烈不推荐依赖此特性，因为它不可移植且违反标准。
*/

/*
  函数：内存块拷贝函数
  参数：
  1.void* From：源内存块起始地址；
  2.void* To：目的内存块起始地址；
  3.long Num：字符串总长度
  函数返回值：void*，是一个泛型指针，指向目的内存块起始地址void* To
*/
inline void * memcpy(void *From,void * To,long Num)
{
	int d0,d1,d2;
    //CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器（ESI, EDI或SI, DI）的值
    //即从低地址向高地址处理，从源地址块的开头复制到目标地址块的开头（正向复制）
	//STD设置DF=1时，字符串指令在处理内存地址时会自动减少索引寄存器（ESI, EDI或SI, DI）的值，此时为反向处理
	__asm__ __volatile__	("cld	\n\t"
    //rep指令前缀修改其后跟随的字符串操作指令的重复次数（由CX/ECX/RCX决定），并重复执行字符串操作直至CX/ECX/RCX=0
					"rep	\n\t"
    //movsq：重复将(DS:RSI)的一个四字（8B）复制到(ES:RDI)，RSI和RDI寄存器根据方向标志位DF的设置自动增加8
					"movsq	\n\t"
    //testb：执行立即数4和序号占位符为4的寄存器q（存放Num）的最低4字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为4的倍数
					"testb $4,%b4	\n\t"
	//je 1f：若ZF=1则跳转至标号1处执行，f表示汇编器会向当前je指令之后的代码查找最近的定义为1:的标签
					"je 1f	\n\t"
	//movsl：重复将(DS:RSI)的一个双字（4B）复制到(ES:RDI)，RSI和RDI寄存器根据方向标志位DF的设置自动增加4
					"movsl	\n\t"
	//testb：执行立即数2和序号占位符为4的寄存器q（存放Num）的最低4字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为2的倍数
					"1:\ttestb $2,%b4	\n\t"
					"je 2f	\n\t"
	//movsw：重复将(DS:RSI)的一个字（2B）复制到(ES:RDI)，RSI和RDI寄存器根据方向标志位DF的设置自动增加2
					"movsw	\n\t"
	//testb：执行立即数1和序号占位符为4的寄存器q（存放Num）的最低4字节之间的按位逻辑AND运算，为了测试源地址块剩余内存地址空间是否为1的倍数
					"2:\ttestb $1,%b4	\n\t"
					"je	3f	\n\t"
	//movsw：重复将(DS:RSI)的一个字节（1B）复制到(ES:RDI)，RSI和RDI寄存器根据方向标志位DF的设置自动增加1
					"movsb	\n\t"
					"3:	\n\t"
	//输出约束：相关指令执行后，Num/8结果存放到ECX（只写）并传递给d0；RDI自增结果存放到RDI（只写）并传递给d1；RSI自增结果存放到RSI（只写）并将RSI传递给d2
					:"=&c"(d0),"=&D"(d1),"=&S"(d2)
	//输入约束：指令执行前，将变量Num/8存放到ECX作为循环次数；将变量Num存放到RAX/RBX/RDX；将目的地址To存放到RDI；将源地址存放到RSI
					:"0"(Num/8),"q"(Num),"1"(To),"2"(From)
	//(ES:DI)或(ES:EDI)或(ES:RDI)存放内容已经改变，且这些内存地址未在输入或输出部分出现过，应该在损坏部分用memory声明
					:"memory"
				);
	return To;      //返回目标内存块首地址
}

/*
  函数内存块比较函数
  参数：
  1.void* FirstPart：第一个内存块首地址；
  2.void* SecondPart：第二个内存块首地址；
  3.long Count：需要对比的内存块数
  返回值：int，为0则FirstPart内存块=SecondPart内存块；为1则FirstPart内存块>SecondPart内存块；为-1则FirstPart内存块<SecondPart内存块
*/
inline int memcmp(void * FirstPart,void * SecondPart,long Count)
{
	register int __res;		//存放比较结果
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	("cld	\n\t"
	//repe指令前缀修改其后跟随的字符串操作指令的重复次数（由CX/ECX/RCX和ZF=1决定是否继续执行），
	//并重复执行字符串操作直至CX/ECX/RCX=0（此时第一个内存块与第二个内存块存放内容完全相同）或ZF=0（此时(DS:RSI)>(ES:RDI)或(DS:RSI)<(ES:RDI)）
					"repe	\n\t"
	//cmpsb：单个字节读取(DS:RSI)指向的SecondPart内存块，(ES:RDI)指向的FirstPart内存块，并将SecondPart字节-FirstPart字节，不保存减法结果但更新以下寄存器
	//ZF：减法结果为零则=1，否则为0；SF：减法结果为负则=1，否则为0；CF：减法发生借位则=1，否则为0
	//OF：反映有符号运算是否溢出；PF：反映结果低8位中设置的位（1）的个数的奇偶性；AF：未定义
					"cmpsb	\n\t"
	//若SecondPart内存块与FirstPart内存块完全相同，直接往后跳转至标号1处执行，__res=0
					"je 1f	\n\t"
	//EAX=1
					"movl $1,%%eax	\n\t"
	//jl：当ZF=0时，SF！=OF，也即SecondPart字节<FirstPart字节（SecondPart内存块<FirstPart内存块），此时jl条件为真往后跳转至标号1处执行
					"jl 1f	\n\t"
	//否则SecondPart字节>FirstPart字节（SecondPart内存块>FirstPart内存块），此时将EAX取反
					"negl %%eax	\n\t"
					"1:	\n\t"
	//输出约束：将movl，negl等指令执行结果存放至EAX（只写），并将EAX传递给32位int类型变量__res
					:"=a"(__res)
	//输入约束：将0传入序号占位符为0的第1个约束即寄存器EAX，并将EAX传递给__res；将FirstPart指针传入RDI作为第二内存块首地址
	//将SecondPart指针传入RSI作为第一内存块首地址，将Count变量传入RCX作为循环次数
					:"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
					:
				);
	return __res;
}

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
  函数：字符串复制函数（到达源字符串末尾'\0'自动停止）
  参数：
  1.char* Dest：目的字符串起始地址
  2.char* Src：源字符串起始地址
  返回值：char*，指向目的字符串起始地址char* Dest
*/
inline char * strcpy(char * Dest,char * Src)
{
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
	//将(DS:RSI)的1个字节数据存放到AL寄存器中，RSI寄存器根据DF=0自动增加1
					"lodsb	\n\t"
	//将AL寄存器的1个字节数据存放到(ES:RDI)，RDI寄存器根据DF=0自动增加1
					"stosb	\n\t"
	//检查AL寄存器是否为零（C语言标准中字符串最后的结束符为'\0'），而不改变原始数据
					"testb %%al,%%al	\n\t"
	//若AL!=0，则向前跳转至标号1处继续执行
					"jne 1b	\n\t"
	//输出约束：无
					:
	//输入约束：执行指令前，将Src值存入RSI作为源内存块起始地址；将Dest值存入RDI作为目的内存块起始地址
					:"S"(Src),"D"(Dest)
	//损坏部分：无
					:
				);
	return 	Dest;
}

/*
  函数：字符串复制函数（函数参数规定复制长度）
  参数：
  1.char* Dest：目的字符串起始地址
  2.char* Src：源字符串起始地址
  3.long Count：需要复制的字符串长度
  返回值：char*，指向目的字符串起始地址char* Dest
*/
inline char * strncpy(char * Dest,char * Src,long Count)
{
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
	//decq：DEC指令的四字（操作数长度为8B）版本，作用是将其操作数的值减去1，此处是将RCX-1
					"decq %2	\n\t"
	//如果符号标志位SF=1（通常表示前一个算术或逻辑运算的结果为负），则程序将向后跳转到最近的标签2处继续执行，意味着RCX<0，跳出循环
					"js 2f	\n\t"
	//RCX-1>0，继续进入循环，将(DS:RSI)的1个字节数据存放到AL寄存器中，RSI寄存器根据DF=0自动增加1
					"lodsb	\n\t"
	//将AL寄存器的1个字节数据存放到(ES:RDI)，RDI寄存器根据DF=0自动增加1
					"stosb	\n\t"
	//检查AL寄存器是否为零（C语言标准中字符串最后的结束符为'\0'），而不改变原始数据
					"testb %%al,%%al	\n\t"
	//若AL!=0，则向前跳转至标号1处继续执行
					"jne 1b	\n\t"
	//AL=0但是RCX!=0，意味着已经到达Src所指向的源字符串末尾，需要用'\0'填充Dest目标字符串的剩余长度
					"rep	\n\t"
	//将AL寄存器（AL=0）的1个字节数据存放到(ES:RDI)，RDI寄存器根据DF=0自动增加1
					"stosb	\n\t"
					"2:	\n\t"
	//输出约束：无
					:
	//输入约束：指令执行前，将Src指针存入RSI；将Dest指针存入RDI，将Count传入RCX作为循环次数
					:"S"(Src),"D"(Dest),"c"(Count)
	//损坏描述：无
					:					
				);
	return Dest;
}

/*
  函数字符串拼接函数（到达需要拼接的字符串末尾则停止）
  参数：
  1.char* Dest：目的字符串起始地址
  2.char* Src：源字符串起始地址
  返回值：char*，指向目的字符串起始地址char* Dest
*/
inline char * strcat(char * Dest,char * Src)
{
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
	//根据计数寄存器（RCX）和零标志位（ZF）的状态，当RCX!=0且ZF!=1时重复执行紧随其后的字符串指令scasb
	//目的是找到Dest复制目的地的字符串末尾
					"repne	\n\t"
	//将AL寄存器（初始时AL=0）中的1个字节数据与(ES:RDI)的1个字节数据进行比较。实质是执行AL-(ES:RDI)并更新标志位，不保存结果
	//影响的寄存器与cmp指令类似，ZF：如果累加器值与内存数据相等ZF=1，否则ZF=0；SF：如果比较结果为负SF=1，否则SF=0；CF：如果比较发生借位CF=1，否则CF=0
					"scasb	\n\t"
	//decq：DEC指令的四字（操作数长度为8B）版本，作用是将其操作数的值减去1，此处是将Dest-1以去掉末尾'\0'，指针向前移动一个位置
					"decq %1	\n\t"
					"1:	\n\t"
	//此时已经找到Dest目的字符串末尾ES:RDI，Src需要复制的字符串仍是头部DS:RSI
	//将(DS:RSI)的1个字节数据存放到AL寄存器中，RSI寄存器根据DF=0自动增加1
					"lodsb	\n\t"
	//将AL寄存器的1个字节数据存放到(ES:RDI)，RDI寄存器根据DF=0自动增加1
					"stosb	\n\r"
	//检查AL寄存器是否为零（C语言标准中字符串最后的结束符为'\0'），而不改变原始数据
					"testb %%al,%%al	\n\t"
	//若AL!=0，则向前跳转至标号1处继续执行
					"jne 1b	\n\t"
	//输出约束：无
					:
	//输入约束：指令执行前，将Src存入RSI；将Dest存入RDI；将0存入RAX；将0xFFFFFFFF存入RCX作为字符串长度上限
					:"S"(Src),"D"(Dest),"a"(0),"c"(0xffffffff)
	//损坏描述：无
					:					
				);
	return Dest;
}

/*
  函数：字符串比较函数（比较到字符串末尾为止）
  参数：
  1.char* FirstPart：第一个字符串起始地址
  2.char* SecondPart：第二个字符串起始地址
  返回值：int，第一个字符串=第二个字符串为0；第一个字符串>第二个字符串为1；第一个字符串<第二个字符串为-1
*/
inline int strcmp(char * FirstPart,char * SecondPart)
{
	register int __res;
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
	//ZF=1，继续进入循环，将(DS:RSI)（SecondPart字符串）的1个字节数据存放到AL寄存器中，RSI寄存器根据DF=0自动增加1
					"lodsb	\n\t"
	//将AL寄存器中的1个字节数据与(ES:RDI)（FirstPart字符串）的1个字节数据进行比较。实质是执行SecondPart-FirstPart并更新标志位，不保存结果
	//影响的寄存器与cmp指令类似，ZF：如果累加器值与内存数据相等ZF=1，否则ZF=0；SF：如果比较结果为负SF=1，否则SF=0；CF：如果比较发生借位CF=1，否则CF=0
					"scasb	\n\t"
	//ZF=0（即FirstPart!=SecondPart），跳出循环，往后跳转至标号2处继续执行
					"jne 2f	\n\t"
	//检查AL寄存器是否为零（C语言标准中字符串最后的结束符为'\0'），而不改变原始数据
					"testb %%al,%%al	\n\t"
	//ZF!=1也即AL!=0，说明还未到达SecondPart字符串末尾，往前跳转至标号1处继续执行
					"jne 1b	\n\t"
	//EAX寄存器与自身作异或运算，将EAX设置为0
					"xorl %%eax,%%eax	\n\t"
	//EAX=0，往后跳转至标号3处继续执行
					"jmp 3f	\n\t"
					"2:	\n\t"
	//EAX=1
					"movl $1,%%eax	\n\t"
	//如果SecondPart-FirstPart<0，也即FirstPart>SecondPart，则保持EAX=1并向后跳转至标号3处继续执行
					"jl 3f	\n\t"
	//如果SecondPart-FirstPart>0，也即FirstPart<SecondPart，则EAX=-1
					"negl %%eax	\n\t"
					"3:	\n\t"
	//输出约束：相关指令执行后，将结果存入EAX（只写），再将EAX存入int __res
					:"=a"(__res)
	//输入约束：指令执行前，将FirstPart指针值存入RDI，将SecondPart指针值存入RSI
					:"D"(FirstPart),"S"(SecondPart)
	//损坏描述：无
					:				
				);
	return __res;
}

/*
  函数：字符串比较函数（比较Count数目个字符为止）
  参数：
  1.char* FirstPart：第一个字符串起始地址
  2.char* SecondPart：第二个字符串起始地址
  3.long Count：需比较的字符串长度
  返回值：int，第一个字符串=第二个字符串为0；第一个字符串>第二个字符串为1；第一个字符串<第二个字符串为-1
*/

inline int strncmp(char * FirstPart,char * SecondPart,long Count)
{	
	register int __res;
	//CLD设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
	__asm__	__volatile__	(	"cld	\n\t"
					"1:	\n\t"
	//decq：DEC指令的四字（操作数长度为8B）版本，作用是将其操作数的值减去1，此处是将RCX-1
					"decq %3	\n\t"
	//如果符号标志位SF=1（通常表示前一个算术或逻辑运算的结果为负），则程序将向后跳转到最近的标签2处继续执行，意味着RCX<0，跳出循环
					"js 2f	\n\t"
	//将(DS:RSI)（SecondPart字符串）的1个字节数据存放到AL寄存器中，RSI寄存器根据DF=0自动增加1
					"lodsb	\n\t"
	//将AL寄存器中的1个字节数据与(ES:RDI)（FirstPart字符串）的1个字节数据进行比较。实质是执行SecondPart-FirstPart并更新标志位，不保存结果
					"scasb	\n\t"
	//ZF=0（即FirstPart!=SecondPart），跳出循环，往后跳转至标号3处继续执行
					"jne 3f	\n\t"
	//检查AL寄存器是否为零（C语言标准中字符串最后的结束符为'\0'），而不改变原始数据
					"testb %%al,%%al	\n\t"
	//ZF!=1也即AL!=0，说明还未到达SecondPart字符串末尾，往前跳转至标号1处继续执行
					"jne 1b	\n\t"
					"2:	\n\t"
	//EAX寄存器与自身作异或运算，将EAX设置为0
					"xorl %%eax,%%eax	\n\t"
	//此时RCX=0，且EAX=0，证明FirstPart=SecondPart，往后跳转至标号4处继续执行
					"jmp 4f	\n\t"
					"3:	\n\t"
	//EAX=1
					"movl $1,%%eax	\n\t"
	//如果SecondPart-FirstPart<0，也即FirstPart>SecondPart，则保持EAX=1并向后跳转至标号4处继续执行
					"jl	4f	\n\t"
	//如果SecondPart-FirstPart>0，也即FirstPart<SecondPart，则EAX=-1
					"negl %%eax	\n\t"
					"4:	\n\t"
	//输出约束：相关指令执行后，将结果存入EAX，再将EAX存入int __res
					:"=a"(__res)
	//输入约束：所有指令执行前，将FirstPart指针值存入RDI；将SecondPart指针值存入RSI；将Count值存入RCX作为循环次数
					:"D"(FirstPart),"S"(SecondPart),"c"(Count)
	//损坏描述：无
					:
				);
	return __res;
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

/*
  函数：设置内存块的某个位为1
  参数：
  1.unsigned long* addr：内存块首地址指针
  2.unsigned long nr：需要设置的位的位置
  返回值：unsigned long，设置后的值
*/
inline unsigned long bit_set(unsigned long * addr,unsigned long nr)
{
	return *addr | (1UL << nr);
}

/*
  函数：获取某个内存块的位的值
  参数：
  1.unsigned long* addr：内存块首地址指针
  2.unsigned long nr：需要获取具体值的位的位置
  返回值：unsigned long，获取到的位的值
*/
inline unsigned long bit_get(unsigned long * addr,unsigned long nr)
{
	return	*addr & (1UL << nr);
}

/*
  函数清除内存块中的某个位，使其为0
  参数：
  1.unsigned long* addr：内存块首地址指针
  2.unsigned long nr：需要获取具体值的位的位置
  返回值：unsigned long，将某个位取0后的addr指向的内存块所存储的值
*/
inline unsigned long bit_clean(unsigned long * addr,unsigned long nr)
{
	return	*addr & (~(1UL << nr));
}

/*
  函数：CPU从设备端口读入8位（1B）数据
  参数：
  1.unsigned short port：16位设备端口号（0～65535）
  返回值：unsigned char，存放从设备端口读出的1B数据
*/
inline unsigned char io_in8(unsigned short port)
{
	unsigned char ret = 0;
	//inb：从指定的端口port（DX）读入1B数据到AL
	__asm__ __volatile__(	"inb	%%dx,	%0	\n\t"
	//mfence：Memory Fence（内存栅栏）用于强制内存操作的顺序性
	//lfence(Load Fence)：只保证在lfence之前的加载操作在之后的加载操作之前完成。不保证存储操作的顺序。
	//sfence(Store Fence)：只保证在sfence之前的存储操作在之后的存储操作之前完成。不保证加载操作的顺序。
	//mfence(Memory Fence)：保证在mfence之前的所有加载和存储操作在之后的所有加载和存储操作之前完成。它是lfence和sfence功能的集合
	//主要用于需要严格控制内存访问顺序的底层代码，包括：多线程库和同步原语的实现（如互斥锁、读写锁、条件变量）；
	//无锁数据结构（Lock-Free Data Structures）的实现。设备驱动程序中与内存映射I/O端口的交互。
				"mfence			\n\t"
	//输出部分：相关指令执行后，将结果存入AL，并将AL存入unsigned char ret
				:"=a"(ret)
	//输入部分：所有指令执行前，将port存入DX
				:"d"(port)
	//损坏描述：访问内存映射IO需要memory声明
				:"memory");
	return ret;
}

/*
  函数：从设备端口读入32位（4B）数据
  参数：
  1.unsigned short port：16位设备端口号（0～65535）
  返回值：unsigned int，存放从设备端口读出的4B数据
*/
inline unsigned int io_in32(unsigned short port)
{
	unsigned int ret = 0;
	//inl：从指定的端口port（DX）读入4B数据到序号占位符0代表的寄存器EAX（只写）
	__asm__ __volatile__(	"inl %%dx,%0	\n\t"
				"mfence	\n\t"
	//输出部分：相关指令执行后，将结果存入EAX，并将EAX存入unsigned int ret
				:"=a"(ret)
	//输入部分：所有指令执行前，将port存入DX
				:"d"(port)
	//损坏描述：访问内存映射IO需要memory声明
				:"memory");
	return ret;
}

/*
  函数：CPU将8位（1B）数据输出到设备端口
  参数：
  1.unsigned short port：16位设备端口号（0～65535）
  2.unsigned char value：需要输出的1B数据
  返回值：void
*/
inline void io_out8(unsigned short port,unsigned char value)
{
	//outb：将序号占位符0代表的寄存器AL存储的内容输出到DX代表的端口处
	__asm__ __volatile__(	"outb %0,%%dx	\n\t"
				"mfence	\n\t"
	//输出部分：无
				:
	//输入部分：所有指令执行前，将value存入AL，将port存入DX
				:"a"(value),"d"(port)
	//损坏描述：访问内存映射IO需要memory声明
				:"memory");
}

/*
  函数：将32位（4B）数据输出到设备端口
  参数：
  1.unsigned short port：16位设备端口号（0～65535）
  2.unsigned int value：需要输出的4B数据
  返回值：void
*/
inline void io_out32(unsigned short port,unsigned int value)
{
	//outl：将序号占位符0代表的寄存器EAX存储的内容输出到DX代表的端口处
	__asm__ __volatile__(	"outl %0,%%dx	\n\t"
				"mfence			\n\t"
	//输出部分：无
				:
	//输入部分：所有指令执行前，将value存入EAX，将port存入DX
				:"a"(value),"d"(port)
	//损坏描述：访问内存映射IO需要memory声明
				:"memory");
}

/*
  宏定义：从port端口读入buffer指向的内存块中的nr*2字节数据
  参数：
  1.port：16位设备端口号（0～65535）
  2.buffer：64位指针，指向源内存块
  3.nr：需要从源内存块读取的字数

  指令部分：
  1.cld：设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
  2.rep：修改其后跟随的字符串操作指令的重复次数（由RCX决定），并重复执行字符串操作直至RCX=0
  3.insw：从DX寄存器指定的I/O端口读取1个字（2B）数据，并存储到(ES:RDI)，执行后RDI寄存器根据DF=0自动增加2
  输出部分：无
  输入部分：所有指令执行前，将port存入DX，将buffer存入RDI，将nr存入RCX作为循环次数
  损坏描述：访问内存映射IO需要memory声明
*/
#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

/*
  宏定义：将nr*2字节数据从port端口输出到buffer指向的内存块中
  参数：
  1.port：16位设备端口号（0～65535）
  2.buffer：64位指针，指向目的内存块
  3.nr：需要写入目标内存块的字数

  指令部分：
  1.cld：设置DF=0时，字符串指令在处理内存地址时会自动增加索引寄存器的值，即从低地址向高地址处理
  2.rep：修改其后跟随的字符串操作指令的重复次数（由RCX决定），并重复执行字符串操作直至RCX=0
  3.outsw：从(DS:RSI)读取1个字（2个字节）数据，并写入到DX寄存器指定的I/O端口，执行后RSI寄存器根据DF=0自动增加2
  输出部分：无
  输入部分：所有指令执行前，将port存入DX，将buffer存入RSI，将nr存入RCX作为循环次数
  损坏描述：访问内存映射IO需要memory声明
*/
#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

#endif