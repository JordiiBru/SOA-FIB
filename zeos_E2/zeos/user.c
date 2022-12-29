#include <libc.h>

char buff[256];

void printInt(int n){
  char buff2[256];
  itoa(n,buff2);
  write(1,buff2,strlen(buff2));
}


void print_stats(int pid, struct stats st){
  get_stats(pid, st);

  write(1,"soy el proc ", 13);
  printInt(pid);
  write(1,"\n",2);

  write(1,"USER TICKS: ", 13);
  printInt(st.user_ticks);
  write(1,"\n",2);

  write(1,"SYSTEM TICKS: ", 15);
  printInt(st.system_ticks);
  write(1,"\n",2);

  write(1,"BLOCKED TICKS: ", 16);
  printInt(st.blocked_ticks);
  write(1,"\n",2);

  write(1,"READY TICKS: ", 14);
  printInt(st.ready_ticks);
  write(1,"\n",2);

  write(1,"ELAPSED TICKS: ", 16);
  printInt(st.elapsed_total_ticks);
  write(1,"\n",2);

  write(1,"TRANS TICKS: ", 14);
  printInt(st.total_trans);
  write(1,"\n",2);

  write(1,"REMAINING TICKS: ", 18);
  printInt(st.remaining_ticks);
  write(1,"\n",2);
}

void for_fork(){
  write(1,"\n",2);
  write(1,"soy el hijo padre ", 19);
  int pd = getpid();
  itoa(pd,buff);
  write(1,buff,strlen(buff));
  write(1,"\n",2);
  for(int i = 0; i < 3; ++i){
    int child = fork();
    if(child==0){
      write(1,"soy el hijo ", 13);
      int pd = getpid();
      itoa(pd,buff);
      write(1,buff,strlen(buff));
      write(1,"\n",2);
      exit();
    }
  }

  //get stats test. No funciona correctament. Problema al buffer?
  /*struct stats pid_stats;
  print_stats(getpid(), pid_stats);*/
  
  exit();
}



int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  int hijo = fork();
  if(hijo == 0) for_fork();
  else{
    write(1,"\n",2);
    write(1,"soy el padre ", 13);
    int pd = getpid();
    itoa(pd,buff);
    write(1,buff,strlen(buff));
    exit();
  }

	while(1){}

}
