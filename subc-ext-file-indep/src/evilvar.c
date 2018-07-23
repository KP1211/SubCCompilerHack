//#include <stdio.h>
#include "evilvar.h"

// Checks if a is \r or \n
int es_char(char a) {
	switch(a) {
		case '\n' : 			
			return 1;
			break;
		case '\r' :			
			return 1;
			break;
		default :
			return 0;
	}
}

char* get_es(char a) {
	switch(a) {
		case '\n' : 		
			return "\\n";
			break;
		case '\r' :	
			return "\\r";
			break;
		default :	
			return "\0";	// Returns NULL
	}
}

// compare if inword[] is same str as compare[].
int wordcheck(char *inword, char *compare) {
	if ( strcmp(inword,compare) == 0 ) 			// strcmp returns 0 on same string.
		return 1;
	return 0;
}

// For now, assume var replacement and var word are of same length.
// For now only replace string of same length, when replacing string of different length, will add function to cut off at some point, and then concat the rest on, after modifying.
// var i, is index to start in var word.
// Assumes var replacement is not a proper c-string, with a \0 at the end, cuz that char won't be inserted into var word, and will mess with some of the lenth variables.
// Maybe I don't have to worry about len of a \0, cuz strlen() doesn't count \0.
int modstr(char *word, int i, int modlen, char *replacement) {	// modlen is length of modification on var word, the part that's getting taken out, not the length that is to replace it that gap.
	int j, k;
	int replacelen, wordlen, newlen, tmplen;
	char tmpstr[513];
	// Have some checks here that var word have enough space to replace some part of it with var replacement. such as replacement len, and rest of word len.
	// Also have check if replacement exceed \0 of var word, got to append a \0 at the end if that's the case.
	replacelen = strlen(replacement);
	wordlen = strlen(word);
	newlen = (wordlen - modlen) + replacelen;
	
	if( newlen > 513 ) {	// 513 is the max size of the char* used in scanstr()'s argument.
		return 0; 			// Error here is new word is too long to be accomadated.
	}
	//WHAT ABOUT MAKING buf SMALLER? it works. I tested.
	printf("%s, of size: %d\n", replacement,replacelen);
	printf("Modifying length of word is: %d\n", modlen);

	j = 0;
	k = 0;
	//appending the 2nd half of var word, right after where we will end our replace.
	for(j = i + modlen; j < wordlen; ++j, ++k) {
		tmpstr[k] = word[j];
		printf("Appending 2nd half to tmpstr[]:'%c'\n",word[j]);
	}
	tmpstr[k] = '\0'; 		// Making tmpstr a prope| 2nd half: '%s'r c-string, to avoid confusion later on.
	//printf("word: '%s' | modification len: %d\n",word,modlen);
	//printf("2nd half: '%s'\n",tmpstr);

	j = 0;
	k = 0;
	// Starting from i, copy var replacement over var word.
	for(j = i; j <= replacelen; ++ j, ++ k) {
		printf("Replacing each char: '%c' with '%c'\n",word[j], replacement[k]);
		word[j] = replacement[k];
	}
	word[j] = '\0'; 		// Making this a proper c-string. To avoid confusion.
	// Append the rest of the string back on.
	j = strlen(word);;
	k = 0;
	tmplen = strlen(tmpstr);
	//printf("Why so low? : %d\n",tmplen);
	for(; k < tmplen; ++j, ++k) {
		printf("Appending the rest of the word back in: '%c'\n", tmpstr[k]);
		word[j] = tmpstr[k];
	}
	word[j] = '\0';	// Making this a proper c-string. To avoid confusion.
	printf("The new word: %s\n",word);

	return 1;
}

// As this fct construct subword, it will compare at every step to account for starting and ending indexing 
// -> when constructing subword, to avoid duplicated calculation all possible subword and returning it, easier to check it at this fct.
int cmpwithin(char *inword, char *compare, int i, int *index, int *cmplength) {
	int len;
	int sublen;
	char subword[100];

	len = strlen(inword);
	strcpy(subword,"");	// Make a proper empty c-string.
	
	if( i >= len )
		return 0;

	for(; i < len; ++i) {
		sublen = strlen(subword);
		subword[sublen] = inword[i];
		subword[sublen+1] = '\0';
		if( wordcheck(subword,compare) ) {
			*cmplength = strlen(subword);
			*index = i + 1 - *cmplength;		// At this point, the correct starting index has been offset by subword length. Got adjust by one because index starts counting at 0, while length starts counting at 1, it's to even things out.
			return 1;
		}
	}
	return 0;
}

// Call this, don't call cmpwithin by it self, this fct is the over head that checks.
// Compare if inword has a sub word of compare, this fct is called after wordcheck.
//CHECK IF A exact copy of string when pass into this fct will work.---> It works.
int subwordcheck(char *inword, char *compare, int *index, int *cmplength) {
	int i;
	int len;

	len = strlen(inword);
	for( i = 0; i < len; ++i ) {
		if( cmpwithin(inword,compare,i,index,cmplength) ) {
			return 1;
		}
	}
	return 0;
}
