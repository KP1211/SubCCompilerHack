#include <stdio.h>

#ifndef EVILVAR_H
#define EVILVAR_H

#define READ_LINE_SIZE 100
#define HC_MAX_LINE 100

extern char *target_src[];
extern char buffer[];
extern int dummy_i;
extern int buffer_index;
extern int beginpos;
extern FILE *incopy;	

void ini_src(void);
void print_tsrc(void);
void cmp_src_hc(FILE *incopy);
void prt_stdin(FILE *incopy);
// Returns 1 if variable a is a '\r' or '\n'.
int es_char(char a);
// Returns the ascii number of the argument, the argument must be a escaped character.
char* get_es(char a);

#endif