/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */

#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

int getpid();

int fork();

int gettime();

void exit();

void get_stats();

void print_stats(int pid, struct stats st);
void get_all_stats();
void printInt(int n);


#endif  /* __LIBC_H__ */
