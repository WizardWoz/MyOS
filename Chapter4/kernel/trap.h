#ifndef __TRAP_H__
#define __TRAP_H__

#include "linkage.h"
#include "printk.h"
#include "lib.h"

void divide_error();    //trap.h中声明，entry.S中定义
void nmi();             //trap.h中声明，entry.S中定义
void invalid_TSS();     //trap.h中声明，entry.S中定义
void page_fault();      //trap.h中声明，entry.S中定义

void sys_vector_init(); //trap.h中声明，trap.c中定义

#endif