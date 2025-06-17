/*
  初级内存管理单元对应的头文件
  一、下边界对齐（Lower Boundary Alignment / Start Alignment）：关注点是数据开始位置
  1.定义：指一个内存块的起始地址是某个特定对齐值（N）的整数倍
  2.目的：这是最常见的内存对齐形式。确保数据结构的起始地址对齐，可以提高CPU访问内存的效率。很多CPU架构在访问未对齐的数据时可能会产生性能惩罚，
  甚至直接抛出硬件异常。例如，一个4字节的整数，如果其起始地址是4的倍数，CPU通常可以一次性读取；如果不是，则可能需要多次内存访问
  3.如何实现：在数据实际开始前填充一些字节（padding）使得数据的起始地址落在对齐的边界上。
  例如：如果要求4字节对齐，而当前可分配的地址是0x1001，那么分配器会跳过0x1001,0x1002,0x1003（填充3字节），从0x1004开始分配内存给请求的数据
  二、上边界对齐（Upper Boundary Alignment / End Alignment）：关注点是数据结束位置
  1.定义：指一个内存块的 结束地址的下一个地址（即内存块所占用的总大小加上起始地址后得到的地址）是某个特定对齐值（N）的整数倍。
  换句话说，如果内存块大小为S，起始地址为A，那么A+S需要是N的整数倍。
  2.目的：确保该内存块之后紧邻分配的下一个内存块能够自然地满足下边界对齐的要求（如果下一个块的对齐要求与当前块的上边界对齐值相同）。
  在某些特定的内存管理或数据结构布局中，可能需要确保整个内存区域的总大小（包括可能的数据和末尾的填充）是某个值的倍数。
  例如，一个内存池的每个单元，或者一个固定大小记录的集合。
  3.如何实现：在数据的末尾填充一些字节，使得整个内存块（数据 + 末尾填充）的总大小是N的倍数，从而让“结束地址的下一个地址”落在对齐的边界上
  例如：如果要求4字节对齐，一个数据大小为9字节，起始地址为 0x1000（已下边界对齐）数据会占据 0x1000到0x1008。结束地址的下一个地址是 0x1009。
  为了使其4字节上边界对齐，0x1000+S_aligned必须是4的倍数。所以S_aligned=12-9=3字节。整个分配块占据0x1000到0x100B，下一个可用地址是0x100C是4的倍数。
*/
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

#define PTRS_PER_PAGE 512								// 页表项个数，每个页表项占8B，每个页表大小为4KB，因此页表项个数为4KB/8B=512
#define PAGE_OFFSET ((unsigned long)0xFFFF800000000000) // 物理地址0经过页表重映射=内核层的起始线性地址0xFFFF800000000000
#define PAGE_GDT_SHIFT 39								// 代表2^39B=512GB
#define PAGE_1G_SHIFT 30								// 代表2^30B=1GB
#define PAGE_2M_SHIFT 21								// 代表2^21B=2MB
#define PAGE_4K_SHIFT 12								// 代表2^12B=4KB
#define PAGE_2M_SIZE (1UL << PAGE_2M_SHIFT)				// 代表2MB物理页的容量
#define PAGE_4K_SIZE (1UL << PAGE_4K_SHIFT)				// 代表4KB物理页的容量
#define PAGE_2M_MASK (~(PAGE_2M_SIZE - 1))				// 代表2MB数值的掩码，用于屏蔽低于2MB的数值
#define PAGE_4K_MASK (~(PAGE_4K_SIZE - 1))				// 代表4KB数值的掩码，用于屏蔽低于4KB的数值

// struct Zone内存段的属性（alloc_pages函数的int zone_select参数）
#define ZONE_DMA (1 << 0) 		// ZONE_DMA=1
#define ZONE_NORMAL (1 << 1)	// ZONE_NORMAL=2
#define ZONE_UNMAPPED (1 << 2)	// ZONE_UNMAPPED=4

