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

/*函数：带颜色打印单个字符
  函数参数：
  1.unsigned int * fb：帧缓冲区首地址
  2.int Xsize：屏幕横向分辨率（即屏幕单行包括的像素点数）
  3.int x：符光标横向坐标*字符像素矩阵横向尺寸
  4.int y：字符光标纵向坐标*字符像素矩阵纵向尺寸
  5.unsigned int FRcolor：字符颜色
  6.unsigned int BKcolor：背景颜色
  7.unsigned char font：需要打印的字符在printk.h的font_ascii数组的行下标
  函数返回值：void
*/
void putchar(unsigned int *fb, int Xsize, int x, int y, unsigned int FRcolor, unsigned int BKcolor, unsigned char font)
{
    int i = 0, j = 0;
    unsigned int *addr = NULL;   // 待显示字符矩阵的起始线性地址
    unsigned char *fontp = NULL; // 某个ASCII编码在字库中首地址
    int testval = 0;             // testval补码表示，用来测试当前要显示的是字体颜色还是背景颜色
    fontp = font_ascii[font];    // fontp是需要打印的字符在font.h的font_ascii数组对应像素位图的首行地址
    // 对于当前字符的像素位图（16行*8列），从第一个像素点开始将字体颜色和背景颜色按位图描述填充到相应线性地址空间中
    // 字符显示实质是填写像素点的帧缓存：只需将位图中为1的位的写入字体颜色值FRcolor，为0的位写入背景颜色值BKcolor
    // 例：'!'对应的像素位图{0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x10,0x10,0x00,0x00}
    for (i = 0; i < 16; i++) // i：字符像素位图当前绘制行的行号
    {
        // 待显示字符矩阵的起始线性地址=帧缓存区首地址fb+屏幕横向分辨率*(字符光标纵向坐标*字符像素矩阵纵向尺寸+当前字符像素位图的行号)+字符光标纵向坐标*字符像素矩阵纵向尺寸
        addr = fb + Xsize * (y + i) + x;
        testval = 0x100; // testval=0000 0000 0000 0000 0000 0001 0000 0000，方便每一行8个像素点的填充
        for (j = 0; j < 8; j++)
        {
            // 对于int类型的非负数，>>执行逻辑右移（填充0）
            // 对于int类型的负数，C标准规定>>的行为是实现定义的，但实际上几乎所有常见的编译器都实现为算术右移（填充符号位）
            // j=0，testval=1000 0000；j=1，testval=0100 0000；......j=7，testval=0000 0001
            testval = testval >> 1;
            // 取出fontp指向当前像素点位图地址存放的内容，并与testval作与操作，两者均不为0
            if (*fontp & testval)
                *addr = FRcolor;
            else
                *addr = BKcolor;
            addr++; // 转至当前行下一个像素点地址
        }
        fontp++; // 当前字符像素位图的下一行地址
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
  1.char * buf：存放格式化字符串的缓冲区首地址指针
  2.const char * fmt：存放格式化字符串的缓冲区首地址指针
  函数返回值：int，字符串长度
*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
    char *str, *s;  //str：指向缓冲区buf的指针，解析字符串时移动str；s：指向va_list可变参数args中字符串参数的指针
    int flags;       // 记录数据区域的对齐方式：用'0'还是用' '对齐
    int field_width; // 记录数据区域的宽度
    int precision;   // 记录数据的精度
    int len, i;     //len记录va_list可变参数args中字符串的长度，i是处理字符串复制时的下标
    int qualifier; // 记录数据的规格：'h'、'l'、'L'、'Z'
    for (str = buf; *fmt; fmt++)
    {
        // 如果字符不为'%'则是可显示字符，直接存入缓冲区buf
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
        // 如果字符是'%'，后面可接'-'、'+'、' '、'#'、'0'等格式符，
        flags = 0;
    // 如果后一个字符是上述格式符，则设置flags标志；若不是则跳出repeat
    repeat:
        fmt++;
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;    //回到repeat标号处继续扫描格式化字符串fmt
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
        // 计算出数据区域的宽度
        field_width = -1;
        // 如果'%'的下一字符是数字字符，则直接表示数据区域宽度，并将后续数字字符转化为数值以表示数据区域宽度
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        // 如果'%'的下一字符是'*'，则数据区域宽度将由可变参数提供，根据可变参数值判断数据区域的左/右对齐显示方式
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT; // 设置为左对齐
            }
        }
        // 确定显示数据的精度
        precision = -1;
        if (*fmt == '.')
        {
            fmt++;
            if (is_digit(*fmt))
            {
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == '*')
            {
                fmt++;
                precision = va_arg(args, int);
            }
            // 默认精度为0
            if (precision < 0)
            {
                precision = 0;
            }
        }
        // 检测显示数据的规格
        // 例：%ld格式化字符串的字母'l'，就表示显示数据的规格是长整形（long）
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
        {
            qualifier = *fmt;
            fmt++;
        }
        // va_list args可变参数的字符串转化过程，并存入buf缓冲区
        switch (*fmt)
        {
        case 'c':   // 匹配出格式符c，将可变参数转换为一个字符
            if (!(flags & LEFT)) // 如果是右对齐
            {
                while (--field_width > 0)
                {
                    *str++ = ' '; // 输出field_width个' '
                }
            }
            *str++ = (unsigned char)va_arg(args, int);
            while (--field_width>0) //如果是左对齐
            {
                *str++=' ';     // 输出field_width个' '
            }
            break;
        case 's':   // 匹配出格式符s，将可变参数转换为一个字符串
            s =va_arg(args,char*);
            if (!s)     //如果字符串s为空
            {
                s='\0'; //字符串结尾添加终结符'\0'
            }
            //如果字符串非空
            len=strlen(s);
            if (precision<0)    //将字符串长度与显示精度进行比较
            {
                precision=len;
            }
            else if (len>precision)
            {
                len=precision;
            }
            if (!(flags&LEFT))  //如果字符串按右对齐
            {
                while (len<field_width--)
                {
                    *str++=' ';     //数据区域宽度仍比字符串长度大，继续输出' '
                }
            }
            for ( i = 0; i < len; i++)
            {
                *str++=*s++;    //输出字符串
            }
            while (len<field_width--)//如果字符串按左对齐
            {
                *str++=' ';     //数据区域宽度仍比字符串长度大，继续输出' '
            }
            break;
        case 'o':   // 匹配出格式符o，将可变参数转换为一个八进制数
            if (qualifier=='l')
            {
                //number函数将long长整形变量值转换成指定八进制规格（由第3个参数int size=8决定）
                str=number(str,va_arg(args,unsigned long),8,field_width,precision,flags);
            }
            else
            {
                //number函数将int整形变量值转换成指定八进制规格（由第3个参数int size=8决定）
                str=number(str,va_arg(args,unsigned int),8,field_width,precision,flags);
            }
            break;
        case 'p':   // 匹配出格式符p，将可变参数转换为一个十六进制地址
            if (field_width==-1)    //当前为默认数据区长度
            {
                field_width=2*sizeof(void *);   //在IA-32e 64位长模式下sizeof(void *)=8
                flags|=ZEROPAD;     //使用'0'填充数据区的剩余位置 
            }
            str=number(str,(unsigned long)va_arg(args,void *),16,field_width,precision,flags);
            break;
        case 'x':   // 匹配出格式符x，将可变参数按小写格式打印为一个十六进制数
            flags|=SMALL;   //没有break;将会继续执行以下的case 'X'标号语句
        case 'X':   // 匹配出格式符X，将可变参数按源格式打印为一个十六进制数
            if (qualifier=='l')
            {
                //number函数将long长整形变量值转换成指定十六进制规格（由第3个参数int size=16决定）
                str=number(str,va_arg(args,unsigned long),16,field_width,precision,flags);
            }
            else
            {
                //number函数将int整形变量值转换成指定十六进制规格（由第3个参数int size=16决定）
                str=number(str,va_arg(args,unsigned int),16,field_width,precision,flags);
            }
            break;
        case 'd':   // 匹配出格式符d，目前没有对应操作
        case 'i':   // 匹配出格式符i，将可变参数按有符号数打印为一个十进制数
            flags|=SIGN;    //没有break;将会继续执行以下的case 'u'标号语句
        case 'u':
            if (qualifier=='l')
            {
                //number函数将long长整形变量值转换成指定十进制规格（由第3个参数int size=10决定）
                str=number(str,va_arg(args,unsigned long),10,field_width,precision,flags);
            }
            else
            {
                //number函数将int整形变量值转换成指定十进制规格（由第3个参数int size=10决定）
                str=number(str,va_arg(args,unsigned int),10,field_width,precision,flags);
            }
            break;
        case 'n':   // 匹配出格式符n，把目前已经格式化的字符串长度返回给函数调用者
            if (qualifier=='l')
            {
                long *ip=va_arg(args,long *);
                *ip=(str-buf);
            }
            else
            {
                int *ip=va_arg(args,int *);
                *ip=(str-buf);
            }
            break;
        case '%':   // 匹配出格式符%，即出现%%；把第1个%视作转义符，最终只显示一个%
            *str++='%';
            break;
        default:    //在格式符解析过程中出现任何不支持的格式符，直接将其作为字符串输出到buf缓冲区
            *str++='%';
            if (*fmt)
            {
                *str++=*fmt;
            }
            else
            {
                fmt--;
            }
            break;
        }
    }
    *str = '\0';        //为解析后的格式化字符串添加终结符'\0'
    return str - buf;   //返回所解析的格式化字符串长度
}

/*函数：带颜色打印传入的格式化字符串
  函数参数：
  1.unsigned int FRcolor：字符颜色
  2.unsigned int BKcolor：背景颜色
  3.const char * fmt：指向格式化字符串的指针
  函数返回值：int，打印的格式化字符串的长度
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
        // 如果当前\t的制表符位置对应的空格符还未调整完毕，直接goto跳转到继续调整的语句
        if (line > 0)
        {
            count--;
            goto Label_tab;
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
            // 此时是从当前行当前位置回到当前行前一位置
            if (Pos.XPosition < 0)
            {
                // 如果字符光标横向坐标-1后小于0，则调整字符光标横向坐标至该行最后一个位置
                Pos.XPosition = Pos.XResolution / Pos.XCharSize - 1;
                Pos.YPosition--; // 字符光标纵向坐标-1
                // 此时是从当前行的首部回到了上一行的尾部
                if (Pos.YPosition < 0)
                {
                    // 如果字符光标纵向坐标-1后小于0，则调整字符光标纵向坐标至该列最后一个位置
                    Pos.YPosition = Pos.YResolution / Pos.YCharSize - 1;
                    // 此时是从当前页坐标原点(0,0)位置到了上一页最右下角
                }
            }
            // 行，列位置都调整好了，调用putchar函数在屏幕上输出' '覆盖之前的字符
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize,
                    FRcolor, BKcolor, ' ');
        }
        //\t的含义是制表符，相当于Tab键
        else if ((unsigned char)*(buf + count) == '\t')
        {
            // 计算当前光标距离下一个制表位需要填充的空格符数量，将计算结果保存到line变量中
            // Pos.XPosition+8表示MyOS的一个标准制表符占用8个空格符；~(8-1)=0x1000；((Pos.XPosition + 8) & ~(8 - 1))
            // 假设Pos.XPosition=0，则需要绘制(0+8)&0x1000-0=8个空格；假设Pos.XPosition=1，则需要绘制(1+8)&0x1000-1=7个空格；......假设Pos.XPosition=7，则需要绘制(7+8)&0x1000-7=1个空格
            // 以此类推，Pos.XPosition=8，则需要绘制(8+8)&0x1000-8=8个空格，再次进入绘制1~8个空格的循环
            line = ((Pos.XPosition + 8) & ~(8 - 1)) - Pos.XPosition;
        Label_tab:
            line--; // 需要填充的空格符数量-1
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize,
                    FRcolor, BKcolor, ' ');
            Pos.XPosition++; // 字符光标横向坐标+1
        }
        // 待显示字符排除是\n、\b、\t后，则为一个普通的字符
        else
        {
            // 直接使用putchar函数打印字符
            putchar(Pos.FB_addr, Pos.XResolution, Pos.XPosition * Pos.XCharSize, Pos.YPosition * Pos.YCharSize,
                    FRcolor, BKcolor, (unsigned char)*(buf + count));
            Pos.XPosition++; // 字符光标横向坐标+1
        }
        // 字符显示结束后还要为下次字符显示作准备，即更新当前字符的显示位置
        if (Pos.XPosition >= (Pos.XResolution / Pos.XCharSize))
        {
            // 当前行已写满字符，需要将字符光标纵向坐标下移一行，并将字符光标横向坐标设置为0
            Pos.YPosition++;
            Pos.XPosition = 0;
        }
        if (Pos.YPosition >= (Pos.YPosition / Pos.YCharSize))
        {
            // 当前列已写满字符，需要将字符光标纵向坐标设置为0，实际上是当前页已经写满了，进入下一页
            Pos.YPosition = 0;
        }
    }
    return i;
}