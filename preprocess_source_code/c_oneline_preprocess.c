#include <stdio.h>
#include <string.h>

// Expect input files to be fully correct with C syntax grammar.
    // Might cause bug otherwise.

// Removes all comments.
// Escape single quote, double quote, and back slash.
// Replace new line and tabs with two char in a string, to make it oneline.

static int maxstrlen;

void removecomment(char *source, int *sourcesize) {
    int j, l, k;
    
    for( j = 0, l = 0; j < *sourcesize - 1; ++j ) {
        if( source[j] == '/' ) {
            l = j;
            if( source[l+1] == '/' ) {
                for( k = l + 2; (k < *sourcesize) && (source[k] != '\n'); ++k ) { } // To get to the end of comment. k = j + 2 cuze j and j+1 are already parsed to be //.
                for( ; k < *sourcesize; ++l, ++k ) {
                    source[l] = source[k];
                }
                source[l] = '\0';
                *sourcesize = strlen(source);
                continue;
            }
            if( source[l+1] == '*' ) {
                for( k = l + 2; (k < *sourcesize - 1) && ( (source[k] != '*') || (source[k+1] != '/') ); ++k ) { } // To get to the end of comment. k = j + 2 cuze j and j+1 are already parsed to be /*.
                k = k + 2;      // This will be anything after */ since k and k+1 is */.
                for( ; k < *sourcesize; ++l, ++k ) {
                    source[l] = source[k];
                }
                source[l] = '\0';
                *sourcesize = strlen(source);
                continue;
            }
        }
    }
    *sourcesize = strlen(source);

    return;
}

void oneline( char *source, int *sourcesize, char *filename ) {
    int i, j;

    for( i = 0; i < *sourcesize ; ++i ) {
        if( source[i] == '\n' || source[i] == '\t' ) {
            for( j = *sourcesize; (j < maxstrlen) && (j > i); --j ) {     
                if( j == *sourcesize ) 
                    source[j+1] = '\0';     
                source[j] = source[j-1];
            }
            if( source[i+1] == '\t' ) {
                source[i+1] = 't';
            } else if ( source[i+1] == '\n' ) {
                source[i+1] = 'n';
            } else {
                printf("Unrecognized character: %c", source[j]);
            }
            source[i] = '\\';
            
            *sourcesize = strlen(source);
            ++i;    // To move the index past what is already processed.
            if( strlen(source) >=  maxstrlen ) {
                printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", filename);
                break;
            }
        }
    }

    return;
}

void escapesequence( char *source, int *sourcesize , char *filename) {
    int j, l;
    char c;

    for( j = 0, l = 0; j < *sourcesize; ) {
        c = source[j];
        ++j;

        if( c == '\\' || c == '\'' || c == '\"' ) {
            for( l = *sourcesize; (l < maxstrlen) && (l > j-1); --l ) {     
                if( l == *sourcesize ) 
                    source[l+1] = '\0';     
                source[l] = source[l-1];
            }
            source[l] = '\\';
            *sourcesize = strlen(source);
            if( strlen(source) >=  maxstrlen ) {
                printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", filename);
                break;
            }
            ++j;    // To continue on stuff that not parsed, meaning after \\ or \' or \"
        }
    }

    return;
}

void main( int argc, char *argv[] ) {
    int i, j, k, l, sourcesize;
    FILE *input;
    FILE *output;
    char tmpfilename[100];
    char tmpword[100];
    char source[200000];
    char c;
    

    if( argc < 2 ) {
        printf("Expecting 1 or more argument\n");
        return;
    }
    
    maxstrlen = 200000;
    for( i = 1; i < argc; ++i ) {
        if( !strcmp(argv[1],"-o") ) {
            if( argc < 4 ) {
                printf("Expecting 2 or more argument after '-o'\n");
                return;
            }
            i = 3;
            strcpy(tmpfilename,argv[2]);
        } else if( !strcmp(argv[1],"-a") ) {
            if( argc < 4 ) {
                printf("Expecting 2 or more argument after '-o'\n");
                return;
            }
            i = 3;
            strcpy(tmpfilename,argv[i]);
            strcat(tmpfilename,argv[2]);
        } else { 
            strcpy(tmpfilename,argv[i]);
            strcat(tmpfilename,"new");
        }
        if( (input = fopen(argv[i], "r")) == NULL ) {
            printf("Cannot open input file '%s'. Skipping.\n",argv[i]);
            printf("-------------------------------------------\n");
            continue;
        }
        if( (output = fopen(tmpfilename, "w")) == NULL) {
            printf("Cannot open output file '%s'. Skipping.\n",tmpfilename);
            printf("-------------------------------------------\n");
            continue;
        }
        strcpy(tmpword,"");
        strcpy(source,"");

// At this point, all filenaming issue have been resolve.
        // have a storage char variable for whole source.
        for( j = 0; j < maxstrlen && ((c = fgetc(input)) != EOF); ++j ) {
            source[j] = c;
        }
        source[j] = '\0';
        sourcesize = strlen(source);
        printf("Parsing input file '%s'\n",argv[i]);
        printf("clean sourcelen: %d\n",sourcesize);
        
//***** Delete all comments. 

        removecomment( source, &sourcesize );

// At this point, string should be a one liner.

        escapesequence( source, &sourcesize, argv[i] );  

// At this point, all comments have been removed.

        oneline( source, &sourcesize, argv[i] );


// At this point, all characters that need to be escaped has been escaped.

        printf("sourcelen after preprocess: %d\n",sourcesize);
        printf("-------------------------------------------\n");
                

//printf("%s\n",source);
printf("%s\n",source);

        //vvvvv outputing to file vvvvv
        fputs(source, output);
        //^^^^^ outputing to file ^^^^^

        fclose(input); fclose(output);
    }

    return;
}