// struct Page物理页的属性（alloc_pages函数的unsigned long page_flags参数）
#define PG_PTable_Mapped (1 << 0)	//已映射物理页，PG_PTable_Mapped=1
#define PG_Kernel_Init (1 << 1)		//内核初始化，PG_Kernel_Init=2
#define PG_Referenced (1 << 2)		//被引用物理页，PG_Referenced=4
#define PG_Dirty (1 << 3)			//有脏数据的物理页，PG_Dirty=8
#define PG_Active (1 << 4)			//活跃的物理页，PG_Active=16
#define PG_Up_To_Date (1 << 5)		//已更新物理页，PG_Up_To_Date=32
#define PG_Device (1 << 6)			//外设备使用，PG_Device=64
#define PG_Kernel (1 << 7)			//内核层独享，PG_Kernel=128
#define PG_K_Share_To_U (1 << 8)	//内核层与用户层共享，PG_K_Share_To_U=256
#define PG_Slab (1 << 9)			//SLAB内存池使用，PG_Slab=512

/*
  C语言typedef关键字：typedef <已有类型> <新的别名>;
  用于为已有的数据类型创建一个别名。它本身并不创建一个新的数据类型，而是为现有的类型提供一个更容易理解或更方便使用的名称。提高代码的可读性和可维护性。
  1.简化复杂类声明，例如结构体（struct）、联合体（union）、数组、指针或函数指针。
  // 不使用 typedef                      // 使用 typedef			struct _Point { // 通常会用一个不同的名字来区分
  struct Point {						typedef struct {				int x;
      int x;								int x;						int y;
      int y;								int y;				    };
  };									} Point;					typedef struct _Point Point;
  // 每次声明变量都需要写 struct Point	  // 声明变量更简洁			   Point p3;
  struct Point p1;						Point p2;

  typedef int* IntPtr; // IntPtr 是 'int*' 的别名	typedef char* String; // String 是 'char*' 的别名，常用于表示字符串
  IntPtr ptr1, ptr2;   // 声明了两个整型指针		String name = "你好";

  // 不使用typedef声明一个函数指针变量			   typedef void (*MathOperationFunc)(int, int);
  void (*math_operation_func_ptr)(int, int);	MathOperationFunc add_func, subtract_func; // 声明函数指针变量清晰得多

  2.提高代码可读性和可维护性
  为类型使用有意义的别名可以使代码一目了然。如果以后需要更改底层类型只需要修改typedef的定义，而不需要在代码中每个使用该类型的地方都进行修改
  typedef int Age;		Age personAge = 30;

  3.增强代码可移植性
  数据类型在不同的平台或编译器上可能有不同的大小或表示方式。你可以使用typedef来定义通用的类型名称，然后根据具体的平台调整它们的实际定义，从而使代码更具可移植性。
  // 在特定平台的头文件中
  typedef long int int32_platform; // 假设在这个平台上 long int 是 32 位
  // 在你的代码中
  int32_platform counter;

  4.与#define的区别
  #define：预处理器指令，它在预处理阶段执行简单的文本替换，可能会导致意外行为，尤其是在处理指针类型
  通常具有文件作用域（从定义点开始到文件结束，或直到遇到#undef）

  typedef：编译阶段由编译器处理，进行类型解释；创建一个真正的类型别名，编译器会进行类型检查；允许在一条语句中声明多个相同别名的变量；
  遵循C语言的作用域规则（可以是全局的，也可以是块/函数局部的）
*/

typedef struct
{
	unsigned long pml4t;
}pml4t_t;

