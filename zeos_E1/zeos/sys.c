/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <errno.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>


#define LECTURA 0
#define ESCRIPTURA 1
#define buffer_size 256

extern int zeos_ticks;

void perror();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process

  return PID;
}

void sys_exit()
{
}

char buffer_aux[256];

int sys_write(int fd, char *buffer, int size) {
 	int error_val = check_fd(fd,1);
	if (error_val != 0) return error_val;
	if (buffer == NULL) return -EFAULT; //ERROR
	if (size < 0) return -EINVAL; //ERROR
	//mirem quants bytes hem d'escriure al buffer,(while) si hi queden més que buffer_size, escribim el que podem, fem més gran 	    //el buffer i el que queda
	// comprobem que sigui <= buffer size i ho escribim un ultim cop.

	//int copy_from_user(void *start, void *dest, int size)
	//
	int bytes = size; //bytes que queden per escriure
	int written_bytes = 0;
	while (bytes > buffer_size){
		int err = copy_from_user(buffer+(size-bytes), buffer_aux, buffer_size);
	       	//copy de les dades que puguem escriure (que no haguem escrit) en tot un buffer de 256
		if (err == -1) return -ECOPYUM;
		written_bytes = sys_write_console(buffer_aux,buffer_size);
		buffer += buffer_size;
		bytes -= written_bytes;
	}

	int err = copy_from_user(buffer+(size-bytes), buffer_aux, bytes);
	if (err == -1) return -ECOPYUM;
	written_bytes = sys_write_console(buffer_aux,bytes);
	bytes -= written_bytes;
	return size-bytes; //si tot ha anat bé ha de retornar size-0
}

int sys_gettime(){
	return zeos_ticks;
}
