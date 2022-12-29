/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}


extern struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct * idle_task;
int gpid = 1000;
int quantum_ticks;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void schedule(){
  update_sched_data_rr();
  if(needs_sched_rr() == 1) {
    update_process_state_rr(current(),&readyqueue);
    sched_next_rr();
  }
}

/*
Function to update the current state of a process to a new state. This function deletes the
process from its current queue (state) and inserts it into a new queue (for example, the free
queue or the ready queue). If the current state of the process is running, then there is no need
to delete it from any queue. The parameters of this function are the task_struct of the process
and the queue according to the new state of the process. If the new state of the process is
running, then the queue parameter should be NULL.
*/
void update_process_state_rr(struct task_struct *t, struct list_head *dest){
    //si esta running no sha de borrar de cap llista
    struct list_head * actual = &t->list;
    //if(actual->prev != NULL && actual->next != NULL) list_del(actual);
    if(t->state == ST_READY){
      t->state = ST_RUN;
      list_del(actual);
      ready_2_sys(t);
    }
    if(t->state == ST_RUN){
      t->state = ST_READY;
      list_add_tail(actual, dest); //si peta comprobar que existeix la llista destí
      sys_2_ready(t);
    }

}

/*
Update the readyqueue, if current process is not the idle process, by inserting the current
process at the end of the readyqueue.
– Extract the first process of the readyqueue, which will become the current process;
– And perform a context switch to this selected process. Remember that when a process
returns to the execution stage after a context switch, its quantum ticks must be restored.
*/

void sched_next_rr(){
    struct task_struct * pseg;
    //si la readyqueue esta buida, hem de inicialitzar un procés idle
    if (list_empty(&readyqueue)) {
      pseg = idle_task;
    }
    else {
        struct list_head * e  = list_first(&(readyqueue));
        list_del(e);
        pseg = list_head_to_task_struct(e);
    }
    pseg->state = ST_RUN;
    sys_2_ready(current());
    ready_2_sys(pseg);
    quantum_ticks = pseg->qticks;
    task_switch((union task_union *) pseg);
}

int needs_sched_rr(){
  if (list_empty(&readyqueue)) {
    quantum_ticks = current()->qticks;
    return 0;
  }
  if (quantum_ticks < 1) return 1;
  return 0;

}

void update_sched_data_rr(){
  --quantum_ticks;
}

void init_idle (void)
{
  struct list_head * e = list_first(&(freequeue));
  list_del(e);
  idle_task = list_head_to_task_struct(e);
  (*idle_task).PID = 0;
  (*idle_task).qticks = QTICKS;

  //init_stats(idle_task); //prueba

  allocate_DIR(idle_task);

  union task_union * idle_task_union = (union task_union *)idle_task;

  (*idle_task_union).stack[KERNEL_STACK_SIZE-1] = (unsigned long)cpu_idle;
  (*idle_task_union).stack[KERNEL_STACK_SIZE-2] = (unsigned long)0;

  idle_task->kernel_esp = &(idle_task_union->stack[KERNEL_STACK_SIZE-2]);

}

void init_task1(void)
{
  struct list_head * e = list_first(&(freequeue));
  list_del(e);
  struct task_struct * first = list_head_to_task_struct(e);
  (*first).PID = 1;
  (*first).qticks = QTICKS;
  (*first).state = ST_RUN;
  quantum_ticks = (*first).qticks;
  //init_stats(&first); //prueba

  allocate_DIR(first);
  set_user_pages(first);

  tss.esp0 = KERNEL_ESP((union task_union *)first);
  writeMSR(0x175, tss.esp0);
  union task_union * init_task_union = (union task_union *)first;
  set_cr3(get_DIR(&(init_task_union->task)));

}

int get_quantum (struct task_struct *t){
  return t->qticks;
}

void set_quantum (struct task_struct *t, int new_quantum){
  t->qticks = new_quantum;
}


void init_sched()
{
  INIT_LIST_HEAD(&freequeue);
  INIT_LIST_HEAD(&readyqueue);

  for(int i = 0; i < NR_TASKS; ++i){
    init_stats(&(task[i].task)); //!!
    list_add(&(task[i].task.list), &freequeue);
  }

}

void init_stats(struct task_struct * actual){
  actual->stat.user_ticks = 0;
  actual->stat.system_ticks = 0;
  actual->stat.blocked_ticks = 0;
  actual->stat.ready_ticks = 0;
  actual->stat.elapsed_total_ticks = get_ticks(); //ticks principi d'estat
  actual->stat.total_trans = 0;
  actual->stat.remaining_ticks = get_quantum(actual);
}

void user_2_sys(struct task_struct * actual){
  actual->stat.user_ticks += get_ticks()-actual->stat.elapsed_total_ticks;
  actual->stat.elapsed_total_ticks = get_ticks();
}

void sys_2_user(struct task_struct * actual){
  actual->stat.system_ticks += get_ticks()-actual->stat.elapsed_total_ticks;
  actual->stat.elapsed_total_ticks = get_ticks();
}

void sys_2_ready(struct task_struct * actual){
  actual->stat.system_ticks += get_ticks()-actual->stat.elapsed_total_ticks;
  actual->stat.elapsed_total_ticks = get_ticks();
}

void ready_2_sys(struct task_struct * actual){
  actual->stat.ready_ticks += get_ticks()-actual->stat.elapsed_total_ticks;
  actual->stat.elapsed_total_ticks = get_ticks();
  actual->stat.total_trans++;
}


//check & en els parametres current i maybe new
void inner_task_switch(union task_union *new){
    tss.esp0 = (DWord)&(new->stack[1024]);
    writeMSR(0x175, tss.esp0);
    set_cr3(get_DIR(&(new->task)));

    swap_esp(current()->kernel_esp, new->task.kernel_esp);

}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}