/*
  宏函数：将参数addr地址按2MB页的上边界对齐
  参数：
  1.addr：64位虚拟线性地址
*/
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
/*
  宏函数：将参数addr地址按4KB页的上边界对齐
  参数：
  1.addr：64位虚拟线性地址
*/
#define PAGE_4K_ALIGN(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)
/*
  宏函数：将64位虚拟地址-0xFFFF800000000000转换为64位物理地址
  参数：
  1.addr：64位虚拟线性地址
  注意：目前只有物理地址的前10MB被映射到线性地址0xFFFF800000000000处（在head.S文件定义的页表中），也只有这10MB空间供宏函数使用
*/
#define Virt_To_Phy(addr) ((unsigned long)(addr) - PAGE_OFFSET)
/*
  宏函数：将真实物理地址（<=64bit）+0xFFFF800000000000转换为64位虚拟线性地址
  参数：
  1.addr：真实物理地址（<=64bit）
*/
#define Phy_To_Virt(addr) ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

/*
  宏函数：重新赋值一次CR3寄存器，使更改后的页表项生效；因为更改页表项后原页表项依然缓存在TLB，重新加载页目录基地址到
  CR3控制寄存器将迫使TLB自动更新
  参数：无

  指令部分：movq %%cr3,%0	//某一个输入/输出型寄存器=CR3
		   movq %0,%%cr3   //CR3=某一个输入/输出型寄存器
  输出部分：相关指令执行后，将结果存放至某一个输入/输出型寄存器，再转存到unsigned long tmpreg变量
  输入部分：无
  损坏描述：指令执行期间可能修改了内存，故使用memory声明
*/
#define flush_tlb()              \
	do                           \
	{                            \
		unsigned long tmpreg;    \
		__asm__ __volatile__(    \
			"movq %%cr3,%0 \n\t" \
			"movq %0,%%cr3 \n\t" \
			: "=r"(tmpreg)       \
			:                    \
			: "memory");         \
	} while (0)

// 每种不同的物理内存区域的下标，目前Bochs虚拟机只能开辟2GB物理空间，以至于虚拟平台仅有一个可用物理内存段
// 因此ZONE_DMA_INDEX、ZONE_NORMAL_INDEX、ZONE_UNMAPPED_INDEX均代表同一内存区域空间（使用默认值0代表）
int ZONE_DMA_INDEX = 0;			  // DMA区域空间（memory.h中定义为0）
int ZONE_NORMAL_INDEX = 0;		  // 低1GBRAM，已经被映射到页表中（memory.h中定义为0）
int ZONE_UNMAPPED_INDEX = 0;	  // 高1GBRAM，还未被映射到页表中（memory.h中定义为0）
unsigned long *Global_CR3 = NULL; // 全局变量保存CR3寄存器表示的PML4E顶层页目录物理基地址0x0000000000101000

/*
  结构体：Memory_E820_Formate，用于数据解析；因为使用的物理地址空间信息已经在Loader引导加载程序中通过BIOS中断服务程序
  int 15h，AX=E820H获得，并保存在物理地址0x7E00处，本结构体就是存储从0x7E00提取的信息，每条物理地址空间信息占20B
*/
struct Memory_E820_Formate
{
	unsigned int address1; // 64位线性地址的低32位
	unsigned int address2; // 64位线性地址的高32位
	unsigned int length1;  // 64位段长度的低32位
	unsigned int length2;  // 64位段长度的高32位
	unsigned int type;	   // 当前物理内存段的类型：1.RAM；2.ROM或保留；3.ACPI Reclaim；4.ACPI NVS；5.Others
};

