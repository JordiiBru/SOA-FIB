/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */

#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>
#include <list.h>
/* estructura para iterar cada celda del display */
struct screen {
  unsigned short int display[80][25];
};

struct semaforo {
  int sem_id;
  int count;
  struct list_head blocked;
};

extern int errno;

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

void perror();

int getpid();

int fork();

void exit();

int yield();

void * alloc();

int get_stats(int pid, struct stats *st);

int createthread(int (*function)(void *param), void *param);

int terminatethread();

void thread_wrapper(int (*function)(void *param), void *param);

int get_key(char* c);

void paint_display(struct screen *p);

int dump_screen(void *address);

int sem_init (int n_sem, unsigned int value);

int sem_wait (int n_sem);

int sem_signal (int n_sem);

int sem_destroy (int n_sem);

#endif  /* __LIBC_H__ */
