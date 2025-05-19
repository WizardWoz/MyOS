#include "memory.h"
#include "lib.h"

/*函数：初始化目标物理页对应的struct Page结构体，并更新目标物理页所在的物理区域struct Zone内的统计信息
  参数：
  1.struct Page *page：需要初始化的新页面对应的结构体指针
  2.unsigned long flags：指示当前新页面属性的一些宏定义
  返回值：unsigned long，目前暂时为0
*/
unsigned long page_init(struct Page *page, unsigned long flags)
{
    if (!page->attribute) // 新页面的属性不是0
    {
        // 先置位bits_map页映射位图的相应位（(page->PHY_address>>PAGE_2M_SHIFT)>>6)和(page->PHY_address>>PAGE_2M_SHIFT)%64意义相同
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->attribute = flags;               // struct Page结构体的属性是传入的宏定义参数
        page->reference_count++;               // 该页的引用次数+1
        page->zone_struct->page_using_count++; // 该页所属的可用内存区域的已使用物理内存页数量+1
        page->zone_struct->page_free_count--;  // 该页所属的可用内存区域的空闲物理内存页数量-1
        page->zone_struct->total_pages_link++; // 该页所属的可用内存区域的物理页被引用次数+1
    }
    // 如果新页面属性（或者宏定义参数flags）只有PG_Referenced引用属性或者PG_K_Share_To_U共享属性
    else if ((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
    {
        page->attribute |= flags;              // struct Page结构体的属性是传入的宏定义参数
        page->reference_count++;               // 该页的引用次数+1
        page->zone_struct->total_pages_link++; // 该页所属的可用内存区域的物理页被引用次数+1
    }
    else
    {
        // 否则只是置位bit映射位图的相应位
        *(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        // 并添加页表属性
        page->attribute |= flags;
    }
    return 0;
}

/*函数：初始化物理地址空间，获得并打印物理内存信息
  参数：无
  返回值：无
*/
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
        p++;                                              // 指针自增struct E820 *=20B
        if (p->type > 4 || p->length == 0 || p->type < 1) // 遇到程序运行的脏数据，直接跳出循环
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

    // 计算出物理地址空间的的结束地址（目前的结束地址位于最后一条物理内存段信息中，但不排除其他可能性）
    TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address +
               memory_management_struct.e820[memory_management_struct.e820_length].length;
    // 物理地址空间页映射位图指针，指向内核程序结束地址end_brk的4KB上边界对齐位置处，没有使用PAGE_4K_ALIGN宏函数
    //+PAGE_4K_SIZE-1此操作是为了保留一小段隔离空间，以防止误操作损坏其它空间的数据
    memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    // 把物理地址空间的结束地址按2MB对齐，从而统计出物理地址空间可分页数（包括可用物理内存RAM、内存空洞、ROM地址空间）
    memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
    //(A+B-1)/C的形式在这里是(num_2m_pages+BITS_PER_LONG-1)/BITS_PER_BYTE
    // 计算的是：为了存储num_2m_pages个位，并且确保这些位所在的内存块是以long为单位进行组织的（即总位数是BITS_PER_LONG的整数倍），需要多少字节
    //((unsigned long)物理地址空间结束地址TotalMem>>21)得到2MB物理页的数量（每个2MB物理页对应一个位），加上sizeof(long)*8-1后得到2MB物理页数+63；
    // 2MB物理页数+63与8相除得到存储所有物理页所需的总字节数，这个值已经是 sizeof(long) 的整数倍了
    //(~(sizeof(long)-1))=1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1000
    //&(~(sizeof(long)-1))是一个“确保”或“显式”的对齐操作，即使前面的计算已经隐式地完成了这一点，显式写出来可能是为了代码的明确性或者防止某些不常见的编译器/平台行为
    memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
    // 将整个bits_map映射位图空间全部置为0xFF，以标注非内存页（内存空洞、ROM空间）以被使用，随后再将可用物理内存页（RAM空间）复位
    memset(memory_management_struct.bits_map, 0xFF, memory_management_struct.bits_length);

    // struct Page结构体数组存储空间位于bit映射位图之后，元素数量为物理地址空间可分页数，分配与计算方式与bit映射位图相似，使用PAGE_4K_ALIGN宏函数
    memory_management_struct.pages_struct = (struct Page *)PAGE_4K_ALIGN(memory_management_struct.bits_map + memory_management_struct.bits_length);
    // 把物理地址空间的结束地址按2MB对齐，从而统计出物理地址空间可分页数（包括可用物理内存RAM、内存空洞、ROM地址空间）
    memory_management_struct.pages_size = TotalMem >> PAGE_2M_SHIFT;
    // struct Page结构体数组长度=(2MB物理页数量*sizeof(struct Page)+sizeof(long)-1)&(~(sizeof(long)-1))
    memory_management_struct.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    // 将struct Page结构体数组全部清零以备后续初始化程序使用
    memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_length);

    // struct Zone结构体数组的存储空间位于pages结构体
    memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct +
                                                             memory_management_struct.pages_length + PAGE_4K_SIZE - 1) &
                                                            PAGE_4K_MASK);
    // 目前暂时无法计算出struct Zone结构体数组元素个数，只能将zones_size成员赋值为0
    memory_management_struct.zones_size = 0;
    // 将struct Zone结构体数组成员暂时按照5个来计算
    memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
    // 将struct Zone结构体数组全部清零以备后续初始化程序使用
    memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_length);

    // 再次遍历E820数组完成各数组成员变量（struct Zone、struct Page、0~2MB特殊可用物理内存页等）的初始化
    for (i = 0; i <= memory_management_struct.e820_length; i++)
    {
        unsigned long start, end;
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
        // 新的struct Zone结构体初始化
        z = memory_management_struct.zones_struct + memory_management_struct.zones_size;
        memory_management_struct.zones_size++;
        z->zone_start_address = start;
        z->zone_end_address = end;
        z->zone_length = end - start;
        z->page_using_count = 0;
        z->page_free_count = (end - start) >> PAGE_2M_SHIFT;
        z->total_pages_link = 0;
        z->attribute = 0;
        z->GMD_struct = &memory_management_struct;
        z->pages_length = (end - start) >> PAGE_2M_SHIFT;
        z->pages_group = (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));
        // 从该可用物理内存段的首个可用物理内存页开始，初始化该可用物理内存段（区域）内包含的每个可用物理内存页
        p = z->pages_group;
        for (j = 0; j < z->pages_length; j++, p++)
        {
            p->zone_struct = z;
            p->PHY_address = start + PAGE_2M_SIZE * j;
            p->attribute = 0;
            p->reference_count = 0;
            p->age = 0;
            // 把当前struct Page结构体代表的物理地址转换成bits_map映射位图中对应的位；由于此前已将bits_map全部置位
            // 此刻再将可用物理页对应的位与1进行异或操作，将对应的可用物理页标记为未使用
            *(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
        }
    }
    // 因为0~2MB的物理内存页包含多个物理内存段，还囊括内核程序，所以必须对该页进行特殊初始化
    memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;
    memory_management_struct.pages_struct->PHY_address = 0UL;
    memory_management_struct.pages_struct->attribute = 0;
    memory_management_struct.pages_struct->reference_count = 0;
    memory_management_struct.pages_struct->age = 0;
    memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) +
                                             sizeof(long) - 1) &
                                            (~(sizeof(long) - 1));
    // 各数组成员变量初始化结束后，接下来将其中的某些关键信息打印在屏幕上
    color_printk(ORANGE, BLACK, "bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n", memory_management_struct.bits_map, memory_management_struct.bits_size, memory_management_struct.bits_length);
    color_printk(ORANGE, BLACK, "pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n", memory_management_struct.pages_struct,
                 memory_management_struct.pages_size, memory_management_struct.pages_length);
    color_printk(ORANGE, BLACK, "zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n", memory_management_struct.zones_struct,
                 memory_management_struct.zones_size, memory_management_struct.zones_length);

    // 全局变量ZONE_DMA_INDEX和ZONE_NORMAL_INDEX暂且无法区分，故先将它们指向同一个struct Zone区域空间
    ZONE_DMA_INDEX = 0;
    ZONE_NORMAL_INDEX = 0;
    // 遍历显示各区域空间结构体struct Zone详细统计信息
    for (i = 0; i < memory_management_struct.zones_size; i++)
    {
        struct Zone *z = memory_management_struct.zones_struct + i;
        color_printk(ORANGE, BLACK, "zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n",
                     z->zone_start_address, z->zone_end_address, z->zone_length, z->pages_group, z->pages_length);
        // 如果当前区域的起始地址是0x100000000，就将此区域索引值记录在全局变量ZONE_UNMAPPED_INDEX中，表示从该区域空间开始的物理内存页未经过页表映射
        if (z->zone_start_address == 0x100000000)
        {
            ZONE_UNMAPPED_INDEX = i;
        }
    }
    // 最后调整成员变量end_of_struct的值，记录内存页管理结构的结尾地址，+sizeof(long)*32是预留了一段内存空间防止越界访问
    memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct +
                                                             memory_management_struct.zones_length + sizeof(long) * 32) &
                                             (~(sizeof(long) - 1));

    // 内存管理单元初始化完毕后，必须初始化内存管理单元结构struct Global_Memory_Descriptor所占物理页的struct Page
    color_printk(ORANGE, BLACK, "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",
                 memory_management_struct.start_code, memory_management_struct.end_code, memory_management_struct.end_data, memory_management_struct.end_brk, memory_management_struct.end_of_struct);
    i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;
    for (j = 0; j <= i; j++)
    {
        page_init(memory_management_struct.pages_struct + j, PG_PTable_Mapped | PG_Kernel_Init | PG_Active | PG_Kernel);
    }

    // 用于清空曾经用于一致性页表映射的页表项，此刻无需再保留一致性页表映射
    Global_CR3 = Get_gdt(); // 读取CR3保存的PML4顶层页目录表基地址
    // Glonal_CR3=0x0000000000101000；*Glonal_CR3=0x0000000000102000；**Glonal_CR3=0x0000000000103000
    color_printk(INDIGO, BLACK, "Glonal_CR3\t:%#018lx\n", Global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n", *Phy_To_Virt(Global_CR3) & (~0xff));
    color_printk(PURPLE, BLACK, "**Global_CR3\t:%018lx\n", *Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));
    // 为消除一致性页表映射，特将PML4顶层页目录表的前10个页表项清零（其实只需将第1项清零就好，因为head.S中只初始化了第1项）
    for (i = 0; i < 10; i++)
    {
        *(Phy_To_Virt(Global_CR3) + i) = 0UL; // Phy_To_Virt宏函数操作对象强制转换成unsigned long *，所以+1相当于+8B
    }
    flush_tlb(); // 虽然已将页表项清零，但不会立即生效，必须调用flush_tlb()函数才能使更改生效
}

