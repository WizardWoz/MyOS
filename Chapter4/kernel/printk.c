#include <stdarg.h> //GNU C编译环境自带的头文件，因为color_printk函数支持可变参数，只有添加该头文件才可使用可变参数
#include "printk.h"
#include "lib.h"
#include "linkage.h"

/*声明并定义可变参数函数
  1.在函数声明中使用省略号（...）：在函数的参数列表的末尾，紧跟在最后一个固定参数之后，使用省略号表示可变参数的存在。
  函数必须至少有一个固定参数，这个固定参数通常用于指示可变参数的数量或类型
  2.包含<stdarg.h>头文件：访问可变参数所需的宏定义都在这个头文件中
  3.定义va_list类型的变量：在函数内部，需要定义一个va_list类型的变量，用于存储可变参数的信息
  4.使用va_start初始化va_list：在访问可变参数之前，必须调用va_start宏来初始化va_list变量。它需要两个参数：
  第一个是va_list变量本身，第二个是函数中的最后一个固定参数的名称；作用是让args指向可变参数列表的起始位置
  5.使用va_arg访问可变参数：每次调用va_arg宏都会获取可变参数列表中的下一个参数。它需要两个参数：
  第一个是va_list变量，第二个是当前要获取的参数的类型。使用va_arg时，你需要知道每个可变参数的类型
  va_arg会根据指定的类型从内存中读取相应字节的数据，并自动将va_list指向下一个参数。因此，调用va_arg的顺序和类型必须与实际传递的可变参数一致
  6.使用va_end清理：在函数返回之前，必须调用va_end宏来清理va_list。这通常会释放与可变参数列表相关的任何资源
*/

/*函数：打印\n、\b、\t等转义符
  函数参数：
  1.unsigned int * fb：帧缓冲区首地址
  2.int Xsize：屏幕横向分辨率
  3.int x：字符光标横向坐标*字符像素矩阵横向尺寸
  4.int y：字符光标纵向坐标*字符像素矩阵纵向尺寸
  5.unsigned int FRcolor：
  6.unsigned int BKcolor：
  7.unsigned char font：
  函数返回值：void
*/
void putchar(unsigned int *fb, int Xsize, int x, int y, unsigned int FRcolor, unsigned int BKcolor, unsigned char font)
{
    putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, ' ');
    int i = 0, j = 0;
    unsigned int *addr = NULL;   // 待显示字符矩阵的起始线性地址
    unsigned char *fontp = NULL; // 某个ASCII编码在字库中首地址
    int testval = 0;
    fontp = font_ascii[font];
    // 从字符首像素地址开始，将字体颜色和背景色的数值
    for (i = 0; i < 16; i++)
    {
        addr = fb + Xsize * (y + i) + x;
        testval = 0x100;
        for (j = 0; j < 8; j++)
        {
            testval = testval >> 1;
            if (*fontp & testval)
                *addr = FRcolor;
            else
                *addr = BKcolor;
            addr++;
        }
        fontp++;
    }
}

/*

*/
int skip_atoi(const char **s)
{
    int i = 0;

    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

/*

*/
static char *number(char *str, long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[50];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;

    if (type & SMALL)
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return 0;
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN && num < 0)
    {
        sign = '-';
        num = -num;
    }
    else
        sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
    if (sign)
        size--;
    if (type & SPECIAL)
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
        while (num != 0)
            tmp[i++] = digits[do_div(num, base)];
    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type & (ZEROPAD + LEFT)))
        while (size-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL)
        if (base == 8)
            *str++ = '0';
        else if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33];
        }
    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;

    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

/*解析color_printk函数提供的格式化字符串及其参数
  函数参数：
  1.
  函数返回值：int，字符串长度
*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
    char *str, *s;
    int flags;
    int field_width;
    int precision;
    int len, i;

    int qualifier; /* 'h', 'l', 'L' or 'Z' for integer fields */

    for (str = buf; *fmt; fmt++)
    {

        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
        flags = 0;
    repeat:
        fmt++;
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */

        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */

        precision = -1;
        if (*fmt == '.')
        {
            fmt++;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                fmt++;
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
        {
            qualifier = *fmt;
            fmt++;
        }

        switch (*fmt)
        {
        case 'c':

            if (!(flags & LEFT))
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char)va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            break;

        case 's':

            s = va_arg(args, char *);
            if (!s)
                s = '\0';
            len = strlen(s);
            if (precision < 0)
                precision = len;
            else if (len > precision)
                len = precision;

            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; i++)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            break;

        case 'o':

            if (qualifier == 'l')
                str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flags);
            else
                str = number(str, va_arg(args, unsigned int), 8, field_width, precision, flags);
            break;

        case 'p':

            if (field_width == -1)
            {
                field_width = 2 * sizeof(void *);
                flags |= ZEROPAD;
            }

            str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
            break;

        case 'x':

            flags |= SMALL;

        case 'X':

            if (qualifier == 'l')
                str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
            else
                str = number(str, va_arg(args, unsigned int), 16, field_width, precision, flags);
            break;

        case 'd':
        case 'i':

            flags |= SIGN;
        case 'u':

            if (qualifier == 'l')
                str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
            else
                str = number(str, va_arg(args, unsigned int), 10, field_width, precision, flags);
            break;

        case 'n':

            if (qualifier == 'l')
            {
                long *ip = va_arg(args, long *);
                *ip = (str - buf);
            }
            else
            {
                int *ip = va_arg(args, int *);
                *ip = (str - buf);
            }
            break;

        case '%':

            *str++ = '%';
            break;

        default:

            *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                fmt--;
            break;
        }
    }
    *str = '\0';
    return str - buf;
}

/*函数：带颜色打印传入的格式化字符串
  函数参数：
  1.unsigned int FRcolor：字符颜色
  2.unsigned int BKcolor：背景颜色
  3.const char * fmt：指向格式化字符串的指针
  函数返回值：int，打印的格式化字符串
*/
int color_printk(unsigned int FRcolor, unsigned int BKcolor, const char *fmt, ...)
{
    int i = 0;
    int count = 0;
    int line = 0;
    // 定义va_list类型的变量：在函数内部，需要定义一个va_list类型的变量，用于存储可变参数的信息
    va_list args;
    // 使用va_start初始化va_list：在访问可变参数之前，必须调用va_start宏来初始化va_list变量。它需要两个参数：
    // 第一个是va_list变量本身，第二个是函数中的最后一个固定参数的名称；作用是让args指向可变参数列表的起始位置
    va_start(args, fmt);
    // vsprintf解析color_printk函数提供的格式化字符串（保存在4096Bytes的缓冲区buf）及其参数，并返回字符串长度
    i = vsprintf(buf, fmt, args);
    // 使用va_end清理：在函数返回之前，必须调用va_end宏来清理va_list。这通常会释放与可变参数列表相关的任何资源
    va_end(args);
    // 逐个字符检索buf缓冲区内的格式化字符串，找出\n、\b、\t等转义符，并在屏幕打印格式化字符串的时候解析这些转义符
    for (count = 0; count < i || line; count++)
    {
        // 如果当前为空行
        if (line > 0)
        {
            count--;
            goto Label_tab; // 无条件跳转至Label_tab标号处执行
        }
        //\n的含义是：在Windows中表示换行且回到下一行的最开始位置。在Linux、unix中只表示换行，但不会回到下一行的开始位置
        if ((unsigned char)*(buf + count) == '\n')
        {
            // MyOS模拟Windows的\n效果：换行且回到下一行的最开始位置
            Pos.YPosition++;   // 字符光标纵向坐标+1
            Pos.XPosition = 0; // 字符光标横向坐标=0
        }
        //\b的含义是：将光标从当前位置向前（左）移动一个字符（遇到\n或\r则停止移动），并从此位置开始输出后面的字符（空字符\0和换行符\n除外）
        else if ((unsigned char)*(buf + count) == '\b')
        {
            Pos.XPosition--; // 字符光标横向坐标-1
            if (Pos.XPosition < 0)
            {
                // 如果字符光标横向坐标-1后小于0，则调整字符光标横向坐标
                Pos.XPosition = (Pos.XResolution / Pos.XCharSize - 1) * Pos.XCharSize;
                Pos.YPosition--; // 字符光标纵向坐标-1
                if (Pos.YPosition < 0)
                    //
                    Pos.YPosition = (Pos.YResolution / Pos.YCharSize - 1) * Pos.YCharSize;
            }
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, ' ');
        }
        else if ((unsigned char)*(buf + count) == '\t')
        {
            line = ((Pos.XPosition + 8) & ~(8 - 1)) - Pos.XPosition;

        Label_tab:
            line--;
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, ' ');
            Pos.XPosition++;
        }
        else
        {
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize, FRcolor, BKcolor, (unsigned char)*(buf + count));
            Pos.XPosition++;
        }

        if (Pos.XPosition >= (Pos.XResolution / Pos.XCharSize))
        {
            Pos.YPosition++;
            Pos.XPosition = 0;
        }
        if (Pos.YPosition >= (Pos.YResolution / Pos.YCharSize))
        {
            Pos.YPosition = 0;
        }
    }
    return i;
}