#include "memory.h"
#include "lib.h"

void init_memory()
{
    int i, j;
    unsigned long TotalMem = 0; // 系统可用的RAM内存总量
    // struct Memory_E820_Formate *p=NULL;
    struct E820 *p = NULL;
    color_printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    // 0x7E00是物理地址，经过页表映射转换后的线性地址是0xFFFF800000007E00
    // p=(struct Memory_E820_Formate *)0xFFFF800000007E00;
    p = (struct E820 *)0xFFFF800000007E00;
    for (i = 0; i < 32; i++)
    {
        // 把64位的线性地址拆分为两段显示：%#010x显示'0x'和高32位；%08x显示低32位
        //  color_printk(ORANGE,BLACK,"Address:%#010x,%08x\tLength:%#010x,%08x\tType:%#010x\n",p->address2,p->address1
        //  ,p->length2,p->length1,p->type);
        color_printk(ORANGE, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        unsigned long tmp = 0; // 暂时存储当前物理段长度的高32位
        // 如果当前获得的物理内存段是RAM
        //  if(p->type==1)
        //  {
        //      tmp=p->length2;         //tmp存放存储当前物理段长度的高32位
        //      TotalMem+=p->length1;   //TotalMem先加上当前物理段长度的低32位
        //      TotalMem+=tmp<<32;      //TotalMem再加上当前物理段长度的高32位
        //  }
        // 如果当前获得的物理内存段是RAM
        if (p->type == 1)
        {
            TotalMem += p->length; // 因为当前物理段长度不再分为高32位和低32位，直接TotalMem加上即可
        }
        // 全局内存管理结构体变量memory_management_struct的其中一个物理内存数据单元e820[i]存放当前获得的64位线性地址
        memory_management_struct.e820[i].address += p->address;
        // 全局内存管理结构体变量memory_management_struct的其中一个物理内存数据单元e820[i]存放当前获得的段长度
        memory_management_struct.e820[i].length += p->length;
        // 全局内存管理结构体变量memory_management_struct的其中一个物理内存数据单元e820[i]存放当前获得的段类型
        memory_management_struct.e820[i].type += p->type;
        // 全局内存管理结构体变量memory_management_struct的e820数组长度更新为i
        memory_management_struct.e820_length = i;
        p++;             // 指针自增struct E820 *=20B
        if (p->type > 4) // 遇到程序运行的脏数据，直接跳出循环
        {
            break;
        }
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx\n", TotalMem);
    TotalMem = 0; // TotalMem置为0，用于统计2MB可用物理内存页数
    for (i = 0; i <= memory_management_struct.e820_length; i++)
    {
        unsigned long start, end; // 可用物理内存段的起始地址和结束地址
        if (memory_management_struct.e820[i].type != 1)
        {
            continue;
        }
        // 对e820结构体数组中的可用物理内存段的起始地址进行2MB物理页的上边界对齐
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        // e820结构体数组中的可用物理内存段的结束地址=((段原起始地址+段长度)>>21)<<21，进行2MB物理页的下边界对齐
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (end <= start) // 如果计算后的起始地址大于等于计算后的结束地址，则为无效内存段，继续下一次循环
        {
            continue;
        }
        // 如果计算后的起始地址小于计算后的结束地址，则为有效内存段，进而计算其可用物理页数量
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGES:%#010x=%010d\n", TotalMem, TotalMem);
}
