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

void prt_stdin(FILE *incopy) {
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
	//printf( ":::%s",buffer );
	
	return;
}