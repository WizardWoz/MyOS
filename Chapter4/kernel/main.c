/*  内核主程序（内核主函数），相当于应用程序的主函数，在正常情况下不会返回，内核头程序并没有给内核程序提供返回地址
    内核头程序head.S通过lretq跳转至此而不是通过call；并且关机、重启等功能也并非在内核主程序返回过程中实现
    负责调用各系统模块初始化函数，模块初始化结束后会创建出系统第一个进程init，并将控制权交给init
*/
void Start_Kernel(void)
{
    //帧缓存的格式：即一个像素点能显示的颜色值位宽；loader.bin设置显示模式可支持32位颜色深度的像素点
    //0～7：代表蓝色；8～15：代表绿色；16～23：代表红色；24～31：保留位；32bit可组成16M种不同颜色（32位真彩色）
    //1.计算出该点在屏幕上的位置；2.计算出该点与屏幕原点（屏幕原点(0,0)位于屏幕左上角）的偏移值；3.最后设置该点颜色值
    //成功显示图4-3 RGB颜色带图
    int *addr=(int *)0xffff800000a00000;
    int i;
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
    
    while (1)
    {
        ;
    }
}