/*  内核主程序（内核主函数），相当于应用程序的主函数，在正常情况下不会返回，内核头程序并没有给内核程序提供返回地址
    内核头程序head.S通过lretq跳转至此而不是通过call；并且关机、重启等功能也并非在内核主程序返回过程中实现
    负责调用各系统模块初始化函数，模块初始化结束后会创建出系统第一个进程init，并将控制权交给init
*/
#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"

/*经过声明后的extern变量（标识符）会被放在kernel.lds链接脚本指定的位置处*/
extern char _text; // Kernel.lds链接脚本将_text放在线性地址0xFFFF800000100000，使得_text位于内核程序的代码段起始地址
extern char _etext;
extern char _edata;
extern char _end;

/*全局内存管理结构体变量memory_management_struct初始化
  struct e820[32]={0,0,0......,0};
  unsigned long e820_length=0;
*/
struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

void Start_Kernel(void)
{
    // 帧缓存的格式：即一个像素点能显示的颜色值位宽；loader.bin设置显示模式可支持32位颜色深度的像素点
    // 0～7：代表蓝色；8～15：代表绿色；16～23：代表红色；24～31：保留位；32bit可组成16M种不同颜色（32位真彩色）
    // 1.计算出该点在屏幕上的位置；2.计算出该点与屏幕原点（屏幕原点(0,0)位于屏幕左上角）的偏移值；3.最后设置该点颜色值
    // 成功显示P87 图4-3 RGB颜色带图
    int *addr = (int *)0xffff800000a00000;
    int i;                    // 屏幕绘制色带操作显存空间时的索引下标
    struct Page *page = NULL; // 指向分配到的物理内存页的首页起始地址
    // 设置屏幕分辨率（屏幕横、纵向占用像素点数目）
    Pos.XResolution = 1440;
    Pos.YResolution = 900;
    // 设置光标位置
    Pos.XPosition = 0;
    Pos.YPosition = 0;
    // 设置单个字符的横、纵向占用像素点数目
    Pos.XCharSize = 8;
    Pos.YCharSize = 16;
    // 设置帧缓冲区起始地址、容量大小
    Pos.FB_addr = (int *)0xffff800000a00000;
    Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK; // 将帧缓冲区容量与4KB页大小对齐
    // 绘制4个彩色横条（长：1440像素点；宽：20像素点）
    for (i = 0; i < 1440 * 20; i++)
    {
        *((char *)addr + 0) = (char)0x00;
        *((char *)addr + 1) = (char)0x00;
        *((char *)addr + 2) = (char)0xff;
        *((char *)addr + 3) = (char)0x00;
        addr += 1;
    }
    for (i = 0; i < 1440 * 20; i++)
    {
        *((char *)addr + 0) = (char)0x00;
        *((char *)addr + 1) = (char)0xff;
        *((char *)addr + 2) = (char)0x00;
        *((char *)addr + 3) = (char)0x00;
        addr += 1;
    }
    for (i = 0; i < 1440 * 20; i++)
    {
        *((char *)addr + 0) = (char)0xff;
        *((char *)addr + 1) = (char)0x00;
        *((char *)addr + 2) = (char)0x00;
        *((char *)addr + 3) = (char)0x00;
        addr += 1;
    }
    for (i = 0; i < 1440 * 20; i++)
    {
        *((char *)addr + 0) = (char)0xff;
        *((char *)addr + 1) = (char)0xff;
        *((char *)addr + 2) = (char)0xff;
        *((char *)addr + 3) = (char)0x00;
        addr += 1;
    }
    // 打印"Hello World!"字符串，成功显示P100 图4-5
    color_printk(YELLOW, BLACK, "Hello\t\tWorld!\n");
    load_TR(8); // 将TSS段描述符的段选择子加载到TR寄存器
    set_tss64(0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
    sys_vector_init();
    memory_management_struct.start_code = (unsigned long)&_text;
    memory_management_struct.end_code = (unsigned long)&_etext;
    memory_management_struct.end_data = (unsigned long)&_edata;
    memory_management_struct.end_brk = (unsigned long)&_end;
    // 触发向量号为0的#DE除法错误异常，成功显示P109 图4-8
    // i=1/0;
    // 触发向量号为14的#PF页错误异常，成功显示P120 图4-11、P121 图4-12
    // i=*(int *)0xffff80000aa00000;
    // 从物理地址0x7E00（线性地址为0xFFFF800000007E00）处获取物理内存信息
    color_printk(RED, BLACK, "memory init\n");
    init_memory();

    color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *memory_management_struct.bits_map);
    color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *(memory_management_struct.bits_map + 1));
    page = alloc_pages(ZONE_NORMAL, 64, PG_PTable_Mapped | PG_Active | PG_Kernel);
    // 虚拟平台前64个内存页结构的属性值被设置成0x91，而且物理地址从0x200000开始，与zonr_start_address成员变量记录一致
    // 进而说明alloc_pages函数从本区域空间起始地址处分配物理内存页
    // 区域空间第64，65内存页属性依然为0，说明这两个内存页未被分配过，bit映射位图从0x0000 0000 0000 0001和0x0000 0000 0000 0000
    // 变为如今0xFFFF FFFF FFFF FFFF和0x0000 0000 0000 0001；同样是置位64个映射位
    for (i = 0; i <= 64; i++)
    {
        color_printk(INDIGO, BLACK, "page%d\tattribute:%#018lx\taddress:%#018lx\t", i, (page + i)->attribute, (page + i)->PHY_address);
        i++;
        color_printk(INDIGO, BLACK, "page%d\tattribute:%#018lx\taddress:%#018lx\n", i, (page + i)->attribute, (page + i)->PHY_address);
    }
    color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *memory_management_struct.bits_map);
    color_printk(RED, BLACK, "memory_management_struct.bits_map:%#018lx\n", *(memory_management_struct.bits_map + 1));

    while (1)
    {
        ;
    }
}