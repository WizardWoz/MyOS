#Makefile文件的第一个目标是默认目标，终端只运行make而不指定具体目标时，make尝试构建该目标（所以all放在第一）
#all:代表项目的完整构建其他目标：例如clean、install，以及任何自定义目标（如您的 system 目标）会跟在后面
#这些后续目标的顺序通常不会影响构建过程，除非它们之间存在明确的依赖关系
#开始项目的完整构建：通过nasm编译出boot.bin和loader.bin可执行文件
all: boot.bin loader.bin
#使用nasm -o将boot.asm编译成boot.bin
boot.bin:
	nasm boot.asm -o boot.bin
#使用nasm -o将loader.asm编译成loader.bin
loader.bin:
	nasm loader.asm -o loader.bin
#在重新生成boot.bin和loader.bin之前，先执行make clean清除旧文件
clean:
	rm -rf *.bin *.asm~ Makefile~ boot.bin loader.bin