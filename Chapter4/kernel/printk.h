/*position结构体：存储屏幕信息，记录当前屏幕分辨率、字符光标所在位置、
  字符像素矩阵尺寸、帧缓冲区起始地址和帧缓冲区容量大小
*/
#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>
#include "font.h"
#include "linkage.h"

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

/*宏函数：判断当前字符是否是0~9以内的数字
*/
#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED	0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫

/*ASCII字库文件数组
*/
extern unsigned char font_ascii[256][16];

/*缓冲区
*/
char buf[4096]={0};

/*用于屏幕信息的结构体：
  1.当前屏幕分辨率
  2.字符光标所在位置
  3.字符像素矩阵尺寸
  4.帧缓存区起始地址
  5.帧缓存区容量大小
*/
struct position
{
	int XResolution;		 // 屏幕横向分辨率
	int YResolution;		 // 屏幕纵向分辨率
	int XPosition;			 // 字符光标横向坐标
	int YPosition;			 // 字符光标纵向坐标
	int XCharSize;			 // 字符像素矩阵横向尺寸
	int YCharSize;			 // 字符像素矩阵纵向尺寸
	unsigned int *FB_addr;	 // 帧缓冲区起始地址
	unsigned long FB_length; // 帧缓冲区容量大小
} Pos;

void putchar(unsigned int * fb,int Xsize,int x,int y,unsigned int FRcolor,unsigned int BKcolor,unsigned char font);

int skip_atoi(const char **s);

#define do_div(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })

static char * number(char * str, long num, int base, int size, int precision ,int type);

int vsprintf(char * buf,const char *fmt, va_list args);

int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...);

#endif