/*
  __attribute__是一种特殊的关键字，用于向编译器提供有关函数、变量、类型或标签的附加信息。它允许开发者指导编译器进行特定的优化或代码生成。
  从而优化代码、管理内存布局、控制符号可见性以及实现其他高级功能。然而，由于其非标准性，使用时需要考虑到代码的可移植性问题。
  __attribute__是 GNU C 编译器的扩展，也被许多其他编译器（如Clang和ARM编译器）所支持。但其可用性和具体行为可能因编译器和版本而异。为了编写可移植的代码，应谨慎使用或通过宏进行封装。
  1.__attribute__((packed)) 用于结构体或联合体，告诉编译器尽可能地压缩其成员，减少内存占用，不进行字节对齐填充。
  2.__attribute__((aligned(N))) 用于变量或类型，指定其最小对齐字节数。N必须是2的幂。这对于需要特定内存对齐以提高性能或满足硬件要求的场景非常有用。
  3.__attribute__((deprecated))或 __attribute__((deprecated("message"))) 用于函数、变量或类型，标记它们为已弃用。当代码中使用到被标记为弃用的实体时，编译器会发出警告。
    可以附带一条消息，向用户解释为什么弃用以及建议使用什么替代方案。
  4.__attribute__((constructor)) 和 __attribute__((destructor)) 用于函数
    constructor: 标记的函数会在main()函数执行之前被自动调用。
    destructor: 标记的函数会在main()函数执行完毕或调用exit()之后被自动调用。可以为这些函数指定优先级（较小的数字表示较高的优先级）。
  5.__attribute__((visibility("default" | "hidden" | "internal" | "protected"))) 主要用于共享库，控制符号（函数或变量）的可见性。
    default: 符号正常导出。hidden: 符号不被导出，在库外部不可见。
  6.__attribute__((format(archetype, string-index, first-to-check))) 用于函数
    告诉编译器该函数接受类似printf、scanf、strftime或strfmon风格的格式化字符串。编译器可以据此检查格式化字符串与参数类型是否匹配。
    archetype: 指定格式化字符串的类型，如printf、scanf。
    string-index: 指示哪个参数是格式化字符串（从 1 开始计数）。
    first-to-check: 指示从哪个参数开始检查与格式化字符串的匹配性（从 1 开始计数）。
  7.__attribute__((unused)) 用于变量或函数参数，告知编译器该实体可能未被使用，从而抑制相关的编译器警告。
  8.__attribute__((section("section_name"))) 用于函数或变量
    允许开发者将它们放置在可执行文件或共享库的特定节（section）中。这对于需要将特定数据或代码放置在特定内存区域（如只读内存、特定硬件相关的内存区域）的嵌入式编程或底层开发非常有用。
  9.__attribute__((weak)) 用于函数或变量声明，表示该符号为弱符号。如果链接器找到另一个同名的非弱符号（强符号），则弱符号会被忽略。
    如果没有其他定义，弱符号可以有一个默认实现。如果多个弱符号同名但没有强符号，链接器会选择其中一个（行为可能因链接器而异）。
*/

/*
  结构体：E820，是Memory_E820_Formate的替代版本，存储从0x7E00提取的物理内存段信息，每条物理地址空间信息占20B
  特殊属性__attribute__((packed))修饰该结构体不会生成对齐空间，改用紧凑格式，只有这样才能正确索引出线性地址0xFFFF800000007E00的内存空间分布信息
*/
struct E820
{
	unsigned long address; // 完整的64位线性地址
	unsigned long length;  // 完整的该内存段长度
	unsigned int type;	   // 当前物理内存段的类型：1.RAM；2.ROM或保留；3.ACPI Reclaim；4.ACPI NVS；5.Others
} __attribute__((packed));

/*
  结构体：Page内存页结构（汇总可用物理内存信息，并方便以后的管理）
  将整个内存空间（通过int 15h，AX=E820返回的各内存段信息（包括RAM空间、ROM空间、保留空间等）按2MB大小物理内存页进行分割或对齐
  分割后每个物理内存页由一个struct Page结构体负责管理
*/
struct Page
{
	struct Zone *zone_struct;	   // 指向本页所属的可用物理区域结构体
	unsigned long PHY_address;	   // 页的物理地址
	unsigned long attribute;	   // 页的属性：当前页映射状态、活动状态、使用者等信息
	unsigned long reference_count; // 该页的引用次数
	unsigned long age;			   // 该页的创建时间
};

