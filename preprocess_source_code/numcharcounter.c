#include <stdio.h>
void main(int argc, char *argv[]) {
    int i, totalchar;
    FILE *file;
    
    for( i = 1; i < argc; ++i ) {
        totalchar = 0;
        //open file and count here
        if( (file = fopen(argv[i],"r")) == NULL) {
            printf("no such file: %s\n", argv[i]);
            continue;
        }

        while(fgetc(file) != EOF ) {
            ++totalchar;
        }
        printf( "%s has: %d chars\n", argv[i], totalchar );
        fclose(file);
    }

    return;
}