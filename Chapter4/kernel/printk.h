/*position结构体：存储屏幕信息，记录当前屏幕分辨率、字符光标所在位置、
  字符像素矩阵尺寸、帧缓冲区起始地址和帧缓冲区容量大小
*/
struct position
{
    int XResolution;    //屏幕横向分辨率
    int YResolution;    //屏幕纵向分辨率
    int XPosition;      //字符光标横向坐标
    int YPosition;      //字符光标纵向坐标
    int XCharSize;      //字符像素矩阵横向尺寸
    int YCharSize;      //字符像素矩阵纵向尺寸
    unsigned int *FB_addr;  //帧缓冲区起始地址
    unsigned long FB_length;//帧缓冲区容量大小
}Pos;

