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
/*宏函数：将参数addr地址按2MB页的上边界对齐
  参数：
  1.addr：64位虚拟线性地址
*/
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
/*宏函数：将参数addr地址按4KB页的上边界对齐
  参数：
  1.addr：64位虚拟线性地址
*/
#define PAGE_4K_ALIGN(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)
/*宏函数：将64位虚拟地址-0xFFFF800000000000转换为64位物理地址
 参数：
 1.addr：64位虚拟线性地址
 注意：目前只有物理地址的前10MB被映射到线性地址0xFFFF800000000000处（在head.S文件定义的页表中），也只有这10MB空间供宏函数使用
*/
#define Virt_To_Phy(addr) ((unsigned long)(addr) - PAGE_OFFSET)
/*宏函数：将真实物理地址（<=64bit）+0xFFFF800000000000转换为64位虚拟线性地址
 参数：
 1.addr：真实物理地址（<=64bit）
*/
#define Phy_To_Virt(addr) ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

/*结构体：Memory_E820_Formate，用于数据解析；因为使用的物理地址空间信息已经在Loader引导加载程序中通过BIOS中断服务程序
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

/*结构体：E820，是Memory_E820_Formate的替代版本，存储从0x7E00提取的物理内存段信息，每条物理地址空间信息占20B
  特殊属性__attribute__((packed))修饰该结构体不会生成对齐空间，改用紧凑格式，只有这样才能正确索引出线性地址0xFFFF800000007E00的内存空间分布信息
 */
struct E820
{
	unsigned long address; // 完整的64位线性地址
	unsigned long length;  // 完整的该内存段长度
	unsigned int type;	   // 当前物理内存段的类型：1.RAM；2.ROM或保留；3.ACPI Reclaim；4.ACPI NVS；5.Others
} __attribute__((packed));

/*结构体：Page内存页结构（汇总可用物理内存信息，并方便以后的管理）
  将整个内存空间（通过int 15h，AX=E820返回的各内存段信息（包括RAM空间、ROM空间、保留空间等）按2MB大小物理内存页进行分割或对齐
  分割后每个物理内存页由一个struct Page结构体负责管理
*/
struct Page
{
	struct Zone *zone_struct;		//指向本页所属的区域结构体
	unsigned long PHY_address;		//页的物理地址
	unsigned long attribute;		//页的属性：当前页映射状态、活动状态、使用者等信息
	unsigned long reference_count;	//该页的引用次数
	unsigned long age;				//该页的创建时间
};

/*结构体：Zone区域空间结构
  区域空间结构体struct Zone代表各个可用物理内存区域（可用物理内存段），并记录和管理本区域物理内存页的分配情况
  由于物理页在页表的映射中可以是一对多的关系，一个物理页可以同时映射到线性地址空间的多个位置，
  所以成员变量total_pages_link与page_using_count在数值上不一定相等
*/
struct Zone
{
	struct Page *pages_group;			//struct Page结构体数组指针
	unsigned long pages_length;			//本区域（本段）包含的struct Page结构体数量
	unsigned long zone_start_address;	//本区域（本段）的起始页对齐地址
	unsigned long zone_end_address;		//本区域（本段）的结束页对齐地址
	unsigned long zone_length;			//本区域（本段）经过页对齐后的地址长度
	unsigned long attribute;			//本区域（本段）的属性（当前区域是否支持DMA、页是否经过页表映射等信息）
	struct Global_Memory_Descriptor *GMD_struct;	//struct Global_Memory_Descriptor全局结构体指针
	unsigned long page_using_count;		//本区域（本段）已使用物理内存页数量
	unsigned long page_free_count;		//本区域（本段）空闲物理内存页数量
	unsigned long total_pages_link;		//本区域（本段）物理页被引用次数
};

/*结构体：Global_Memory_Descriptor保存全局内存信息以供内存管理模块使用
 */
struct Global_Memory_Descriptor
{
	struct E820 e820[32];	   	// 物理内存段结构数组，每个物理内存段信息结构体占用一个数组单元
	unsigned long e820_length; 	// 物理内存段结构数组长度，记录当前获得的物理段数量
	unsigned long *bits_map;	//物理地址空间页映射位图
	unsigned long bits_size;	//物理地址空间页数量
	unsigned long bits_length;
};
extern struct Global_Memory_Descriptor memory_management_struct; // 在main.c中定义

void init_memory();

#endif