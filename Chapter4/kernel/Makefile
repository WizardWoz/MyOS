#Makefile文件的第一个目标是默认目标，终端只运行make而不指定具体目标时，make尝试构建该目标（所以all放在第一）
#all:代表项目的完整构建其他目标：例如clean、install，以及任何自定义目标（如您的 system 目标）会跟在后面
#这些后续目标的顺序通常不会影响构建过程，除非它们之间存在明确的依赖关系
#开始项目的完整构建：剔除system程序内多余的段信息，并提取出程序段数据（包括.text、.data、.bss）
#提取出的二进制文件即为kernel.bin，只保存着程序的二进制机器码，无法通过该文件查询指令地址
all:		system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
#使用链接脚本Kernel.lds将编译生成的head.o与main.o链接成可执行内核程序文件system（链接）
#只有system文件记录着内核程序各个段信息，并显示出程序的每个段地址以及其他相关信息
system:		head.o entry.o printk.o trap.o memory.o interrupt.o main.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o printk.o trap.o memory.o interrupt.o main.o -T Kernel.lds
#为head.S生成编译文件head.o，非可执行程序（编译）
head.o:		head.S
	gcc -E head.S > head.s
	as --64 -o head.o head.s
#为entry.S生成编译文件entry.o，非可执行程序（编译）
entry.o:	entry.S
	gcc -E entry.S > entry.s
	as --64 -o entry.o entry.s
#-mcmodel选项用于指定编译时的“内存模型”（memory model）。内存模型决定了编译器如何生成代码来访问内存，特别是关于地址寻址的方式
#-mcmodel=large：当程序需要访问超过默认“小”模型所能覆盖的地址范围（通常是2GB）时使用。编译器会生成能够访问整个64位虚拟地址空间的代码。
#这意味着所有地址（包括函数入口点和数据地址）都会被视为完整的64位指针。为了实现这一点，编译器可能需要生成更多的指令来加载和操作64位地址，这通常会导致：
#更大的代码尺寸：因为需要更多指令来处理64位地址。略微降低的性能：因为有了更多的指令和更复杂的寻址模式。
#主要用于以下情况：大型应用程序，拥有巨大数据段或需要加载大量动态库的应用程序；内核或系统级编程：当程序需要对整个虚拟地址空间进行低级别控制时。
#共享库（PIC）：程序是位置无关代码（PIC）并且需要能够被加载到任意64位地址空间中的任何位置

#fno-前缀的参数用于关闭一个在当前编译条件下默认启用，或者由某个更高级别的优化选项（如-O1,-O2,-Os等）所隐式启用的特定编译特性或优化
#-fno-builtins：禁止GCC使用任何内建函数
#-fno-stack-protector：禁用堆栈保护。在某些性能极其敏感或内存极度受限且确信代码安全的场合，可能会考虑禁用，但这会降低安全性

#为printk.c生成编译文件printk.o（编译）
printk.o: 	printk.c
	gcc -mcmodel=large -fno-builtin -m64 -c printk.c
#为trap.c生成编译文件trap.o（编译）
trap.o: 	trap.c
	gcc -mcmodel=large -fno-builtin -m64 -c trap.c
#为memory.c生成编译文件memory.o（编译）
memory.o: 	memory.c
	gcc -mcmodel=large -fno-builtin -m64 -c memory.c
#为interrupt.c生成编译文件interrupt.o（编译）
interrupt.o: 	interrupt.c
	gcc -mcmodel=large -fno-builtin -m64 -c interrupt.c
#为main.c生成编译文件main.o（编译）
main.o:		main.c 
	gcc -mcmodel=large -fno-builtin -m64 -c main.c
#在重新生成.o、.bin等文件前，先执行make clean清除旧文件
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin