RootDirSectors equ 14   ;根目录占用的扇区数14=(根目录容纳目录项数224*每个目录项大小32B+每个扇区字节数-1)/每个扇区字节数512
SectorNumOfRootDirStart equ 19  ;根目录起始扇区号19=保留扇区数1+FAT表扇区数9*FAT表份数2
SectorNumOfFAT1Start equ 1  ;FAT1表的起始扇区号为1（因为FAT表1前只有一个保留扇区（引导扇区）且扇区号为0）
SectorBalance equ 17    ;因为FAT[0]与FAT[1]簇为无效簇，所以将根目录起始扇区号-2，则间接将数据区起始扇区号-2

BS_OEMName db 'MINEboot'    ;软盘生产厂家名称
BPB_BytesPerSec dw 512      ;每个扇区占用字节数，可能是512B，1024B，2048B，4096B
BPB_SecPerClus db 1         ;每个簇占用扇区数，取值必须是2的幂次（从2的0次方开始）
BPB_RsvdSecCnt dw 1         ;保留扇区数，FAT12此字段值必为1，因为引导扇区需要占用一个保留扇区
BPB_NumFATs db 2            ;FAT表份数，FAT12多数保留2份表，FAT表1为主表，FAT表2为备份表
BPB_RootEntCnt dw 224       ;根目录可容纳的FAT目录项数
BPB_TotSec16 dw 2880        ;FAT12文件系统总扇区数，BPB_TotSec16与BPB_TotSec32必有一个不为0
BPB_Media db 0xF0           ;介质描述符，不可移动介质：0xF8、可移动介质：0xF0，且应该写入FAT[0]表项低字节
BPB_FATSz16 dw 9            ;FAT表占用扇区数
BPB_SecPerTrk dw 18         ;每个磁道扇区数
BPB_NumHeads dw 2           ;软盘磁头数，因为1.44M软盘只有一张盘片，所以只需要上下两个磁头
BPB_HiddSec dd 0            ;隐藏扇区数
BPB_TotSec32 dd 0           ;FAT12文件系统总扇区数，BPB_TotSec16与BPB_TotSec32必有一个不为0
BS_DrvNum db 0              ;int 13h的驱动器号
BS_Reserved1 db 0           ;未使用，作为保留字段
BS_BootSig db 29H           ;扩展引导标志，必须是0x29
BS_VolID dd 0               ;卷序列号
BS_VOlLab db 'boot loader'  ;卷标，即Windows和Linux中的磁盘名
BS_FileSysType db 'FAT12   '   ;文件系统类型，字段可自定义任意值，操作系统不依靠该字段识别文件系统