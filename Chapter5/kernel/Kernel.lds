/*手动编写内核程序的链接脚本，按照链接脚本描述的地址空间布局，把编译好的各个程序片段（.text、.data、.bss）
  填充到内核程序文件system中
*/
OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
	/*
	  内核层（即boot.bin）的起始线性地址0xffff800000000000对应着物理地址0处
	  内核程序（即head.o）的起始线性地址0xffff800000000000+0x100000处
	*/
	. = 0xffff800000000000 + 0x100000;
	.text :				/*.text和:之间必须用" "隔开*/
	{
		_text = .;		/*_text和=和.之间必须用" "隔开*/
		*(.text)
		_etext = .;
	}
	. = ALIGN(8);
	.data :
	{
		_data = .;
		*(.data)
		_edata = .;
	}
	.rodata :
	{
		_rodata = .;
		*(.rodata)
		_erodata = .;
	}
	/*
	  .data.init_task段（在task.h中定义）被放置在只读数据段.rodata段之后，并按照32KB对齐，采用32KB对齐而非8B对齐，是因为除init_task_union之外
	  其它union task_union联合体都使用kmalloc函数申请内存空间，而函数kmalloc返回的内存空间起始地址都按32KB对齐
	  如果按8B对齐，今后使用宏current和宏函数GET_CURRENT的过程中难免会存在隐患
	*/
	.=ALIGN(32768);
	.data.init_task :
	{
		*(.data.init_task)
	}
	.bss :
	{
		_bss = .;
		*(.bss)
		_ebss = .;
	}
	_end = .;
}