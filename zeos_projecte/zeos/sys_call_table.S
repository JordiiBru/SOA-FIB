#include <asm.h>
#include <segment.h>

ENTRY (sys_call_table)
	.long sys_ni_syscall	//0
	.long sys_exit		//1
	.long sys_fork		//2
	.long sys_ni_syscall	//3
	.long sys_write		//4
	.long sys_ni_syscall	//5
	.long sys_ni_syscall	//6
	.long sys_ni_syscall	//7
	.long sys_ni_syscall	//8
	.long sys_ni_syscall	//9
	.long sys_gettime	//10
	.long sys_ni_syscall	//11
	.long sys_ni_syscall	//12
	.long sys_yield		//13
	.long sys_ni_syscall	//14
	.long sys_ni_syscall	//15
	.long sys_ni_syscall	//16
	.long sys_ni_syscall	//17
	.long sys_ni_syscall	//18
	.long sys_ni_syscall	//19
	.long sys_getpid	//20
	.long sys_sem_init	//21
	.long sys_sem_wait	//22
	.long sys_sem_signal	//23
	.long sys_sem_destroy	//24
	.long sys_ni_syscall	//25
	.long sys_get_key	//26
	.long sys_ni_syscall	//27
	.long sys_dump_screen	//28
	.long sys_ni_syscall	//29
	.long sys_alloc	//30
	.long sys_dealloc	//31
	.long sys_ni_syscall	//32
	.long sys_ni_syscall	//33
	.long sys_ni_syscall	//34
	.long sys_get_stats	//35
	.long sys_createthread// 36
	.long sys_terminatethread // 37
	.long sys_ni_syscall	//38
	.long sys_ni_syscall	//39
	.long sys_ni_syscall	//40
.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4
