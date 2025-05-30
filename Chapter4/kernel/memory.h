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
  结构体：Global_Memory_Descriptor保存全局内存信息以供内存管理模块使用
*/
struct Global_Memory_Descriptor
{
	struct E820 e820[32];	   // 物理内存段结构数组，每个物理内存段信息结构体占用一个数组单元
	unsigned long e820_length; // 物理内存段结构数组长度，记录当前获得的物理段数量
};
extern struct Global_Memory_Descriptor memory_management_struct; // 在main.c中定义

void init_memory();

#endif