/*
  结构体：Zone区域空间结构
  区域空间结构体struct Zone代表各个可用物理内存区域（可用物理内存段，包括多个可用物理内存页），
  并记录和管理本区域物理内存页的分配情况由于物理页在页表的映射中可以是一对多的关系，一个物理页可以同时映射到线性地址空间的多个位置，
  所以成员变量total_pages_link与page_using_count在数值上不一定相等
*/
struct Zone
{
	struct Page *pages_group;					 // struct Page结构体数组指针
	unsigned long pages_length;					 // 本区域（本段）包含的struct Page结构体数量
	unsigned long zone_start_address;			 // 本区域（本段）的起始页对齐地址
	unsigned long zone_end_address;				 // 本区域（本段）的结束页对齐地址
	unsigned long zone_length;					 // 本区域（本段）经过页对齐后的地址长度
	unsigned long attribute;					 // 本区域（本段）的属性（当前区域是否支持DMA、页是否经过页表映射等信息）
	struct Global_Memory_Descriptor *GMD_struct; // struct Global_Memory_Descriptor全局结构体指针
	unsigned long page_using_count;				 // 本区域（本段）已使用物理内存页数量
	unsigned long page_free_count;				 // 本区域（本段）空闲物理内存页数量
	unsigned long total_pages_link;				 // 本区域（本段）物理页被引用次数
};

/*
  结构体：Global_Memory_Descriptor保存全局内存信息以供内存管理模块使用
*/
struct Global_Memory_Descriptor
{
	struct E820 e820[32];	   // 物理内存段结构数组，每个物理内存段信息结构体占用一个数组单元
	unsigned long e820_length; // 物理内存段结构数组长度，记录当前获得的物理段数量
	// bits_****相关字段：建立bits位图映射的目的是方便检索pages_struct中的空闲页表
	unsigned long *bits_map;   // 物理地址空间页映射位图
	unsigned long bits_size;   // 物理地址空间页数量
	unsigned long bits_length; // 物理地址空间页映射位图长度
	// pages_****、zones_****：记录struct Page、struct Zone结构体数组的首地址以及资源分配情况等信息
	struct Page *pages_struct;	// 全局struct Page结构体的指针
	unsigned long pages_size;	// struct Page结构体总数
	unsigned long pages_length; // struct Page结构体数组长度
	struct Zone *zones_struct;	// 全局struct Zone结构体的指针
	unsigned long zones_size;	// struct Zone结构体总数
	unsigned long zones_length; // struct Zone结构体数组长度
	// start_****、end_****：保存内核程序编译后各段首尾地址（代码段、数据段、BSS段等），首尾地址在kernel.lds文件定义
	unsigned long start_code;	 // 内核程序起始代码段地址
	unsigned long end_code;		 // 内核程序结束代码段地址
	unsigned long end_data;		 // 内核程序结束数据段地址
	unsigned long end_brk;		 // 内核程序结束地址
	unsigned long end_of_struct; // 内存页管理结构的结尾地址
};
extern struct Global_Memory_Descriptor memory_management_struct; // 在main.c中定义

unsigned long page_init(struct Page *page, unsigned long flags);
void init_memory();
struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags);

/*
  函数：读取CR3寄存器中的页目录物理基地址，并将其传递给函数调用者
  参数：无
  返回值：unsigned long *，即页目录物理基地址
*/
inline unsigned long *Get_gdt()
{
	unsigned long *tmp;
	__asm__ __volatile__(
		"movq %%cr3,%0 \n\t" // 某一个输入/输出型寄存器=CR3
		// 输出部分：相关指令执行后，将结果存放至某一个输入/输出型寄存器，再转存到unsigned long *tmp变量
		: "=r"(tmp)
		// 输入部分：无
		:
		// 损坏描述：指令执行期间可能修改了内存，故使用memory声明
		: "memory");
	return tmp;
}

#endif