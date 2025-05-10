#Makefile文件的第一个目标是默认目标，终端只运行make而不指定具体目标时，make尝试构建该目标（所以all放在第一）
#all:代表项目的完整构建其他目标：例如clean、install，以及任何自定义目标（如您的 system 目标）会跟在后面
#这些后续目标的顺序通常不会影响构建过程，除非它们之间存在明确的依赖关系
#开始项目的完整构建：剔除system程序内多余的段信息，并提取出程序段数据（包括.text、.data、.bss）
#提取出的二进制文件即为kernel.bin，只保存着程序的二进制机器码，无法通过该文件查询指令地址
all:		system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
#使用链接脚本Kernel.lds将编译生成的head.o与main.o链接成可执行内核程序文件system（链接）
#只有system文件记录着内核程序各个段信息，并显示出程序的每个段地址以及其他相关信息
system:		head.o entry.o main.o printk.o trap.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o -T Kernel.lds
#为head.S生成编译文件head.o，非可执行程序（编译）
head.o:		head.S
	gcc -E head.S > head.s
	as --64 -o head.o head.s
#为entry.S生成编译文件entry.o，非可执行程序（编译）
entry.o:		entry.S
	gcc -E entry.S > entry.s
	as --64 -o entry.o entry.s
#为printk.c生成编译文件printk.o（编译）
printk.o: 	printk.c
	gcc -mcmodel=large -fno-builtin -m64 -c printk.c -fno-stack-protector
#为trap.c生成编译文件trap.o（编译）
trap.o: 	trap.c
	gcc -mcmodel=large -fno-builtin -m64 -c trap.c -fno-stack-protector
#为main.c生成编译文件main.o（编译）
main.o:		main.c
	gcc -mcmodel=large -fno-builtin -m64 -c main.c -fno-stack-protector
#在重新生成.o、.bin等文件前，先执行make clean清除旧文件
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin