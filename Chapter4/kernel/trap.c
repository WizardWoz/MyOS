#include "trap.h"
#include "gate.h"
// void sys_vector_init()
// {
//     set_trap_gate(0,1,divide_error);
//     set_trap_gate(1,1,debug);

// }

/*函数：向量号为0，#DE错误，无错误码 div或idiv指令触发除法错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_divide_error(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = NULL;
    // 0x98对应上文的符号常量RIP=0x98，意思是将RSP的值向上索引0x98个字节，以获取被中断程序执行现场的RIP寄存器值
    p = (unsigned long *)(rsp + 0x98);
    // #DE异常无错误码，所以显示ERROR_CODE:0x0000000000000000，栈指针值rsp，异常产生的程序地址
    // ERROR_CODE、RSP、RIP的数据区域宽度为18的长整形十六进制数
    color_printk(RED, BLACK, "do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n", error_code, rsp, *p);
    while (1)
    {
    }
}

/*函数：向量号为1，#DB错误/陷阱，无错误码 Intel处理器调试异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_debug(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = NULL;
    p = (unsigned long *)(rsp + 0x98);
    color_printk(RED, BLACK, "do_debug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n", error_code, rsp, *p);
    while (1)
        ;
}

/*函数：向量号为2，#NMI内部中断，无错误码 不可屏蔽中断的中断处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_nmi(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = NULL;
    p = (unsigned long *)(rsp + 0x98);
    // #NMI异常无错误码，所以显示ERROR_CODE:0x0000000000000000，栈指针值rsp，异常产生的程序地址
    color_printk(RED, BLACK, "do_nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n", error_code, rsp, *p);
    while (1)
    {
    }
}

/*函数：向量号为3，#BP陷阱，无错误码 int3指令触发断点异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_int3(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_int3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为4，#OF陷阱，无错误码 into指令触发的溢出异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_overflow(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_overflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为5，#BR错误，无错误码 bound指令触发的越界异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_bounds(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_bounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为6，#UD错误，无错误码 ud2指令或保留的机器码触发的无效/未定义机器码的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_undefined_opcode(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为7，#NM错误，无错误码 浮点指令wait/fwait触发的设备异常（FPU不存在）的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_dev_not_available(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_dev_not_available(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为8，#DF终止，有错误码0 任何异常、NMI内部中断、INTR外部中断触发的双重错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_double_fault(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_double_fault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为9，#-错误（保留），无错误码 浮点指令触发的协处理器段越界的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_coprocessor_segment_overrun(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_coprocessor_segment_overrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为10，#TS错误，有错误码 访问TSS段或任务切换触发的无效的TSS段异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void

  根据中断门、陷阱门、任务门的位宽；错误码可以是一个字（16B）或者一个双字（32B），为保证双字错误码入栈时的栈对齐，高半部分被保留
  低半部分格式与段选择子相似
  第0位EXT：EXT=1说明异常是在向程序投递外部事件的过程中触发（例如一个中断或更早期的异常）
  第1位IDT：IDT=1说明错误码的段选择子部分记录中断描述符表IDT内的描述符；IDT=0说明记录的是全局/局部描述符表GDT/LDT内的描述符
  第2位TI：只有IDT=0时该位才有效；TI=1说明错误码的段选择子部分记录局部描述符表LDT内的描述符，TI=0说明记录的是全局描述符表GDT内的描述符
  第3~15位：段选择子，可以索引IDT、GDT、LDT等描述符表内的段描述符或门描述符；某些条件下错误码是NULL（除EXT外所有位被清零），表明错误并非由引用特殊段或访问NULL段描述符产生
*/
void do_invalid_TSS(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = NULL;
    p = (unsigned long *)(rsp + 0x98);
    color_printk(RED, BLACK, "do_invalid_TSS(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n", error_code, rsp, *p);
    // 使用error_code&0x01的按位与运算来判断error_code的第0位是否为1
    if (error_code & 0x01)
    {
        color_printk(RED, BLACK, "The exception occurred during delivery of an event external to the program, \
        such as an interrupt or an earlier exception.\n");
    }
    // 使用error_code&0x02的按位与运算来判断error_code的第1位是否为1
    if (error_code & 0x02)
    {
        color_printk(RED, BLACK, "Refers to a gate descriptor in the IDT;\n");
    }
    else
    {
        color_printk(RED, BLACK, "Refers to a gate descriptor in the GDT or the current LDT;\n");
    }
    // 在error_code的第1位为0的前提下
    if ((error_code & 0x02) == 0)
    {
        // 使用error_code&0x04的按位与运算来判断error_code的第2位是否为1
        if (error_code & 0x04)
        {
            color_printk(RED, BLACK, "Refers to a segment or gate descriptor in the LDT;\n");
        }
        else
        {
            color_printk(RED, BLACK, "Refers to a descriptor in the current GDT;\n");
        }
    }
    color_printk(RED, BLACK, "Segment Selector Index:%#0x10x\n", error_code & 0xFFF8);
    while (1)
    {
    }
}

/*函数：向量号为11，#NP错误，有错误码 加载段寄存器或访问系统段触发的段不存在的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_segment_not_present(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_segment_not_present(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01)
		color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02)
		color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else
		color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0)
		if(error_code & 0x04)
			color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

	color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/*函数：向量号为12，#SS错误，有错误码 加载栈段寄存器SS或栈操作触发的SS段错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_stack_segment_fault(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_stack_segment_fault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01)
		color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02)
		color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else
		color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0)
		if(error_code & 0x04)
			color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

	color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/*函数：向量号为13，#GP错误，有错误码 任何内存引用和保护检测触发的通用保护性异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_general_protection(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);

	if(error_code & 0x01)
		color_printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(error_code & 0x02)
		color_printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else
		color_printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

	if((error_code & 0x02) == 0)
		if(error_code & 0x04)
			color_printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			color_printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

	color_printk(RED,BLACK,"Segment Selector Index:%#010x\n",error_code & 0xfff8);

	while(1);
}

/*函数：向量号为14，#PF错误，有错误码 任何内存引用触发的页错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void

  一些延时页操作技术基于#PF实现；处理器为#PF提供了两条信息（线索）帮助诊断异常产生的原因以及恢复方法
  一、栈中错误码：#PF的异常错误码与其他异常完全不同，处理器使用5个标志位来描述
  第0位P：P=0说明由一个不存在的页引发（页不存在）；P=1说明进入了违规区域或使用了保留位（页级保护）
  第1位W/R：W/R=0由读取页产生；W/R=1由写入页产生
  第2位U/S：U/S=0由超级用户模式产生；U/S=1由普通用户模式产生
  第3位RSVD：RSVD=0页表项保留位未引发异常；RSVD=1置位页表项保留位引发异常（前提：CR4控制寄存器的PSE或PAE标志位=1）
  第4位I/D：I/D=0获取指令时未引发异常；I/D=1获取指令时引发异常

  二、CR2控制寄存器：保存着触发异常时的线性地址，异常处理程序可根据此地址定位到页目录项和页表项；页错误处理程序应该在
  第二个页错误发生前保存CR2寄存器的值
*/
void do_page_fault(unsigned long rsp, unsigned long error_code)
{
    unsigned long *p = NULL;
    // CR2控制寄存器保存着触发异常时的线性地址；页错误处理程序应该在第二个页错误发生前保存CR2寄存器的值
    unsigned long cr2 = 0;
    // 指令部分：movq %%cr2,%0     //将CR2寄存器的值存放至序号占位符为0的任意输入/输出型寄存器
    // 输出部分：相关指令执行后，将结果存放至任意输入/输出型寄存器，再转存至unsigned long cr2变量中
    // 输入部分：无
    // 损坏描述：可能涉及到内存的变动，用memory声明
    __asm__ __volatile__("movq %%cr2,%0" : "=r"(cr2)::"memory"); // C语言不支持寄存器操作，只能使用内嵌汇编语句
    p = (unsigned long *)(rsp + 0x98);
    color_printk(RED, BLACK, "do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n", error_code, rsp, *p);
    if (!(error_code & 0x01))
    {
        color_printk(RED, BLACK, "Page Not-Present,\t");
    }
    if (error_code & 0x02)
    {
        color_printk(RED, BLACK, "Write Cause Fault,\t");
    }
    else
    {
        color_printk(RED, BLACK, "Read Cause Fault,\t");
    }
    if (error_code & 0x04)
    {
        color_printk(RED, BLACK, "Fault in user(3)\t");
    }
    else
    {
        color_printk(RED, BLACK, "Fault in supervisor(0,1,2)\t");
    }
    if (error_code & 0x08)
    {
        color_printk(RED, BLACK, ",Reversed Bit Cause Fault\t");
    }
    if (error_code & 0x10)
    {
        color_printk(RED, BLACK, ",Instruction Fetch Cause Fault");
    }
    color_printk(RED, BLACK, "\n");
    color_printk(RED, BLACK, "CR2:%#018lx\n", cr2);
    while (1)
    {
    }
}

/*函数：向量号为15，#-错误（Intel保留，请勿使用）无错误码
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/

/*函数：向量号为16，#MF错误，无错误码   x87 FPU浮点指令或wait/fwait指令触发的x87 FPU计算错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_x87_FPU_error(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_x87_FPU_error(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为17，#AC错误，有错误码0   引用内存中的任意数据触发的对齐检测错误的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_alignment_check(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_alignment_check(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为18，#MC终止，无错误码（如果有错误码则和CPU类型有关）   机器检测的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_machine_check(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_machine_check(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为19，#XM错误，无错误码   SSE/SSE2/SSE3浮点指令触发的SIMD浮点异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_SIMD_exception(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_SIMD_exception(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为20，#VE错误，无错误码   违反EPT触发的虚拟化异常的异常处理函数
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/
void do_virtualization_exception(unsigned long rsp,unsigned long error_code)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	color_printk(RED,BLACK,"do_virtualization_exception(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",error_code , rsp , *p);
	while(1);
}

/*函数：向量号为21~31，#-错误（Intel保留，请勿使用）无错误码
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/

/*函数：向量号为32~255，#-用户自定义中断
  参数：
  1.unsigned long rsp：当前栈指针寄存器RSP的值
  2.unsigned long error_code：错误码
  返回值：void
*/

/*函数：初始化系统中断向量表内的中断向量
  参数：无
  返回值：void
*/
void sys_vector_init()
{
    set_trap_gate(0,1,divide_error);
    set_trap_gate(1,1,debug);
    set_intr_gate(2,1,nmi);
    set_system_gate(3,1,int3);
    set_system_gate(4,1,overflow);
    set_system_gate(5,1,bounds);
    set_trap_gate(6,1,undefined_opcode);
    set_trap_gate(7,1,dev_not_available);
    set_trap_gate(8,1,double_fault);
    set_trap_gate(9,1,coprocessor_segment_overrun);
    set_trap_gate(10,1,invalid_TSS);
    set_trap_gate(11,1,segment_not_present);
    set_trap_gate(12,1,stack_segment_fault);
    set_trap_gate(13,1,general_protection);
    set_trap_gate(14,1,page_fault);
    //向量号为15，#-错误（Intel保留，请勿使用）无错误码
    set_trap_gate(16,1,x87_FPU_error);
    set_trap_gate(17,1,alignment_check);
    set_trap_gate(18,1,machine_check);
    set_trap_gate(19,1,SIMD_exception);
    set_trap_gate(20,1,virtualization_exception);
}
