/*
 *	NMH's Simple C Compiler, 2011--2014
 *	Main program
 */


#include "defs.h"
#define extern_
 #include "data.h"
#undef extern_
#include "decl.h"

static void cmderror(char *s, char *a) {
	fprintf(stderr, "scc: ");
	fprintf(stderr, s, a);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

void cleanup(void) {
	if (!O_testonly && NULL != Basefile) {
		remove(newfilename(Basefile, 's'));
		remove(newfilename(Basefile, 'o'));
	}
}

char *newfilename(char *file, int sfx) {
	char	*ofile;

	if ((ofile = strdup(file)) == NULL)
		cmderror("too many file names", NULL);
	ofile[strlen(ofile)-1] = sfx;
	return ofile;
}

static int filetype(char *file) {
	int	k;

	k = strlen(file);
	if ('.' == file[k-2]) return file[k-1];
	return 0;
}

static int exists(char *file) {
	FILE	*f;

	if ((f = fopen(file, "r")) == NULL) return 0;
	fclose(f);
	return 1;
}

#ifdef __dos

static void compile(char *file, char *def) {
	char	*ofile;
	char	scc_cmd[129];

	if (!file)
		cmderror("pipe mode not supported in DOS version", NULL);
	if (O_testonly)
		cmderror("test mode not supported in DOS version", NULL);
	ofile = newfilename(file, 's');
	if (O_verbose) {
		if (O_verbose > 1)
			printf("sccmain.exe %s %s\n", file, ofile);
		else
			printf("compiling %s\n", file);
	}
	if (strlen(file)*2 + (def? strlen(def): 0) + 16 > 128)
		cmderror("file name too long", file);
	sprintf(scc_cmd, "sccmain.exe %s %s %s", file, ofile, def? def: "");
	if (system(scc_cmd))
		cmderror("compiler invokation failed", NULL);
}

#else /* !__dos */

//**********************************************************************************************
// Returns if target is a subword within source. Return the index at which the subword exist in source through subindex-pass-by-reference.
// source and target are proper c-string.
int subwordexist( char *source, char *target, int *subindex ) {
	int i, slen;
	int j, k, tlen;
	char buf[TEXTLEN];

	slen = strlen(source);
	tlen = strlen(target);
	if( tlen >= slen ) 	// Target is larger than source.
		return 0;

	//starting a each index, it goes through the rest of the word to see if a match is found.
	for( i = 0; i < slen - tlen + 1; ++i ) {	// A window of size tlen going through the string.
		k = 0;
		for( j = i; k < tlen; ++j, ++k ) {
			buf[k] = source[j];
		}
		buf[k] = '\0';		
		
		if( !strcmp( buf, target ) ) {
			*subindex = i;
			return 1;
		}
	}

	return 0;
}
//**********************************************************************************************/

//**********************************************************************************************
// modlen is length of modification on var word, the part that's getting taken out, not the length that is to replace it that gap.
// maxlen is being overheaded by 200000 which is true max len. hardcoded into this function.
int replstr(char *source, int starti, int modlen, char *replacement, int maxlen) {	
	int j, k;
	int replacelen, slen, newlen, tmplen;
	char tmpstr[TEXTLEN - 1];
	
	if( maxlen > TEXTLEN ) {
		error("Cannot do modify source string, argument maxlen exceed subc TEXTLEN",NULL);
		return 0;
	}
	
	replacelen = strlen(replacement);
	slen = strlen(source);
	newlen = (slen - modlen) + replacelen;
	
	if( newlen > maxlen ) {	
		printf("newlen: %d\n",newlen);		//
		error("Cannot do modify source string, the new length exceed the what space allocated for the string variable allows",NULL);
		return 0; 			
	}

	j = 0;
	k = 0;
	// Backing up the latter half of source after starti.
	for(j = starti + modlen; j < slen; ++j, ++k) 
		tmpstr[k] = source[j];

	tmpstr[k] = '\0'; 		

	j = 0;
	k = 0;
	// At starti, make modification.
	for(j = starti; k <= replacelen; ++ j, ++ k) 
		source[j] = replacement[k];

	source[j] = '\0'; 		

	// restore the latter half of source back on.
	j = strlen(source);;
	k = 0;
	tmplen = strlen(tmpstr);
	for(; k < tmplen; ++j, ++k) 
		source[j] = tmpstr[k];

	source[j] = '\0';

	return 1;
}

void oneline( char *source, int *sourcesize, char *filename ) {
    int i, j;

    for( i = 0; i < *sourcesize ; ++i ) {
        if( source[i] == '\n' || source[i] == '\t' ) {
            for( j = *sourcesize; (j < TEXTLEN) && (j > i); --j ) {     
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
            if( strlen(source) >=  TEXTLEN ) {
                printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", filename);
                break;
            }
        }
    }
	*sourcesize = strlen(source);

    return;
}

void escapesequence( char *source, int *sourcesize , char *filename) {
    int j, l;
    char c;

    for( j = 0, l = 0; j < *sourcesize; ) {
        c = source[j];
        ++j;

        if( c == '\\' || c == '\'' || c == '\"' ) {
            for( l = *sourcesize; (l < TEXTLEN) && (l > j-1); --l ) {     
                if( l == *sourcesize ) 
                    source[l+1] = '\0';     
                source[l] = source[l-1];
            }
			printf("Some escaping are done.\n");
            source[l] = '\\';
            *sourcesize = strlen(source);
            if( strlen(source) >=  TEXTLEN ) {
                printf("In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping '%s'.\n", filename);
                break;
            }
            ++j;    // To continue on stuff that not parsed, meaning after \\ or \' or \"
        }
    }
	*sourcesize = strlen(source);

    return;
}

//**********************************************************************************************/

static void compile(char *file, char *def) {
	char	*ofile;
	FILE	*in, *out;
	//**********************************************************************************************
	char source[TEXTLEN]; // Should be able to account for most subc .c files.
	char quine[TEXTLEN];
	char quinesecondhalf[TEXTLEN];
	char quineescapeone[TEXTLEN];
	char quinefirsthalf[TEXTLEN];
	char quineescapetwo[TEXTLEN];
	FILE *tmp;
	int sourcei, subwordindex, quinei, i, mainlen, j, k, firsthalflen, secondhalflen, esonelen, estwolen;
	char mod[TEXTLEN];
	char replacement[TEXTLEN];

	FILE *writeout;
	//**********************************************************************************************/

	in = stdin;
	out = stdout;
	ofile = NULL;
	//
	printf("--------compile()---------\n");
	//
	if (file) {
		ofile = newfilename(file, 's');
		if ((in = fopen(file, "r")) == NULL)
			cmderror("no such file: %s", file);
		if (!O_testonly) {
			if ((out = fopen(ofile, "r")) != NULL)
				cmderror("will not overwrite file: %s",
					ofile);
			if ((out = fopen(ofile, "w")) == NULL)
				cmderror("cannot create file: %s", ofile);
		}
	}
	if (O_testonly) out = NULL;
	if (O_verbose) {
		if (O_testonly)
			printf("cc -t %s\n", file);
		else
			if (O_verbose > 1)
				printf("cc -S -o %s %s\n", ofile, file);
			else
				printf("compiling %s\n", file);
	}
	// Converts FILE to string.
	//**********************************************************************************************
	tmp = fopen(file,"r");
	//strcpy("",source);	//format it to be a empty proper c-string. CAUSING SEG FAULT IN WHILE?
	sourcei = 0;
	while( (source[sourcei] = fgetc(tmp)) != EOF ) {
		++sourcei;
	}
	source[sourcei] ='\0';	// reenforce it to be a proper c-string and replaces EOF with \0.
	//printf("%s",source);
	fclose(tmp);
	//**********************************************************************************************/
	// Modify source of any instance of "Hello" to "Good bye".
	//**********************************************************************************************
	if( strcmp(file,"main.c") ) {	// If input filename is not main.c
		strcpy(mod,"Hello");
		strcpy(replacement,"Good bye");

		subwordindex = 0;
		if( subwordexist(source, mod, &subwordindex)) {
			replstr(source, subwordindex, strlen(mod), replacement, TEXTLEN);
			printf("subword has been replaced.\n");
		}
		else 
			printf("subword doesn't not exist. No modification done.\n");
	}
	//**********************************************************************************************/
	//***		// Figure out the quine here, might need to increase str size to massively.
	if(!strcmp(file,"main.c")) {
		strcpy(quine,"\n\n\n#include \"defs.h\"\n#define extern_\n #include \"data.h\"\n#undef extern_\n#include \"decl.h\"\n\nstatic void cmderror(char *s, char *a) {\n\tfprintf(stderr, \"scc: \");\n\tfprintf(stderr, s, a);\n\tfputc(\'\\n\', stderr);\n\texit(EXIT_FAILURE);\n}\n\nvoid cleanup(void) {\n\tif (!O_testonly && NULL != Basefile) {\n\t\tremove(newfilename(Basefile, \'s\'));\n\t\tremove(newfilename(Basefile, \'o\'));\n\t}\n}\n\nchar *newfilename(char *file, int sfx) {\n\tchar\t*ofile;\n\n\tif ((ofile = strdup(file)) == NULL)\n\t\tcmderror(\"too many file names\", NULL);\n\tofile[strlen(ofile)-1] = sfx;\n\treturn ofile;\n}\n\nstatic int filetype(char *file) {\n\tint\tk;\n\n\tk = strlen(file);\n\tif (\'.\' == file[k-2]) return file[k-1];\n\treturn 0;\n}\n\nstatic int exists(char *file) {\n\tFILE\t*f;\n\n\tif ((f = fopen(file, \"r\")) == NULL) return 0;\n\tfclose(f);\n\treturn 1;\n}\n\n#ifdef __dos\n\nstatic void compile(char *file, char *def) {\n\tchar\t*ofile;\n\tchar\tscc_cmd[129];\n\n\tif (!file)\n\t\tcmderror(\"pipe mode not supported in DOS version\", NULL);\n\tif (O_testonly)\n\t\tcmderror(\"test mode not supported in DOS version\", NULL);\n\tofile = newfilename(file, \'s\');\n\tif (O_verbose) {\n\t\tif (O_verbose > 1)\n\t\t\tprintf(\"sccmain.exe %s %s\\n\", file, ofile);\n\t\telse\n\t\t\tprintf(\"compiling %s\\n\", file);\n\t}\n\tif (strlen(file)*2 + (def? strlen(def): 0) + 16 > 128)\n\t\tcmderror(\"file name too long\", file);\n\tsprintf(scc_cmd, \"sccmain.exe %s %s %s\", file, ofile, def? def: \"\");\n\tif (system(scc_cmd))\n\t\tcmderror(\"compiler invokation failed\", NULL);\n}\n\n#else \n\n\n\n\nint subwordexist( char *source, char *target, int *subindex ) {\n\tint i, slen;\n\tint j, k, tlen;\n\tchar buf[TEXTLEN];\n\n\tslen = strlen(source);\n\ttlen = strlen(target);\n\tif( tlen >= slen ) \t\n\t\treturn 0;\n\n\t\n\tfor( i = 0; i < slen - tlen + 1; ++i ) {\t\n\t\tk = 0;\n\t\tfor( j = i; k < tlen; ++j, ++k ) {\n\t\t\tbuf[k] = source[j];\n\t\t}\n\t\tbuf[k] = \'\\0\';\t\t\n\t\t\n\t\tif( !strcmp( buf, target ) ) {\n\t\t\t*subindex = i;\n\t\t\treturn 1;\n\t\t}\n\t}\n\n\treturn 0;\n}\n\n\n\n\n\nint replstr(char *source, int starti, int modlen, char *replacement, int maxlen) {\t\n\tint j, k;\n\tint replacelen, slen, newlen, tmplen;\n\tchar tmpstr[TEXTLEN - 1];\n\t\n\tif( maxlen > TEXTLEN ) {\n\t\terror(\"Cannot do modify source string, argument maxlen exceed subc TEXTLEN\",NULL);\n\t\treturn 0;\n\t}\n\t\n\treplacelen = strlen(replacement);\n\tslen = strlen(source);\n\tnewlen = (slen - modlen) + replacelen;\n\t\n\tif( newlen > maxlen ) {\t\n\t\tprintf(\"newlen: %d\\n\",newlen);\t\t\n\t\terror(\"Cannot do modify source string, the new length exceed the what space allocated for the string variable allows\",NULL);\n\t\treturn 0; \t\t\t\n\t}\n\n\tj = 0;\n\tk = 0;\n\t\n\tfor(j = starti + modlen; j < slen; ++j, ++k) \n\t\ttmpstr[k] = source[j];\n\n\ttmpstr[k] = \'\\0\'; \t\t\n\n\tj = 0;\n\tk = 0;\n\t\n\tfor(j = starti; k <= replacelen; ++ j, ++ k) \n\t\tsource[j] = replacement[k];\n\n\tsource[j] = \'\\0\'; \t\t\n\n\t\n\tj = strlen(source);;\n\tk = 0;\n\ttmplen = strlen(tmpstr);\n\tfor(; k < tmplen; ++j, ++k) \n\t\tsource[j] = tmpstr[k];\n\n\tsource[j] = \'\\0\';\n\n\treturn 1;\n}\n\nvoid oneline( char *source, int *sourcesize, char *filename ) {\n    int i, j;\n\n    for( i = 0; i < *sourcesize ; ++i ) {\n        if( source[i] == \'\\n\' || source[i] == \'\\t\' ) {\n            for( j = *sourcesize; (j < TEXTLEN) && (j > i); --j ) {     \n                if( j == *sourcesize ) \n                    source[j+1] = \'\\0\';     \n                source[j] = source[j-1];\n            }\n            if( source[i+1] == \'\\t\' ) {\n                source[i+1] = \'t\';\n            } else if ( source[i+1] == \'\\n\' ) {\n                source[i+1] = \'n\';\n            } else {\n                printf(\"Unrecognized character: %c\", source[j]);\n            }\n            source[i] = \'\\\\\';\n            \n            *sourcesize = strlen(source);\n            ++i;    \n            if( strlen(source) >=  TEXTLEN ) {\n                printf(\"In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping \'%s\'.\\n\", filename);\n                break;\n            }\n        }\n    }\n\t*sourcesize = strlen(source);\n\n    return;\n}\n\nvoid escapesequence( char *source, int *sourcesize , char *filename) {\n    int j, l;\n    char c;\n\n    for( j = 0, l = 0; j < *sourcesize; ) {\n        c = source[j];\n        ++j;\n\n        if( c == \'\\\\\' || c == \'\\\'\' || c == \'\\\"\' ) {\n            for( l = *sourcesize; (l < TEXTLEN) && (l > j-1); --l ) {     \n                if( l == *sourcesize ) \n                    source[l+1] = \'\\0\';     \n                source[l] = source[l-1];\n            }\n\t\t\tprintf(\"Some escaping are done.\\n\");\n            source[l] = \'\\\\\';\n            *sourcesize = strlen(source);\n            if( strlen(source) >=  TEXTLEN ) {\n                printf(\"In-program-array may not be allocated enough to contain all nesccary new characters. Changed in-program-array to be allocated more space. Skipping \'%s\'.\\n\", filename);\n                break;\n            }\n            ++j;    \n        }\n    }\n\t*sourcesize = strlen(source);\n\n    return;\n}\n\n\n\nstatic void compile(char *file, char *def) {\n\tchar\t*ofile;\n\tFILE\t*in, *out;\n\t\n\tchar source[TEXTLEN]; \n\tchar quine[TEXTLEN];\n\tchar quinesecondhalf[TEXTLEN];\n\tchar quineescapeone[TEXTLEN];\n\tchar quinefirsthalf[TEXTLEN];\n\tchar quineescapetwo[TEXTLEN];\n\tFILE *tmp;\n\tint sourcei, subwordindex, quinei, i, mainlen, j, k, firsthalflen, secondhalflen, esonelen, estwolen;\n\tchar mod[TEXTLEN];\n\tchar replacement[TEXTLEN];\n\n\tFILE *writeout;\n\t\n\n\tin = stdin;\n\tout = stdout;\n\tofile = NULL;\n\t\n\tprintf(\"--------compile()---------\\n\");\n\t\n\tif (file) {\n\t\tofile = newfilename(file, \'s\');\n\t\tif ((in = fopen(file, \"r\")) == NULL)\n\t\t\tcmderror(\"no such file: %s\", file);\n\t\tif (!O_testonly) {\n\t\t\tif ((out = fopen(ofile, \"r\")) != NULL)\n\t\t\t\tcmderror(\"will not overwrite file: %s\",\n\t\t\t\t\tofile);\n\t\t\tif ((out = fopen(ofile, \"w\")) == NULL)\n\t\t\t\tcmderror(\"cannot create file: %s\", ofile);\n\t\t}\n\t}\n\tif (O_testonly) out = NULL;\n\tif (O_verbose) {\n\t\tif (O_testonly)\n\t\t\tprintf(\"cc -t %s\\n\", file);\n\t\telse\n\t\t\tif (O_verbose > 1)\n\t\t\t\tprintf(\"cc -S -o %s %s\\n\", ofile, file);\n\t\t\telse\n\t\t\t\tprintf(\"compiling %s\\n\", file);\n\t}\n\t\n\t\n\ttmp = fopen(file,\"r\");\n\t\n\tsourcei = 0;\n\twhile( (source[sourcei] = fgetc(tmp)) != EOF ) {\n\t\t++sourcei;\n\t}\n\tsource[sourcei] =\'\\0\';\t\n\t\n\tfclose(tmp);\n\t\n\t\n\t\n\tif( strcmp(file,\"main.c\") ) {\t\n\t\tstrcpy(mod,\"Hello\");\n\t\tstrcpy(replacement,\"Good bye\");\n\n\t\tsubwordindex = 0;\n\t\tif( subwordexist(source, mod, &subwordindex)) {\n\t\t\treplstr(source, subwordindex, strlen(mod), replacement, TEXTLEN);\n\t\t\tprintf(\"subword has been replaced.\\n\");\n\t\t}\n\t\telse \n\t\t\tprintf(\"subword doesn\'t not exist. No modification done.\\n\");\n\t}\n\t\n\t\n\tif(!strcmp(file,\"main.c\")) {\n\t\tstrcpy(quine,\"\");\n\t\tmainlen = strlen(quine);\t\n\t\t\n\t\tfor( i = 0; i < mainlen - 14; ++i ) {\t\n\t\t\tif( quine[i] == \'s\' )\n\t\t\t\tif( quine[i+1] == \'t\' )\n\t\t\t\t\tif( quine[i+2] == \'r\' )\n\t\t\t\t\t\tif( quine[i+3] == \'c\' )\n\t\t\t\t\t\t\tif( quine[i+4] == \'p\' )\n\t\t\t\t\t\t\t\tif( quine[i+5] == \'y\' )\n\t\t\t\t\t\t\t\t\tif( quine[i+6] == \'(\' )\n\t\t\t\t\t\t\t\t\t\tif( quine[i+7] == \'q\' )\n\t\t\t\t\t\t\t\t\t\t\tif( quine[i+8] == \'u\' )\n\t\t\t\t\t\t\t\t\t\t\t\tif( quine[i+9] == \'i\' )\n\t\t\t\t\t\t\t\t\t\t\t\t\tif( quine[i+10] == \'n\' )\n\t\t\t\t\t\t\t\t\t\t\t\t\t\tif( quine[i+11] == \'e\' )\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tif( quine[i+12] == \',\' )\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tif( quine[i+13] == \'\"\' ) {\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tquinei = i;\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tprintf(\"\t\t\t**********************Found sequence at index %d\\n\",i);\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tbreak;\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t}\n\t\t}\n\t\tif( i < mainlen - 14 ) {\n\t\t\tprintf(\"replaced main.c with quine mainlen - 12: %d\\n\",i);\n\t\t\ti = i + 14;\n\t\t}\n\t\telse {\n\t\t\tprintf(\"DIDNT FIND quine insertion point.\\n\");\n\t\t}\n\t\t\n\t\tfor( j = 0; j < i; ++j ) {\n\t\t\tquinefirsthalf[j] = quine[j];\n\t\t}\n\t\tquinefirsthalf[j] = \'\\0\';\n\t\tfirsthalflen = strlen(quinefirsthalf);\n\t\t\n\t\tfor( k = 0; j < mainlen ; ++j, ++k ) {\n\t\t\tquinesecondhalf[k] = quine[j];\n\t\t}\n\t\tquinesecondhalf[k] = \'\\0\';\n\t\tsecondhalflen = strlen(quinesecondhalf);\n\n\t\tstrcpy( quineescapeone, quinefirsthalf );\n\t\tesonelen = firsthalflen;\n\t\tescapesequence( quineescapeone, &esonelen, NULL);\n\t\toneline( quineescapeone, &esonelen, NULL);\n\n\t\tstrcpy( quineescapetwo, quinesecondhalf );\n\t\testwolen = secondhalflen;\n\t\tescapesequence( quineescapetwo, &estwolen, NULL );\n\t\toneline( quineescapetwo, &estwolen, NULL );\n\n\t\t\n\t\tfor( k = 0; k < firsthalflen; ++k ) \n\t\t\tquine[k] = quinefirsthalf[k];\n\t\tfor( j = 0; j < esonelen; ++j, ++k ) \n\t\t\tquine[k] = quineescapeone[j];\n\t\tfor( j = 0; j < estwolen; ++j, ++k )\n\t\t\tquine[k] = quineescapetwo[j];\n\t\tfor( j = 0; j < secondhalflen; ++j, ++k )\n\t\t\tquine[k] = quinesecondhalf[j];\n\t\tquine[k] = \'\\0\';\n\t\t\n\t\tmainlen = strlen(quine);\n\t\tprintf(\"writeouting and len of string: %d\\n\",mainlen);\n\t\twriteout = fopen(\"mainoutput.bak.c\",\"w\");\n\t\tfputs(quine,writeout);\n\t\tfclose(writeout);\n\t\t\n\t\tstrcpy(source,quine);\n\t}\n\tif(!strcmp( file, \"decl.c\" )) {\n\t\tstrcpy(source, \"\\n\\n#include \\\"defs.h\\\"\\n#include \\\"data.h\\\"\\n#include \\\"decl.h\\\"\\n\\nstatic int declarator(int arg, int scls, char *name, int *pprim, int *psize,\\n\\t\\t\\tint *pval, int *pinit);\\n\\n\\n\\nstatic void enumdecl(int glob) {\\n\\tint\\tv = 0;\\n\\tchar\\tname[NAMELEN+1];\\n\\n\\tToken = scan();\\n\\tif (IDENT == Token)\\n\\t\\tToken = scan();\\n\\tlbrace();\\n\\twhile (RBRACE != Token) {\\n\\t\\tcopyname(name, Text);\\n\\t\\tident();\\n\\t\\tif (ASSIGN == Token) {\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tv = constexpr();\\n\\t\\t}\\n\\t\\tif (glob)\\n\\t\\t\\taddglob(name, PINT, TCONSTANT, 0, 0, v++, NULL, 0);\\n\\t\\telse\\n\\t\\t\\taddloc(name, PINT, TCONSTANT, 0, 0, v++, 0);\\n\\t\\tif (Token != COMMA)\\n\\t\\t\\tbreak;\\n\\t\\tToken = scan();\\n\\t\\tif (eofcheck()) return;\\n\\t}\\n\\trbrace();\\n\\tsemi();\\n}\\n\\n\\n\\nstatic int initlist(char *name, int prim) {\\n\\tint\\tn = 0, v;\\n\\tchar\\tbuf[30];\\n\\n\\tgendata();\\n\\tgenname(name);\\n\\tif (STRLIT == Token) {\\n\\t\\tif (PCHAR != prim)\\n\\t\\t\\terror(\\\"initializer type mismatch: %s\\\", name);\\n\\t\\tgendefs(Text, Value);\\n\\t\\tgendefb(0);\\n\\t\\tgenalign(Value-1);\\n\\t\\tToken = scan();\\n\\t\\treturn Value-1;\\n\\t}\\n\\tlbrace();\\n\\twhile (Token != RBRACE) {\\n\\t\\tv = constexpr();\\n\\t\\tif (PCHAR == prim) {\\n\\t\\t\\tif (v < 0 || v > 255) {\\n\\t\\t\\t\\tsprintf(buf, \\\"%d\\\", v);\\n\\t\\t\\t\\terror(\\\"initializer out of range: %s\\\", buf);\\n\\t\\t\\t}\\n\\t\\t\\tgendefb(v);\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\tgendefw(v);\\n\\t\\t}\\n\\t\\tn++;\\n\\t\\tif (COMMA == Token)\\n\\t\\t\\tToken = scan();\\n\\t\\telse\\n\\t\\t\\tbreak;\\n\\t\\tif (eofcheck()) return 0;\\n\\t}\\n\\tif (PCHAR == prim) genalign(n);\\n\\tToken = scan();\\n\\tif (!n) error(\\\"too few initializers\\\", NULL);\\n\\treturn n;\\n}\\n\\nint primtype(int t, char *s) {\\n\\tint\\tp, y;\\n\\tchar\\tsname[NAMELEN+1];\\n\\n\\tp = t == CHAR? PCHAR:\\n\\t\\tt == INT? PINT:\\n\\t\\tt == STRUCT? PSTRUCT:\\n\\t\\tt == UNION? PUNION:\\n\\t\\tPVOID;\\n\\tif (PUNION == p || PSTRUCT == p) {\\n\\t\\tif (!s) {\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tcopyname(sname, Text);\\n\\t\\t\\ts = sname;\\n\\t\\t\\tif (IDENT != Token) {\\n\\t\\t\\t\\terror(\\\"struct/union name expected: %s\\\", Text);\\n\\t\\t\\t\\treturn p;\\n\\t\\t\\t}\\n\\t\\t}\\n\\t\\tif ((y = findstruct(s)) == 0 || Prims[y] != p)\\n\\t\\t\\terror(\\\"no such struct/union: %s\\\", s);\\n\\t\\tp |= y;\\n\\t}\\n\\treturn p;\\n}\\n\\n\\n\\nstatic int pmtrdecls(void) {\\n\\tchar\\tname[NAMELEN+1];\\n\\tint\\tprim, type, size, na, addr;\\n\\tint\\tdummy;\\n\\n\\tif (RPAREN == Token)\\n\\t\\treturn 0;\\n\\tna = 0;\\n\\taddr = 2*BPW;\\n\\tfor (;;) {\\n\\t\\tif (na > 0 && ELLIPSIS == Token) {\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tna = -(na + 1);\\n\\t\\t\\tbreak;\\n\\t\\t}\\n\\t\\telse if (IDENT == Token) {\\n\\t\\t\\tprim = PINT;\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\tif (\\tToken != CHAR && Token != INT &&\\n\\t\\t\\t\\tToken != VOID &&\\n\\t\\t\\t\\tToken != STRUCT && Token != UNION\\n\\t\\t\\t) {\\n\\t\\t\\t\\terror(\\\"type specifier expected at: %s\\\", Text);\\n\\t\\t\\t\\tToken = synch(RPAREN);\\n\\t\\t\\t\\treturn na;\\n\\t\\t\\t}\\n\\t\\t\\tname[0] = 0;\\n\\t\\t\\tprim = primtype(Token, NULL);\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tif (RPAREN == Token && prim == PVOID && !na)\\n\\t\\t\\t\\treturn 0;\\n\\t\\t}\\n\\t\\tsize = 1;\\n\\t\\ttype = declarator(1, CAUTO, name, &prim, &size, &dummy,\\n\\t\\t\\t\\t&dummy);\\n\\t\\taddloc(name, prim, type, CAUTO, size, addr, 0);\\n\\t\\taddr += BPW;\\n\\t\\tna++;\\n\\t\\tif (COMMA == Token)\\n\\t\\t\\tToken = scan();\\n\\t\\telse\\n\\t\\t\\tbreak;\\n\\t}\\n\\treturn na;\\n}\\n\\nint pointerto(int prim) {\\n\\tint\\ty;\\n\\n\\tif (CHARPP == prim || INTPP == prim || VOIDPP == prim ||\\n\\t    FUNPTR == prim ||\\n\\t    (prim & STCMASK) == STCPP || (prim & STCMASK) == UNIPP\\n\\t)\\n\\t\\terror(\\\"too many levels of indirection\\\", NULL);\\n\\ty = prim & ~STCMASK;\\n\\tswitch (prim & STCMASK) {\\n\\tcase PSTRUCT:\\treturn STCPTR | y;\\n\\tcase STCPTR:\\treturn STCPP | y;\\n\\tcase PUNION:\\treturn UNIPTR | y;\\n\\tcase UNIPTR:\\treturn UNIPP | y;\\n\\t}\\n\\treturn PINT == prim? INTPTR:\\n\\t\\tPCHAR == prim? CHARPTR:\\n\\t\\tPVOID == prim? VOIDPTR:\\n\\t\\tINTPTR == prim? INTPP:\\n\\t\\tCHARPTR == prim? CHARPP: VOIDPP;\\n}\\n\\n\\n\\nstatic int declarator(int pmtr, int scls, char *name, int *pprim, int *psize,\\n\\t\\t\\tint *pval, int *pinit)\\n{\\n\\tint\\ttype = TVARIABLE;\\n\\tint\\tptrptr = 0;\\n\\n\\tif (STAR == Token) {\\n\\t\\tToken = scan();\\n\\t\\t*pprim = pointerto(*pprim);\\n\\t\\tif (STAR == Token) {\\n\\t\\t\\tToken = scan();\\n\\t\\t\\t*pprim = pointerto(*pprim);\\n\\t\\t\\tptrptr = 1;\\n\\t\\t}\\n\\t}\\n\\telse if (LPAREN == Token) {\\n\\t\\tif (*pprim != PINT)\\n\\t\\t\\terror(\\\"function pointers are limited to type \\\'int\\\'\\\",\\n\\t\\t\\t\\tNULL);\\n\\t\\tToken = scan();\\n\\t\\t*pprim = FUNPTR;\\n\\t\\tmatch(STAR, \\\"(*name)()\\\");\\n\\t}\\n\\tif (IDENT != Token) {\\n\\t\\terror(\\\"missing identifier at: %s\\\", Text);\\n\\t\\tname[0] = 0;\\n\\t}\\n\\telse {\\n\\t\\tcopyname(name, Text);\\n\\t\\tToken = scan();\\n\\t}\\n\\tif (FUNPTR == *pprim) {\\n\\t\\trparen();\\n\\t\\tlparen();\\n\\t\\trparen();\\n\\t}\\n\\tif (!pmtr && ASSIGN == Token) {\\n\\t\\tToken = scan();\\n\\t\\t*pval = constexpr();\\n\\t\\tif (PCHAR == *pprim)\\n\\t\\t\\t*pval &= 0xff;\\n\\t\\tif (*pval && !inttype(*pprim))\\n\\t\\t\\terror(\\\"non-zero pointer initialization\\\", NULL);\\n\\t\\t*pinit = 1;\\n\\t}\\n\\telse if (!pmtr && LPAREN == Token) {\\n\\t\\tToken = scan();\\n\\t\\t*psize = pmtrdecls();\\n\\t\\trparen();\\n\\t\\treturn TFUNCTION;\\n\\t}\\n\\telse if (LBRACK == Token) {\\n\\t\\tif (ptrptr)\\n\\t\\t\\terror(\\\"too many levels of indirection: %s\\\", name);\\n\\t\\tToken = scan();\\n\\t\\tif (RBRACK == Token) {\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tif (pmtr) {\\n\\t\\t\\t\\t*pprim = pointerto(*pprim);\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\ttype = TARRAY;\\n\\t\\t\\t\\t*psize = 1;\\n\\t\\t\\t\\tif (ASSIGN == Token) {\\n\\t\\t\\t\\t\\tToken = scan();\\n\\t\\t\\t\\t\\tif (!inttype(*pprim))\\n\\t\\t\\t\\t\\t\\terror(\\\"initialization of\\\"\\n\\t\\t\\t\\t\\t\\t\\t\\\" pointer array not\\\"\\n\\t\\t\\t\\t\\t\\t\\t\\\" supported\\\",\\n\\t\\t\\t\\t\\t\\t\\tNULL);\\n\\t\\t\\t\\t\\t*psize = initlist(name, *pprim);\\n\\t\\t\\t\\t\\tif (CAUTO == scls)\\n\\t\\t\\t\\t\\t\\terror(\\\"initialization of\\\"\\n\\t\\t\\t\\t\\t\\t\\t\\\" local arrays\\\"\\n\\t\\t\\t\\t\\t\\t\\t\\\" not supported: %s\\\",\\n\\t\\t\\t\\t\\t\\t\\tname);\\n\\t\\t\\t\\t\\t*pinit = 1;\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\telse if (CEXTERN != scls) {\\n\\t\\t\\t\\t\\terror(\\\"automatically-sized array\\\"\\n\\t\\t\\t\\t\\t\\t\\\" lacking initialization: %s\\\",\\n\\t\\t\\t\\t\\t\\tname);\\n\\t\\t\\t\\t}\\n\\t\\t\\t}\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\t*psize = constexpr();\\n\\t\\t\\tif (*psize < 0) {\\n\\t\\t\\t\\terror(\\\"invalid array size\\\", NULL);\\n\\t\\t\\t\\t*psize = 0;\\n\\t\\t\\t}\\n\\t\\t\\ttype = TARRAY;\\n\\t\\t\\trbrack();\\n\\t\\t}\\n\\t}\\n\\tif (PVOID == *pprim)\\n\\t\\terror(\\\"\\\'void\\\' is not a valid type: %s\\\", name);\\n\\treturn type;\\n}\\n\\n\\n\\nstatic int localdecls(void) {\\n\\tchar\\tname[NAMELEN+1];\\n\\tint\\tprim, type, size, addr = 0, val, ini;\\n\\tint\\tstat, extn;\\n\\tint\\tpbase, rsize;\\n\\n\\tNli = 0;\\n\\twhile ( AUTO == Token || EXTERN == Token || REGISTER == Token ||\\n\\t\\tSTATIC == Token || VOLATILE == Token ||\\n\\t\\tINT == Token || CHAR == Token || VOID == Token ||\\n\\t\\tENUM == Token ||\\n\\t\\tSTRUCT == Token || UNION == Token\\n\\t) {\\n\\t\\tif (ENUM == Token) {\\n\\t\\t\\tenumdecl(0);\\n\\t\\t\\tcontinue;\\n\\t\\t}\\n\\t\\textn = stat = 0;\\n\\t\\tif (AUTO == Token || REGISTER == Token || STATIC == Token ||\\n\\t\\t\\tVOLATILE == Token || EXTERN == Token\\n\\t\\t) {\\n\\t\\t\\tstat = STATIC == Token;\\n\\t\\t\\textn = EXTERN == Token;\\n\\t\\t\\tToken = scan();\\n\\t\\t\\tif (\\tINT == Token || CHAR == Token ||\\n\\t\\t\\t\\tVOID == Token ||\\n\\t\\t\\t\\tSTRUCT == Token || UNION == Token\\n\\t\\t\\t) {\\n\\t\\t\\t\\tprim = primtype(Token, NULL);\\n\\t\\t\\t\\tToken = scan();\\n\\t\\t\\t}\\n\\t\\t\\telse\\n\\t\\t\\t\\tprim = PINT;\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\tprim = primtype(Token, NULL);\\n\\t\\t\\tToken = scan();\\n\\t\\t}\\n\\t\\tpbase = prim;\\n\\t\\tfor (;;) {\\n\\t\\t\\tprim = pbase;\\n\\t\\t\\tif (eofcheck()) return 0;\\n\\t\\t\\tsize = 1;\\n\\t\\t\\tini = val = 0;\\n\\t\\t\\ttype = declarator(0, CAUTO, name, &prim, &size,\\n\\t\\t\\t\\t\\t&val, &ini);\\n\\t\\t\\trsize = objsize(prim, type, size);\\n\\t\\t\\trsize = (rsize + INTSIZE-1) / INTSIZE * INTSIZE;\\n\\t\\t\\tif (stat) {\\n\\t\\t\\t\\taddloc(name, prim, type, CLSTATC, size,\\n\\t\\t\\t\\t\\tlabel(), val);\\n\\t\\t\\t}\\n\\t\\t\\telse if (extn) {\\n\\t\\t\\t\\taddloc(name, prim, type, CEXTERN, size,\\n\\t\\t\\t\\t\\t0, val);\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\taddr -= rsize;\\n\\t\\t\\t\\taddloc(name, prim, type, CAUTO, size, addr, 0);\\n\\t\\t\\t}\\n\\t\\t\\tif (ini && !stat) {\\n\\t\\t\\t\\tif (Nli >= MAXLOCINIT) {\\n\\t\\t\\t\\t\\terror(\\\"too many local initializers\\\",\\n\\t\\t\\t\\t\\t\\tNULL);\\n\\t\\t\\t\\t\\tNli = 0;\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\tLIaddr[Nli] = addr;\\n\\t\\t\\t\\tLIval[Nli++] = val;\\n\\t\\t\\t}\\n\\t\\t\\tif (COMMA == Token)\\n\\t\\t\\t\\tToken = scan();\\n\\t\\t\\telse\\n\\t\\t\\t\\tbreak;\\n\\t\\t}\\n\\t\\tsemi();\\n\\t}\\n\\treturn addr;\\n}\\n\\nstatic int intcmp(int *x1, int *x2) {\\n\\twhile (*x1 && *x1 == *x2)\\n\\t\\tx1++, x2++;\\n\\treturn *x1 - *x2;\\n}\\n\\nstatic void signature(int fn, int from, int to) {\\n\\tint\\ttypes[MAXFNARGS+1], i;\\n\\n\\tif (to - from > MAXFNARGS)\\n\\t\\terror(\\\"too many function parameters\\\", Names[fn]);\\n\\tfor (i=0; i<MAXFNARGS && from < to; i++)\\n\\t\\ttypes[i] = Prims[--to];\\n\\ttypes[i] = 0;\\n\\tif (NULL == Mtext[fn]) {\\n\\t\\tMtext[fn] = galloc((i+1) * sizeof(int), 1);\\n\\t\\tmemcpy(Mtext[fn], types, (i+1) * sizeof(int));\\n\\t}\\n\\telse if (intcmp((int *) Mtext[fn], types))\\n\\t\\terror(\\\"declaration does not match prior prototype: %s\\\",\\n\\t\\t\\tNames[fn]);\\n}\\n\\n\\n\\nvoid decl(int clss, int prim) {\\n\\tchar\\tname[NAMELEN+1];\\n\\tint\\tpbase, type, size = 0, val, init;\\n\\tint\\tlsize;\\n\\n\\tpbase = prim;\\n\\tfor (;;) {\\n\\t\\tprim = pbase;\\n\\t\\tval = 0;\\n\\t\\tinit = 0;\\n\\t\\ttype = declarator(0, clss, name, &prim, &size, &val, &init);\\n\\t\\tif (TFUNCTION == type) {\\n\\t\\t\\tclss = clss == CSTATIC? CSPROTO: CEXTERN;\\n\\t\\t\\tThisfn = addglob(name, prim, type, clss, size, 0,\\n\\t\\t\\t\\t\\tNULL, 0);\\n\\t\\t\\tsignature(Thisfn, Locs, NSYMBOLS);\\n\\t\\t\\tif (LBRACE == Token) {\\n\\t\\t\\t\\tclss = clss == CSPROTO? CSTATIC:\\n\\t\\t\\t\\t\\tclss == CEXTERN? CPUBLIC: clss;\\n\\t\\t\\t\\tThisfn = addglob(name, prim, type, clss, size,\\n\\t\\t\\t\\t\\t0, NULL, 0);\\n\\t\\t\\t\\tToken = scan();\\n\\t\\t\\t\\tlsize = localdecls();\\n\\t\\t\\t\\tgentext();\\n\\t\\t\\t\\tif (CPUBLIC == clss) genpublic(name);\\n\\t\\t\\t\\tgenaligntext();\\n\\t\\t\\t\\tgenname(name);\\n\\t\\t\\t\\tgenentry();\\n\\t\\t\\t\\tgenstack(lsize);\\n\\t\\t\\t\\tgenlocinit();\\n\\t\\t\\t\\tRetlab = label();\\n\\t\\t\\t\\tcompound(0);\\n\\t\\t\\t\\tgenlab(Retlab);\\n\\t\\t\\t\\tgenstack(-lsize);\\n\\t\\t\\t\\tgenexit();\\n\\t\\t\\t\\tif (O_debug & D_LSYM)\\n\\t\\t\\t\\t\\tdumpsyms(\\\"LOCALS: \\\", name, Locs,\\n\\t\\t\\t\\t\\t\\tNSYMBOLS);\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tsemi();\\n\\t\\t\\t}\\n\\t\\t\\tclrlocs();\\n\\t\\t\\treturn;\\n\\t\\t}\\n\\t\\tif (CEXTERN == clss && init) {\\n\\t\\t\\terror(\\\"initialization of \\\'extern\\\': %s\\\", name);\\n\\t\\t}\\n\\t\\taddglob(name, prim, type, clss, size, val, NULL, init);\\n\\t\\tif (COMMA == Token)\\n\\t\\t\\tToken = scan();\\n\\t\\telse\\n\\t\\t\\tbreak;\\n\\t}\\n\\tsemi();\\n}\\n\\n\\n\\nvoid structdecl(int clss, int uniondecl) {\\n\\tint\\tbase, prim, size, dummy, type, addr = 0;\\n\\tchar\\tname[NAMELEN+1], sname[NAMELEN+1];\\n\\tint\\ty, usize = 0;\\n\\n\\tToken = scan();\\n\\tcopyname(sname, Text);\\n\\tident();\\n\\tif (Token != LBRACE) {\\n\\t\\tprim = primtype(uniondecl? UNION: STRUCT, sname);\\n\\t\\tdecl(clss, prim);\\n\\t\\treturn;\\n\\t}\\n\\ty = addglob(sname, uniondecl? PUNION: PSTRUCT, TSTRUCT,\\n\\t\\t\\tCMEMBER, 0, 0, NULL, 0);\\n\\tToken = scan();\\n\\twhile (\\tINT == Token || CHAR == Token || VOID == Token ||\\n\\t\\tSTRUCT == Token || UNION == Token\\n\\t) {\\n\\t\\tbase = primtype(Token, NULL);\\n\\t\\tsize = 0;\\n\\t\\tToken = scan();\\n\\t\\tfor (;;) {\\n\\t\\t\\tif (eofcheck()) return;\\n\\t\\t\\tprim = base;\\n\\t\\t\\ttype = declarator(1, clss, name, &prim, &size,\\n\\t\\t\\t\\t\\t\\t&dummy, &dummy);\\n\\t\\t\\taddglob(name, prim, type, CMEMBER, size, addr,\\n\\t\\t\\t\\tNULL, 0);\\n\\t\\t\\tsize = objsize(prim, type, size);\\n\\t\\t\\tif (size < 0)\\n\\t\\t\\t\\terror(\\\"size of struct/union member\\\"\\n\\t\\t\\t\\t\\t\\\" is unknown: %s\\\",\\n\\t\\t\\t\\t\\tname);\\n\\t\\t\\tif (uniondecl) {\\n\\t\\t\\t\\tusize = size > usize? size: usize;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\taddr += size;\\n\\t\\t\\t\\taddr = (addr + INTSIZE-1) / INTSIZE * INTSIZE;\\n\\t\\t\\t}\\n\\t\\t\\tif (Token != COMMA) break;\\n\\t\\t\\tToken = scan();\\n\\t\\t}\\n\\t\\tsemi();\\n\\t}\\n\\trbrace();\\n\\tsemi();\\n\\tSizes[y] = uniondecl? usize: addr;\\n}\\n\\n\\n\\nvoid top(void) {\\t\\n\\t\\n\\tint\\tprim, clss = CPUBLIC;\\n\\t\\n\\t\\n\\t\\n\\n\\tswitch (Token) {\\n\\tcase EXTERN:\\tclss = CEXTERN; Token = scan(); break;\\n\\tcase STATIC:\\tclss = CSTATIC; Token = scan(); break;\\n\\tcase VOLATILE:\\tToken = scan(); break;\\n\\t}\\n\\tswitch (Token) {\\n\\tcase ENUM:\\n\\t\\tenumdecl(1);\\n\\t\\tbreak;\\n\\tcase STRUCT:\\n\\tcase UNION:\\n\\t\\tstructdecl(clss, UNION == Token);\\n\\t\\tbreak;\\n\\tcase CHAR:\\n\\tcase INT:\\n\\tcase VOID:\\n\\t\\tprim = primtype(Token, NULL);\\n\\t\\tToken = scan();\\n\\t\\tdecl(clss, prim);\\n\\t\\tbreak;\\n\\tcase IDENT:\\n\\t\\tdecl(clss, PINT);\\n\\t\\tbreak;\\n\\tdefault:\\n\\t\\terror(\\\"type specifier expected at: %s\\\", Text);\\n\\t\\tToken = synch(SEMI);\\n\\t\\tbreak;\\n\\t}\\n}\\n\\nstatic void stats(void) {\\n\\tprintf(\\t\\\"Memory usage: \\\"\\n\\t\\t\\\"Symbols: %5d/%5d, \\\"\\n\\t\\t\\\"Names: %5d/%5d, \\\"\\n\\t\\t\\\"Nodes: %5d/%5d\\\\n\\\",\\n\\t\\tGlobs, NSYMBOLS,\\n\\t\\tNbot, POOLSIZE,\\n\\t\\tNdmax, NODEPOOLSZ);\\n}\\n\\nvoid defarg(char *s) {\\n\\tchar\\t*p;\\n\\n\\tif (NULL == s) return;\\n\\tif ((p = strchr(s, \\\'=\\\')) != NULL)\\n\\t\\t*p++ = 0;\\n\\telse\\n\\t\\tp = \\\"\\\";\\n\\taddglob(s, 0, TMACRO, 0, 0, 0, globname(p), 0);\\n\\tif (*p) *--p = \\\'=\\\';\\n}\\n\\n\\nvoid program(char *name, FILE *in, FILE *out, char *def, char *source) {\\n\\n\\tprintf(\\\"------------------program()-------------------------file name: \\\'%s\\\'\\\\n\\\",name);\\n\\tinit();\\n\\tdefarg(def);\\n\\t\\n\\tInsource = source;\\n\\tInsourcelen = strlen(Insource);\\n\\tInsourcei = 0;\\t\\t\\t\\n\\t\\n\\t\\n\\tInfile = in;\\n\\tOutfile = out;\\n\\tFile = Basefile = name;\\n\\tgenprelude();\\n\\tToken = scan();\\n\\t\\n\\twhile (XEOF != Token) {\\n\\t\\ttop();\\n\\t}\\n\\tgenpostlude();\\n\\tif (O_debug & D_GSYM) dumpsyms(\\\"GLOBALS\\\", \\\"\\\", 1, Globs);\\n\\tif (O_debug & D_STAT) stats();\\n}\\n\");\n\t\t\n\t}\n\tif(!strcmp( file, \"scan.c\" )) {\n\t\tstrcpy(source, \"\\n\\n#include \\\"defs.h\\\"\\n#include \\\"data.h\\\"\\n#include \\\"decl.h\\\"\\n\\n\\n\\n\\nchar strmanager() {\\t\\n\\tchar c;\\n\\tif(Insourcei == Insourcelen)\\t\\n\\t\\treturn -1;\\n\\telse {\\n\\t\\tc = Insource[Insourcei];\\n\\t\\t++Insourcei;\\n\\t\\treturn c;\\n\\t}\\n}\\n\\n\\nint next(void) {\\n\\tint\\tc;\\n\\n\\tif (Putback) {\\n\\t\\tc = Putback;\\n\\t\\tPutback = 0;\\n\\t\\treturn c;\\n\\t}\\n\\tif (Mp) {\\n\\t\\tif (\\\'\\\\0\\\' == *Macp[Mp-1]) {\\n\\t\\t\\tMacp[Mp-1] = NULL;\\n\\t\\t\\treturn Macc[--Mp];\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\treturn *Macp[Mp-1]++;\\n\\t\\t}\\n\\t}\\n\\t\\n\\t\\n\\tc = strmanager();\\n\\t\\n\\t\\n\\t\\n\\tif (\\\'\\\\n\\\' == c) Line++;\\n\\treturn c;\\n}\\n\\nvoid putback(int c) {\\n\\tPutback = c;\\n}\\n\\nstatic int hexchar(void) {\\n\\tint\\tc, h, n = 0, f = 0;\\n\\n\\twhile (isxdigit(c = next())) {\\n\\t\\th = chrpos(\\\"0123456789abcdef\\\", tolower(c));\\n\\t\\tn = n * 16 + h;\\n\\t\\tf = 1;\\n\\t}\\n\\tputback(c);\\n\\tif (!f)\\n\\t\\terror(\\\"missing digits after \\\'\\\\\\\\x\\\'\\\", NULL);\\n\\tif (n > 255)\\n\\t\\terror(\\\"value out of range after \\\'\\\\\\\\x\\\'\\\", NULL);\\n\\treturn n;\\n}\\n\\nstatic int scanch(void) {\\n\\tint\\ti, c, c2;\\n\\n\\tc = next();\\n\\tif (\\\'\\\\\\\\\\\' == c) {\\n\\t\\tswitch (c = next()) {\\n\\t\\tcase \\\'a\\\': return \\\'\\\\a\\\';\\n\\t\\tcase \\\'b\\\': return \\\'\\\\b\\\';\\n\\t\\tcase \\\'f\\\': return \\\'\\\\f\\\';\\n\\t\\tcase \\\'n\\\': return \\\'\\\\n\\\';\\n\\t\\tcase \\\'r\\\': return \\\'\\\\r\\\';\\n\\t\\tcase \\\'t\\\': return \\\'\\\\t\\\';\\n\\t\\tcase \\\'v\\\': return \\\'\\\\v\\\';\\n\\t\\tcase \\\'\\\\\\\\\\\': return \\\'\\\\\\\\\\\';\\n\\t\\tcase \\\'\\\"\\\': return \\\'\\\"\\\' | 256;\\n\\t\\tcase \\\'\\\\\\\'\\\': return \\\'\\\\\\\'\\\';\\n\\t\\tcase \\\'0\\\': case \\\'1\\\': case \\\'2\\\':\\n\\t\\tcase \\\'3\\\': case \\\'4\\\': case \\\'5\\\':\\n\\t\\tcase \\\'6\\\': case \\\'7\\\':\\n\\t\\t\\tfor (i = c2 = 0; isdigit(c) && c < \\\'8\\\'; c = next()) {\\n\\t\\t\\t\\tif (++i > 3) break;\\n\\t\\t\\t\\tc2 = c2 * 8 + (c - \\\'0\\\');\\n\\t\\t\\t}\\n\\t\\t\\tputback(c);\\n\\t\\t\\treturn c2;\\n\\t\\tcase \\\'x\\\':\\n\\t\\t\\treturn hexchar();\\n\\t\\tdefault:\\n\\t\\t\\tscnerror(\\\"unknown escape sequence: %s\\\", c);\\n\\t\\t\\treturn \\\' \\\';\\n\\t\\t}\\n\\t}\\n\\telse {\\n\\t\\treturn c;\\n\\t}\\n}\\n\\nstatic int scanint(int c) {\\n\\tint\\tval, radix, k, i = 0;\\n\\n\\tval = 0;\\n\\tradix = 10;\\n\\tif (\\\'0\\\' == c) {\\n\\t\\tText[i++] = \\\'0\\\';\\n\\t\\tif ((c = next()) == \\\'x\\\') {\\n\\t\\t\\tradix = 16;\\n\\t\\t\\tText[i++] = c;\\n\\t\\t\\tc = next();\\n\\t\\t}\\n\\t\\telse {\\n\\t\\t\\tradix = 8;\\n\\t\\t}\\n\\t}\\n\\twhile ((k = chrpos(\\\"0123456789abcdef\\\", tolower(c))) >= 0) {\\n\\t\\tText[i++] = c;\\n\\t\\tif (k >= radix)\\n\\t\\t\\tscnerror(\\\"invalid digit in integer literal: %s\\\", c);\\n\\t\\tval = val * radix + k;\\n\\t\\tc = next();\\n\\t}\\n\\tputback(c);\\n\\tText[i] = 0;\\n\\treturn val;\\n}\\n\\nstatic int scanstr(char *buf) {\\n\\tint\\ti, c;\\n\\n\\tbuf[0] = \\\'\\\"\\\';\\n\\tfor (i=1; i<TEXTLEN-2; i++) {\\n\\t\\tif ((c = scanch()) == \\\'\\\"\\\') {\\n\\t\\t\\tbuf[i++] = \\\'\\\"\\\';\\n\\t\\t\\tbuf[i] = 0;\\n\\t\\t\\treturn Value = i;\\n\\t\\t}\\n\\t\\tbuf[i] = c;\\n\\t}\\n\\tfatal(\\\"string literal too long\\\");\\n\\treturn 0;\\n}\\n\\nstatic int scanident(int c, char *buf, int lim) {\\n\\tint\\ti = 0;\\n\\n\\twhile (isalpha(c) || isdigit(c) || \\\'_\\\' == c) {\\n\\t\\tif (lim-1 == i) {\\n\\t\\t\\terror(\\\"identifier too long\\\", NULL);\\n\\t\\t\\ti++;\\n\\t\\t}\\n\\t\\telse if (i < lim-1) {\\n\\t\\t\\tbuf[i++] = c;\\n\\t\\t}\\n\\t\\tc = next();\\n\\t}\\n\\tputback(c);\\n\\tbuf[i] = 0;\\n\\treturn i;\\n}\\n\\nint skip(void) {\\n\\tint\\tc, p, nl;\\n\\n\\tc = next();\\n\\tnl = 0;\\n\\tfor (;;) {\\n\\t\\tif (EOF == c) {\\n\\t\\t\\tstrcpy(Text, \\\"<EOF>\\\");\\n\\t\\t\\treturn EOF;\\n\\t\\t}\\n\\t\\twhile (\\\' \\\' == c || \\\'\\\\t\\\' == c || \\\'\\\\n\\\' == c ||\\n\\t\\t\\t\\\'\\\\r\\\' == c || \\\'\\\\f\\\' == c\\n\\t\\t) {\\n\\t\\t\\tif (\\\'\\\\n\\\' == c) nl = 1;\\n\\t\\t\\tc = next();\\n\\t\\t}\\n\\t\\tif (nl && c == \\\'#\\\') {\\n\\t\\t\\tpreproc();\\n\\t\\t\\tc = next();\\n\\t\\t\\tcontinue;\\n\\t\\t}\\n\\t\\tnl = 0;\\n\\t\\tif (c != \\\'/\\\')\\n\\t\\t\\tbreak;\\n\\t\\tc = next();\\n\\t\\tif (c != \\\'*\\\' && c != \\\'/\\\') {\\n\\t\\t\\tputback(c);\\n\\t\\t\\tc = \\\'/\\\';\\n\\t\\t\\tbreak;\\n\\t\\t}\\n\\t\\tif (c == \\\'/\\\') {\\n\\t\\t\\twhile ((c = next()) != EOF) {\\n\\t\\t\\t\\tif (c == \\\'\\\\n\\\') break;\\n\\t\\t\\t}\\n                }\\n                else {\\n\\t\\t\\tp = 0;\\n\\t\\t\\twhile ((c = next()) != EOF) {\\n\\t\\t\\t\\tif (\\\'/\\\' == c && \\\'*\\\' == p) {\\n\\t\\t\\t\\t\\tc = next();\\n\\t\\t\\t\\t\\tbreak;\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\tp = c;\\n\\t\\t\\t}\\n\\t\\t}\\n\\t}\\n\\treturn c;\\n}\\n\\nstatic int keyword(char *s) {\\n\\tswitch (*s) {\\n\\tcase \\\'#\\\':\\n\\t\\tswitch (s[1]) {\\n\\t\\tcase \\\'d\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#define\\\")) return P_DEFINE;\\n\\t\\t\\tbreak;\\n\\t\\tcase \\\'e\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#else\\\")) return P_ELSE;\\n\\t\\t\\tif (!strcmp(s, \\\"#endif\\\")) return P_ENDIF;\\n\\t\\t\\tif (!strcmp(s, \\\"#error\\\")) return P_ERROR;\\n\\t\\t\\tbreak;\\n\\t\\tcase \\\'i\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#ifdef\\\")) return P_IFDEF;\\n\\t\\t\\tif (!strcmp(s, \\\"#ifndef\\\")) return P_IFNDEF;\\n\\t\\t\\tif (!strcmp(s, \\\"#include\\\")) return P_INCLUDE;\\n\\t\\t\\tbreak;\\n\\t\\tcase \\\'l\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#line\\\")) return P_LINE;\\n\\t\\t\\tbreak;\\n\\t\\tcase \\\'p\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#pragma\\\")) return P_PRAGMA;\\n\\t\\t\\tbreak;\\n\\t\\tcase \\\'u\\\':\\n\\t\\t\\tif (!strcmp(s, \\\"#undef\\\")) return P_UNDEF;\\n\\t\\t\\tbreak;\\n\\t\\t}\\n\\t\\tbreak;\\n\\tcase \\\'a\\\':\\n\\t\\tif (!strcmp(s, \\\"auto\\\")) return AUTO;\\n\\t\\tbreak;\\n\\tcase \\\'b\\\':\\n\\t\\tif (!strcmp(s, \\\"break\\\")) return BREAK;\\n\\t\\tbreak;\\n\\tcase \\\'c\\\':\\n\\t\\tif (!strcmp(s, \\\"case\\\")) return CASE;\\n\\t\\tif (!strcmp(s, \\\"char\\\")) return CHAR;\\n\\t\\tif (!strcmp(s, \\\"continue\\\")) return CONTINUE;\\n\\t\\tbreak;\\n\\tcase \\\'d\\\':\\n\\t\\tif (!strcmp(s, \\\"default\\\")) return DEFAULT;\\n\\t\\tif (!strcmp(s, \\\"do\\\")) return DO;\\n\\t\\tbreak;\\n\\tcase \\\'e\\\':\\n\\t\\tif (!strcmp(s, \\\"else\\\")) return ELSE;\\n\\t\\tif (!strcmp(s, \\\"enum\\\")) return ENUM;\\n\\t\\tif (!strcmp(s, \\\"extern\\\")) return EXTERN;\\n\\t\\tbreak;\\n\\tcase \\\'f\\\':\\n\\t\\tif (!strcmp(s, \\\"for\\\")) return FOR;\\n\\t\\tbreak;\\n\\tcase \\\'i\\\':\\n\\t\\tif (!strcmp(s, \\\"if\\\")) return IF;\\n\\t\\tif (!strcmp(s, \\\"int\\\")) return INT;\\n\\t\\tbreak;\\n\\tcase \\\'r\\\':\\n\\t\\tif (!strcmp(s, \\\"register\\\")) return REGISTER;\\n\\t\\tif (!strcmp(s, \\\"return\\\")) return RETURN;\\n\\t\\tbreak;\\n\\tcase \\\'s\\\':\\n\\t\\tif (!strcmp(s, \\\"sizeof\\\")) return SIZEOF;\\n\\t\\tif (!strcmp(s, \\\"static\\\")) return STATIC;\\n\\t\\tif (!strcmp(s, \\\"struct\\\")) return STRUCT;\\n\\t\\tif (!strcmp(s, \\\"switch\\\")) return SWITCH;\\n\\t\\tbreak;\\n\\tcase \\\'u\\\':\\n\\t\\tif (!strcmp(s, \\\"union\\\")) return UNION;\\n\\t\\tbreak;\\n\\tcase \\\'v\\\':\\n\\t\\tif (!strcmp(s, \\\"void\\\")) return VOID;\\n\\t\\tif (!strcmp(s, \\\"volatile\\\")) return VOLATILE;\\n\\t\\tbreak;\\n\\tcase \\\'w\\\':\\n\\t\\tif (!strcmp(s, \\\"while\\\")) return WHILE;\\n\\t\\tbreak;\\n\\t}\\n\\treturn 0;\\n}\\n\\nstatic int macro(char *name) {\\n\\tint\\ty;\\n\\n\\ty = findmac(name);\\n\\tif (!y || Types[y] != TMACRO)\\n\\t\\treturn 0;\\n\\tplaymac(Mtext[y]);\\n\\treturn 1;\\n}\\n\\nstatic int scanpp(void) {\\n\\tint\\tc, t;\\n\\n\\tif (Rejected != -1) {\\n\\t\\tt = Rejected;\\n\\t\\tRejected = -1;\\n\\t\\tstrcpy(Text, Rejtext);\\n\\t\\tValue = Rejval;\\n\\t\\treturn t;\\n\\t}\\n\\tfor (;;) {\\n\\t\\tValue = 0;\\n\\t\\tc = skip();\\n\\t\\tmemset(Text, 0, 4);\\n\\t\\tText[0] = c;\\n\\t\\tswitch (c) {\\n\\t\\tcase \\\'!\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn NOTEQ;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn XMARK;\\n\\t\\t\\t}\\n\\t\\tcase \\\'%\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASMOD;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn MOD;\\n\\t\\t\\t}\\n\\t\\tcase \\\'&\\\':\\n\\t\\t\\tif ((c = next()) == \\\'&\\\') {\\n\\t\\t\\t\\tText[1] = \\\'&\\\';\\n\\t\\t\\t\\treturn LOGAND;\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASAND;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn AMPER;\\n\\t\\t\\t}\\n\\t\\tcase \\\'(\\\':\\n\\t\\t\\treturn LPAREN;\\n\\t\\tcase \\\')\\\':\\n\\t\\t\\treturn RPAREN;\\n\\t\\tcase \\\'*\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASMUL;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn STAR;\\n\\t\\t\\t}\\n\\t\\tcase \\\'+\\\':\\n\\t\\t\\tif ((c = next()) == \\\'+\\\') {\\n\\t\\t\\t\\tText[1] = \\\'+\\\';\\n\\t\\t\\t\\treturn INCR;\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASPLUS;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn PLUS;\\n\\t\\t\\t}\\n\\t\\tcase \\\',\\\':\\n\\t\\t\\treturn COMMA;\\n\\t\\tcase \\\'-\\\':\\n\\t\\t\\tif ((c = next()) == \\\'-\\\') {\\n\\t\\t\\t\\tText[1] = \\\'-\\\';\\n\\t\\t\\t\\treturn DECR;\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASMINUS;\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'>\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'>\\\';\\n\\t\\t\\t\\treturn ARROW;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn MINUS;\\n\\t\\t\\t}\\n\\t\\tcase \\\'/\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASDIV;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn SLASH;\\n\\t\\t\\t}\\n\\t\\tcase \\\':\\\':\\n\\t\\t\\treturn COLON;\\n\\t\\tcase \\\';\\\':\\n\\t\\t\\treturn SEMI;\\n\\t\\tcase \\\'<\\\':\\n\\t\\t\\tif ((c = next()) == \\\'<\\\') {\\n\\t\\t\\t\\tText[1] = \\\'<\\\';\\n\\t\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\t\\tText[2] = \\\'=\\\';\\n\\t\\t\\t\\t\\treturn ASLSHIFT;\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\telse {\\n\\t\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\t\\treturn LSHIFT;\\n\\t\\t\\t\\t}\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn LTEQ;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn LESS;\\n\\t\\t\\t}\\n\\t\\tcase \\\'=\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn EQUAL;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn ASSIGN;\\n\\t\\t\\t}\\n\\t\\tcase \\\'>\\\':\\n\\t\\t\\tif ((c = next()) == \\\'>\\\') {\\n\\t\\t\\t\\tText[1] = \\\'>\\\';\\n\\t\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\t\\treturn ASRSHIFT;\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\telse {\\n\\t\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\t\\treturn RSHIFT;\\n\\t\\t\\t\\t}\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn GTEQ;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn GREATER;\\n\\t\\t\\t}\\n\\t\\tcase \\\'?\\\':\\n\\t\\t\\treturn QMARK;\\n\\t\\tcase \\\'[\\\':\\n\\t\\t\\treturn LBRACK;\\n\\t\\tcase \\\']\\\':\\n\\t\\t\\treturn RBRACK;\\n\\t\\tcase \\\'^\\\':\\n\\t\\t\\tif ((c = next()) == \\\'=\\\') {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASXOR;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn CARET;\\n\\t\\t\\t}\\n\\t\\tcase \\\'{\\\':\\n\\t\\t\\treturn LBRACE;\\n\\t\\tcase \\\'|\\\':\\n\\t\\t\\tif ((c = next()) == \\\'|\\\') {\\n\\t\\t\\t\\tText[1] = \\\'|\\\';\\n\\t\\t\\t\\treturn LOGOR;\\n\\t\\t\\t}\\n\\t\\t\\telse if (\\\'=\\\' == c) {\\n\\t\\t\\t\\tText[1] = \\\'=\\\';\\n\\t\\t\\t\\treturn ASOR;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\treturn PIPE;\\n\\t\\t\\t}\\n\\t\\tcase \\\'}\\\':\\n\\t\\t\\treturn RBRACE;\\n\\t\\tcase \\\'~\\\':\\n\\t\\t\\treturn TILDE;\\n\\t\\tcase EOF:\\n\\t\\t\\tstrcpy(Text, \\\"<EOF>\\\");\\n\\t\\t\\treturn XEOF;\\n\\t\\tcase \\\'\\\\\\\'\\\':\\n\\t\\t\\tText[1] = Value = scanch();\\n\\t\\t\\tif ((c = next()) != \\\'\\\\\\\'\\\')\\n\\t\\t\\t\\terror(\\n\\t\\t\\t\\t \\\"expected \\\'\\\\\\\\\\\'\\\' at end of char literal\\\",\\n\\t\\t\\t\\t\\tNULL);\\n\\t\\t\\tText[2] = \\\'\\\\\\\'\\\';\\n\\t\\t\\treturn INTLIT;\\n\\t\\tcase \\\'\\\"\\\':\\n\\t\\t\\tValue = scanstr(Text);\\n\\t\\t\\treturn STRLIT;\\n\\t\\tcase \\\'#\\\':\\n\\t\\t\\tText[0] = \\\'#\\\';\\n\\t\\t\\tscanident(next(), &Text[1], TEXTLEN-1);\\n\\t\\t\\tif ((t = keyword(Text)) != 0)\\n\\t\\t\\t\\treturn t;\\n\\t\\t\\terror(\\\"unknown preprocessor command: %s\\\", Text);\\n\\t\\t\\treturn IDENT;\\n\\t\\tcase \\\'.\\\':\\n\\t\\t\\tif ((c = next()) == \\\'.\\\') {\\n\\t\\t\\t\\tText[1] = Text[2] = \\\'.\\\';\\n\\t\\t\\t\\tText[3] = 0;\\n\\t\\t\\t\\tif ((c = next()) == \\\'.\\\')\\n\\t\\t\\t\\t\\treturn ELLIPSIS;\\n\\t\\t\\t\\tputback(c);\\n\\t\\t\\t\\terror(\\\"incomplete \\\'...\\\'\\\", NULL);\\n\\t\\t\\t\\treturn ELLIPSIS;\\n\\t\\t\\t}\\n\\t\\t\\tputback(c);\\n\\t\\t\\treturn DOT;\\n\\t\\tdefault:\\n\\t\\t\\tif (isdigit(c)) {\\n\\t\\t\\t\\tValue = scanint(c);\\n\\t\\t\\t\\treturn INTLIT;\\n\\t\\t\\t}\\n\\t\\t\\telse if (isalpha(c) || \\\'_\\\' == c) {\\n\\t\\t\\t\\tValue = scanident(c, Text, TEXTLEN);\\n\\t\\t\\t\\tif (Expandmac && macro(Text))\\n\\t\\t\\t\\t\\tbreak;\\n\\t\\t\\t\\tif ((t = keyword(Text)) != 0)\\n\\t\\t\\t\\t\\treturn t;\\n\\t\\t\\t\\treturn IDENT;\\n\\t\\t\\t}\\n\\t\\t\\telse {\\n\\t\\t\\t\\tscnerror(\\\"funny input character: %s\\\", c);\\n\\t\\t\\t\\tbreak;\\n\\t\\t\\t}\\n\\t\\t}\\n\\t}\\n}\\n\\nint scan(void) {\\n\\tint\\tt;\\n\\n\\tdo {\\n\\t\\tt = scanpp();\\n\\t\\tif (!Inclev && Isp && XEOF == t)\\n\\t\\t\\tfatal(\\\"missing \\\'#endif\\\'\\\");\\n\\t} while (frozen(1));\\n\\tif (t == Syntoken)\\n\\t\\tSyntoken = 0;\\n\\treturn t;\\n}\\n\\nint scanraw(void) {\\n\\tint\\tt, oisp;\\n\\n\\toisp = Isp;\\n\\tIsp = 0;\\n\\tExpandmac = 0;\\n\\tt = scan();\\n\\tExpandmac = 1;\\n\\tIsp = oisp;\\n\\treturn t;\\n}\\n\\nvoid reject(void) {\\n\\tRejected = Token;\\n\\tRejval = Value;\\n\\tstrcpy(Rejtext, Text);\\n}\\n\");\n\t}\n\tif(!strcmp( file, \"prep.c\" )) {\n\t\tstrcpy(source, \"\\n\\n#include \\\"defs.h\\\"\\n#include \\\"data.h\\\"\\n#include \\\"decl.h\\\"\\n\\nvoid playmac(char *s) {\\n\\tif (Mp >= MAXNMAC) fatal(\\\"too many nested macros\\\");\\n\\tMacc[Mp] = next();\\n\\tMacp[Mp++] = s;\\n}\\n\\nint getln(char *buf, int max) {\\t\\n\\tint\\tk;\\n\\t\\n\\tchar c;\\n\\tint i;\\n\\t\\n\\n\\t\\n\\t\\n\\t\\n\\tfor( i = 0; i < max - 1 ; ++i ) {\\n\\t\\tc = strmanager();\\n\\t\\tbuf[i] = c;\\n\\t\\tif( c == \\\'\\\\n\\\' || c == EOF ) {\\n\\t\\t\\t++i;\\t\\t\\t\\n\\t\\t\\tbreak;\\n\\t\\t}\\n\\t}\\n\\tbuf[i] = \\\'\\\\0\\\';\\n\\tif( strlen(buf) == 0 ) \\n\\t\\treturn 0;\\n\\t\\n\\t\\n\\t\\n\\tk = strlen(buf);\\n\\tif (k) buf[--k] = 0;\\t\\t\\t\\t\\t\\t\\n\\tif (k && \\\'\\\\r\\\' == buf[k-1]) buf[--k] = 0;\\t\\n\\treturn k;\\n}\\n\\nstatic void defmac(void) {\\n\\tchar\\tname[NAMELEN+1];\\n\\tchar\\tbuf[TEXTLEN+1], *p;\\n\\tint\\ty;\\n\\n\\tToken = scanraw();\\n\\tif (Token != IDENT)\\n\\t\\terror(\\\"identifier expected after \\\'#define\\\': %s\\\", Text);\\n\\tcopyname(name, Text);\\n\\tif (\\\'\\\\n\\\' == Putback)\\n\\t\\tbuf[0] = 0;\\n\\telse\\n\\t\\tgetln(buf, TEXTLEN-1);\\n\\tfor (p = buf; isspace(*p); p++)\\n\\t\\t;\\n\\tif ((y = findmac(name)) != 0) {\\n\\t\\tif (strcmp(Mtext[y], buf))\\n\\t\\t\\terror(\\\"macro redefinition: %s\\\", name);\\n\\t}\\n\\telse {\\n\\t\\taddglob(name, 0, TMACRO, 0, 0, 0, globname(p), 0);\\n\\t}\\n\\tLine++;\\n}\\n\\nstatic void undef(void) {\\n\\tchar\\tname[NAMELEN+1];\\n\\tint\\ty;\\n\\n\\tToken = scanraw();\\n\\tcopyname(name, Text);\\n\\tif (IDENT != Token)\\n\\t\\terror(\\\"identifier expected after \\\'#undef\\\': %s\\\", Text);\\n\\tif ((y = findmac(name)) != 0)\\n\\t\\tNames[y] = \\\"#undef\\\'d\\\";\\n}\\n\\nstatic void include(void) {\\n\\tchar\\tfile[TEXTLEN+1], path[TEXTLEN+1];\\n\\tint\\tc, k;\\n\\tFILE\\t*inc, *oinfile;\\n\\tchar\\t*ofile;\\n\\tint\\toc, oline;\\n\\t\\n\\tchar incsource[TEXTLEN]; \\n\\tFILE *inctmp;\\n\\tint tmpi;\\n\\tchar *main_src;\\n\\tint main_src_i, main_src_len;\\n\\t\\n\\n\\tif ((c = skip()) == \\\'<\\\')\\n\\t\\tc = \\\'>\\\';\\n\\tk = getln(file, TEXTLEN-strlen(SCCDIR)-9);\\t\\n\\tLine++;\\n\\tif (!k || file[k-1] != c)\\t\\t\\t\\t\\t\\n\\t\\terror(\\\"missing delimiter in \\\'#include\\\'\\\", NULL);\\n\\tif (k) file[k-1] = 0;\\t\\t\\t\\t\\t\\t\\n\\tif (c == \\\'\\\"\\\')\\n\\t\\tstrcpy(path, file);\\n\\telse {\\n\\t\\tstrcpy(path, SCCDIR);\\n\\t\\tstrcat(path, \\\"/include/\\\");\\n\\t\\tstrcat(path, file);\\n\\t}\\n\\tif ((inc = fopen(path, \\\"r\\\")) == NULL)\\n\\t\\terror(\\\"cannot open include file: \\\'%s\\\'\\\", path);\\n\\telse {\\n\\t\\t\\n\\t\\t\\n\\t\\t\\n\\t\\tinctmp = fopen(path,\\\"r\\\");\\n\\t\\t\\n\\t\\ttmpi = 0;\\n\\t\\twhile( (incsource[tmpi] = fgetc(inctmp)) != EOF ) {\\n\\t\\t\\t++tmpi;\\n\\t\\t}\\n\\t\\tincsource[tmpi] =\\\'\\\\0\\\';\\t\\n\\t\\tfclose(inctmp);\\n\\t\\t\\n\\t\\t\\n\\t\\t\\n\\t\\tif(!strcmp( file, \\\"data.h\\\" )) {\\n\\t\\t\\tstrcpy(incsource, \\\"\\\\n\\\\n#ifndef extern_\\\\n #define extern_ extern\\\\n#endif\\\\n\\\\n\\\\n\\\\nextern_ char *Insource;\\\\nextern_ int Insourcei;\\\\nextern_ int Insourcelen;\\\\n\\\\nextern_ FILE\\\\t*Infile;\\\\nextern_ FILE\\\\t*Outfile;\\\\nextern_ int\\\\tToken;\\\\nextern_ char\\\\tText[TEXTLEN+1];\\\\nextern_ int\\\\tValue;\\\\nextern_ int\\\\tLine;\\\\nextern_ int\\\\tErrors;\\\\nextern_ int\\\\tSyntoken;\\\\nextern_ int\\\\tPutback;\\\\nextern_ int\\\\tRejected;\\\\nextern_ int\\\\tRejval;\\\\nextern_ char\\\\tRejtext[TEXTLEN+1];\\\\nextern_ char\\\\t*File;\\\\nextern_ char\\\\t*Basefile;\\\\nextern_ char\\\\t*Macp[MAXNMAC];\\\\nextern_ int\\\\tMacc[MAXNMAC];\\\\nextern_ int\\\\tMp;\\\\nextern_ int\\\\tExpandmac;\\\\nextern_ int\\\\tIfdefstk[MAXIFDEF], Isp;\\\\nextern_ int\\\\tInclev;\\\\nextern_ int\\\\tTextseg;\\\\nextern_ int\\\\tNodes[NODEPOOLSZ];\\\\nextern_ int\\\\tNdtop;\\\\nextern_ int\\\\tNdmax;\\\\n\\\\n\\\\nextern_ char\\\\t*Names[NSYMBOLS];\\\\nextern_ int\\\\tPrims[NSYMBOLS];\\\\nextern_ char\\\\tTypes[NSYMBOLS];\\\\nextern_ char\\\\tStcls[NSYMBOLS];\\\\nextern_ int\\\\tSizes[NSYMBOLS];\\\\nextern_ int\\\\tVals[NSYMBOLS];\\\\nextern_ char\\\\t*Mtext[NSYMBOLS];\\\\nextern_ int\\\\tGlobs;\\\\nextern_ int\\\\tLocs;\\\\n\\\\nextern_ int\\\\tThisfn;\\\\n\\\\n\\\\nextern_ char\\\\tNlist[POOLSIZE];\\\\nextern_ int\\\\tNbot;\\\\nextern_ int\\\\tNtop;\\\\n\\\\n\\\\nextern_ int\\\\tBreakstk[MAXBREAK], Bsp;\\\\nextern_ int\\\\tContstk[MAXBREAK], Csp;\\\\nextern_ int\\\\tRetlab;\\\\n\\\\n\\\\nextern_ int\\\\tLIaddr[MAXLOCINIT];\\\\nextern_ int\\\\tLIval[MAXLOCINIT];\\\\nextern_ int\\\\tNli;\\\\n\\\\n\\\\nextern_ int\\\\tQ_type;\\\\nextern_ int\\\\tQ_val;\\\\nextern_ char\\\\tQ_name[NAMELEN+1];\\\\nextern_ int\\\\tQ_cmp;\\\\nextern_ int\\\\tQ_bool;\\\\n\\\\n\\\\nextern_ char\\\\t*Files[MAXFILES];\\\\nextern_ char\\\\tTemp[MAXFILES];\\\\nextern_ int\\\\tNf;\\\\n\\\\n\\\\nextern_ int\\\\tO_verbose;\\\\nextern_ int\\\\tO_componly;\\\\nextern_ int\\\\tO_asmonly;\\\\nextern_ int\\\\tO_testonly;\\\\nextern_ int\\\\tO_stdio;\\\\nextern_ char\\\\t*O_outfile;\\\\nextern_ int\\\\tO_debug;\\\\n\\\");\\n\\t\\t}\\n\\t\\tif(!strcmp( file, \\\"defs.h\\\" )) {\\n\\t\\t\\tstrcpy(incsource,\\\"\\\\n\\\\n#include <stdlib.h>\\\\n#include <stdio.h>\\\\n#include <string.h>\\\\n#include <ctype.h>\\\\n#include \\\\\\\"cg.h\\\\\\\"\\\\n#include \\\\\\\"sys.h\\\\\\\"\\\\n\\\\n#define VERSION\\\\t\\\\t\\\\\\\"2016-12-12\\\\\\\"\\\\n\\\\n#ifndef SCCDIR\\\\n #define SCCDIR\\\\t\\\\t\\\\\\\".\\\\\\\"\\\\n#endif\\\\n\\\\n#ifndef AOUTNAME\\\\n #define AOUTNAME\\\\t\\\\\\\"a.out\\\\\\\"\\\\n#endif\\\\n\\\\n#define SCCLIBC\\\\t\\\\t\\\\\\\"%s/lib/libscc.a\\\\\\\"\\\\n\\\\n#define PREFIX\\\\t\\\\t\\\\\\\'C\\\\\\\'\\\\n#define LPREFIX\\\\t\\\\t\\\\\\\'L\\\\\\\'\\\\n\\\\n#define INTSIZE\\\\t\\\\tBPW\\\\n#define PTRSIZE\\\\t\\\\tINTSIZE\\\\n#define CHARSIZE\\\\t1\\\\n\\\\n\\\\n\\\\n#define TEXTLEN\\\\t\\\\t200000\\\\n\\\\n#define NAMELEN\\\\t\\\\t16\\\\n\\\\n#define MAXFILES\\\\t32\\\\n\\\\n#define MAXIFDEF\\\\t16\\\\n#define MAXNMAC\\\\t\\\\t32\\\\n#define MAXCASE\\\\t\\\\t256\\\\n#define MAXBREAK\\\\t16\\\\n#define MAXLOCINIT\\\\t32\\\\n#define MAXFNARGS\\\\t32\\\\n\\\\n\\\\n#define NSYMBOLS\\\\t1024\\\\n#define POOLSIZE\\\\t16384\\\\n#define NODEPOOLSZ\\\\t4096\\\\t\\\\n\\\\n\\\\nenum {\\\\n\\\\tTVARIABLE = 1,\\\\n\\\\tTARRAY,\\\\n\\\\tTFUNCTION,\\\\n\\\\tTCONSTANT,\\\\n\\\\tTMACRO,\\\\n\\\\tTSTRUCT\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tPCHAR = 1,\\\\n\\\\tPINT,\\\\n\\\\tCHARPTR,\\\\n\\\\tINTPTR,\\\\n\\\\tCHARPP,\\\\n\\\\tINTPP,\\\\n\\\\tPVOID,\\\\n\\\\tVOIDPTR,\\\\n\\\\tVOIDPP,\\\\n\\\\tFUNPTR,\\\\n\\\\tPSTRUCT = 0x2000,\\\\n\\\\tPUNION  = 0x4000,\\\\n\\\\tSTCPTR  = 0x6000,\\\\n\\\\tSTCPP   = 0x8000,\\\\n\\\\tUNIPTR  = 0xA000,\\\\n\\\\tUNIPP   = 0xC000,\\\\n\\\\tSTCMASK = 0xE000\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tCPUBLIC = 1,\\\\n\\\\tCEXTERN,\\\\n\\\\tCSTATIC,\\\\n\\\\tCLSTATC,\\\\n\\\\tCAUTO,\\\\n\\\\tCSPROTO,\\\\n\\\\tCMEMBER,\\\\n\\\\tCSTCDEF\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tLVSYM,\\\\n\\\\tLVPRIM,\\\\n\\\\tLVADDR,\\\\n\\\\tLV\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tD_LSYM = 1,\\\\n\\\\tD_GSYM = 2,\\\\n\\\\tD_STAT = 4\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tempty,\\\\n\\\\taddr_auto,\\\\n\\\\taddr_static,\\\\n\\\\taddr_globl,\\\\n\\\\taddr_label,\\\\n\\\\tliteral,\\\\n\\\\tauto_byte,\\\\n\\\\tauto_word,\\\\n\\\\tstatic_byte,\\\\n\\\\tstatic_word,\\\\n\\\\tglobl_byte,\\\\n\\\\tglobl_word\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tcnone,\\\\n\\\\tequal,\\\\n\\\\tnot_equal,\\\\n\\\\tless,\\\\n\\\\tgreater,\\\\n\\\\tless_equal,\\\\n\\\\tgreater_equal,\\\\n\\\\tbelow,\\\\n\\\\tabove,\\\\n\\\\tbelow_equal,\\\\n\\\\tabove_equal\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tbnone,\\\\n\\\\tlognot,\\\\n\\\\tnormalize\\\\n};\\\\n\\\\n\\\\nstruct node_stc {\\\\n\\\\tint\\\\t\\\\top;\\\\n\\\\tstruct node_stc\\\\t*left, *right;\\\\n\\\\tint\\\\t\\\\targs[1];\\\\n};\\\\n\\\\n#define node\\\\tstruct node_stc\\\\n\\\\n\\\\nenum {\\\\n\\\\tSLASH, STAR, MOD, PLUS, MINUS, LSHIFT, RSHIFT,\\\\n\\\\tGREATER, GTEQ, LESS, LTEQ, EQUAL, NOTEQ, AMPER,\\\\n\\\\tCARET, PIPE, LOGAND, LOGOR,\\\\n\\\\n\\\\tARROW, ASAND, ASXOR, ASLSHIFT, ASMINUS, ASMOD, ASOR, ASPLUS,\\\\n\\\\tASRSHIFT, ASDIV, ASMUL, ASSIGN, AUTO, BREAK, CASE, CHAR, COLON,\\\\n\\\\tCOMMA, CONTINUE, DECR, DEFAULT, DO, DOT, ELLIPSIS, ELSE, ENUM,\\\\n\\\\tEXTERN, FOR, IDENT, IF, INCR, INT, INTLIT, LBRACE, LBRACK,\\\\n\\\\tLPAREN, NOT, QMARK, RBRACE, RBRACK, REGISTER, RETURN, RPAREN,\\\\n\\\\tSEMI, SIZEOF, STATIC, STRLIT, STRUCT, SWITCH, TILDE, UNION,\\\\n\\\\tVOID, VOLATILE, WHILE, XEOF, XMARK,\\\\n\\\\n\\\\tP_DEFINE, P_ELSE, P_ELSENOT, P_ENDIF, P_ERROR, P_IFDEF,\\\\n\\\\tP_IFNDEF, P_INCLUDE, P_LINE, P_PRAGMA, P_UNDEF\\\\n};\\\\n\\\\n\\\\nenum {\\\\n\\\\tOP_GLUE, OP_ADD, OP_ADDR, OP_ASSIGN, OP_BINAND, OP_BINIOR,\\\\n\\\\tOP_BINXOR, OP_BOOL, OP_BRFALSE, OP_BRTRUE, OP_CALL, OP_CALR,\\\\n\\\\tOP_COMMA, OP_DEC, OP_DIV, OP_EQUAL, OP_GREATER, OP_GTEQ,\\\\n\\\\tOP_IDENT, OP_IFELSE, OP_LAB, OP_LDLAB, OP_LESS, OP_LIT,\\\\n\\\\tOP_LOGNOT, OP_LSHIFT, OP_LTEQ, OP_MOD, OP_MUL, OP_NEG,\\\\n\\\\tOP_NOT, OP_NOTEQ, OP_PLUS, OP_PREDEC, OP_PREINC, OP_POSTDEC,\\\\n\\\\tOP_POSTINC, OP_RSHIFT, OP_RVAL, OP_SCALE, OP_SCALEBY, OP_SUB\\\\n};\\\\n\\\\n\\\");\\n\\t\\t}\\n\\t\\tif(!strcmp( file, \\\"decl.h\\\" )) {\\n\\t\\t\\tstrcpy(incsource,\\\"\\\\n\\\\nint\\\\taddglob(char *name, int prim, int type, int scls, int size, int val,\\\\n\\\\t\\\\tchar *mval, int init);\\\\nint\\\\taddloc(char *name, int prim, int type, int scls, int size, int val,\\\\n\\\\t\\\\tint init);\\\\nint\\\\tbinoptype(int op, int p1, int p2);\\\\nint\\\\tchrpos(char *s, int c);\\\\nvoid\\\\tclear(int q);\\\\nvoid\\\\tcleanup(void);\\\\nvoid\\\\tclrlocs(void);\\\\nvoid\\\\tcolon(void);\\\\nvoid\\\\tcommit(void);\\\\nvoid\\\\tcommit_bool(void);\\\\nvoid\\\\tcommit_cmp(void);\\\\nvoid\\\\tcompound(int lbr);\\\\nint\\\\tcomptype(int p);\\\\nint\\\\tconstexpr(void);\\\\nvoid\\\\tcopyname(char *name, char *s);\\\\nint\\\\tderef(int p);\\\\nvoid\\\\tdumpsyms(char *title, char *sub, int from, int to);\\\\nvoid\\\\tdumptree(node *a);\\\\nvoid\\\\temittree(node *a);\\\\nint\\\\teofcheck(void);\\\\nvoid\\\\terror(char *s, char *a);\\\\nvoid\\\\texpr(int *lv, int cvoid);\\\\nvoid\\\\tfatal(char *s);\\\\nint\\\\tfindglob(char *s);\\\\nint\\\\tfindloc(char *s);\\\\nint\\\\tfindmem(int y, char *s);\\\\nint\\\\tfindstruct(char *s);\\\\nint\\\\tfindsym(char *s);\\\\nint\\\\tfindmac(char *s);\\\\nnode\\\\t*fold_reduce(node *n);\\\\nint\\\\tfrozen(int depth);\\\\nchar\\\\t*galloc(int k, int align);\\\\nvoid\\\\tgen(char *s);\\\\nint\\\\tgenadd(int p1, int p2, int swap);\\\\nvoid\\\\tgenaddr(int y);\\\\nvoid\\\\tgenalign(int k);\\\\nvoid\\\\tgenaligntext(void);\\\\nvoid\\\\tgenand(void);\\\\nvoid\\\\tgenasop(int op, int *lv, int p2);\\\\nvoid\\\\tgenbool(void);\\\\nvoid\\\\tgenbrfalse(int dest);\\\\nvoid\\\\tgenbrtrue(int dest);\\\\nvoid\\\\tgenbss(char *name, int len, int statc);\\\\nvoid\\\\tgencall(int y);\\\\nvoid\\\\tgencalr(void);\\\\nvoid\\\\tgencmp(char *inst);\\\\nvoid\\\\tgendata(void);\\\\nvoid\\\\tgendefb(int v);\\\\nvoid\\\\tgendefp(int v);\\\\nvoid\\\\tgendefs(char *s, int len);\\\\nvoid\\\\tgendefw(int v);\\\\nvoid\\\\tgendiv(int swap);\\\\nvoid\\\\tgenentry(void);\\\\nvoid\\\\tgenexit(void);\\\\nvoid\\\\tgeninc(int *lv, int inc, int pre);\\\\nvoid\\\\tgenind(int p);\\\\nvoid\\\\tgenior(void);\\\\nvoid\\\\tgenjump(int dest);\\\\nvoid\\\\tgenlab(int id);\\\\nvoid\\\\tgenldlab(int id);\\\\nvoid\\\\tgenlit(int v);\\\\nvoid\\\\tgenln(char *s);\\\\nvoid\\\\tgenlocinit(void);\\\\nvoid\\\\tgenlognot(void);\\\\nvoid\\\\tgenmod(int swap);\\\\nvoid\\\\tgenmul(void);\\\\nvoid\\\\tgenname(char *name);\\\\nvoid\\\\tgenneg(void);\\\\nvoid\\\\tgennot(void);\\\\nvoid\\\\tgenpostlude(void);\\\\nvoid\\\\tgenprelude(void);\\\\nvoid\\\\tgenpublic(char *name);\\\\nvoid\\\\tgenpush(void);\\\\nvoid\\\\tgenpushlit(int n);\\\\nvoid\\\\tgenraw(char *s);\\\\nvoid\\\\tgenrval(int *lv);\\\\nvoid\\\\tgenscale(void);\\\\nvoid\\\\tgenscale2(void);\\\\nvoid\\\\tgenscaleby(int v);\\\\nvoid\\\\tgenshl(int swap);\\\\nvoid\\\\tgenshr(int swap);\\\\nvoid\\\\tgenstack(int n);\\\\nvoid\\\\tgenstore(int *lv);\\\\nint\\\\tgensub(int p1, int p2, int swap);\\\\nvoid\\\\tgenswitch(int *vals, int *labs, int nc, int dflt);\\\\nvoid\\\\tgentext(void);\\\\nvoid\\\\tgenxor(void);\\\\nchar\\\\t*globname(char *s);\\\\nchar\\\\t*gsym(char *s);\\\\nvoid\\\\tident(void);\\\\nvoid\\\\tinit(void);\\\\nvoid\\\\tinitopt(void);\\\\nint\\\\tinttype(int p);\\\\nint\\\\tlabel(void);\\\\nchar\\\\t*labname(int id);\\\\nvoid\\\\tlbrace(void);\\\\nvoid\\\\tlgen(char *s, char *inst, int n);\\\\nvoid\\\\tlgen2(char *s, int v1, int v2);\\\\nvoid\\\\tload(void);\\\\nvoid\\\\tlparen(void);\\\\nnode\\\\t*mkbinop(int op, node *left, node *right);\\\\nnode\\\\t*mkbinop1(int op, int n, node *left, node *right);\\\\nnode\\\\t*mkbinop2(int op, int n1, int n2, node *left, node *right);\\\\nnode\\\\t*mkleaf(int op, int n);\\\\nnode\\\\t*mkunop(int op, node *left);\\\\nnode\\\\t*mkunop1(int op, int n, node *left);\\\\nnode\\\\t*mkunop2(int op, int n1, int n2, node *left);\\\\nvoid\\\\tmatch(int t, char *what);\\\\nchar\\\\t*newfilename(char *name, int sfx);\\\\nint\\\\tnext(void);\\\\nvoid\\\\tngen(char *s, char *inst, int n);\\\\nvoid\\\\tngen2(char *s, char *inst, int n, int a);\\\\nvoid\\\\tnotvoid(int p);\\\\nint\\\\tobjsize(int prim, int type, int size);\\\\nnode\\\\t*optimize(node *n);\\\\nvoid\\\\topt_init(void);\\\\nvoid\\\\tplaymac(char *s);\\\\nint\\\\tpointerto(int prim);\\\\nvoid\\\\tpreproc(void);\\\\nint\\\\tprimtype(int t, char *s);\\\\n\\\\n\\\\nvoid\\\\tprogram(char *name, FILE *in, FILE *out, char *def, char *source);\\\\nchar \\\\tstrmanager();\\\\n\\\\nvoid\\\\tputback(int t);\\\\nvoid\\\\tqueue_cmp(int op);\\\\nvoid\\\\trbrace(void);\\\\nvoid\\\\trbrack(void);\\\\nvoid\\\\treject(void);\\\\nvoid\\\\trexpr(void);\\\\nvoid\\\\trparen(void);\\\\nint\\\\tscan(void);\\\\nint\\\\tscanraw(void);\\\\nvoid\\\\tscnerror(char *s, int c);\\\\nvoid\\\\tsemi(void);\\\\nvoid\\\\tsgen(char *s, char *inst, char *s2);\\\\nvoid\\\\tsgen2(char *s, char *inst, int v, char *s2);\\\\nint\\\\tskip(void);\\\\nvoid\\\\tspill(void);\\\\nint\\\\tsynch(int syn);\\\\nvoid\\\\ttop(void);\\\\nint\\\\ttypematch(int p1, int p2);\\\\n\\\");\\n\\t\\t}\\n\\t\\t\\n\\t\\tInclev++;\\n\\t\\toc = next();\\n\\t\\toline = Line;\\n\\t\\tofile = File;\\n\\t\\t\\n\\t\\t\\n\\t\\tmain_src = Insource;\\n\\t\\tmain_src_i = Insourcei;\\n\\t\\tmain_src_len = Insourcelen;\\n\\t\\tInsource = incsource;\\n\\t\\tInsourcei = 0;\\n\\t\\tInsourcelen = strlen(incsource);\\n\\t\\t\\n\\t\\t\\n\\t\\toinfile = Infile;\\n\\t\\tLine = 1;\\n\\t\\tputback(\\\'\\\\n\\\');\\n\\t\\tFile = path;\\n\\t\\tInfile = inc;\\n\\t\\tToken = scan();\\n\\t\\twhile (XEOF != Token)\\n\\t\\t\\ttop();\\n\\t\\tLine = oline;\\n\\t\\tFile = ofile;\\n\\t\\tInfile = oinfile;\\n\\t\\t\\n\\t\\t\\n\\t\\tInsource = main_src;\\n\\t\\tInsourcei = main_src_i;\\n\\t\\tInsourcelen = main_src_len;\\n\\t\\t\\n\\t\\t\\n\\t\\tfclose(inc);\\n\\t\\tputback(oc);\\n\\t\\tInclev--;\\n\\t}\\n}\\n\\nstatic void ifdef(int expect) {\\n\\tchar\\tname[NAMELEN+1];\\n\\n\\tif (Isp >= MAXIFDEF)\\n\\t\\tfatal(\\\"too many nested \\\'#ifdef\\\'s\\\");\\n\\tToken = scanraw();\\n\\tcopyname(name, Text);\\n\\tif (IDENT != Token)\\n\\t\\terror(\\\"identifier expected in \\\'#ifdef\\\'\\\", NULL);\\n\\tif (frozen(1))\\n\\t\\tIfdefstk[Isp++] = P_IFNDEF;\\n\\telse if ((findmac(name) != 0) == expect)\\n\\t\\tIfdefstk[Isp++] = P_IFDEF;\\n\\telse\\n\\t\\tIfdefstk[Isp++] = P_IFNDEF;\\n}\\n\\nstatic void p_else(void) {\\n\\tif (!Isp)\\n\\t\\terror(\\\"\\\'#else\\\' without matching \\\'#ifdef\\\'\\\", NULL);\\n\\telse if (frozen(2))\\n\\t\\t;\\n\\telse if (P_IFDEF == Ifdefstk[Isp-1])\\n\\t\\tIfdefstk[Isp-1] = P_ELSENOT;\\n\\telse if (P_IFNDEF == Ifdefstk[Isp-1])\\n\\t\\tIfdefstk[Isp-1] = P_ELSE;\\n\\telse\\n\\t\\terror(\\\"\\\'#else\\\' without matching \\\'#ifdef\\\'\\\", NULL);\\n\\t\\t\\n}\\n\\nstatic void endif(void) {\\n\\tif (!Isp)\\n\\t\\terror(\\\"\\\'#endif\\\' without matching \\\'#ifdef\\\'\\\", NULL);\\n\\telse\\n\\t\\tIsp--;\\n}\\n\\nstatic void pperror(void) {\\n\\tchar\\tbuf[TEXTLEN+1];\\n\\n\\tif (\\\'\\\\n\\\' == Putback)\\n\\t\\tbuf[0] = 0;\\n\\telse\\n\\t\\tgetln(buf, TEXTLEN-1);\\n\\terror(\\\"#error: %s\\\", buf);\\n\\texit(1);\\n}\\n\\nstatic void setline(void) {\\n\\tchar\\tbuf[TEXTLEN+1];\\n\\n\\tif (\\\'\\\\n\\\' == Putback)\\n\\t\\tbuf[0] = 0;\\n\\telse\\n\\t\\tgetln(buf, TEXTLEN-1);\\n\\tLine = atoi(buf) - 1;\\n}\\n\\nstatic void junkln(void) {\\n\\twhile (!feof(Infile) && fgetc(Infile) != \\\'\\\\n\\\')\\n\\t\\t;\\n\\t\\n\\tLine++;\\n}\\n\\nint frozen(int depth) {\\n\\treturn Isp >= depth &&\\n\\t\\t(P_IFNDEF == Ifdefstk[Isp-depth] ||\\n\\t\\tP_ELSENOT == Ifdefstk[Isp-depth]);\\n}\\n\\nvoid preproc(void) {\\n\\tputback(\\\'#\\\');\\n\\tToken = scanraw();\\n\\tif (\\tfrozen(1) &&\\n\\t\\tP_IFDEF != Token && P_IFNDEF != Token &&\\n\\t\\tP_ELSE != Token && P_ENDIF != Token\\n\\t) {\\n\\t\\tjunkln();\\n\\t\\treturn;\\n\\t}\\n\\tswitch (Token) {\\n\\tcase P_DEFINE:\\tdefmac(); break;\\n\\tcase P_UNDEF:\\tundef(); break;\\n\\tcase P_INCLUDE:\\tinclude(); break;\\n\\tcase P_IFDEF:\\tifdef(1); break;\\n\\tcase P_IFNDEF:\\tifdef(0); break;\\n\\tcase P_ELSE:\\tp_else(); break;\\n\\tcase P_ENDIF:\\tendif(); break;\\n\\tcase P_ERROR:\\tpperror(); break;\\n\\tcase P_LINE:\\tsetline(); break;\\n\\tcase P_PRAGMA:\\tjunkln(); break;\\n\\tdefault:\\tjunkln(); break;\\n\\t\\t\\tbreak;\\n\\t}\\n}\\n\");\n\t\t\n\t}\n\t\n\t\n\tprogram(file, in, out, def, source);\n\tif (file) {\n\t\tfclose(in);\n\t\t\n\t\tprintf(\"**********VAR in is closed**********\\n\\n\");\n\t\t\n\t\tif (out) fclose(out);\n\t}\n}\n\n#endif \n\nstatic void collect(char *file, int temp) {\n\tif (O_componly || O_asmonly) return;\n\tif (Nf >= MAXFILES)\n\t\tcmderror(\"too many input files\", NULL);\n\tTemp[Nf] = temp;\n\tFiles[Nf++] = file;\n}\n\nstatic void assemble(char *file, int delete) {\n\tchar\t*ofile;\n\tchar\tcmd[TEXTLEN+1];\n\n\tfile = newfilename(file, \'s\');\n\tif (O_componly && O_outfile)\n\t\tofile = O_outfile;\n\telse\n\t\tcollect(ofile = newfilename(file, \'o\'), 1);\n\tif (strlen(file) + strlen(ofile) + strlen(ASCMD) >= TEXTLEN)\n\t\tcmderror(\"assembler command too long\", NULL);\n\tsprintf(cmd, ASCMD, ofile, file);\n\tif (O_verbose > 1) printf(\"%s\\n\", cmd);\n\tif (system(cmd))\n\t\tcmderror(\"assembler invocation failed\", NULL);\n\tif (delete) {\n\t\tif (O_verbose > 2) printf(\"rm %s\\n\", file);\n\t\tremove(file);\n\t}\n}\n\nstatic int concat(int k, char *buf, char *s) {\n\tint\tn;\n\n\tn = strlen(s);\n\tif (k + n + 2 >= TEXTLEN)\n\t\tcmderror(\"linker command too long\", buf);\n\tstrcat(buf, \" \");\n\tstrcat(buf, s);\n\treturn k + n + 1;\n}\n\nstatic void link(void) {\n\tint\ti, k;\n\tchar\tcmd[TEXTLEN+2];\n\tchar\tcmd2[TEXTLEN+2];\n\tchar\t*ofile;\n\n\tofile = O_outfile? O_outfile: AOUTNAME;\n\tif (strlen(ofile) + strlen(LDCMD) + strlen(SCCDIR)*2 >= TEXTLEN)\n\t\tcmderror(\"linker command too long\", NULL);\n\tsprintf(cmd, LDCMD, ofile, SCCDIR, O_stdio? \"\": \"n\");\n\tk = strlen(cmd);\n\tfor (i=0; i<Nf; i++)\n\t\tk = concat(k, cmd, Files[i]);\n\tk = concat(k, cmd, SCCLIBC);\n\tconcat(k, cmd, SYSLIBC);\n\tsprintf(cmd2, cmd, SCCDIR);\n\tif (O_verbose > 1) printf(\"%s\\n\", cmd2);\n\tif (system(cmd2))\n\t\tcmderror(\"linker invocation failed\", NULL);\n\tif (O_verbose > 2) printf(\"rm \");\n\tfor (i=0; i<Nf; i++) {\n\t\tif (Temp[i]) {\n\t\t\tif (O_verbose > 2) printf(\" %s\", Files[i]);\n\t\t\tremove(Files[i]);\n\t\t}\n\t}\n\tif (O_verbose > 2) printf(\"\\n\");\n}\n\nstatic void usage(void) {\n\tprintf(\"Usage: scc [-h] [-ctvNSV] [-d opt] [-o file] [-D macro[=text]]\"\n\t\t\" file [...]\\n\");\n}\n\nstatic void longusage(void) {\n\tprintf(\"\\n\");\n\tusage();\n\tprintf(\t\"\\n\"\n\t\t\"-c       compile only, do not link\\n\"\n\t\t\"-d opt   activate debug option OPT, ? = list\\n\"\n\t\t\"-o file  write linker output to FILE\\n\"\n\t\t\"-t       test only, generate no code\\n\"\n\t\t\"-v       verbose, more v\'s = more verbose\\n\"\n\t\t\"-D m=v   define macro M with optional value V\\n\"\n\t\t\"-N       do not use stdio (can\'t use printf, etc)\\n\"\n\t\t\"-S       compile to assembly language\\n\"\n\t\t\"-V       print version and exit\\n\"\n\t\t\"\\n\" );\n}\n\nstatic void version(void) {\n\tprintf(\"SubC version %s for %s/%s\\n\", VERSION, OS, CPU);\n}\n\nstatic char *nextarg(int argc, char *argv[], int *pi, int *pj) {\n\tchar\t*s;\n\n\tif (argv[*pi][*pj+1] || *pi >= argc-1) {\n\t\tusage();\n\t\texit(EXIT_FAILURE);\n\t}\n\ts = argv[++*pi];\n\t*pj = strlen(s)-1;\n\treturn s;\n}\n\nstatic int dbgopt(int argc, char *argv[], int *pi, int *pj) {\n\tchar\t*s;\n\n\ts = nextarg(argc, argv, pi, pj);\n\tif (!strcmp(s, \"lsym\")) return D_LSYM;\n\tif (!strcmp(s, \"gsym\")) return D_GSYM;\n\tif (!strcmp(s, \"stat\")) return D_STAT;\n\tprintf(\t\"\\n\"\n\t\t\"scc: valid -d options are: \\n\\n\"\n\t\t\"lsym - dump local symbol tables\\n\"\n\t\t\"gsym - dump global symbol table\\n\"\n\t\t\"stat - print usage statistics\\n\"\n\t\t\"\\n\");\n\texit(EXIT_FAILURE);\n}\n\nint main(int argc, char *argv[]) {\n\tint\ti, j;\n\tchar\t*def;\n\t\n\n\tdef = NULL;\n\tO_debug = 0;\n\tO_verbose = 0;\n\tO_componly = 0;\n\tO_asmonly = 0;\n\tO_testonly = 0;\n\tO_stdio = 1;\n\tO_outfile = NULL;\n\tfor (i=1; i<argc; i++) {\n\t\tif (*argv[i] != \'-\') break;\n\t\tif (!strcmp(argv[i], \"-\")) {\n\t\t\tcompile(NULL, def);\n\t\t\texit(Errors? EXIT_FAILURE: EXIT_SUCCESS);\n\t\t}\n\t\tfor (j=1; argv[i][j]; j++) {\n\t\t\tswitch (argv[i][j]) {\n\t\t\tcase \'c\':\n\t\t\t\tO_componly = 1;\n\t\t\t\tbreak;\n\t\t\tcase \'d\':\n\t\t\t\tO_debug |= dbgopt(argc, argv, &i, &j);\n\t\t\t\tO_testonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase \'h\':\n\t\t\t\tlongusage();\n\t\t\t\texit(EXIT_SUCCESS);\n\t\t\tcase \'o\':\n\t\t\t\tO_outfile = nextarg(argc, argv, &i, &j);\n\t\t\t\tbreak;\n\t\t\tcase \'t\':\n\t\t\t\tO_testonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase \'v\':\n\t\t\t\tO_verbose++;\n\t\t\t\tbreak;\n\t\t\tcase \'D\':\n\t\t\t\tif (def) cmderror(\"too many -D\'s\", NULL);\n\t\t\t\tdef = nextarg(argc, argv, &i, &j);\n\t\t\t\tbreak;\n\t\t\tcase \'N\':\n\t\t\t\tO_stdio = 0;\n\t\t\t\tbreak;\n\t\t\tcase \'S\':\n\t\t\t\tO_componly = O_asmonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase \'V\':\n\t\t\t\tversion();\n\t\t\t\texit(EXIT_SUCCESS);\n\t\t\tdefault:\n\t\t\t\tusage();\n\t\t\t\texit(EXIT_FAILURE);\n\t\t\t}\n\t\t}\n\t}\n\tif (i >= argc) {\n\t\tusage();\n\t\texit(EXIT_FAILURE);\n\t}\n\tNf = 0;\n\twhile (i < argc) {\n\t\tif (filetype(argv[i]) == \'c\') {\n\t\t\tcompile(argv[i], def);\n\t\t\tif (Errors && !O_testonly)\n\t\t\t\tcmderror(\"compilation stopped\", NULL);\n\t\t\tif (!O_asmonly && !O_testonly)\n\t\t\t\tassemble(argv[i], 1);\n\t\t\ti++;\n\t\t}\n\t\telse if (filetype(argv[i]) == \'s\') {\n\t\t\tif (!O_testonly) assemble(argv[i++], 0);\n\t\t}\n\t\telse {\n\t\t\tif (!exists(argv[i])) cmderror(\"no such file: %s\",\n\t\t\t\t\t\t\targv[i]);\n\t\t\tcollect(argv[i++], 0);\n\t\t}\n\t}\n\tif (!O_componly && !O_testonly) link();\n\treturn EXIT_SUCCESS;\n}\n");
		mainlen = strlen(quine);	
		//mod str to make quine over here.
		for( i = 0; i < mainlen - 14; ++i ) {	// find first instance of 'strcpy(quine,' 
			if( quine[i] == 's' )
				if( quine[i+1] == 't' )
					if( quine[i+2] == 'r' )
						if( quine[i+3] == 'c' )
							if( quine[i+4] == 'p' )
								if( quine[i+5] == 'y' )
									if( quine[i+6] == '(' )
										if( quine[i+7] == 'q' )
											if( quine[i+8] == 'u' )
												if( quine[i+9] == 'i' )
													if( quine[i+10] == 'n' )
														if( quine[i+11] == 'e' )
															if( quine[i+12] == ',' )
																if( quine[i+13] == '"' ) {
																	quinei = i;
																	printf("			**********************Found sequence at index %d\n",i);
																	break;
																}
		}
		if( i < mainlen - 14 ) {
			printf("replaced main.c with quine mainlen - 12: %d\n",i);
			i = i + 14;
		}
		else {
			printf("DIDNT FIND quine insertion point.\n");
		}
		// Backing up first half of the string.
		for( j = 0; j < i; ++j ) {
			quinefirsthalf[j] = quine[j];
		}
		quinefirsthalf[j] = '\0';
		firsthalflen = strlen(quinefirsthalf);
		// Backing up second half of the string.
		for( k = 0; j < mainlen ; ++j, ++k ) {
			quinesecondhalf[k] = quine[j];
		}
		quinesecondhalf[k] = '\0';
		secondhalflen = strlen(quinesecondhalf);

		strcpy( quineescapeone, quinefirsthalf );
		esonelen = firsthalflen;
		escapesequence( quineescapeone, &esonelen, NULL);
		oneline( quineescapeone, &esonelen, NULL);

		strcpy( quineescapetwo, quinesecondhalf );
		estwolen = secondhalflen;
		escapesequence( quineescapetwo, &estwolen, NULL );
		oneline( quineescapetwo, &estwolen, NULL );

		// first half of S + S + second half of S
		for( k = 0; k < firsthalflen; ++k ) 
			quine[k] = quinefirsthalf[k];
		for( j = 0; j < esonelen; ++j, ++k ) 
			quine[k] = quineescapeone[j];
		for( j = 0; j < estwolen; ++j, ++k )
			quine[k] = quineescapetwo[j];
		for( j = 0; j < secondhalflen; ++j, ++k )
			quine[k] = quinesecondhalf[j];
		quine[k] = '\0';
		//*****
		mainlen = strlen(quine);
		printf("writeouting and len of string: %d\n",mainlen);
		writeout = fopen("mainoutput.bak.c","w");
		fputs(quine,writeout);
		fclose(writeout);
		//*****/
		strcpy(source,quine);
	}
	if(!strcmp( file, "decl.c" )) {
		strcpy(source, "\n\n#include \"defs.h\"\n#include \"data.h\"\n#include \"decl.h\"\n\nstatic int declarator(int arg, int scls, char *name, int *pprim, int *psize,\n\t\t\tint *pval, int *pinit);\n\n\n\nstatic void enumdecl(int glob) {\n\tint\tv = 0;\n\tchar\tname[NAMELEN+1];\n\n\tToken = scan();\n\tif (IDENT == Token)\n\t\tToken = scan();\n\tlbrace();\n\twhile (RBRACE != Token) {\n\t\tcopyname(name, Text);\n\t\tident();\n\t\tif (ASSIGN == Token) {\n\t\t\tToken = scan();\n\t\t\tv = constexpr();\n\t\t}\n\t\tif (glob)\n\t\t\taddglob(name, PINT, TCONSTANT, 0, 0, v++, NULL, 0);\n\t\telse\n\t\t\taddloc(name, PINT, TCONSTANT, 0, 0, v++, 0);\n\t\tif (Token != COMMA)\n\t\t\tbreak;\n\t\tToken = scan();\n\t\tif (eofcheck()) return;\n\t}\n\trbrace();\n\tsemi();\n}\n\n\n\nstatic int initlist(char *name, int prim) {\n\tint\tn = 0, v;\n\tchar\tbuf[30];\n\n\tgendata();\n\tgenname(name);\n\tif (STRLIT == Token) {\n\t\tif (PCHAR != prim)\n\t\t\terror(\"initializer type mismatch: %s\", name);\n\t\tgendefs(Text, Value);\n\t\tgendefb(0);\n\t\tgenalign(Value-1);\n\t\tToken = scan();\n\t\treturn Value-1;\n\t}\n\tlbrace();\n\twhile (Token != RBRACE) {\n\t\tv = constexpr();\n\t\tif (PCHAR == prim) {\n\t\t\tif (v < 0 || v > 255) {\n\t\t\t\tsprintf(buf, \"%d\", v);\n\t\t\t\terror(\"initializer out of range: %s\", buf);\n\t\t\t}\n\t\t\tgendefb(v);\n\t\t}\n\t\telse {\n\t\t\tgendefw(v);\n\t\t}\n\t\tn++;\n\t\tif (COMMA == Token)\n\t\t\tToken = scan();\n\t\telse\n\t\t\tbreak;\n\t\tif (eofcheck()) return 0;\n\t}\n\tif (PCHAR == prim) genalign(n);\n\tToken = scan();\n\tif (!n) error(\"too few initializers\", NULL);\n\treturn n;\n}\n\nint primtype(int t, char *s) {\n\tint\tp, y;\n\tchar\tsname[NAMELEN+1];\n\n\tp = t == CHAR? PCHAR:\n\t\tt == INT? PINT:\n\t\tt == STRUCT? PSTRUCT:\n\t\tt == UNION? PUNION:\n\t\tPVOID;\n\tif (PUNION == p || PSTRUCT == p) {\n\t\tif (!s) {\n\t\t\tToken = scan();\n\t\t\tcopyname(sname, Text);\n\t\t\ts = sname;\n\t\t\tif (IDENT != Token) {\n\t\t\t\terror(\"struct/union name expected: %s\", Text);\n\t\t\t\treturn p;\n\t\t\t}\n\t\t}\n\t\tif ((y = findstruct(s)) == 0 || Prims[y] != p)\n\t\t\terror(\"no such struct/union: %s\", s);\n\t\tp |= y;\n\t}\n\treturn p;\n}\n\n\n\nstatic int pmtrdecls(void) {\n\tchar\tname[NAMELEN+1];\n\tint\tprim, type, size, na, addr;\n\tint\tdummy;\n\n\tif (RPAREN == Token)\n\t\treturn 0;\n\tna = 0;\n\taddr = 2*BPW;\n\tfor (;;) {\n\t\tif (na > 0 && ELLIPSIS == Token) {\n\t\t\tToken = scan();\n\t\t\tna = -(na + 1);\n\t\t\tbreak;\n\t\t}\n\t\telse if (IDENT == Token) {\n\t\t\tprim = PINT;\n\t\t}\n\t\telse {\n\t\t\tif (\tToken != CHAR && Token != INT &&\n\t\t\t\tToken != VOID &&\n\t\t\t\tToken != STRUCT && Token != UNION\n\t\t\t) {\n\t\t\t\terror(\"type specifier expected at: %s\", Text);\n\t\t\t\tToken = synch(RPAREN);\n\t\t\t\treturn na;\n\t\t\t}\n\t\t\tname[0] = 0;\n\t\t\tprim = primtype(Token, NULL);\n\t\t\tToken = scan();\n\t\t\tif (RPAREN == Token && prim == PVOID && !na)\n\t\t\t\treturn 0;\n\t\t}\n\t\tsize = 1;\n\t\ttype = declarator(1, CAUTO, name, &prim, &size, &dummy,\n\t\t\t\t&dummy);\n\t\taddloc(name, prim, type, CAUTO, size, addr, 0);\n\t\taddr += BPW;\n\t\tna++;\n\t\tif (COMMA == Token)\n\t\t\tToken = scan();\n\t\telse\n\t\t\tbreak;\n\t}\n\treturn na;\n}\n\nint pointerto(int prim) {\n\tint\ty;\n\n\tif (CHARPP == prim || INTPP == prim || VOIDPP == prim ||\n\t    FUNPTR == prim ||\n\t    (prim & STCMASK) == STCPP || (prim & STCMASK) == UNIPP\n\t)\n\t\terror(\"too many levels of indirection\", NULL);\n\ty = prim & ~STCMASK;\n\tswitch (prim & STCMASK) {\n\tcase PSTRUCT:\treturn STCPTR | y;\n\tcase STCPTR:\treturn STCPP | y;\n\tcase PUNION:\treturn UNIPTR | y;\n\tcase UNIPTR:\treturn UNIPP | y;\n\t}\n\treturn PINT == prim? INTPTR:\n\t\tPCHAR == prim? CHARPTR:\n\t\tPVOID == prim? VOIDPTR:\n\t\tINTPTR == prim? INTPP:\n\t\tCHARPTR == prim? CHARPP: VOIDPP;\n}\n\n\n\nstatic int declarator(int pmtr, int scls, char *name, int *pprim, int *psize,\n\t\t\tint *pval, int *pinit)\n{\n\tint\ttype = TVARIABLE;\n\tint\tptrptr = 0;\n\n\tif (STAR == Token) {\n\t\tToken = scan();\n\t\t*pprim = pointerto(*pprim);\n\t\tif (STAR == Token) {\n\t\t\tToken = scan();\n\t\t\t*pprim = pointerto(*pprim);\n\t\t\tptrptr = 1;\n\t\t}\n\t}\n\telse if (LPAREN == Token) {\n\t\tif (*pprim != PINT)\n\t\t\terror(\"function pointers are limited to type \'int\'\",\n\t\t\t\tNULL);\n\t\tToken = scan();\n\t\t*pprim = FUNPTR;\n\t\tmatch(STAR, \"(*name)()\");\n\t}\n\tif (IDENT != Token) {\n\t\terror(\"missing identifier at: %s\", Text);\n\t\tname[0] = 0;\n\t}\n\telse {\n\t\tcopyname(name, Text);\n\t\tToken = scan();\n\t}\n\tif (FUNPTR == *pprim) {\n\t\trparen();\n\t\tlparen();\n\t\trparen();\n\t}\n\tif (!pmtr && ASSIGN == Token) {\n\t\tToken = scan();\n\t\t*pval = constexpr();\n\t\tif (PCHAR == *pprim)\n\t\t\t*pval &= 0xff;\n\t\tif (*pval && !inttype(*pprim))\n\t\t\terror(\"non-zero pointer initialization\", NULL);\n\t\t*pinit = 1;\n\t}\n\telse if (!pmtr && LPAREN == Token) {\n\t\tToken = scan();\n\t\t*psize = pmtrdecls();\n\t\trparen();\n\t\treturn TFUNCTION;\n\t}\n\telse if (LBRACK == Token) {\n\t\tif (ptrptr)\n\t\t\terror(\"too many levels of indirection: %s\", name);\n\t\tToken = scan();\n\t\tif (RBRACK == Token) {\n\t\t\tToken = scan();\n\t\t\tif (pmtr) {\n\t\t\t\t*pprim = pointerto(*pprim);\n\t\t\t}\n\t\t\telse {\n\t\t\t\ttype = TARRAY;\n\t\t\t\t*psize = 1;\n\t\t\t\tif (ASSIGN == Token) {\n\t\t\t\t\tToken = scan();\n\t\t\t\t\tif (!inttype(*pprim))\n\t\t\t\t\t\terror(\"initialization of\"\n\t\t\t\t\t\t\t\" pointer array not\"\n\t\t\t\t\t\t\t\" supported\",\n\t\t\t\t\t\t\tNULL);\n\t\t\t\t\t*psize = initlist(name, *pprim);\n\t\t\t\t\tif (CAUTO == scls)\n\t\t\t\t\t\terror(\"initialization of\"\n\t\t\t\t\t\t\t\" local arrays\"\n\t\t\t\t\t\t\t\" not supported: %s\",\n\t\t\t\t\t\t\tname);\n\t\t\t\t\t*pinit = 1;\n\t\t\t\t}\n\t\t\t\telse if (CEXTERN != scls) {\n\t\t\t\t\terror(\"automatically-sized array\"\n\t\t\t\t\t\t\" lacking initialization: %s\",\n\t\t\t\t\t\tname);\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t\telse {\n\t\t\t*psize = constexpr();\n\t\t\tif (*psize < 0) {\n\t\t\t\terror(\"invalid array size\", NULL);\n\t\t\t\t*psize = 0;\n\t\t\t}\n\t\t\ttype = TARRAY;\n\t\t\trbrack();\n\t\t}\n\t}\n\tif (PVOID == *pprim)\n\t\terror(\"\'void\' is not a valid type: %s\", name);\n\treturn type;\n}\n\n\n\nstatic int localdecls(void) {\n\tchar\tname[NAMELEN+1];\n\tint\tprim, type, size, addr = 0, val, ini;\n\tint\tstat, extn;\n\tint\tpbase, rsize;\n\n\tNli = 0;\n\twhile ( AUTO == Token || EXTERN == Token || REGISTER == Token ||\n\t\tSTATIC == Token || VOLATILE == Token ||\n\t\tINT == Token || CHAR == Token || VOID == Token ||\n\t\tENUM == Token ||\n\t\tSTRUCT == Token || UNION == Token\n\t) {\n\t\tif (ENUM == Token) {\n\t\t\tenumdecl(0);\n\t\t\tcontinue;\n\t\t}\n\t\textn = stat = 0;\n\t\tif (AUTO == Token || REGISTER == Token || STATIC == Token ||\n\t\t\tVOLATILE == Token || EXTERN == Token\n\t\t) {\n\t\t\tstat = STATIC == Token;\n\t\t\textn = EXTERN == Token;\n\t\t\tToken = scan();\n\t\t\tif (\tINT == Token || CHAR == Token ||\n\t\t\t\tVOID == Token ||\n\t\t\t\tSTRUCT == Token || UNION == Token\n\t\t\t) {\n\t\t\t\tprim = primtype(Token, NULL);\n\t\t\t\tToken = scan();\n\t\t\t}\n\t\t\telse\n\t\t\t\tprim = PINT;\n\t\t}\n\t\telse {\n\t\t\tprim = primtype(Token, NULL);\n\t\t\tToken = scan();\n\t\t}\n\t\tpbase = prim;\n\t\tfor (;;) {\n\t\t\tprim = pbase;\n\t\t\tif (eofcheck()) return 0;\n\t\t\tsize = 1;\n\t\t\tini = val = 0;\n\t\t\ttype = declarator(0, CAUTO, name, &prim, &size,\n\t\t\t\t\t&val, &ini);\n\t\t\trsize = objsize(prim, type, size);\n\t\t\trsize = (rsize + INTSIZE-1) / INTSIZE * INTSIZE;\n\t\t\tif (stat) {\n\t\t\t\taddloc(name, prim, type, CLSTATC, size,\n\t\t\t\t\tlabel(), val);\n\t\t\t}\n\t\t\telse if (extn) {\n\t\t\t\taddloc(name, prim, type, CEXTERN, size,\n\t\t\t\t\t0, val);\n\t\t\t}\n\t\t\telse {\n\t\t\t\taddr -= rsize;\n\t\t\t\taddloc(name, prim, type, CAUTO, size, addr, 0);\n\t\t\t}\n\t\t\tif (ini && !stat) {\n\t\t\t\tif (Nli >= MAXLOCINIT) {\n\t\t\t\t\terror(\"too many local initializers\",\n\t\t\t\t\t\tNULL);\n\t\t\t\t\tNli = 0;\n\t\t\t\t}\n\t\t\t\tLIaddr[Nli] = addr;\n\t\t\t\tLIval[Nli++] = val;\n\t\t\t}\n\t\t\tif (COMMA == Token)\n\t\t\t\tToken = scan();\n\t\t\telse\n\t\t\t\tbreak;\n\t\t}\n\t\tsemi();\n\t}\n\treturn addr;\n}\n\nstatic int intcmp(int *x1, int *x2) {\n\twhile (*x1 && *x1 == *x2)\n\t\tx1++, x2++;\n\treturn *x1 - *x2;\n}\n\nstatic void signature(int fn, int from, int to) {\n\tint\ttypes[MAXFNARGS+1], i;\n\n\tif (to - from > MAXFNARGS)\n\t\terror(\"too many function parameters\", Names[fn]);\n\tfor (i=0; i<MAXFNARGS && from < to; i++)\n\t\ttypes[i] = Prims[--to];\n\ttypes[i] = 0;\n\tif (NULL == Mtext[fn]) {\n\t\tMtext[fn] = galloc((i+1) * sizeof(int), 1);\n\t\tmemcpy(Mtext[fn], types, (i+1) * sizeof(int));\n\t}\n\telse if (intcmp((int *) Mtext[fn], types))\n\t\terror(\"declaration does not match prior prototype: %s\",\n\t\t\tNames[fn]);\n}\n\n\n\nvoid decl(int clss, int prim) {\n\tchar\tname[NAMELEN+1];\n\tint\tpbase, type, size = 0, val, init;\n\tint\tlsize;\n\n\tpbase = prim;\n\tfor (;;) {\n\t\tprim = pbase;\n\t\tval = 0;\n\t\tinit = 0;\n\t\ttype = declarator(0, clss, name, &prim, &size, &val, &init);\n\t\tif (TFUNCTION == type) {\n\t\t\tclss = clss == CSTATIC? CSPROTO: CEXTERN;\n\t\t\tThisfn = addglob(name, prim, type, clss, size, 0,\n\t\t\t\t\tNULL, 0);\n\t\t\tsignature(Thisfn, Locs, NSYMBOLS);\n\t\t\tif (LBRACE == Token) {\n\t\t\t\tclss = clss == CSPROTO? CSTATIC:\n\t\t\t\t\tclss == CEXTERN? CPUBLIC: clss;\n\t\t\t\tThisfn = addglob(name, prim, type, clss, size,\n\t\t\t\t\t0, NULL, 0);\n\t\t\t\tToken = scan();\n\t\t\t\tlsize = localdecls();\n\t\t\t\tgentext();\n\t\t\t\tif (CPUBLIC == clss) genpublic(name);\n\t\t\t\tgenaligntext();\n\t\t\t\tgenname(name);\n\t\t\t\tgenentry();\n\t\t\t\tgenstack(lsize);\n\t\t\t\tgenlocinit();\n\t\t\t\tRetlab = label();\n\t\t\t\tcompound(0);\n\t\t\t\tgenlab(Retlab);\n\t\t\t\tgenstack(-lsize);\n\t\t\t\tgenexit();\n\t\t\t\tif (O_debug & D_LSYM)\n\t\t\t\t\tdumpsyms(\"LOCALS: \", name, Locs,\n\t\t\t\t\t\tNSYMBOLS);\n\t\t\t}\n\t\t\telse {\n\t\t\t\tsemi();\n\t\t\t}\n\t\t\tclrlocs();\n\t\t\treturn;\n\t\t}\n\t\tif (CEXTERN == clss && init) {\n\t\t\terror(\"initialization of \'extern\': %s\", name);\n\t\t}\n\t\taddglob(name, prim, type, clss, size, val, NULL, init);\n\t\tif (COMMA == Token)\n\t\t\tToken = scan();\n\t\telse\n\t\t\tbreak;\n\t}\n\tsemi();\n}\n\n\n\nvoid structdecl(int clss, int uniondecl) {\n\tint\tbase, prim, size, dummy, type, addr = 0;\n\tchar\tname[NAMELEN+1], sname[NAMELEN+1];\n\tint\ty, usize = 0;\n\n\tToken = scan();\n\tcopyname(sname, Text);\n\tident();\n\tif (Token != LBRACE) {\n\t\tprim = primtype(uniondecl? UNION: STRUCT, sname);\n\t\tdecl(clss, prim);\n\t\treturn;\n\t}\n\ty = addglob(sname, uniondecl? PUNION: PSTRUCT, TSTRUCT,\n\t\t\tCMEMBER, 0, 0, NULL, 0);\n\tToken = scan();\n\twhile (\tINT == Token || CHAR == Token || VOID == Token ||\n\t\tSTRUCT == Token || UNION == Token\n\t) {\n\t\tbase = primtype(Token, NULL);\n\t\tsize = 0;\n\t\tToken = scan();\n\t\tfor (;;) {\n\t\t\tif (eofcheck()) return;\n\t\t\tprim = base;\n\t\t\ttype = declarator(1, clss, name, &prim, &size,\n\t\t\t\t\t\t&dummy, &dummy);\n\t\t\taddglob(name, prim, type, CMEMBER, size, addr,\n\t\t\t\tNULL, 0);\n\t\t\tsize = objsize(prim, type, size);\n\t\t\tif (size < 0)\n\t\t\t\terror(\"size of struct/union member\"\n\t\t\t\t\t\" is unknown: %s\",\n\t\t\t\t\tname);\n\t\t\tif (uniondecl) {\n\t\t\t\tusize = size > usize? size: usize;\n\t\t\t}\n\t\t\telse {\n\t\t\t\taddr += size;\n\t\t\t\taddr = (addr + INTSIZE-1) / INTSIZE * INTSIZE;\n\t\t\t}\n\t\t\tif (Token != COMMA) break;\n\t\t\tToken = scan();\n\t\t}\n\t\tsemi();\n\t}\n\trbrace();\n\tsemi();\n\tSizes[y] = uniondecl? usize: addr;\n}\n\n\n\nvoid top(void) {\t\n\t\n\tint\tprim, clss = CPUBLIC;\n\t\n\t\n\t\n\n\tswitch (Token) {\n\tcase EXTERN:\tclss = CEXTERN; Token = scan(); break;\n\tcase STATIC:\tclss = CSTATIC; Token = scan(); break;\n\tcase VOLATILE:\tToken = scan(); break;\n\t}\n\tswitch (Token) {\n\tcase ENUM:\n\t\tenumdecl(1);\n\t\tbreak;\n\tcase STRUCT:\n\tcase UNION:\n\t\tstructdecl(clss, UNION == Token);\n\t\tbreak;\n\tcase CHAR:\n\tcase INT:\n\tcase VOID:\n\t\tprim = primtype(Token, NULL);\n\t\tToken = scan();\n\t\tdecl(clss, prim);\n\t\tbreak;\n\tcase IDENT:\n\t\tdecl(clss, PINT);\n\t\tbreak;\n\tdefault:\n\t\terror(\"type specifier expected at: %s\", Text);\n\t\tToken = synch(SEMI);\n\t\tbreak;\n\t}\n}\n\nstatic void stats(void) {\n\tprintf(\t\"Memory usage: \"\n\t\t\"Symbols: %5d/%5d, \"\n\t\t\"Names: %5d/%5d, \"\n\t\t\"Nodes: %5d/%5d\\n\",\n\t\tGlobs, NSYMBOLS,\n\t\tNbot, POOLSIZE,\n\t\tNdmax, NODEPOOLSZ);\n}\n\nvoid defarg(char *s) {\n\tchar\t*p;\n\n\tif (NULL == s) return;\n\tif ((p = strchr(s, \'=\')) != NULL)\n\t\t*p++ = 0;\n\telse\n\t\tp = \"\";\n\taddglob(s, 0, TMACRO, 0, 0, 0, globname(p), 0);\n\tif (*p) *--p = \'=\';\n}\n\n\nvoid program(char *name, FILE *in, FILE *out, char *def, char *source) {\n\n\tprintf(\"------------------program()-------------------------file name: \'%s\'\\n\",name);\n\tinit();\n\tdefarg(def);\n\t\n\tInsource = source;\n\tInsourcelen = strlen(Insource);\n\tInsourcei = 0;\t\t\t\n\t\n\t\n\tInfile = in;\n\tOutfile = out;\n\tFile = Basefile = name;\n\tgenprelude();\n\tToken = scan();\n\t\n\twhile (XEOF != Token) {\n\t\ttop();\n\t}\n\tgenpostlude();\n\tif (O_debug & D_GSYM) dumpsyms(\"GLOBALS\", \"\", 1, Globs);\n\tif (O_debug & D_STAT) stats();\n}\n");
		//printf("repacling decl.c:\n%s\n",source);
	}
	if(!strcmp( file, "scan.c" )) {
		strcpy(source, "\n\n#include \"defs.h\"\n#include \"data.h\"\n#include \"decl.h\"\n\n\n\n\nchar strmanager() {\t\n\tchar c;\n\tif(Insourcei == Insourcelen)\t\n\t\treturn -1;\n\telse {\n\t\tc = Insource[Insourcei];\n\t\t++Insourcei;\n\t\treturn c;\n\t}\n}\n\n\nint next(void) {\n\tint\tc;\n\n\tif (Putback) {\n\t\tc = Putback;\n\t\tPutback = 0;\n\t\treturn c;\n\t}\n\tif (Mp) {\n\t\tif (\'\\0\' == *Macp[Mp-1]) {\n\t\t\tMacp[Mp-1] = NULL;\n\t\t\treturn Macc[--Mp];\n\t\t}\n\t\telse {\n\t\t\treturn *Macp[Mp-1]++;\n\t\t}\n\t}\n\t\n\t\n\tc = strmanager();\n\t\n\t\n\t\n\tif (\'\\n\' == c) Line++;\n\treturn c;\n}\n\nvoid putback(int c) {\n\tPutback = c;\n}\n\nstatic int hexchar(void) {\n\tint\tc, h, n = 0, f = 0;\n\n\twhile (isxdigit(c = next())) {\n\t\th = chrpos(\"0123456789abcdef\", tolower(c));\n\t\tn = n * 16 + h;\n\t\tf = 1;\n\t}\n\tputback(c);\n\tif (!f)\n\t\terror(\"missing digits after \'\\\\x\'\", NULL);\n\tif (n > 255)\n\t\terror(\"value out of range after \'\\\\x\'\", NULL);\n\treturn n;\n}\n\nstatic int scanch(void) {\n\tint\ti, c, c2;\n\n\tc = next();\n\tif (\'\\\\\' == c) {\n\t\tswitch (c = next()) {\n\t\tcase \'a\': return \'\\a\';\n\t\tcase \'b\': return \'\\b\';\n\t\tcase \'f\': return \'\\f\';\n\t\tcase \'n\': return \'\\n\';\n\t\tcase \'r\': return \'\\r\';\n\t\tcase \'t\': return \'\\t\';\n\t\tcase \'v\': return \'\\v\';\n\t\tcase \'\\\\\': return \'\\\\\';\n\t\tcase \'\"\': return \'\"\' | 256;\n\t\tcase \'\\\'\': return \'\\\'\';\n\t\tcase \'0\': case \'1\': case \'2\':\n\t\tcase \'3\': case \'4\': case \'5\':\n\t\tcase \'6\': case \'7\':\n\t\t\tfor (i = c2 = 0; isdigit(c) && c < \'8\'; c = next()) {\n\t\t\t\tif (++i > 3) break;\n\t\t\t\tc2 = c2 * 8 + (c - \'0\');\n\t\t\t}\n\t\t\tputback(c);\n\t\t\treturn c2;\n\t\tcase \'x\':\n\t\t\treturn hexchar();\n\t\tdefault:\n\t\t\tscnerror(\"unknown escape sequence: %s\", c);\n\t\t\treturn \' \';\n\t\t}\n\t}\n\telse {\n\t\treturn c;\n\t}\n}\n\nstatic int scanint(int c) {\n\tint\tval, radix, k, i = 0;\n\n\tval = 0;\n\tradix = 10;\n\tif (\'0\' == c) {\n\t\tText[i++] = \'0\';\n\t\tif ((c = next()) == \'x\') {\n\t\t\tradix = 16;\n\t\t\tText[i++] = c;\n\t\t\tc = next();\n\t\t}\n\t\telse {\n\t\t\tradix = 8;\n\t\t}\n\t}\n\twhile ((k = chrpos(\"0123456789abcdef\", tolower(c))) >= 0) {\n\t\tText[i++] = c;\n\t\tif (k >= radix)\n\t\t\tscnerror(\"invalid digit in integer literal: %s\", c);\n\t\tval = val * radix + k;\n\t\tc = next();\n\t}\n\tputback(c);\n\tText[i] = 0;\n\treturn val;\n}\n\nstatic int scanstr(char *buf) {\n\tint\ti, c;\n\n\tbuf[0] = \'\"\';\n\tfor (i=1; i<TEXTLEN-2; i++) {\n\t\tif ((c = scanch()) == \'\"\') {\n\t\t\tbuf[i++] = \'\"\';\n\t\t\tbuf[i] = 0;\n\t\t\treturn Value = i;\n\t\t}\n\t\tbuf[i] = c;\n\t}\n\tfatal(\"string literal too long\");\n\treturn 0;\n}\n\nstatic int scanident(int c, char *buf, int lim) {\n\tint\ti = 0;\n\n\twhile (isalpha(c) || isdigit(c) || \'_\' == c) {\n\t\tif (lim-1 == i) {\n\t\t\terror(\"identifier too long\", NULL);\n\t\t\ti++;\n\t\t}\n\t\telse if (i < lim-1) {\n\t\t\tbuf[i++] = c;\n\t\t}\n\t\tc = next();\n\t}\n\tputback(c);\n\tbuf[i] = 0;\n\treturn i;\n}\n\nint skip(void) {\n\tint\tc, p, nl;\n\n\tc = next();\n\tnl = 0;\n\tfor (;;) {\n\t\tif (EOF == c) {\n\t\t\tstrcpy(Text, \"<EOF>\");\n\t\t\treturn EOF;\n\t\t}\n\t\twhile (\' \' == c || \'\\t\' == c || \'\\n\' == c ||\n\t\t\t\'\\r\' == c || \'\\f\' == c\n\t\t) {\n\t\t\tif (\'\\n\' == c) nl = 1;\n\t\t\tc = next();\n\t\t}\n\t\tif (nl && c == \'#\') {\n\t\t\tpreproc();\n\t\t\tc = next();\n\t\t\tcontinue;\n\t\t}\n\t\tnl = 0;\n\t\tif (c != \'/\')\n\t\t\tbreak;\n\t\tc = next();\n\t\tif (c != \'*\' && c != \'/\') {\n\t\t\tputback(c);\n\t\t\tc = \'/\';\n\t\t\tbreak;\n\t\t}\n\t\tif (c == \'/\') {\n\t\t\twhile ((c = next()) != EOF) {\n\t\t\t\tif (c == \'\\n\') break;\n\t\t\t}\n                }\n                else {\n\t\t\tp = 0;\n\t\t\twhile ((c = next()) != EOF) {\n\t\t\t\tif (\'/\' == c && \'*\' == p) {\n\t\t\t\t\tc = next();\n\t\t\t\t\tbreak;\n\t\t\t\t}\n\t\t\t\tp = c;\n\t\t\t}\n\t\t}\n\t}\n\treturn c;\n}\n\nstatic int keyword(char *s) {\n\tswitch (*s) {\n\tcase \'#\':\n\t\tswitch (s[1]) {\n\t\tcase \'d\':\n\t\t\tif (!strcmp(s, \"#define\")) return P_DEFINE;\n\t\t\tbreak;\n\t\tcase \'e\':\n\t\t\tif (!strcmp(s, \"#else\")) return P_ELSE;\n\t\t\tif (!strcmp(s, \"#endif\")) return P_ENDIF;\n\t\t\tif (!strcmp(s, \"#error\")) return P_ERROR;\n\t\t\tbreak;\n\t\tcase \'i\':\n\t\t\tif (!strcmp(s, \"#ifdef\")) return P_IFDEF;\n\t\t\tif (!strcmp(s, \"#ifndef\")) return P_IFNDEF;\n\t\t\tif (!strcmp(s, \"#include\")) return P_INCLUDE;\n\t\t\tbreak;\n\t\tcase \'l\':\n\t\t\tif (!strcmp(s, \"#line\")) return P_LINE;\n\t\t\tbreak;\n\t\tcase \'p\':\n\t\t\tif (!strcmp(s, \"#pragma\")) return P_PRAGMA;\n\t\t\tbreak;\n\t\tcase \'u\':\n\t\t\tif (!strcmp(s, \"#undef\")) return P_UNDEF;\n\t\t\tbreak;\n\t\t}\n\t\tbreak;\n\tcase \'a\':\n\t\tif (!strcmp(s, \"auto\")) return AUTO;\n\t\tbreak;\n\tcase \'b\':\n\t\tif (!strcmp(s, \"break\")) return BREAK;\n\t\tbreak;\n\tcase \'c\':\n\t\tif (!strcmp(s, \"case\")) return CASE;\n\t\tif (!strcmp(s, \"char\")) return CHAR;\n\t\tif (!strcmp(s, \"continue\")) return CONTINUE;\n\t\tbreak;\n\tcase \'d\':\n\t\tif (!strcmp(s, \"default\")) return DEFAULT;\n\t\tif (!strcmp(s, \"do\")) return DO;\n\t\tbreak;\n\tcase \'e\':\n\t\tif (!strcmp(s, \"else\")) return ELSE;\n\t\tif (!strcmp(s, \"enum\")) return ENUM;\n\t\tif (!strcmp(s, \"extern\")) return EXTERN;\n\t\tbreak;\n\tcase \'f\':\n\t\tif (!strcmp(s, \"for\")) return FOR;\n\t\tbreak;\n\tcase \'i\':\n\t\tif (!strcmp(s, \"if\")) return IF;\n\t\tif (!strcmp(s, \"int\")) return INT;\n\t\tbreak;\n\tcase \'r\':\n\t\tif (!strcmp(s, \"register\")) return REGISTER;\n\t\tif (!strcmp(s, \"return\")) return RETURN;\n\t\tbreak;\n\tcase \'s\':\n\t\tif (!strcmp(s, \"sizeof\")) return SIZEOF;\n\t\tif (!strcmp(s, \"static\")) return STATIC;\n\t\tif (!strcmp(s, \"struct\")) return STRUCT;\n\t\tif (!strcmp(s, \"switch\")) return SWITCH;\n\t\tbreak;\n\tcase \'u\':\n\t\tif (!strcmp(s, \"union\")) return UNION;\n\t\tbreak;\n\tcase \'v\':\n\t\tif (!strcmp(s, \"void\")) return VOID;\n\t\tif (!strcmp(s, \"volatile\")) return VOLATILE;\n\t\tbreak;\n\tcase \'w\':\n\t\tif (!strcmp(s, \"while\")) return WHILE;\n\t\tbreak;\n\t}\n\treturn 0;\n}\n\nstatic int macro(char *name) {\n\tint\ty;\n\n\ty = findmac(name);\n\tif (!y || Types[y] != TMACRO)\n\t\treturn 0;\n\tplaymac(Mtext[y]);\n\treturn 1;\n}\n\nstatic int scanpp(void) {\n\tint\tc, t;\n\n\tif (Rejected != -1) {\n\t\tt = Rejected;\n\t\tRejected = -1;\n\t\tstrcpy(Text, Rejtext);\n\t\tValue = Rejval;\n\t\treturn t;\n\t}\n\tfor (;;) {\n\t\tValue = 0;\n\t\tc = skip();\n\t\tmemset(Text, 0, 4);\n\t\tText[0] = c;\n\t\tswitch (c) {\n\t\tcase \'!\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn NOTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn XMARK;\n\t\t\t}\n\t\tcase \'%\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASMOD;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn MOD;\n\t\t\t}\n\t\tcase \'&\':\n\t\t\tif ((c = next()) == \'&\') {\n\t\t\t\tText[1] = \'&\';\n\t\t\t\treturn LOGAND;\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASAND;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn AMPER;\n\t\t\t}\n\t\tcase \'(\':\n\t\t\treturn LPAREN;\n\t\tcase \')\':\n\t\t\treturn RPAREN;\n\t\tcase \'*\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASMUL;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn STAR;\n\t\t\t}\n\t\tcase \'+\':\n\t\t\tif ((c = next()) == \'+\') {\n\t\t\t\tText[1] = \'+\';\n\t\t\t\treturn INCR;\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASPLUS;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn PLUS;\n\t\t\t}\n\t\tcase \',\':\n\t\t\treturn COMMA;\n\t\tcase \'-\':\n\t\t\tif ((c = next()) == \'-\') {\n\t\t\t\tText[1] = \'-\';\n\t\t\t\treturn DECR;\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASMINUS;\n\t\t\t}\n\t\t\telse if (\'>\' == c) {\n\t\t\t\tText[1] = \'>\';\n\t\t\t\treturn ARROW;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn MINUS;\n\t\t\t}\n\t\tcase \'/\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASDIV;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn SLASH;\n\t\t\t}\n\t\tcase \':\':\n\t\t\treturn COLON;\n\t\tcase \';\':\n\t\t\treturn SEMI;\n\t\tcase \'<\':\n\t\t\tif ((c = next()) == \'<\') {\n\t\t\t\tText[1] = \'<\';\n\t\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\t\tText[2] = \'=\';\n\t\t\t\t\treturn ASLSHIFT;\n\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\tputback(c);\n\t\t\t\t\treturn LSHIFT;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn LTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn LESS;\n\t\t\t}\n\t\tcase \'=\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn EQUAL;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn ASSIGN;\n\t\t\t}\n\t\tcase \'>\':\n\t\t\tif ((c = next()) == \'>\') {\n\t\t\t\tText[1] = \'>\';\n\t\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\t\tText[1] = \'=\';\n\t\t\t\t\treturn ASRSHIFT;\n\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\tputback(c);\n\t\t\t\t\treturn RSHIFT;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn GTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn GREATER;\n\t\t\t}\n\t\tcase \'?\':\n\t\t\treturn QMARK;\n\t\tcase \'[\':\n\t\t\treturn LBRACK;\n\t\tcase \']\':\n\t\t\treturn RBRACK;\n\t\tcase \'^\':\n\t\t\tif ((c = next()) == \'=\') {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASXOR;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn CARET;\n\t\t\t}\n\t\tcase \'{\':\n\t\t\treturn LBRACE;\n\t\tcase \'|\':\n\t\t\tif ((c = next()) == \'|\') {\n\t\t\t\tText[1] = \'|\';\n\t\t\t\treturn LOGOR;\n\t\t\t}\n\t\t\telse if (\'=\' == c) {\n\t\t\t\tText[1] = \'=\';\n\t\t\t\treturn ASOR;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn PIPE;\n\t\t\t}\n\t\tcase \'}\':\n\t\t\treturn RBRACE;\n\t\tcase \'~\':\n\t\t\treturn TILDE;\n\t\tcase EOF:\n\t\t\tstrcpy(Text, \"<EOF>\");\n\t\t\treturn XEOF;\n\t\tcase \'\\\'\':\n\t\t\tText[1] = Value = scanch();\n\t\t\tif ((c = next()) != \'\\\'\')\n\t\t\t\terror(\n\t\t\t\t \"expected \'\\\\\'\' at end of char literal\",\n\t\t\t\t\tNULL);\n\t\t\tText[2] = \'\\\'\';\n\t\t\treturn INTLIT;\n\t\tcase \'\"\':\n\t\t\tValue = scanstr(Text);\n\t\t\treturn STRLIT;\n\t\tcase \'#\':\n\t\t\tText[0] = \'#\';\n\t\t\tscanident(next(), &Text[1], TEXTLEN-1);\n\t\t\tif ((t = keyword(Text)) != 0)\n\t\t\t\treturn t;\n\t\t\terror(\"unknown preprocessor command: %s\", Text);\n\t\t\treturn IDENT;\n\t\tcase \'.\':\n\t\t\tif ((c = next()) == \'.\') {\n\t\t\t\tText[1] = Text[2] = \'.\';\n\t\t\t\tText[3] = 0;\n\t\t\t\tif ((c = next()) == \'.\')\n\t\t\t\t\treturn ELLIPSIS;\n\t\t\t\tputback(c);\n\t\t\t\terror(\"incomplete \'...\'\", NULL);\n\t\t\t\treturn ELLIPSIS;\n\t\t\t}\n\t\t\tputback(c);\n\t\t\treturn DOT;\n\t\tdefault:\n\t\t\tif (isdigit(c)) {\n\t\t\t\tValue = scanint(c);\n\t\t\t\treturn INTLIT;\n\t\t\t}\n\t\t\telse if (isalpha(c) || \'_\' == c) {\n\t\t\t\tValue = scanident(c, Text, TEXTLEN);\n\t\t\t\tif (Expandmac && macro(Text))\n\t\t\t\t\tbreak;\n\t\t\t\tif ((t = keyword(Text)) != 0)\n\t\t\t\t\treturn t;\n\t\t\t\treturn IDENT;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tscnerror(\"funny input character: %s\", c);\n\t\t\t\tbreak;\n\t\t\t}\n\t\t}\n\t}\n}\n\nint scan(void) {\n\tint\tt;\n\n\tdo {\n\t\tt = scanpp();\n\t\tif (!Inclev && Isp && XEOF == t)\n\t\t\tfatal(\"missing \'#endif\'\");\n\t} while (frozen(1));\n\tif (t == Syntoken)\n\t\tSyntoken = 0;\n\treturn t;\n}\n\nint scanraw(void) {\n\tint\tt, oisp;\n\n\toisp = Isp;\n\tIsp = 0;\n\tExpandmac = 0;\n\tt = scan();\n\tExpandmac = 1;\n\tIsp = oisp;\n\treturn t;\n}\n\nvoid reject(void) {\n\tRejected = Token;\n\tRejval = Value;\n\tstrcpy(Rejtext, Text);\n}\n");
	}
	if(!strcmp( file, "prep.c" )) {
		strcpy(source, "\n\n#include \"defs.h\"\n#include \"data.h\"\n#include \"decl.h\"\n\nvoid playmac(char *s) {\n\tif (Mp >= MAXNMAC) fatal(\"too many nested macros\");\n\tMacc[Mp] = next();\n\tMacp[Mp++] = s;\n}\n\nint getln(char *buf, int max) {\t\n\tint\tk;\n\t\n\tchar c;\n\tint i;\n\t\n\n\t\n\t\n\t\n\tfor( i = 0; i < max - 1 ; ++i ) {\n\t\tc = strmanager();\n\t\tbuf[i] = c;\n\t\tif( c == \'\\n\' || c == EOF ) {\n\t\t\t++i;\t\t\t\n\t\t\tbreak;\n\t\t}\n\t}\n\tbuf[i] = \'\\0\';\n\tif( strlen(buf) == 0 ) \n\t\treturn 0;\n\t\n\t\n\t\n\tk = strlen(buf);\n\tif (k) buf[--k] = 0;\t\t\t\t\t\t\n\tif (k && \'\\r\' == buf[k-1]) buf[--k] = 0;\t\n\treturn k;\n}\n\nstatic void defmac(void) {\n\tchar\tname[NAMELEN+1];\n\tchar\tbuf[TEXTLEN+1], *p;\n\tint\ty;\n\n\tToken = scanraw();\n\tif (Token != IDENT)\n\t\terror(\"identifier expected after \'#define\': %s\", Text);\n\tcopyname(name, Text);\n\tif (\'\\n\' == Putback)\n\t\tbuf[0] = 0;\n\telse\n\t\tgetln(buf, TEXTLEN-1);\n\tfor (p = buf; isspace(*p); p++)\n\t\t;\n\tif ((y = findmac(name)) != 0) {\n\t\tif (strcmp(Mtext[y], buf))\n\t\t\terror(\"macro redefinition: %s\", name);\n\t}\n\telse {\n\t\taddglob(name, 0, TMACRO, 0, 0, 0, globname(p), 0);\n\t}\n\tLine++;\n}\n\nstatic void undef(void) {\n\tchar\tname[NAMELEN+1];\n\tint\ty;\n\n\tToken = scanraw();\n\tcopyname(name, Text);\n\tif (IDENT != Token)\n\t\terror(\"identifier expected after \'#undef\': %s\", Text);\n\tif ((y = findmac(name)) != 0)\n\t\tNames[y] = \"#undef\'d\";\n}\n\nstatic void include(void) {\n\tchar\tfile[TEXTLEN+1], path[TEXTLEN+1];\n\tint\tc, k;\n\tFILE\t*inc, *oinfile;\n\tchar\t*ofile;\n\tint\toc, oline;\n\t\n\tchar incsource[TEXTLEN]; \n\tFILE *inctmp;\n\tint tmpi;\n\tchar *main_src;\n\tint main_src_i, main_src_len;\n\t\n\n\tif ((c = skip()) == \'<\')\n\t\tc = \'>\';\n\tk = getln(file, TEXTLEN-strlen(SCCDIR)-9);\t\n\tLine++;\n\tif (!k || file[k-1] != c)\t\t\t\t\t\n\t\terror(\"missing delimiter in \'#include\'\", NULL);\n\tif (k) file[k-1] = 0;\t\t\t\t\t\t\n\tif (c == \'\"\')\n\t\tstrcpy(path, file);\n\telse {\n\t\tstrcpy(path, SCCDIR);\n\t\tstrcat(path, \"/include/\");\n\t\tstrcat(path, file);\n\t}\n\tif ((inc = fopen(path, \"r\")) == NULL)\n\t\terror(\"cannot open include file: \'%s\'\", path);\n\telse {\n\t\t\n\t\t\n\t\t\n\t\tinctmp = fopen(path,\"r\");\n\t\t\n\t\ttmpi = 0;\n\t\twhile( (incsource[tmpi] = fgetc(inctmp)) != EOF ) {\n\t\t\t++tmpi;\n\t\t}\n\t\tincsource[tmpi] =\'\\0\';\t\n\t\tfclose(inctmp);\n\t\t\n\t\t\n\t\t\n\t\tif(!strcmp( file, \"data.h\" )) {\n\t\t\tstrcpy(incsource, \"\\n\\n#ifndef extern_\\n #define extern_ extern\\n#endif\\n\\n\\n\\nextern_ char *Insource;\\nextern_ int Insourcei;\\nextern_ int Insourcelen;\\n\\nextern_ FILE\\t*Infile;\\nextern_ FILE\\t*Outfile;\\nextern_ int\\tToken;\\nextern_ char\\tText[TEXTLEN+1];\\nextern_ int\\tValue;\\nextern_ int\\tLine;\\nextern_ int\\tErrors;\\nextern_ int\\tSyntoken;\\nextern_ int\\tPutback;\\nextern_ int\\tRejected;\\nextern_ int\\tRejval;\\nextern_ char\\tRejtext[TEXTLEN+1];\\nextern_ char\\t*File;\\nextern_ char\\t*Basefile;\\nextern_ char\\t*Macp[MAXNMAC];\\nextern_ int\\tMacc[MAXNMAC];\\nextern_ int\\tMp;\\nextern_ int\\tExpandmac;\\nextern_ int\\tIfdefstk[MAXIFDEF], Isp;\\nextern_ int\\tInclev;\\nextern_ int\\tTextseg;\\nextern_ int\\tNodes[NODEPOOLSZ];\\nextern_ int\\tNdtop;\\nextern_ int\\tNdmax;\\n\\n\\nextern_ char\\t*Names[NSYMBOLS];\\nextern_ int\\tPrims[NSYMBOLS];\\nextern_ char\\tTypes[NSYMBOLS];\\nextern_ char\\tStcls[NSYMBOLS];\\nextern_ int\\tSizes[NSYMBOLS];\\nextern_ int\\tVals[NSYMBOLS];\\nextern_ char\\t*Mtext[NSYMBOLS];\\nextern_ int\\tGlobs;\\nextern_ int\\tLocs;\\n\\nextern_ int\\tThisfn;\\n\\n\\nextern_ char\\tNlist[POOLSIZE];\\nextern_ int\\tNbot;\\nextern_ int\\tNtop;\\n\\n\\nextern_ int\\tBreakstk[MAXBREAK], Bsp;\\nextern_ int\\tContstk[MAXBREAK], Csp;\\nextern_ int\\tRetlab;\\n\\n\\nextern_ int\\tLIaddr[MAXLOCINIT];\\nextern_ int\\tLIval[MAXLOCINIT];\\nextern_ int\\tNli;\\n\\n\\nextern_ int\\tQ_type;\\nextern_ int\\tQ_val;\\nextern_ char\\tQ_name[NAMELEN+1];\\nextern_ int\\tQ_cmp;\\nextern_ int\\tQ_bool;\\n\\n\\nextern_ char\\t*Files[MAXFILES];\\nextern_ char\\tTemp[MAXFILES];\\nextern_ int\\tNf;\\n\\n\\nextern_ int\\tO_verbose;\\nextern_ int\\tO_componly;\\nextern_ int\\tO_asmonly;\\nextern_ int\\tO_testonly;\\nextern_ int\\tO_stdio;\\nextern_ char\\t*O_outfile;\\nextern_ int\\tO_debug;\\n\");\n\t\t}\n\t\tif(!strcmp( file, \"defs.h\" )) {\n\t\t\tstrcpy(incsource,\"\\n\\n#include <stdlib.h>\\n#include <stdio.h>\\n#include <string.h>\\n#include <ctype.h>\\n#include \\\"cg.h\\\"\\n#include \\\"sys.h\\\"\\n\\n#define VERSION\\t\\t\\\"2016-12-12\\\"\\n\\n#ifndef SCCDIR\\n #define SCCDIR\\t\\t\\\".\\\"\\n#endif\\n\\n#ifndef AOUTNAME\\n #define AOUTNAME\\t\\\"a.out\\\"\\n#endif\\n\\n#define SCCLIBC\\t\\t\\\"%s/lib/libscc.a\\\"\\n\\n#define PREFIX\\t\\t\\\'C\\\'\\n#define LPREFIX\\t\\t\\\'L\\\'\\n\\n#define INTSIZE\\t\\tBPW\\n#define PTRSIZE\\t\\tINTSIZE\\n#define CHARSIZE\\t1\\n\\n\\n\\n#define TEXTLEN\\t\\t200000\\n\\n#define NAMELEN\\t\\t16\\n\\n#define MAXFILES\\t32\\n\\n#define MAXIFDEF\\t16\\n#define MAXNMAC\\t\\t32\\n#define MAXCASE\\t\\t256\\n#define MAXBREAK\\t16\\n#define MAXLOCINIT\\t32\\n#define MAXFNARGS\\t32\\n\\n\\n#define NSYMBOLS\\t1024\\n#define POOLSIZE\\t16384\\n#define NODEPOOLSZ\\t4096\\t\\n\\n\\nenum {\\n\\tTVARIABLE = 1,\\n\\tTARRAY,\\n\\tTFUNCTION,\\n\\tTCONSTANT,\\n\\tTMACRO,\\n\\tTSTRUCT\\n};\\n\\n\\nenum {\\n\\tPCHAR = 1,\\n\\tPINT,\\n\\tCHARPTR,\\n\\tINTPTR,\\n\\tCHARPP,\\n\\tINTPP,\\n\\tPVOID,\\n\\tVOIDPTR,\\n\\tVOIDPP,\\n\\tFUNPTR,\\n\\tPSTRUCT = 0x2000,\\n\\tPUNION  = 0x4000,\\n\\tSTCPTR  = 0x6000,\\n\\tSTCPP   = 0x8000,\\n\\tUNIPTR  = 0xA000,\\n\\tUNIPP   = 0xC000,\\n\\tSTCMASK = 0xE000\\n};\\n\\n\\nenum {\\n\\tCPUBLIC = 1,\\n\\tCEXTERN,\\n\\tCSTATIC,\\n\\tCLSTATC,\\n\\tCAUTO,\\n\\tCSPROTO,\\n\\tCMEMBER,\\n\\tCSTCDEF\\n};\\n\\n\\nenum {\\n\\tLVSYM,\\n\\tLVPRIM,\\n\\tLVADDR,\\n\\tLV\\n};\\n\\n\\nenum {\\n\\tD_LSYM = 1,\\n\\tD_GSYM = 2,\\n\\tD_STAT = 4\\n};\\n\\n\\nenum {\\n\\tempty,\\n\\taddr_auto,\\n\\taddr_static,\\n\\taddr_globl,\\n\\taddr_label,\\n\\tliteral,\\n\\tauto_byte,\\n\\tauto_word,\\n\\tstatic_byte,\\n\\tstatic_word,\\n\\tglobl_byte,\\n\\tglobl_word\\n};\\n\\n\\nenum {\\n\\tcnone,\\n\\tequal,\\n\\tnot_equal,\\n\\tless,\\n\\tgreater,\\n\\tless_equal,\\n\\tgreater_equal,\\n\\tbelow,\\n\\tabove,\\n\\tbelow_equal,\\n\\tabove_equal\\n};\\n\\n\\nenum {\\n\\tbnone,\\n\\tlognot,\\n\\tnormalize\\n};\\n\\n\\nstruct node_stc {\\n\\tint\\t\\top;\\n\\tstruct node_stc\\t*left, *right;\\n\\tint\\t\\targs[1];\\n};\\n\\n#define node\\tstruct node_stc\\n\\n\\nenum {\\n\\tSLASH, STAR, MOD, PLUS, MINUS, LSHIFT, RSHIFT,\\n\\tGREATER, GTEQ, LESS, LTEQ, EQUAL, NOTEQ, AMPER,\\n\\tCARET, PIPE, LOGAND, LOGOR,\\n\\n\\tARROW, ASAND, ASXOR, ASLSHIFT, ASMINUS, ASMOD, ASOR, ASPLUS,\\n\\tASRSHIFT, ASDIV, ASMUL, ASSIGN, AUTO, BREAK, CASE, CHAR, COLON,\\n\\tCOMMA, CONTINUE, DECR, DEFAULT, DO, DOT, ELLIPSIS, ELSE, ENUM,\\n\\tEXTERN, FOR, IDENT, IF, INCR, INT, INTLIT, LBRACE, LBRACK,\\n\\tLPAREN, NOT, QMARK, RBRACE, RBRACK, REGISTER, RETURN, RPAREN,\\n\\tSEMI, SIZEOF, STATIC, STRLIT, STRUCT, SWITCH, TILDE, UNION,\\n\\tVOID, VOLATILE, WHILE, XEOF, XMARK,\\n\\n\\tP_DEFINE, P_ELSE, P_ELSENOT, P_ENDIF, P_ERROR, P_IFDEF,\\n\\tP_IFNDEF, P_INCLUDE, P_LINE, P_PRAGMA, P_UNDEF\\n};\\n\\n\\nenum {\\n\\tOP_GLUE, OP_ADD, OP_ADDR, OP_ASSIGN, OP_BINAND, OP_BINIOR,\\n\\tOP_BINXOR, OP_BOOL, OP_BRFALSE, OP_BRTRUE, OP_CALL, OP_CALR,\\n\\tOP_COMMA, OP_DEC, OP_DIV, OP_EQUAL, OP_GREATER, OP_GTEQ,\\n\\tOP_IDENT, OP_IFELSE, OP_LAB, OP_LDLAB, OP_LESS, OP_LIT,\\n\\tOP_LOGNOT, OP_LSHIFT, OP_LTEQ, OP_MOD, OP_MUL, OP_NEG,\\n\\tOP_NOT, OP_NOTEQ, OP_PLUS, OP_PREDEC, OP_PREINC, OP_POSTDEC,\\n\\tOP_POSTINC, OP_RSHIFT, OP_RVAL, OP_SCALE, OP_SCALEBY, OP_SUB\\n};\\n\\n\");\n\t\t}\n\t\tif(!strcmp( file, \"decl.h\" )) {\n\t\t\tstrcpy(incsource,\"\\n\\nint\\taddglob(char *name, int prim, int type, int scls, int size, int val,\\n\\t\\tchar *mval, int init);\\nint\\taddloc(char *name, int prim, int type, int scls, int size, int val,\\n\\t\\tint init);\\nint\\tbinoptype(int op, int p1, int p2);\\nint\\tchrpos(char *s, int c);\\nvoid\\tclear(int q);\\nvoid\\tcleanup(void);\\nvoid\\tclrlocs(void);\\nvoid\\tcolon(void);\\nvoid\\tcommit(void);\\nvoid\\tcommit_bool(void);\\nvoid\\tcommit_cmp(void);\\nvoid\\tcompound(int lbr);\\nint\\tcomptype(int p);\\nint\\tconstexpr(void);\\nvoid\\tcopyname(char *name, char *s);\\nint\\tderef(int p);\\nvoid\\tdumpsyms(char *title, char *sub, int from, int to);\\nvoid\\tdumptree(node *a);\\nvoid\\temittree(node *a);\\nint\\teofcheck(void);\\nvoid\\terror(char *s, char *a);\\nvoid\\texpr(int *lv, int cvoid);\\nvoid\\tfatal(char *s);\\nint\\tfindglob(char *s);\\nint\\tfindloc(char *s);\\nint\\tfindmem(int y, char *s);\\nint\\tfindstruct(char *s);\\nint\\tfindsym(char *s);\\nint\\tfindmac(char *s);\\nnode\\t*fold_reduce(node *n);\\nint\\tfrozen(int depth);\\nchar\\t*galloc(int k, int align);\\nvoid\\tgen(char *s);\\nint\\tgenadd(int p1, int p2, int swap);\\nvoid\\tgenaddr(int y);\\nvoid\\tgenalign(int k);\\nvoid\\tgenaligntext(void);\\nvoid\\tgenand(void);\\nvoid\\tgenasop(int op, int *lv, int p2);\\nvoid\\tgenbool(void);\\nvoid\\tgenbrfalse(int dest);\\nvoid\\tgenbrtrue(int dest);\\nvoid\\tgenbss(char *name, int len, int statc);\\nvoid\\tgencall(int y);\\nvoid\\tgencalr(void);\\nvoid\\tgencmp(char *inst);\\nvoid\\tgendata(void);\\nvoid\\tgendefb(int v);\\nvoid\\tgendefp(int v);\\nvoid\\tgendefs(char *s, int len);\\nvoid\\tgendefw(int v);\\nvoid\\tgendiv(int swap);\\nvoid\\tgenentry(void);\\nvoid\\tgenexit(void);\\nvoid\\tgeninc(int *lv, int inc, int pre);\\nvoid\\tgenind(int p);\\nvoid\\tgenior(void);\\nvoid\\tgenjump(int dest);\\nvoid\\tgenlab(int id);\\nvoid\\tgenldlab(int id);\\nvoid\\tgenlit(int v);\\nvoid\\tgenln(char *s);\\nvoid\\tgenlocinit(void);\\nvoid\\tgenlognot(void);\\nvoid\\tgenmod(int swap);\\nvoid\\tgenmul(void);\\nvoid\\tgenname(char *name);\\nvoid\\tgenneg(void);\\nvoid\\tgennot(void);\\nvoid\\tgenpostlude(void);\\nvoid\\tgenprelude(void);\\nvoid\\tgenpublic(char *name);\\nvoid\\tgenpush(void);\\nvoid\\tgenpushlit(int n);\\nvoid\\tgenraw(char *s);\\nvoid\\tgenrval(int *lv);\\nvoid\\tgenscale(void);\\nvoid\\tgenscale2(void);\\nvoid\\tgenscaleby(int v);\\nvoid\\tgenshl(int swap);\\nvoid\\tgenshr(int swap);\\nvoid\\tgenstack(int n);\\nvoid\\tgenstore(int *lv);\\nint\\tgensub(int p1, int p2, int swap);\\nvoid\\tgenswitch(int *vals, int *labs, int nc, int dflt);\\nvoid\\tgentext(void);\\nvoid\\tgenxor(void);\\nchar\\t*globname(char *s);\\nchar\\t*gsym(char *s);\\nvoid\\tident(void);\\nvoid\\tinit(void);\\nvoid\\tinitopt(void);\\nint\\tinttype(int p);\\nint\\tlabel(void);\\nchar\\t*labname(int id);\\nvoid\\tlbrace(void);\\nvoid\\tlgen(char *s, char *inst, int n);\\nvoid\\tlgen2(char *s, int v1, int v2);\\nvoid\\tload(void);\\nvoid\\tlparen(void);\\nnode\\t*mkbinop(int op, node *left, node *right);\\nnode\\t*mkbinop1(int op, int n, node *left, node *right);\\nnode\\t*mkbinop2(int op, int n1, int n2, node *left, node *right);\\nnode\\t*mkleaf(int op, int n);\\nnode\\t*mkunop(int op, node *left);\\nnode\\t*mkunop1(int op, int n, node *left);\\nnode\\t*mkunop2(int op, int n1, int n2, node *left);\\nvoid\\tmatch(int t, char *what);\\nchar\\t*newfilename(char *name, int sfx);\\nint\\tnext(void);\\nvoid\\tngen(char *s, char *inst, int n);\\nvoid\\tngen2(char *s, char *inst, int n, int a);\\nvoid\\tnotvoid(int p);\\nint\\tobjsize(int prim, int type, int size);\\nnode\\t*optimize(node *n);\\nvoid\\topt_init(void);\\nvoid\\tplaymac(char *s);\\nint\\tpointerto(int prim);\\nvoid\\tpreproc(void);\\nint\\tprimtype(int t, char *s);\\n\\n\\nvoid\\tprogram(char *name, FILE *in, FILE *out, char *def, char *source);\\nchar \\tstrmanager();\\n\\nvoid\\tputback(int t);\\nvoid\\tqueue_cmp(int op);\\nvoid\\trbrace(void);\\nvoid\\trbrack(void);\\nvoid\\treject(void);\\nvoid\\trexpr(void);\\nvoid\\trparen(void);\\nint\\tscan(void);\\nint\\tscanraw(void);\\nvoid\\tscnerror(char *s, int c);\\nvoid\\tsemi(void);\\nvoid\\tsgen(char *s, char *inst, char *s2);\\nvoid\\tsgen2(char *s, char *inst, int v, char *s2);\\nint\\tskip(void);\\nvoid\\tspill(void);\\nint\\tsynch(int syn);\\nvoid\\ttop(void);\\nint\\ttypematch(int p1, int p2);\\n\");\n\t\t}\n\t\t\n\t\tInclev++;\n\t\toc = next();\n\t\toline = Line;\n\t\tofile = File;\n\t\t\n\t\t\n\t\tmain_src = Insource;\n\t\tmain_src_i = Insourcei;\n\t\tmain_src_len = Insourcelen;\n\t\tInsource = incsource;\n\t\tInsourcei = 0;\n\t\tInsourcelen = strlen(incsource);\n\t\t\n\t\t\n\t\toinfile = Infile;\n\t\tLine = 1;\n\t\tputback(\'\\n\');\n\t\tFile = path;\n\t\tInfile = inc;\n\t\tToken = scan();\n\t\twhile (XEOF != Token)\n\t\t\ttop();\n\t\tLine = oline;\n\t\tFile = ofile;\n\t\tInfile = oinfile;\n\t\t\n\t\t\n\t\tInsource = main_src;\n\t\tInsourcei = main_src_i;\n\t\tInsourcelen = main_src_len;\n\t\t\n\t\t\n\t\tfclose(inc);\n\t\tputback(oc);\n\t\tInclev--;\n\t}\n}\n\nstatic void ifdef(int expect) {\n\tchar\tname[NAMELEN+1];\n\n\tif (Isp >= MAXIFDEF)\n\t\tfatal(\"too many nested \'#ifdef\'s\");\n\tToken = scanraw();\n\tcopyname(name, Text);\n\tif (IDENT != Token)\n\t\terror(\"identifier expected in \'#ifdef\'\", NULL);\n\tif (frozen(1))\n\t\tIfdefstk[Isp++] = P_IFNDEF;\n\telse if ((findmac(name) != 0) == expect)\n\t\tIfdefstk[Isp++] = P_IFDEF;\n\telse\n\t\tIfdefstk[Isp++] = P_IFNDEF;\n}\n\nstatic void p_else(void) {\n\tif (!Isp)\n\t\terror(\"\'#else\' without matching \'#ifdef\'\", NULL);\n\telse if (frozen(2))\n\t\t;\n\telse if (P_IFDEF == Ifdefstk[Isp-1])\n\t\tIfdefstk[Isp-1] = P_ELSENOT;\n\telse if (P_IFNDEF == Ifdefstk[Isp-1])\n\t\tIfdefstk[Isp-1] = P_ELSE;\n\telse\n\t\terror(\"\'#else\' without matching \'#ifdef\'\", NULL);\n\t\t\n}\n\nstatic void endif(void) {\n\tif (!Isp)\n\t\terror(\"\'#endif\' without matching \'#ifdef\'\", NULL);\n\telse\n\t\tIsp--;\n}\n\nstatic void pperror(void) {\n\tchar\tbuf[TEXTLEN+1];\n\n\tif (\'\\n\' == Putback)\n\t\tbuf[0] = 0;\n\telse\n\t\tgetln(buf, TEXTLEN-1);\n\terror(\"#error: %s\", buf);\n\texit(1);\n}\n\nstatic void setline(void) {\n\tchar\tbuf[TEXTLEN+1];\n\n\tif (\'\\n\' == Putback)\n\t\tbuf[0] = 0;\n\telse\n\t\tgetln(buf, TEXTLEN-1);\n\tLine = atoi(buf) - 1;\n}\n\nstatic void junkln(void) {\n\twhile (!feof(Infile) && fgetc(Infile) != \'\\n\')\n\t\t;\n\t\n\tLine++;\n}\n\nint frozen(int depth) {\n\treturn Isp >= depth &&\n\t\t(P_IFNDEF == Ifdefstk[Isp-depth] ||\n\t\tP_ELSENOT == Ifdefstk[Isp-depth]);\n}\n\nvoid preproc(void) {\n\tputback(\'#\');\n\tToken = scanraw();\n\tif (\tfrozen(1) &&\n\t\tP_IFDEF != Token && P_IFNDEF != Token &&\n\t\tP_ELSE != Token && P_ENDIF != Token\n\t) {\n\t\tjunkln();\n\t\treturn;\n\t}\n\tswitch (Token) {\n\tcase P_DEFINE:\tdefmac(); break;\n\tcase P_UNDEF:\tundef(); break;\n\tcase P_INCLUDE:\tinclude(); break;\n\tcase P_IFDEF:\tifdef(1); break;\n\tcase P_IFNDEF:\tifdef(0); break;\n\tcase P_ELSE:\tp_else(); break;\n\tcase P_ENDIF:\tendif(); break;\n\tcase P_ERROR:\tpperror(); break;\n\tcase P_LINE:\tsetline(); break;\n\tcase P_PRAGMA:\tjunkln(); break;\n\tdefault:\tjunkln(); break;\n\t\t\tbreak;\n\t}\n}\n");
		//printf("'%s'\n",source);
	}
	//***/
	//program(file, in, out, def);
	program(file, in, out, def, source);
	if (file) {
		fclose(in);
		//
		printf("**********VAR in is closed**********\n\n");
		//
		if (out) fclose(out);
	}
}

#endif /* !__dos */

static void collect(char *file, int temp) {
	if (O_componly || O_asmonly) return;
	if (Nf >= MAXFILES)
		cmderror("too many input files", NULL);
	Temp[Nf] = temp;
	Files[Nf++] = file;
}

static void assemble(char *file, int delete) {
	char	*ofile;
	char	cmd[TEXTLEN+1];

	file = newfilename(file, 's');
	if (O_componly && O_outfile)
		ofile = O_outfile;
	else
		collect(ofile = newfilename(file, 'o'), 1);
	if (strlen(file) + strlen(ofile) + strlen(ASCMD) >= TEXTLEN)
		cmderror("assembler command too long", NULL);
	sprintf(cmd, ASCMD, ofile, file);
	if (O_verbose > 1) printf("%s\n", cmd);
	if (system(cmd))
		cmderror("assembler invocation failed", NULL);
	if (delete) {
		if (O_verbose > 2) printf("rm %s\n", file);
		remove(file);
	}
}

static int concat(int k, char *buf, char *s) {
	int	n;

	n = strlen(s);
	if (k + n + 2 >= TEXTLEN)
		cmderror("linker command too long", buf);
	strcat(buf, " ");
	strcat(buf, s);
	return k + n + 1;
}

static void link(void) {
	int	i, k;
	char	cmd[TEXTLEN+2];
	char	cmd2[TEXTLEN+2];
	char	*ofile;

	ofile = O_outfile? O_outfile: AOUTNAME;
	if (strlen(ofile) + strlen(LDCMD) + strlen(SCCDIR)*2 >= TEXTLEN)
		cmderror("linker command too long", NULL);
	sprintf(cmd, LDCMD, ofile, SCCDIR, O_stdio? "": "n");
	k = strlen(cmd);
	for (i=0; i<Nf; i++)
		k = concat(k, cmd, Files[i]);
	k = concat(k, cmd, SCCLIBC);
	concat(k, cmd, SYSLIBC);
	sprintf(cmd2, cmd, SCCDIR);
	if (O_verbose > 1) printf("%s\n", cmd2);
	if (system(cmd2))
		cmderror("linker invocation failed", NULL);
	if (O_verbose > 2) printf("rm ");
	for (i=0; i<Nf; i++) {
		if (Temp[i]) {
			if (O_verbose > 2) printf(" %s", Files[i]);
			remove(Files[i]);
		}
	}
	if (O_verbose > 2) printf("\n");
}

static void usage(void) {
	printf("Usage: scc [-h] [-ctvNSV] [-d opt] [-o file] [-D macro[=text]]"
		" file [...]\n");
}

static void longusage(void) {
	printf("\n");
	usage();
	printf(	"\n"
		"-c       compile only, do not link\n"
		"-d opt   activate debug option OPT, ? = list\n"
		"-o file  write linker output to FILE\n"
		"-t       test only, generate no code\n"
		"-v       verbose, more v's = more verbose\n"
		"-D m=v   define macro M with optional value V\n"
		"-N       do not use stdio (can't use printf, etc)\n"
		"-S       compile to assembly language\n"
		"-V       print version and exit\n"
		"\n" );
}

static void version(void) {
	printf("SubC version %s for %s/%s\n", VERSION, OS, CPU);
}

static char *nextarg(int argc, char *argv[], int *pi, int *pj) {
	char	*s;

	if (argv[*pi][*pj+1] || *pi >= argc-1) {
		usage();
		exit(EXIT_FAILURE);
	}
	s = argv[++*pi];
	*pj = strlen(s)-1;
	return s;
}

static int dbgopt(int argc, char *argv[], int *pi, int *pj) {
	char	*s;

	s = nextarg(argc, argv, pi, pj);
	if (!strcmp(s, "lsym")) return D_LSYM;
	if (!strcmp(s, "gsym")) return D_GSYM;
	if (!strcmp(s, "stat")) return D_STAT;
	printf(	"\n"
		"scc: valid -d options are: \n\n"
		"lsym - dump local symbol tables\n"
		"gsym - dump global symbol table\n"
		"stat - print usage statistics\n"
		"\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int	i, j;
	char	*def;
	//   ./test-scc -o more-scc cg.c decl.c error.c expr.c gen.c main.c misc.c opt.c prep.c scan.c stmt.c sym.c tree.c 

	def = NULL;
	O_debug = 0;
	O_verbose = 0;
	O_componly = 0;
	O_asmonly = 0;
	O_testonly = 0;
	O_stdio = 1;
	O_outfile = NULL;
	for (i=1; i<argc; i++) {
		if (*argv[i] != '-') break;
		if (!strcmp(argv[i], "-")) {
			compile(NULL, def);
			exit(Errors? EXIT_FAILURE: EXIT_SUCCESS);
		}
		for (j=1; argv[i][j]; j++) {
			switch (argv[i][j]) {
			case 'c':
				O_componly = 1;
				break;
			case 'd':
				O_debug |= dbgopt(argc, argv, &i, &j);
				O_testonly = 1;
				break;
			case 'h':
				longusage();
				exit(EXIT_SUCCESS);
			case 'o':
				O_outfile = nextarg(argc, argv, &i, &j);
				break;
			case 't':
				O_testonly = 1;
				break;
			case 'v':
				O_verbose++;
				break;
			case 'D':
				if (def) cmderror("too many -D's", NULL);
				def = nextarg(argc, argv, &i, &j);
				break;
			case 'N':
				O_stdio = 0;
				break;
			case 'S':
				O_componly = O_asmonly = 1;
				break;
			case 'V':
				version();
				exit(EXIT_SUCCESS);
			default:
				usage();
				exit(EXIT_FAILURE);
			}
		}
	}
	if (i >= argc) {
		usage();
		exit(EXIT_FAILURE);
	}
	Nf = 0;
	while (i < argc) {
		if (filetype(argv[i]) == 'c') {
			compile(argv[i], def);
			if (Errors && !O_testonly)
				cmderror("compilation stopped", NULL);
			if (!O_asmonly && !O_testonly)
				assemble(argv[i], 1);
			i++;
		}
		else if (filetype(argv[i]) == 's') {
			if (!O_testonly) assemble(argv[i++], 0);
		}
		else {
			if (!exists(argv[i])) cmderror("no such file: %s",
							argv[i]);
			collect(argv[i++], 0);
		}
	}
	if (!O_componly && !O_testonly) link();
	return EXIT_SUCCESS;
}
