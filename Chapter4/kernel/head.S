/*以下为内核执行头程序（使用GAS汇编语言：GNU AT&T编写），属于内核程序中的一小段汇编代码
  负责为操作系统创建段结构、页表结构、设置某些结构的默认处理函数、配置关键寄存器
  完成上述工作后仍需借助特殊的远跳转操作movq、lretq进入系统内核主程序
*/

.section .text		//伪指令.section定义段名为.text（代码段）
.globl _start		//GAS编译器使用标识符_start作为默认起始位置；还要使用.globl修饰，若缺少则链接过程中会出现警告
_start:
	mov $0x10,%ax	//GAS汇编指令源操作数在左边，目的操作数在右边
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%ss
	mov $0x7E00,%esp
	//采用RIP_Relative寻址模式，为IA-32e模式新引入的寻址方法
	//Intel汇编语言格式：mov rax,[rel 标识符]；AT&T汇编语言格式：movq 标识符(%rip)，%rax；标识符偏移值是有符号32位整数
	lgdt GDT_POINTER(%rip)		//加载GDT表长和表基址至GDTR
	lidt IDT_POINTER(%rip)		//加载IDT表长和表基址至IDTR
	mov $0x10,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	mov %ax,%ss
	movq $0x7E00,%rsp
	//加载cr3寄存器
	movq $0x101000,%rax
	movq %rax,%cr3
	//模仿lcall的执行现场，结合RIP_Relative寻址模式将段选择子和段内地址偏移保存在栈中
	movq switch_seg(%rip),%rax	//RAX=switch_seg+RIP
	pushq $0x08					//段选择子
	pushq %rax					//段内地址偏移
	//GAS汇编代码使用lretq进行段间切换而不是ljmp或lcall，因为GAS编译器不支持直接远跳转jmp/call far
	lretq		//恢复调用现场，返回到目标代码段的程序地址，实际上改变了CS寄存器
	//在IA-32e模式下的代码
switch_seg:
	.quad entry64
entry64:
	movq $0x10,%rax	//GAS汇编指令源操作数在左边，目的操作数在右边
	movq %rax,%ds
	movq %rax,%es
	movq %rax,%fs
	movq %rax,%ss
	movq $0xFFFF800000007E00,%rsp	//rsp地址
	movq go_to_kernel(%rip),%rax
	pushq $0x08
	pushq %rax
	lretq
go_to_kernel:
	.quad Start_Kernel
/*采用objdump -D system反汇编，而不是kernel.bin，因为只有system文件记录内核程序各个段信息，能显示出每条指令地址及详细信息
  而kernel.bin只保存程序机器码，不含有任何段描述信息，无法通过该文件查询程序的指令地址
*/

//=======创建并初始化各级页表及页表项
//=======IA-32e模式下有4KB，2MB，1GB大小的物理页，使用2MB以下的物理页会导致页颗粒零碎，从而造成频繁的物理页维护工作
//=======4KB页面采用PML4，PDP，PD，PT；2MB页面采用PML4，PDP，PD；1G页面采用PML4,PDP
//=======所以采用2MB物理页最为合理
.align 8		//伪指令.align将对齐标准设置为8B
//伪指令.org固定各个页表的起始地址（相对于内核执行头程序起始地址的偏移量）
//链接器根据链接脚本描述将内核执行头程序的起始线性地址设置在0xFFFF800000000000+0x100000处
//PML4将线性地址0和0xFFFF800000000000映射为同一物理页以方便页表切换
.org 0x1000		//Page Map Level 4顶层页目录表位于0x1000偏移处；所以页目录起始线性地址为0xffff800000101000
__PML4E:		//Page Map Level 4顶层页目录表占用4096B=4KB，因为一个页表项占用8B，所以一共包含512个页表项，占用9bit
//对于PML4存储的每个表项（63～0）：51~12为PDPT基地址
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 0001 0000 0010 | 0000 0000 0111
	.quad 0x102007	//8B	所以PDPT基地址为0x0000 0000 0010 2000
	.fill 255,8,0	//255*8=2040B
	.quad 0x102007	//8B
	.fill 255,8,0	//255*8=2040B
