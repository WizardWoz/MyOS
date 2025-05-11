#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

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