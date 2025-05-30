#include "memory.h"
#include "lib.h"

/*函数：初始化物理地址空间，获得并打印物理内存信息
  参数：无
  返回值：void，无
*/
void init_memory()
{
    int i, j;
    // 1.系统可用的RAM内存总量；2.统计2MB可用物理内存页数；3.物理地址空间的的结束地址
    unsigned long TotalMem = 0;
    struct Memory_E820_Formate *p = NULL;
    color_printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    // 0x7E00是物理地址，经过页表映射转换后的线性地址是0xFFFF800000007E00
    p = (struct Memory_E820_Formate *)0xFFFF800000007E00;
    for (i = 0; i < 32; i++)
    {
        // 把64位的线性地址拆分为两段显示：%#010x显示'0x'和高32位；%08x显示低32位
        color_printk(ORANGE, BLACK, "Address:%#010x,%08x\tLength:%#010x,%08x\tType:%#010x\n", p->address2, p->address1, p->length2, p->length1, p->type);
        unsigned long tmp = 0; // 暂时存储当前物理段长度的高32位
        // 如果当前获得的物理内存段是RAM
        if (p->type == 1)
        {
            tmp = p->length2;       // tmp存放存储当前物理段长度的高32位
            TotalMem += p->length1; // TotalMem先加上当前物理段长度的低32位
            TotalMem += tmp << 32;  // TotalMem再加上当前物理段长度的高32位
        }
        p++;             // 指针自增struct E820 *=20B
        if (p->type > 4) // 遇到程序运行的脏数据，直接跳出循环
        {
            break;
        }
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx\n", TotalMem);
}