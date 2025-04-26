/*手动编写内核程序的链接脚本，按照链接脚本描述的地址空间布局，把编译好的各个程序片段（.text、.data、.bss）
  填充到内核程序文件system中
*/
OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
	/*内核层（即boot.bin）的起始线性地址0xffff800000000000对应着物理地址0处
	  内核程序（即head.o）的起始线性地址0xffff800000000000+0x100000处
	*/
	. = 0xffff800000000000 + 0x100000;
	.text :
	{
		_text = .;
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
	.bss :
	{
		_bss = .;
		*(.bss)
		_ebss = .;
	}

	_end = .;
}