.org 0x2000		//Page Directory Pointer二层页目录表位于0x2000偏移处；所以页目录起始线性地址为0xffff800000102000
__PDPTE:		//Page Directory Pointer二层页目录表占用4KB
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 0001 0000 0011 | 0000 0000 0011
	.quad 0x103003	//8B	所以PDT基地址为0x0000 0000 0010 3000
	.fill 511,8,0	//511*8=4088B
.org 0x3000		//Page Directory Table页目录表位于0x3000偏移处；所以页目录起始线性地址为0xffff800000103000
__PDE:			//Page Directory Table页目录表占用4KB
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 000|0 0000 0000 0000 1000 0011：0x0000 0000 0000 0000
	.quad 0x000083		//从此映射到线性地址0x0000000000000000和0xFFFF800000000000
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 001|0 0000 0000 0000 1000 0011：0x0000 0002 0000 0000
	.quad 0x200083
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 010|0 0000 0000 0000 1000 0011：0x0000 0004 0000 0000
	.quad 0x400083
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 011|0 0000 0000 0000 1000 0011：0x0000 0006 0000 0000
	.quad 0x600083
	//0000 0000 0000 | 0000 0000 0000 0000 0000 0000 0000 100|0 0000 0000 0000 1000 0011：0x0000 0008 0000 0000
	.quad 0x800083		
	.quad 0xe0000083	//从此映射到0x0000000000A00000
	.quad 0xe0200083
	.quad 0xe0400083
	.quad 0xe0600083	//从此映射到0xFFFF800000A00000显存空间
	.quad 0xe0800083
	.quad 0xe0a00083
	.quad 0xe0c00083
	.quad 0xe0e00083	//13*8=104B
	.fill 499,8,0		//499*8=3992B

//=======将全局描述符表GDT结构，中断描述符表IDT结构，任务状态段TSS结构放在内核程序的数据段内
//=======伪指令.globl使GDT、IDT、TSS的个描述符表项可被内核程序的其它部分引用或访问，相当于C语言的extern
//=======例如：1.向IDT表项设置中断/异常处理函数；2.保存/还原各进程任务状态段信息；3.创建LDT描述符表（本系统不使用LDT表功能）
//以下为全局描述符表GDT结构
.section .data      //伪指令.section定义段名为.data（数据段），手动配置全局描述符表GDT内的各个段描述符
.globl GDT_Table
GDT_Table:
    .quad 0x0000000000000000    /*0 NULL descriptor 00~07 64位空描述符*/
    .quad 0x0020980000000000    /*1 KERNEL Code 64-bit Segment 08~0F内核64位代码段*/
    .quad 0x0000920000000000    /*2 KERNEL Data 64-bit Segment 10~17内核64位数据段*/
    .quad 0x0020f80000000000    /*3 USER Code 64-bit Segment 18~1F用户64位代码段*/
    .quad 0x0000f20000000000    /*4 USER Data 64-bit Segment 20~27用户64位数据段*/
    .quad 0x00cf9a000000ffff    /*5 KERNEL Code 32-bit Segment 28~29内核32位代码段*/
    .quad 0x00cf92000000ffff    /*6 KERNEL Data 32-bit Segment 30~31内核32位数据段*/
	//.fill伪指令用于在当前位置填充指定数量的重复字节模式。
	//常用于数据段中初始化内存区域或在代码段中进行填充（padding）以满足对齐要求。
	//.fill 重复次数,重复单元大小（可选，默认单位为1B）,填充值（可选，默认为0）
    .fill 10,8,0                /*8~9 TSS(jmp one segment<7>) in long mode 128-bit 40*/
GDT_END:
GDT_POINTER:
GDT_LIMIT: .word GDT_END-GDT_Table-1	//GDT表长度
GDT_BASE:  .quad GDT_Table				//GDT表基址
//以下为中断描述符表IDT结构
.globl IDT_Table
IDT_Table:
	.fill 512,8,0
IDT_END:
IDT_POINTER:
IDT_LIMIT: .word IDT_END-IDT_Table-1	//IDT表长度
IDT_BASE: .quad IDT_Table				//IDT表基址
//以下为任务状态段TSS结构
.globl TSS64_Table
TSS64_Table:
	.fill 13,8,0
TSS64_END:
TSS64_POINTER:
TSS64_LIMIT: .word TSS64_END-TSS64_Table-1	//TSS表长度
TSS64_BASE: .quad TSS64_Table				//TSS表基址