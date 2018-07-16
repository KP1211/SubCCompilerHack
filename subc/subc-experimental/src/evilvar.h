#include <stdio.h>
#include <stdlib.h>

#ifndef EVILVAR_H
#define EVILVAR_H

// Returns 1 if variable a is a '\r' or '\n'.
int es_char(char a);
// Returns the ascii number of the argument, the argument must be a escaped character.
char* get_es(char a);
int wordcheck(char *inword, char *compare);
int modstr(char *word, int i, int modlen, char *replacement);
int cmpwithin(char *inword, char *subword, int i, int *index, int *cmplength);
int subwordcheck(char *inword, char *compare, int *index, int *cmplength);

#endif