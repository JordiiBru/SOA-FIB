#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#endif /* __KEYBOARD_H__ */


#define SIZE_OF_BUFFER      10


extern int readIndex;
extern int writeIndex;
extern int circ_len;

int circular_buffer_add(char c);

char circular_buffer_get();
