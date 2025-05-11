#include "memory.h"
#include "lib.h"

void init_memory()
{
    int i,j;
    unsigned long TotalMem=0;
    struct Memory_E820_Formate *p=NULL;
    color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    //0x7E00是物理地址，经过页表映射转换后的线性地址是0xFFFF800000007E00
    p=(struct Memory_E820_Formate *)0xFFFF800000007E00;
    for ( i = 0; i < 32; i++)
    {
        color_printk(ORANGE,BLACK,"Address:%#010x,%#08x\tLength:%#010x,%#08x\tType:%#010x\n",p->address2,p->address1
        ,p->length2,p->length1,p->type);
        unsigned long tmp=0;
        if(p->type==1)
        {
            tmp=p->length2;
            TotalMem+=p->length1;
            TotalMem+=tmp<<32;
        }
        p++;
        if (p->type>4)
        {
            break;
        }
    }
    color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);
}
