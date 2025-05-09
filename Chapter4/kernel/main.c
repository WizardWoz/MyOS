/*  内核主程序（内核主函数），相当于应用程序的主函数，在正常情况下不会返回，内核头程序并没有给内核程序提供返回地址
    内核头程序head.S通过lretq跳转至此而不是通过call；并且关机、重启等功能也并非在内核主程序返回过程中实现
    负责调用各系统模块初始化函数，模块初始化结束后会创建出系统第一个进程init，并将控制权交给init
*/
#include "lib.h"
#include "printk.h"
#include "gate.h"

void Start_Kernel(void)
{
    //帧缓存的格式：即一个像素点能显示的颜色值位宽；loader.bin设置显示模式可支持32位颜色深度的像素点
    //0～7：代表蓝色；8～15：代表绿色；16～23：代表红色；24～31：保留位；32bit可组成16M种不同颜色（32位真彩色）
    //1.计算出该点在屏幕上的位置；2.计算出该点与屏幕原点（屏幕原点(0,0)位于屏幕左上角）的偏移值；3.最后设置该点颜色值
    //成功显示P87 图4-3 RGB颜色带图
    int *addr=(int *)0xffff800000a00000;
    int i;
    //设置屏幕分辨率（屏幕横、纵向占用像素点数目）
    Pos.XResolution=1440;
    Pos.YResolution=900;
    //设置光标位置
    Pos.XPosition=0;
    Pos.YPosition=0;
    //设置单个字符的横、纵向占用像素点数目
    Pos.XCharSize=8;
    Pos.YCharSize=16;
    //设置帧缓冲区起始地址、容量大小
    Pos.FB_addr=(int *)0xffff800000a00000;
    Pos.FB_length=(Pos.XResolution*Pos.YResolution*4);
    //绘制4个彩色横条（长：1440像素点；宽：20像素点）
    for ( i = 0; i < 1440*20; i++)
    {
        *((char *)addr+0)=(char)0x00;
        *((char *)addr+1)=(char)0x00;
        *((char *)addr+2)=(char)0xff;
        *((char *)addr+3)=(char)0x00;
        addr+=1;
    }
    for ( i = 0; i < 1440*20; i++)
    {
        *((char *)addr+0)=(char)0x00;
        *((char *)addr+1)=(char)0xff;
        *((char *)addr+2)=(char)0x00;
        *((char *)addr+3)=(char)0x00;
        addr+=1;
    }
    for ( i = 0; i < 1440*20; i++)
    {
        *((char *)addr+0)=(char)0xff;
        *((char *)addr+1)=(char)0x00;
        *((char *)addr+2)=(char)0x00;
        *((char *)addr+3)=(char)0x00;
        addr+=1;
    }
    for ( i = 0; i < 1440*20; i++)
    {
        *((char *)addr+0)=(char)0xff;
        *((char *)addr+1)=(char)0xff;
        *((char *)addr+2)=(char)0xff;
        *((char *)addr+3)=(char)0x00;
        addr+=1;
    }
    //打印"Hello World!"字符串，成功显示P100 图4-5
    color_printk(YELLOW,BLACK,"Hello\t\tWorld!\n");
    //触发向量号为0的除法错误异常,成功显示P109 图4-8
    i=1/0;
    while (1)
    {
        ;
    }
}