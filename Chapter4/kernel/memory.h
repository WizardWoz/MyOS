#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

#define PTRS_PER_PAGE 512   //页表项个数，每个页表项占8B，每个页表大小为4KB，因此页表项个数为4KB/8B=512
#define PAGE_OFFSET ((unsigned long)0xFFFF800000000000) //物理地址0经过页表重映射=内核层的起始线性地址0xFFFF800000000000
#define PAGE_GDT_SHIFT 39
#

/*结构体：Memory_E820_Formate，用于数据解析；因为使用的物理地址空间信息已经在Loader引导加载程序中通过BIOS中断服务程序
  int 15h，AX=E820H获得，并保存在物理地址0x7E00处，本结构体就是存储从0x7E00提取的信息，每条物理地址空间信息占20B
*/
struct Memory_E820_Formate
{
    unsigned int address1;
    unsigned int address2;
    unsigned int length1;
    unsigned int length2;
    unsigned int type;
};

void init_memory();

#endif