/*
 * libc.c
 */

#include <libc.h>

#include <types.h>


int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;

  if (a==0) { b[0]='0'; b[1]=0; return ;}

  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }

  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;

  i=0;

  while (a[i]!=0) i++;

  return i;
}

void perror()
{
  char buffer[256];

  itoa(errno, buffer);

  write(1, buffer, strlen(buffer));
}

void thread_wrapper(int (*function)(void *param), void *param){
  function(param);
  terminatethread();
}

/* funcion para pintar todo el display */
/* para una prueba visual pasamos una pantalla azul y asteriscos que hacen blinking como testing*/
void paint_display(struct screen *p){
  char c = '*';
  Word ch = (Word) (c & 0x00FF) | 0xBF00; //fondo azul y asteriscos blancos con blinking
  for(int i = 0; i < 80; ++i) {
    for (int j = 0; j < 25; ++j) {
      p->display[i][j] = (unsigned short int)ch;
    }
  }
}
