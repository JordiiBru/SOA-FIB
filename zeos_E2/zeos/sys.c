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
extern int gpid;//pid para asignacion de pid a los hijos
extern struct list_head freequeue;
extern struct list_head readyqueue;
void perror();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork(){
  return 0;
}

int sys_fork()
{
  if (list_empty(&freequeue)) return -EFREE; //check errno.h
  struct list_head * e = list_first(&(freequeue));
  list_del(e);
  struct task_struct * first = list_head_to_task_struct(e);

  union task_union * child = (union task_union *)first;
  copy_data(current(), child, sizeof(union task_union));

  allocate_DIR(first);

  page_table_entry * childPT = get_PT(&child->task);

  int pages[NUM_PAG_DATA];
  for(int i = 0; i < NUM_PAG_DATA; ++i){
    pages[i] = alloc_frame();
    if(pages[i] < 0){ // not enough free pages
      for(int j = 0; j < i; ++j){ //liberem les pagines que hem alocatat
        free_frame(pages[j]);
      }
      list_add_tail(e,&freequeue);
      return -EPAGE; //errno.h
    }
  }
  page_table_entry * dadPT = get_PT(current());

  for(int i = 0; i < NUM_PAG_KERNEL; ++i){
    set_ss_pag(childPT, i, get_frame(dadPT, i));
  }

  for(int i = 0; i < NUM_PAG_CODE; ++i){
    set_ss_pag(childPT, PAG_LOG_INIT_CODE+i, get_frame(dadPT, PAG_LOG_INIT_CODE+i));
  }

  for(int i = 0; i < NUM_PAG_DATA; ++i){
    set_ss_pag(childPT, PAG_LOG_INIT_DATA+i, pages[i]);
  }

  int shared = NUM_PAG_KERNEL + NUM_PAG_CODE;
  int total = shared + NUM_PAG_DATA;
  //la part shared no cal recorrela
  for(int i = shared; i < total; ++i){
    //se guardan las direcciones para que el padre acceda al hijo
    set_ss_pag(dadPT, NUM_PAG_DATA+i, get_frame(childPT,i));
    //shift 12 pq las paginas deben estar alineadas a 4k
    copy_data((void *)(i<<12), (void *)((i+NUM_PAG_DATA) << 12), PAGE_SIZE);
    del_ss_pag(dadPT, NUM_PAG_DATA+i);
  }

  set_cr3(get_DIR(current()));

  (*child).task.PID = ++gpid;
  (*child).task.state = ST_READY;
  init_stats(first);


  //ctx hw son 5 registres, ctx sw son 11 registres i despres @handler -> en total 17 registres
  //la seguent es la @ret (registre 18)
  //i la seguent es el fake ebp (registre 19)

  ((unsigned long *) KERNEL_ESP(child))[-0x12] = (unsigned long)&ret_from_fork;
  ((unsigned long *) KERNEL_ESP(child))[-0x13] = (unsigned long)0;
  (*child).task.kernel_esp = &((unsigned long *)KERNEL_ESP(child))[-0x13];
  list_add_tail(&(child->task.list), &readyqueue);
  return (*child).task.PID;
}

void sys_exit()
{
    //alliberar memoria fisica del procés

    struct task_struct * proc = current();
    page_table_entry * procTP = get_PT(proc);

    /* DATA */
     for (int pag=0;pag<NUM_PAG_DATA;pag++){
       //logicas
       free_frame(procTP[PAG_LOG_INIT_DATA+pag].bits.pbase_addr);
       //fisicas
       del_ss_pag(procTP, (PAG_LOG_INIT_DATA+pag));
     }
     //task_struct
     (*proc).PID = -1;
     update_process_state_rr(proc, &freequeue);
     sched_next_rr();
}


int sys_get_stats(int pid, struct stats *st){
   //Be aware that st is a pointer to the user address space. Check possible errors:
   if(pid < 0) return -EINVAL;
   if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -ESTATS;
   //busquem el proces que tingui PID = pid
   for(int i = 0; i < NR_TASKS; ++i){
     if(task[i].task.PID == pid){
       int err = copy_to_user(&(task[i].task.stat), st, sizeof(struct stats));
       if(err == -1) return -ECOPYUM;
       else return 0;
     }
   }
   return -1;
}


char buffer_aux[256];
int sys_write(int fd, char *buffer, int size) {
 	int error_val = check_fd(fd,1);
	if (error_val != 0) return error_val;
	if (buffer == NULL) return -EFAULT; //ERROR
	if (size < 0) return -EINVAL; //ERROR
	//mirem quants bytes hem d'escriure al buffer,(while) si hi queden més que buffer_size, escribim el que podem, fem més gran 	    //el buffer i el que queda
	// comprobem que sigui <= buffer size i ho escribim un ultim cop.

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
