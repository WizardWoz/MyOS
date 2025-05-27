void Start_Kernel(void)
{
    // 帧缓存的格式：即一个像素点能显示的颜色值位宽；loader.bin设置显示模式可支持32位颜色深度的像素点
    // 0～7：代表蓝色；8～15：代表绿色；16～23：代表红色；24～31：保留位；32bit可组成16M种不同颜色（32位真彩色）
    // 1.计算出该点在屏幕上的位置；2.计算出该点与屏幕原点（屏幕原点(0,0)位于屏幕左上角）的偏移值；3.最后设置该点颜色值
    // 成功显示P87 图4-3 RGB颜色带图
    int *addr = (int *)0xffff800000a00000;
    int i;                    // 屏幕绘制色带操作显存空间时的索引下标
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
    while (1)
    {
        ;
    }
}