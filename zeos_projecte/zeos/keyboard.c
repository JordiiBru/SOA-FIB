#include <keyboard.h>
#include <errno.h>
#include <io.h>

#define SIZE_OF_BUFFER      10


char buff_circ[SIZE_OF_BUFFER];
int readIndex = 0;
int writeIndex = 0;
int circ_len = 0;

int circular_buffer_add(char c){
  if(circ_len == SIZE_OF_BUFFER) return -ENOMEM; //el buffer esta lleno
  else{
    buff_circ[writeIndex] = c;
    ++circ_len;
    ++writeIndex;
  }
  if(writeIndex == SIZE_OF_BUFFER) writeIndex = 0;
  return 0;
}

char circular_buffer_get(){
  if(circ_len == 0) return (char)-ENOMEM; //el buffer esta vacío
  char cc = buff_circ[readIndex];
  --circ_len;
  ++readIndex;
  if(readIndex == SIZE_OF_BUFFER) readIndex = 0;
  return cc;
}
