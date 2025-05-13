#include "memory.h"
#include "lib.h"

void init_memory()
{
    int i, j;
    // 1.系统可用的RAM内存总量；2.统计2MB可用物理内存页数；3.物理地址空间的的结束地址
    unsigned long TotalMem = 0;
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
        if (p->type > 4||p->length==0||p->type<1) // 遇到程序运行的脏数据，直接跳出循环
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
        // 对e820结构体数组中的可用物理内存段的起始地址start进行2MB物理页的上边界对齐
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        // e820结构体数组中的可用物理内存段的结束地址end=((段原起始地址+段长度)>>21)<<21，进行2MB物理页的下边界对齐
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (end <= start) // 如果计算后的起始地址大于等于计算后的结束地址，则为无效内存段，继续下一次循环
        {
            continue;
        }
        // 如果计算后的起始地址小于计算后的结束地址，则为有效内存段，进而计算其可用物理页数量
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }
    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGES:%#010x=%010d\n", TotalMem, TotalMem);
    //计算出物理地址空间的的结束地址（目前的结束地址位于最后一条物理内存段信息中，但不排除其他可能性）
    TotalMem=memory_management_struct.e820[memory_management_struct.e820_length].address+
    memory_management_struct.e820[memory_management_struct.e820_length].length;
    //物理地址空间页映射位图指针，指向内核程序结束地址end_brk的4KB上边界对齐位置处，
    //此操作是为了保留一小段隔离空间，以防止误操作损坏其它空间的数据
    memory_management_struct.bits_map=(unsigned long *)((memory_management_struct.end_brk+PAGE_4K_SIZE-1)&PAGE_4K_MASK);
    //把物理地址空间的结束地址按2MB对齐，从而统计出物理地址空间可分页数（包括可用物理内存RAM、内存空洞、ROM地址空间）
    memory_management_struct.bits_size=TotalMem>>PAGE_2M_SHIFT;
    //(A+B-1)/C的形式在这里是(num_2m_pages+BITS_PER_LONG-1)/BITS_PER_BYTE
    //计算的是：为了存储num_2m_pages个位，并且确保这些位所在的内存块是以long为单位进行组织的（即总位数是BITS_PER_LONG的整数倍），需要多少字节
    //((unsigned long)物理地址空间结束地址TotalMem>>21)得到2MB物理页的数量（每个2MB物理页对应一个位），加上sizeof(long)*8-1后得到2MB物理页数+63；
    //2MB物理页数+63与8相除得到存储所有物理页所需的总字节数，这个值已经是 sizeof(long) 的整数倍了
    //(~(sizeof(long)-1))=1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1000
    //&(~(sizeof(long)-1))是一个“确保”或“显式”的对齐操作，即使前面的计算已经隐式地完成了这一点，显式写出来可能是为了代码的明确性或者防止某些不常见的编译器/平台行为
    memory_management_struct.bits_length=(((unsigned long)(TotalMem>>PAGE_2M_SHIFT)+sizeof(long)*8-1)/8)&(~(sizeof(long)-1));
    //将整个bits_map映射位图空间全部置为0xFF，以标注非内存页（内存空洞、ROM空间）以被使用，随后再将可用物理内存页（RAM空间）复位
    memset(memory_management_struct.bits_map,0xFF,memory_management_struct.bits_length);

    //struct Page结构体数组存储空间位于bit映射位图之后，元素数量为物理地址空间可分页数，分配与计算方式与bit映射位图相似
    memory_management_struct.pages_struct=(struct Page *)(((unsigned long)memory_management_struct.bits_map+memory_management_struct.bits_length
    +PAGE_4K_SIZE-1)&PAGE_4K_MASK);
    //把物理地址空间的结束地址按2MB对齐，从而统计出物理地址空间可分页数（包括可用物理内存RAM、内存空洞、ROM地址空间）
    memory_management_struct.pages_size=TotalMem>>PAGE_2M_SHIFT;
    //struct Page结构体数组长度=(2MB物理页数量*sizeof(struct Page)+sizeof(long)-1)&(~(sizeof(long)-1))
    memory_management_struct.pages_length=((TotalMem>>PAGE_2M_SHIFT)*sizeof(struct Page)+sizeof(long)-1)&(~(sizeof(long)-1));
    //将struct Page结构体数组全部清零以备后续初始化程序使用
    memset(memory_management_struct.pages_struct,0x00,memory_management_struct.pages_length);

    memory_management_struct.zones_struct=(struct Zone *)(((unsigned long)memory_management_struct.pages_struct+
    memory_management_struct.pages_length+PAGE_4K_SIZE-1)&PAGE_4K_MASK);
    //目前暂时无法计算出struct Zone结构体数组元素个数，只能将zones_size成员赋值为0
    memory_management_struct.zones_size=0;
    //将struct Zone结构体数组成员暂时按照5个来计算
    memory_management_struct.zones_length=(5*sizeof(struct Zone)+sizeof(long)-1)&(~(sizeof(long)-1));
    //将struct Zone结构体数组全部清零以备后续初始化程序使用
    memset(memory_management_struct.zones_struct,0x00,memory_management_struct.zones_length);

    for ( i = 0; i < memory_management_struct.e820_length; i++)
    {
        unsigned long start,end;
        struct Zone *z;
        struct Page *p;
        unsigned long *b;
        if (memory_management_struct.e820[i].type != 1)
        {
            continue;
        }
        // 对e820结构体数组中的可用物理内存段的起始地址start进行2MB物理页的上边界对齐
        start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        // e820结构体数组中的可用物理内存段的结束地址end=((段原起始地址+段长度)>>21)<<21，进行2MB物理页的下边界对齐
        end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (end <= start) // 如果计算后的起始地址大于等于计算后的结束地址，则为无效内存段，继续下一次循环
        {
            continue;
        }

        z=memory_management_struct.zones_struct+memory_management_struct.zones_size;
        memory_management_struct.zones_size++;
        z->zone_start_address=start;
        z->zone_end_address=end;
        z->zone_length=end-start;
        z->page_using_count=0;
        z->page_free_count=(end-start)>>PAGE_2M_SHIFT;
        z->total_pages_link=0;
        z->attribute=0;
        z->GMD_struct=&memory_management_struct;
        z->pages_length=(end-start)>>PAGE_2M_SHIFT;
        z->pages_group=(struct Page *)(memory_management_struct.pages_struct+(start>>PAGE_2M_SHIFT));

        p=z->pages_group;
        for ( j = 0; j < z->pages_length; j++,p++)
        {
            p->zone_struct=z;
            p->PHY_address=start+PAGE_2M_SIZE*j;
            p->attribute=0;
            p->reference_count=0;
            p->age=0;
            *(memory_management_struct.bits_map+((p->PHY_address>>PAGE_2M_SHIFT)>>6))^=1UL<<
            (p->PHY_address>>PAGE_2M_SHIFT)%64;
        }
    }
}
