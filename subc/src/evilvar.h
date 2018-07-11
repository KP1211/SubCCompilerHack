#include <stdio.h>
#include <stdlib.h>

#ifndef EVILVAR_H
#define EVILVAR_H

#define READ_LINE_SIZE 100
#define HC_MAX_LINE 100

extern char *target_src[];
extern char buffer[];   // Will be reenforce as a proper c-string.
extern int dummy_i;
extern int buffer_index;
extern int beginpos;
extern FILE *incopy;
extern char helloworld[];	

void ini_src(void);
void print_tsrc(void);
void cmp_src_hc(FILE *incopy);
void prt_instream(FILE *incopy);
// Returns 1 if variable a is a '\r' or '\n'.
int es_char(char a);
// Returns the ascii number of the argument, the argument must be a escaped character.
char* get_es(char a);
int wordcheck(char *inword, char *compare);
int cmpwithin(char *inword, char *subword, int i, int *index, int *length);
int subwordcheck(char *inword, char *compare, int *index, int *length);
int scanforword(char *buffer, char *compare, int *index, int *length);
FILE * makeevilbye(FILE *in);

#endif