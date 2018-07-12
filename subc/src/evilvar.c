//#include <stdio.h>
#include "evilvar.h"

// Defines the number of char to read from each line in a file.
#define READ_LINE_SIZE 100
// Defines the number of hardcoded lines to be in our program.
#define HC_MAX_LINE 100

char *target_src[HC_MAX_LINE];
char buffer[READ_LINE_SIZE];
int dummy_i = 0;
int buffer_index = 0;
int beginpos;
FILE *incopy;
//char helloworld[100];
//int helloindex = 0;

// Each string must contain '\r\n' for it to compare well with each string read from a file.
// Because when reading line by line from a file, it has '\r\n' appended to it at the end. At least for files made on Windows.
void ini_src(void) {
    target_src[0] = "#include <stdio.h>\r\n\0";
    target_src[1] = "\r\n\0";
    target_src[2] = "int main() {\r\n\0";
    target_src[3] = "\r\n\0";
    target_src[4] = "	printf(\"Hello World\\n\");\r\n\0";
    target_src[5] = "	\r\n\0";
    target_src[6] = "	return 0;\r\n\0";
    target_src[7] = "	\r\n\0";
    target_src[8] = "}\r\n\0";

    return;
}


void print_tsrc(void) {
    int i;

    for( i = 0; i < 9; ++i ) {
        printf("%s",target_src[i]);
    }

    return;
}

void cmp_src_hc(FILE *incopy) {
	buffer_index = 0;
	if( feof(incopy) ) {
		printf("File stream is flaged as end of file\n");
		printf("Warning, functions didn't complete all its objectives.\n\n");
		buffer_index = 0;
	
		return;
	}
	while( !feof( incopy ) ) {
		fgets(buffer,READ_LINE_SIZE,incopy);
		if ( strcmp( buffer, target_src[buffer_index] ) != 0 ) {
			printf("Source file differ to hard coded helloworld.c:\n\n");
			printf("'%s'\n?------?\n'%s'\n\n",buffer,target_src[buffer_index]);
			printf("Length of string: %d(buffer) vs %d(target_src[%d])\n",strlen(buffer),strlen(target_src[buffer_index]),buffer_index);
		
			break;
		} 
		++buffer_index;
	}
	if( feof(incopy) && buffer_index == 9 ) {
		printf("End of file signal reached, source files is same as hard coded helloworld.c\n");
		printf("Read this many lines: %d\n",buffer_index);
	} else {
		printf("Files doens't match exactly, there might not be enough lines to match with.\n");
		printf("Warning, functions didn't complete all its objectives.\n\n");
		buffer_index = 0;
	
		return;
	}
	//Need to reset var 'in' to begin again.
	// Reseting global variable to be ready for reuse.
	buffer_index = 0;
	dummy_i = 0;
	
	return;
}

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

void prt_instream(FILE *incopy) {
	int i;

	if( feof(incopy) ) {
		printf("File stream is flaged as end of file\n");
		printf("Warning, functions didn't complete all its objectives.\n\n");
	
		return;
	}
	while( !feof( incopy ) ) {
		fgets(buffer,READ_LINE_SIZE,incopy);
		for( i = 0; i < strlen(buffer); ++i ) {
			if( es_char( buffer[i] ) ) {
				printf( get_es( buffer[i] ) );
			} else {
				printf("%c",buffer[i]);
			}
		}
		printf("\n");
	}
	
	return;
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

	/* // Old Method, which was assuming var word and var replacement are of same length.
	for(;j <= modlen; ++j, ++i) {			// <= here cuz i need to go over 'len' times.
		if(replacement[j] != '\0') {	// To make sure the last char of a proper c-string null is not copied over to new string.
			printf("Each char:%c\n",word[i]);
			word[i] = replacement[j];
		}
	}
	*/

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
	char subword[READ_LINE_SIZE];

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

// Parse for a word out in a string, and pass them into a checker function.
int scanforword(char *line, char *compare, int *index, int *cmplength) {
	int i;
	int len;
	char csholder[100];
	
	// This will give it a \0, which will make it a proper c-string.
	strcpy(csholder,"");
	printf("New scanforword()...\n");

	// Should't cause much problem to not bound check holder, cuz buffer should be of size smaller than or equal to READ_LINE_SIZE, since they are both initialize to that size.
	for(i = 0; i < strlen(line); ++i) {
		if (line[i] == ' ' || line[i] == '\r' || line[i] == '\n' || line[i] == '\0' || line[i] == '\t') {
			printf("csholder is this: '%s'\n",csholder);
			
			if( wordcheck( csholder, compare ) ) {
			*index = 0;
			*cmplength = strlen(csholder);
				return 1;
			}
			if( subwordcheck( csholder, compare, index, cmplength ) ) {
				return 1;
			}

			strcpy(csholder,"");	//Make holder empty.
			continue;
		}
		len = strlen(csholder);
		csholder[len] = line[i];
		csholder[len+1] = '\0';
	}
	
	return 0;
}

FILE * makeevilbye(FILE *in) {
	// Read in line by line
	// Output each line into new file.
	// if that line contain some word that we want to replace, make the line we want, and output that into the file for that particular iteration.
	// go back to line 157.
	int index;
	int cmplength;

	index = 0; cmplength = 0;

	if( feof(in) ) {
		printf("File stream is flaged as end of file\n");
		printf("Warning, functions didn't complete all its objectives.\n\n");
	
		return NULL;
	}
	while( !feof( in ) ) {
		fgets(buffer,READ_LINE_SIZE,in);
		if( scanforword(buffer,"Hello",&index,&cmplength) ) {
			printf("Found! at index: %d, and length: %d\n",index,cmplength);
		}
		printf("----------\n");
	}

	return NULL;	// 1 is success.
}