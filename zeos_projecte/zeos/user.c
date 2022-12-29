#include <libc.h>

char buff[24];
char * b;

int pid;

int test(void * param){
  write(1,"\nSOY EL THREAD :)\n", 19);
  write(1,param,5);
  return 0;
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  /* TEST CREATE THREAD */
  /* buff[0] = 'l';
   buff[1] = 'a';
   buff[2] = 'l';
   buff[3] = 'a';
   createthread(test, (void *) buff);*/

  /* TEST DUMP SCREEN */
  /*struct screen *p = alloc();
  paint_display(p);
  dump_screen((void *) p);*/


  char * buffer = (char*) alloc();
  while(1) {
    /* GET KEY TEST */
    if (get_key(buffer) != -1){
      write(1,buffer,1);
    }
  }
}
