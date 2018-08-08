#include <stdio.h>
#include <string.h>

// Expect input files to be fully correct with C syntax grammar.
    // Might cause bug otherwise.

// Removes all comments.
// Replace newlines, tabs and space.
// For Preprocessor commands, it will give it its own line.

void main( int argc, char *argv[] ) {
    int i, j, k, l, sourcesize;
    FILE *input;
    FILE *output;
    char tmpfilename[100];
    char tmpword[100];
    char source[200000];
    char c;
    int maxstrlen;

    if( argc < 2 ) {
        printf("Expecting 1 or more argument\n");
        return;
    }
    
    maxstrlen = 200000;
    for( i = 1; i < argc; ++i ) {
        //printf("i: %d - '%s'\n",i,argv[i]);
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
        for( j = 0, l = 0; j < sourcesize - 1; ++j ) {
            if( source[j] == '/' ) {
                l = j;
                if( source[l+1] == '/' ) {
                    for( k = l + 2; (k < sourcesize) && (source[k] != '\n'); ++k ) { } // To get to the end of comment. k = j + 2 cuze j and j+1 are already parsed to be //.
                    for( ; k < sourcesize; ++l, ++k ) {
                        source[l] = source[k];
                    }
                    source[l] = '\0';
                    sourcesize = strlen(source);
                    continue;
                }
                if( source[l+1] == '*' ) {
                    for( k = l + 2; (k < sourcesize - 1) && ( (source[k] != '*') || (source[k+1] != '/') ); ++k ) { } // To get to the end of comment. k = j + 2 cuze j and j+1 are already parsed to be /*.
                    k = k + 2;      // This will be anything after */ since k and k+1 is */.
                    for( ; k < sourcesize; ++l, ++k ) {
                        source[l] = source[k];
                    }
                    source[l] = '\0';
                    sourcesize = strlen(source);
                    continue;
                }
            }
        }
        sourcesize = strlen(source);
        //****/
        printf("sourcelen: %d\n",sourcesize);
        printf("-------------------------------------------\n");
        //for( int i = 0; i < strlen(source); ++i ) {
        //    printf("%c",source[i]);
        //}

// At this point, all comments have been removed.

        for( j = 0, l = 0; j < sourcesize; ) {
            c = source[j];
            ++j;
            // vvv Escaping sequences vvv
            if( c == '\\' || c == '\'' || c == '\"' ) {
                for( l = sourcesize; (l < maxstrlen) && (l > j-1); --l ) {     // Everything on l and after are moved over to the right once slot. Starting at the end of the c-string iterating to j (iterarting from right to left in the array).
                    if( l == sourcesize ) 
                        source[l+1] = '\0';     // Should only run once, at the first iteration of the loop, to make sure new string is proper c-string.
                    //if(l<200) printf("Replacing source[%d]: '%c' with source[%d] '%c'\n",l,source[l], l-1,source[l-1]);
                    source[l] = source[l-1];
                }
                source[l] = '\\';
                //printf("start: '%c'\n",source[j+1]);
                sourcesize = strlen(source);
                if( strlen(source) >=  maxstrlen ) {
                    printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", argv[i]);
                    break;
                }
                ++j;    // To continue on stuff that not parsed, meaning after \\ or \' or \"
            }
            // ^^^ Escaping sequences ^^^

        }

// At this point, all characters that need to be escaped has been escaped.

        for( j = 0; j < sourcesize; ++j ) {
            c = source[j];

            // vvv Fix spacing issues vvv
            if(c == '\n' || c == '\t' ) {         // Remove all new lines and tabs with space.
                source[j] = ' ';
            } 
            // ^^^ Fix spacing issues ^^^

            //* vvv Dealing with # commands vvv This is special case, as it skips the rest of the loop if this part runs.
            if( c == '#' ) {            // Used to parse anything that starts with '#'
                for( l = sourcesize; (l < maxstrlen) && (l > j); --l ) {     // Everything on l and after are moved over to the right once slot. Starting at the end of the c-string iterating to j (iterarting from right to left in the array).
                    if( l == sourcesize ) 
                        source[l+1] = '\0';     // Should only run once, at the first iteration of the loop, to make sure new string is proper c-string.
                    source[l] = source[l-1];
                }
                source[l] = '\n';
                sourcesize = strlen(source);
                ++j;
                for( ; source[j] != '\n'; ++j ) {

                }
            }
            // ^^^ Dealing with # commands ^^^ */
        }

// At this point, source should be rid of all unnessary charaters. 
//printf("%s\n",source);
        /***vvvvv making it one line vvvvv
        for( j = 0, l = 0; j < sourcesize; ) {
            c = source[j];
            ++j;
            if( c == '\n' ) {
                for( l = sourcesize; (l < maxstrlen) && (l > j); --l ) {     // Everything on l and after are moved over to the right once slot. Starting at the end of the c-string iterating to j (iterarting from right to left in the array).
                    if( l == sourcesize ) 
                        source[l+1] = '\0';     // Should only run once, at the first iteration of the loop, to make sure new string is proper c-string.
                    source[l] = source[l-1];
                }
                source[l-1] = '\\';
                source[l] = 'n';
                sourcesize = strlen(source);
                if( strlen(source) ==  maxstrlen ) {
                    printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", argv[i]);
                    break;
                }
                ++j;    // To continue on stuff that not parsed, meaning after \n
            }
        }
        //^^^^^ making it one line ^^^^^ ***/
printf("%s\n",source);
// At this point, source should be a one liner.

        //vvvvv outputing to file vvvvv
        fputs(source, output);
        //^^^^^ outputing to file ^^^^^

        fclose(input); fclose(output);
    }

    return;
}