/*函数：完成可用物理内存页的分配
  参数：
  1.int zone_select：选择哪种内存区域（DMA区域空间，已映射页表区域空间，未映射页表区域空间）
  2.int number：申请的物理内存页数，单次可申请上限为64个页
  3.unsigned long page_flags：物理页需要设置的struct Page属性
*/
struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags)
{
    int i;
    unsigned long page = 0;
    int zone_start = 0;
    int zone_end = 0;
    // 根据zone_select参数判定需要检索的内存区域空间，若无法匹配到相应的内存空间，则打印错误日志并使函数返回
    switch (zone_select)
    {
    case ZONE_DMA:
        zone_start = 0;
        zone_end = ZONE_DMA_INDEX;
        break;
    case ZONE_NORMAL:
        zone_start = ZONE_DMA_INDEX;
        zone_end = ZONE_NORMAL_INDEX;
        break;
    case ZONE_UNMAPPED:
        zone_start = ZONE_UNMAPPED_INDEX;
        zone_end = memory_management_struct.zones_size - 1;
        break;
    default:
        color_printk(RED, BLACK, "alloc_pages error zone_select index\n");
        return NULL;
        break;
    }

    // 已确定内存区域，接下来从该区域中遍历出符合申请条件的struct Page结构体组
    for (i = zone_start; i <= zone_end; i++) // 从目标内存区域起始内存页结构开始遍历，直到内存区域空间的结尾
    {
        struct Zone *z;
        unsigned long j;
        // start存储当前内存区域的起始页号，end存储当前内存区域的结束页号，length存储当前内存区域长度
        unsigned long start, end, length;
        unsigned long tmp;
        if ((memory_management_struct.zones_struct + i)->page_free_count < number)
        {
            continue;
        }
        // 起始内存页结构对应的bit映射位图往往位于非对齐（unsigned long类型）位置处，每次将按unsigned long类型
        // 作为步进长度，同时按unsigned long对齐，所以起始页的映射位图只能检索tmp=64-start%64次
        z = memory_management_struct.zones_struct + i;
        start = z->zone_start_address >> PAGE_2M_SHIFT;
        end = z->zone_end_address >> PAGE_2M_SHIFT;
        length = z->zone_length >> PAGE_2M_SHIFT;
        tmp = 64 - start % 64;
        // j += j % 64 ? tmp : 64将索引变量j调整到对齐位置
        for (j = start; j <= end; j += j % 64 ? tmp : 64)
        {
            unsigned long *p = memory_management_struct.bits_map + (j >> 6);
            unsigned long shift = j % 64;
            unsigned long k;
            for (k = shift; k < 64 - shift; k++)
            {
                // 为保证alloc_pages函数最多可检索出64个连续的物理页，使用(*p >> k) | (*(p + 1) << (64 - k))
                // 将后一个unsigned long变量的低位部分补齐到正在检索的变量
                // 对64位寄存器来说左移范围是0~63.当申请值number是64必须经过特殊处理number == 64 ? 0xFFFFFFFFFFFFFFFFUL : ((1UL << number) - 1)
                if (!(((*p >> k) | (*(p + 1) << (64 - k))) & (number == 64 ? 0xFFFFFFFFFFFFFFFFUL : ((1UL << number) - 1))))
                {
                    unsigned long l;
                    page = j + k - 1;
                    // 如果检索出满足条件的物理页组，便使用page_init将bit映射位图对应的内存页结构struct page初始化，
                    // 并使用goto find_free_pages返回第一个内存页结构的地址
                    for (l = 0; l < number; l++)
                    {
                        struct Page *x = memory_management_struct.pages_struct + page + l;
                        page_init(x, page_flags);
                    }
                    goto find_free_pages;
                }
            }
        }
    }
    return NULL;
find_free_pages:
    return (struct Page *)(memory_management_struct.pages_struct + page);
}