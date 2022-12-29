/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#include <keyboard.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();

/* misma estructura que tenemos en el libc.h para usarla en el copy_from_user*/
struct screen {
  unsigned short int display[80][25];
};

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;
int global_TID=100;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);

  list_del(lhcurrent);

  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);

  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));

  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);

  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);

      /* Return error */
      return -EAGAIN;
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;

	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }

  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);

  current()->PID=-1;

  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;

  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

/*
The system call alloc will allocate and return the initial address
of one free logical page from the user space.
If no free logical page is available,
(void*)-1 will be returned and errno will hold the error code.
*/
void * sys_alloc() {
  page_table_entry * process_PT =  get_PT(current());
  int INI_USER_PAG = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA;
  for (int i = INI_USER_PAG; i < TOTAL_PAGES; ++i) {
    if (get_frame(process_PT,i) == FREE_FRAME) {
      set_ss_pag(process_PT, i, get_frame(process_PT,i));
      return (void *)(i*PAGE_SIZE);
    }
  }
  return (void *)-EFAULT; //ERRNO MUST HOLD THE ERROR
}

/*
The system call dealloc will deallocate one previously allocated logical free page
which starting address is passed as a first parameter.
*/
int sys_dealloc(void * address) {
  page_table_entry * cur_PT = get_PT(current());
  int USER_ZONE = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA;
  if (address >= (void *)(USER_ZONE*PAGE_SIZE)) {
    if (get_frame(cur_PT, (int)address >> 12) == USED_FRAME) {
      free_frame(get_frame(cur_PT, (int)address >> 12));
      del_ss_pag(cur_PT, (int)address >> 12);
      return 0;
    }
  }
  return -EFAULT;
}


int sys_terminatethread() {
  struct task_struct *actual = current();
  page_table_entry * cur_PT = get_PT(actual);

  union task_union *union_actual = (union task_union*) actual;

  /*coger el id de la primera pagina logica de la user stack del current para poder liberarlo luego*/
  /*el esp de la user stack esta en la posicion 1022 (cxt hw--> ss (1023), esp (1022))*/
  unsigned long id = union_actual->stack[1022] >> 12;
  free_frame(get_frame(cur_PT, id));
  del_ss_pag(cur_PT, id);

  actual->TID=-1;
  actual->PID=-1;

  list_add_tail(&(actual->list), &freequeue);
  set_cr3(get_DIR(actual));
  sched_next_rr();
  return 0;
}

int sys_createthread(void (*tw_function)(void *, void *), int (*function)(void *param), void *param) {

  /* comprueba si la freeQ esta vacía antes de hacer nada */
  if (list_empty(&freequeue)) return -ENOMEM;

  struct list_head *lh_current = list_first(&freequeue);
  list_del(lh_current);
  struct task_struct *actual = list_head_to_task_struct(lh_current);
  union task_union *uchild =(union task_union*) actual;
  copy_data(current(), uchild, sizeof(union task_union));
  uchild->task.TID=++global_TID;

  /* allocatar la pila de del usuario con alloc_frame y luego encontrar donde empieza */
  unsigned long alloc_user_stack = (unsigned long) alloc_frame();

  /* comprobación de errores */
  if (alloc_user_stack == -1) {
    actual->TID=-1;
    actual->PID=-1;
    list_add_tail(&(actual->list), &freequeue);
    return -ENOMEM;
  }

  page_table_entry * current_PT = get_PT(current());
  unsigned long it = NULL;
  for(it = (alloc_user_stack >> 12); it < TOTAL_PAGES; ++it){
    if(get_frame(current_PT, it) == FREE_FRAME){
      set_ss_pag(current_PT, it, alloc_user_stack);
      break;
    }
  }

  /* comprobación de errores */
  if (it == TOTAL_PAGES - 1) {
    free_frame(alloc_user_stack);
    actual->TID=-1;
    actual->PID=-1;
    list_add_tail(&(actual->list), &freequeue);
    return -EFAULT;
  }

  unsigned long * user_stack = (unsigned long *)it;

  /* pila de sistema */
  uchild->stack[1019] = (unsigned long)tw_function; //eip
  uchild->stack[1022] = (unsigned long) &user_stack[1021];

  /* user stack */
  user_stack[1021] = 0;
  user_stack[1022] = (unsigned long)function;
  user_stack[1023] = (unsigned long)param;

  /* Map Parent's ebp to child's stack --> preparar la pila de sistema del thread */
  unsigned int register_ebp;
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);
  uchild->task.register_esp = register_ebp;

  init_stats(&(uchild->task.p_stats));
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  return 0;
}

int sys_get_key(char* c){
  char cc = circular_buffer_get();
  if(cc == (char)-ENOMEM) return -1;
  *c = cc;
  return 0;
}

int sys_dump_screen(void *addr){
  int USER_ZONE = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA;
  if (((unsigned long) addr >> 12) < USER_ZONE || ((unsigned long) addr >> 12) >= TOTAL_PAGES) return -EFAULT;

  page_table_entry * current_PT = get_PT(current());
  if(get_frame(current_PT, (unsigned long) addr >> 12) == USED_FRAME){
    return -EFAULT;
  }


  int err = copy_from_user(addr,(void *)0xb8000, sizeof(struct screen));
  if(err == -1) return -ECOPYUM;
  return 0;
}

/*
Comprobación si hay un semaforo en la posicion n_sem.
Nosotros usamos el n_sem como identificador y también como la posición del vector.
Va de 0 a 9. Solo podemos inicializar 10 semaforos como máximo.
*/
int check_sem(int n_sem){
  if(vector_sem[n_sem].sem_id != -1) return 0;
  else return -1;
}

/*
Comprobamos que el n_sem pasado entra dentro del rango establecido (0-9)
*/
int valid_range(int n_sem){
  if(n_sem < 0 || n_sem > 10) return -1;
  else return 0;
}

int sys_sem_init(int n_sem, unsigned int value){
  //comprobación errores
  if(valid_range(n_sem) == -1) return -1;
  if(check_sem(n_sem) == 0) return -1;

  //init semaforo
  vector_sem[n_sem].sem_id = n_sem;
  vector_sem[n_sem].count = value;
  INIT_LIST_HEAD(&vector_sem[n_sem].blocked);

  return 0;
}

int sys_sem_wait(int n_sem){
  //comprobación errores
  if(valid_range(n_sem) == -1) return -1;
  if(check_sem(n_sem) == -1) return -1;

  //el destroyed lo pasamos para comprobar luego si se le ha destruido el semaforo con un destroy
  current()->destroyed = 0;
  vector_sem[n_sem].count--;

  if(vector_sem[n_sem].count < 0){
    list_add_tail(&(current()->list), &vector_sem[n_sem].blocked);
    sched_next_rr();
  }

  if(current()->destroyed == 1) return -1;
  return 0;
}

//comprobar si la lista blocked de nuetro sem esta vacia
int sys_sem_signal(int n_sem){
  //comprobación errores
  if(valid_range(n_sem) == -1) return -1;
  if(check_sem(n_sem) == -1) return -1;
  if(!list_empty(&vector_sem[n_sem].blocked)) return -1;

  vector_sem[n_sem].count++;

  if(vector_sem[n_sem].count <= 0){
    struct list_head * e = list_first(&vector_sem[n_sem].blocked);
    list_del(e);
    list_add_tail(e, &readyqueue);
  }

  return 0;
}

int sys_sem_destroy(int n_sem){
  //comprobación errores
  if( valid_range(n_sem) == -1) return -1;
  if(check_sem(n_sem) == -1) return -1;

  struct list_head * e = NULL;
  while(!list_empty(&vector_sem[n_sem].blocked)){
    e = list_first(&vector_sem[n_sem].blocked);
    list_head_to_task_struct(e)->destroyed = 1;
    list_del(e);
    list_add_tail(e, &readyqueue);
  }

  //invalidar
  vector_sem[n_sem].sem_id = -1;
  vector_sem[n_sem].count = -1;

  return 0;
}
