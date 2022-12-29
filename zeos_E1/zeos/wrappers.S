#include <asm.h>

ENTRY(write)
	pushl %ebp
	movl %esp, %ebp

	pushl %ebx

	pushl %ecx
	pushl %edx
  pushl %esi

	movl 8(%ebp), %ebx # file descriptor
	movl 12(%ebp), %ecx # buffer
	movl 16(%ebp), %edx # size
	movl $4, %eax #identificador del sys_write al eax


	leal next, %esi # passem @ret a la pila d'usuari
	pushl %esi
	pushl %ebp
	movl %esp, %ebp #preparem tota la pila pila abans del sysenter
	sysenter

next:
	popl %ebp
	popl %esi
	addl $4, %esp
	popl %edx
	popl %ecx
	popl %ebx
	cmp $0, %eax # es compara per mirar si hi ha errors
	jl errors
	#movl %ebp, %esp #MAYBE FALLA Y S'HA DE FER ALGO AMB ESP
	popl %ebp
	ret

errors:
	negl %eax # neguem el valor de eax per deixar lo en valor absolut
	movl %eax, errno
	movl $-1, %eax
	popl %ebp
	ret


ENTRY(gettime)

	pushl %ebp
	movl %esp, %ebp

	pushl %ecx
	pushl %edx
	pushl %esi

	movl $10,%eax

	leal next_time, %esi
	pushl %esi
	pushl %ebp
	movl %esp, %ebp
	sysenter

next_time:
	popl %ebp
	addl $4, %esp
	popl %esi
	popl %edx
	popl %ecx
	cmp $0, %eax
	jl errors_time
	movl %ebp, %esp
	popl %ebp
	ret

errors_time:
	negl %eax
	movl %eax, errno
	movl $-1, %eax
	movl %ebp, %esp
	popl %ebp
	ret
