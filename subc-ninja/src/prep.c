/*
 *	NMH's Simple C Compiler, 2011,2012,2014
 *	Preprocessor
 */

#include "defs.h"
#include "data.h"
#include "decl.h"

void playmac(char *s) {
	if (Mp >= MAXNMAC) fatal("too many nested macros");
	Macc[Mp] = next();
	Macp[Mp++] = s;
}

int getln(char *buf, int max) {	//For some reason this is not being called in INCLUDE(). FInd all instance of it being called and check behavior.
	int	k;
	//**********************************************************************************************
	char c;
	int i;
	//**********************************************************************************************/

	//if (fgets(buf, max, Infile) == NULL) return 0;
	//**********************************************************************************************
	//strcpy("",buf);		// CAUSE SEG FAULT for going through buf.
	for( i = 0; i < max - 1 ; ++i ) {
		c = strmanager();
		buf[i] = c;
		if( c == '\n' || c == EOF ) {
			++i;			// To imitate last increment that would have been done had this condition not run.
			break;
		}
	}
	buf[i] = '\0';
	if( strlen(buf) == 0 ) 
		return 0;
	//**********************************************************************************************/
	k = strlen(buf);
	if (k) buf[--k] = 0;						//Get rids of '/n' and make it a proper c-string with null terminator.
	if (k && '\r' == buf[k-1]) buf[--k] = 0;	//Get rids of '/r' if there is '\r' and make it a proper c-string with null terminator.	
	return k;
}

static void defmac(void) {
	char	name[NAMELEN+1];
	char	buf[TEXTLEN+1], *p;
	int	y;

	Token = scanraw();
	if (Token != IDENT)
		error("identifier expected after '#define': %s", Text);
	copyname(name, Text);
	if ('\n' == Putback)
		buf[0] = 0;
	else
		getln(buf, TEXTLEN-1);
	for (p = buf; isspace(*p); p++)
		;
	if ((y = findmac(name)) != 0) {
		if (strcmp(Mtext[y], buf))
			error("macro redefinition: %s", name);
	}
	else {
		addglob(name, 0, TMACRO, 0, 0, 0, globname(p), 0);
	}
	Line++;
}

static void undef(void) {
	char	name[NAMELEN+1];
	int	y;

	Token = scanraw();
	copyname(name, Text);
	if (IDENT != Token)
		error("identifier expected after '#undef': %s", Text);
	if ((y = findmac(name)) != 0)
		Names[y] = "#undef'd";
}

static void include(void) {
	char	file[TEXTLEN+1], path[TEXTLEN+1];
	int	c, k;
	FILE	*inc, *oinfile;
	char	*ofile;
	int	oc, oline;
	//**********************************************************************************************
	char incsource[TEXTLEN]; // Should be able to account for most subc .c files.
	FILE *inctmp;
	int tmpi;
	char *main_src;
	int main_src_i, main_src_len;
	//**********************************************************************************************/

	if ((c = skip()) == '<')
		c = '>';
	k = getln(file, TEXTLEN-strlen(SCCDIR)-9);	//file will now be the name of the included file.
	Line++;
	if (!k || file[k-1] != c)					// if k is 0, or file[k-1] != '>'
		error("missing delimiter in '#include'", NULL);
	if (k) file[k-1] = 0;						// Just making sure file is a proper c-string with terminating null.
	if (c == '"')
		strcpy(path, file);
	else {
		strcpy(path, SCCDIR);
		strcat(path, "/include/");
		strcat(path, file);
	}
	if ((inc = fopen(path, "r")) == NULL)
		error("cannot open include file: '%s'", path);
	else {
		// CHECK FOR HEADER HERE to replace oneliner.
		// Converts FILE to string.
		//**********************************************************************************************
		inctmp = fopen(path,"r");
		//strcpy("",incsource);	//format it to be a empty proper c-string.		CAUSING SEG-FAULT FOR while loop that follows this.
		tmpi = 0;
		while( (incsource[tmpi] = fgetc(inctmp)) != EOF ) {
			++tmpi;
		}
		incsource[tmpi] ='\0';	// reenforce it to be a proper c-string and replaces EOF to \0.
		fclose(inctmp);
		//**********************************************************************************************/
		// Hardcoded version is here.
		//**********************************************************************************************
		if(!strcmp( file, "data.h" )) {
			strcpy(incsource, "\n\n#ifndef extern_\n #define extern_ extern\n#endif\n\n\n\nextern_ char *Insource;\nextern_ int Insourcei;\nextern_ int Insourcelen;\n\nextern_ FILE\t*Infile;\nextern_ FILE\t*Outfile;\nextern_ int\tToken;\nextern_ char\tText[TEXTLEN+1];\nextern_ int\tValue;\nextern_ int\tLine;\nextern_ int\tErrors;\nextern_ int\tSyntoken;\nextern_ int\tPutback;\nextern_ int\tRejected;\nextern_ int\tRejval;\nextern_ char\tRejtext[TEXTLEN+1];\nextern_ char\t*File;\nextern_ char\t*Basefile;\nextern_ char\t*Macp[MAXNMAC];\nextern_ int\tMacc[MAXNMAC];\nextern_ int\tMp;\nextern_ int\tExpandmac;\nextern_ int\tIfdefstk[MAXIFDEF], Isp;\nextern_ int\tInclev;\nextern_ int\tTextseg;\nextern_ int\tNodes[NODEPOOLSZ];\nextern_ int\tNdtop;\nextern_ int\tNdmax;\n\n\nextern_ char\t*Names[NSYMBOLS];\nextern_ int\tPrims[NSYMBOLS];\nextern_ char\tTypes[NSYMBOLS];\nextern_ char\tStcls[NSYMBOLS];\nextern_ int\tSizes[NSYMBOLS];\nextern_ int\tVals[NSYMBOLS];\nextern_ char\t*Mtext[NSYMBOLS];\nextern_ int\tGlobs;\nextern_ int\tLocs;\n\nextern_ int\tThisfn;\n\n\nextern_ char\tNlist[POOLSIZE];\nextern_ int\tNbot;\nextern_ int\tNtop;\n\n\nextern_ int\tBreakstk[MAXBREAK], Bsp;\nextern_ int\tContstk[MAXBREAK], Csp;\nextern_ int\tRetlab;\n\n\nextern_ int\tLIaddr[MAXLOCINIT];\nextern_ int\tLIval[MAXLOCINIT];\nextern_ int\tNli;\n\n\nextern_ int\tQ_type;\nextern_ int\tQ_val;\nextern_ char\tQ_name[NAMELEN+1];\nextern_ int\tQ_cmp;\nextern_ int\tQ_bool;\n\n\nextern_ char\t*Files[MAXFILES];\nextern_ char\tTemp[MAXFILES];\nextern_ int\tNf;\n\n\nextern_ int\tO_verbose;\nextern_ int\tO_componly;\nextern_ int\tO_asmonly;\nextern_ int\tO_testonly;\nextern_ int\tO_stdio;\nextern_ char\t*O_outfile;\nextern_ int\tO_debug;\n");
		}
		if(!strcmp( file, "defs.h" )) {
			strcpy(incsource,"\n\n#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n#include <ctype.h>\n#include \"cg.h\"\n#include \"sys.h\"\n\n#define VERSION\t\t\"2016-12-12\"\n\n#ifndef SCCDIR\n #define SCCDIR\t\t\".\"\n#endif\n\n#ifndef AOUTNAME\n #define AOUTNAME\t\"a.out\"\n#endif\n\n#define SCCLIBC\t\t\"%s/lib/libscc.a\"\n\n#define PREFIX\t\t\'C\'\n#define LPREFIX\t\t\'L\'\n\n#define INTSIZE\t\tBPW\n#define PTRSIZE\t\tINTSIZE\n#define CHARSIZE\t1\n\n\n\n#define TEXTLEN\t\t200000\n\n#define NAMELEN\t\t16\n\n#define MAXFILES\t32\n\n#define MAXIFDEF\t16\n#define MAXNMAC\t\t32\n#define MAXCASE\t\t256\n#define MAXBREAK\t16\n#define MAXLOCINIT\t32\n#define MAXFNARGS\t32\n\n\n#define NSYMBOLS\t1024\n#define POOLSIZE\t16384\n#define NODEPOOLSZ\t4096\t\n\n\nenum {\n\tTVARIABLE = 1,\n\tTARRAY,\n\tTFUNCTION,\n\tTCONSTANT,\n\tTMACRO,\n\tTSTRUCT\n};\n\n\nenum {\n\tPCHAR = 1,\n\tPINT,\n\tCHARPTR,\n\tINTPTR,\n\tCHARPP,\n\tINTPP,\n\tPVOID,\n\tVOIDPTR,\n\tVOIDPP,\n\tFUNPTR,\n\tPSTRUCT = 0x2000,\n\tPUNION  = 0x4000,\n\tSTCPTR  = 0x6000,\n\tSTCPP   = 0x8000,\n\tUNIPTR  = 0xA000,\n\tUNIPP   = 0xC000,\n\tSTCMASK = 0xE000\n};\n\n\nenum {\n\tCPUBLIC = 1,\n\tCEXTERN,\n\tCSTATIC,\n\tCLSTATC,\n\tCAUTO,\n\tCSPROTO,\n\tCMEMBER,\n\tCSTCDEF\n};\n\n\nenum {\n\tLVSYM,\n\tLVPRIM,\n\tLVADDR,\n\tLV\n};\n\n\nenum {\n\tD_LSYM = 1,\n\tD_GSYM = 2,\n\tD_STAT = 4\n};\n\n\nenum {\n\tempty,\n\taddr_auto,\n\taddr_static,\n\taddr_globl,\n\taddr_label,\n\tliteral,\n\tauto_byte,\n\tauto_word,\n\tstatic_byte,\n\tstatic_word,\n\tglobl_byte,\n\tglobl_word\n};\n\n\nenum {\n\tcnone,\n\tequal,\n\tnot_equal,\n\tless,\n\tgreater,\n\tless_equal,\n\tgreater_equal,\n\tbelow,\n\tabove,\n\tbelow_equal,\n\tabove_equal\n};\n\n\nenum {\n\tbnone,\n\tlognot,\n\tnormalize\n};\n\n\nstruct node_stc {\n\tint\t\top;\n\tstruct node_stc\t*left, *right;\n\tint\t\targs[1];\n};\n\n#define node\tstruct node_stc\n\n\nenum {\n\tSLASH, STAR, MOD, PLUS, MINUS, LSHIFT, RSHIFT,\n\tGREATER, GTEQ, LESS, LTEQ, EQUAL, NOTEQ, AMPER,\n\tCARET, PIPE, LOGAND, LOGOR,\n\n\tARROW, ASAND, ASXOR, ASLSHIFT, ASMINUS, ASMOD, ASOR, ASPLUS,\n\tASRSHIFT, ASDIV, ASMUL, ASSIGN, AUTO, BREAK, CASE, CHAR, COLON,\n\tCOMMA, CONTINUE, DECR, DEFAULT, DO, DOT, ELLIPSIS, ELSE, ENUM,\n\tEXTERN, FOR, IDENT, IF, INCR, INT, INTLIT, LBRACE, LBRACK,\n\tLPAREN, NOT, QMARK, RBRACE, RBRACK, REGISTER, RETURN, RPAREN,\n\tSEMI, SIZEOF, STATIC, STRLIT, STRUCT, SWITCH, TILDE, UNION,\n\tVOID, VOLATILE, WHILE, XEOF, XMARK,\n\n\tP_DEFINE, P_ELSE, P_ELSENOT, P_ENDIF, P_ERROR, P_IFDEF,\n\tP_IFNDEF, P_INCLUDE, P_LINE, P_PRAGMA, P_UNDEF\n};\n\n\nenum {\n\tOP_GLUE, OP_ADD, OP_ADDR, OP_ASSIGN, OP_BINAND, OP_BINIOR,\n\tOP_BINXOR, OP_BOOL, OP_BRFALSE, OP_BRTRUE, OP_CALL, OP_CALR,\n\tOP_COMMA, OP_DEC, OP_DIV, OP_EQUAL, OP_GREATER, OP_GTEQ,\n\tOP_IDENT, OP_IFELSE, OP_LAB, OP_LDLAB, OP_LESS, OP_LIT,\n\tOP_LOGNOT, OP_LSHIFT, OP_LTEQ, OP_MOD, OP_MUL, OP_NEG,\n\tOP_NOT, OP_NOTEQ, OP_PLUS, OP_PREDEC, OP_PREINC, OP_POSTDEC,\n\tOP_POSTINC, OP_RSHIFT, OP_RVAL, OP_SCALE, OP_SCALEBY, OP_SUB\n};\n\n");
		}
		if(!strcmp( file, "decl.h" )) {
			strcpy(incsource,"\n\nint\taddglob(char *name, int prim, int type, int scls, int size, int val,\n\t\tchar *mval, int init);\nint\taddloc(char *name, int prim, int type, int scls, int size, int val,\n\t\tint init);\nint\tbinoptype(int op, int p1, int p2);\nint\tchrpos(char *s, int c);\nvoid\tclear(int q);\nvoid\tcleanup(void);\nvoid\tclrlocs(void);\nvoid\tcolon(void);\nvoid\tcommit(void);\nvoid\tcommit_bool(void);\nvoid\tcommit_cmp(void);\nvoid\tcompound(int lbr);\nint\tcomptype(int p);\nint\tconstexpr(void);\nvoid\tcopyname(char *name, char *s);\nint\tderef(int p);\nvoid\tdumpsyms(char *title, char *sub, int from, int to);\nvoid\tdumptree(node *a);\nvoid\temittree(node *a);\nint\teofcheck(void);\nvoid\terror(char *s, char *a);\nvoid\texpr(int *lv, int cvoid);\nvoid\tfatal(char *s);\nint\tfindglob(char *s);\nint\tfindloc(char *s);\nint\tfindmem(int y, char *s);\nint\tfindstruct(char *s);\nint\tfindsym(char *s);\nint\tfindmac(char *s);\nnode\t*fold_reduce(node *n);\nint\tfrozen(int depth);\nchar\t*galloc(int k, int align);\nvoid\tgen(char *s);\nint\tgenadd(int p1, int p2, int swap);\nvoid\tgenaddr(int y);\nvoid\tgenalign(int k);\nvoid\tgenaligntext(void);\nvoid\tgenand(void);\nvoid\tgenasop(int op, int *lv, int p2);\nvoid\tgenbool(void);\nvoid\tgenbrfalse(int dest);\nvoid\tgenbrtrue(int dest);\nvoid\tgenbss(char *name, int len, int statc);\nvoid\tgencall(int y);\nvoid\tgencalr(void);\nvoid\tgencmp(char *inst);\nvoid\tgendata(void);\nvoid\tgendefb(int v);\nvoid\tgendefp(int v);\nvoid\tgendefs(char *s, int len);\nvoid\tgendefw(int v);\nvoid\tgendiv(int swap);\nvoid\tgenentry(void);\nvoid\tgenexit(void);\nvoid\tgeninc(int *lv, int inc, int pre);\nvoid\tgenind(int p);\nvoid\tgenior(void);\nvoid\tgenjump(int dest);\nvoid\tgenlab(int id);\nvoid\tgenldlab(int id);\nvoid\tgenlit(int v);\nvoid\tgenln(char *s);\nvoid\tgenlocinit(void);\nvoid\tgenlognot(void);\nvoid\tgenmod(int swap);\nvoid\tgenmul(void);\nvoid\tgenname(char *name);\nvoid\tgenneg(void);\nvoid\tgennot(void);\nvoid\tgenpostlude(void);\nvoid\tgenprelude(void);\nvoid\tgenpublic(char *name);\nvoid\tgenpush(void);\nvoid\tgenpushlit(int n);\nvoid\tgenraw(char *s);\nvoid\tgenrval(int *lv);\nvoid\tgenscale(void);\nvoid\tgenscale2(void);\nvoid\tgenscaleby(int v);\nvoid\tgenshl(int swap);\nvoid\tgenshr(int swap);\nvoid\tgenstack(int n);\nvoid\tgenstore(int *lv);\nint\tgensub(int p1, int p2, int swap);\nvoid\tgenswitch(int *vals, int *labs, int nc, int dflt);\nvoid\tgentext(void);\nvoid\tgenxor(void);\nchar\t*globname(char *s);\nchar\t*gsym(char *s);\nvoid\tident(void);\nvoid\tinit(void);\nvoid\tinitopt(void);\nint\tinttype(int p);\nint\tlabel(void);\nchar\t*labname(int id);\nvoid\tlbrace(void);\nvoid\tlgen(char *s, char *inst, int n);\nvoid\tlgen2(char *s, int v1, int v2);\nvoid\tload(void);\nvoid\tlparen(void);\nnode\t*mkbinop(int op, node *left, node *right);\nnode\t*mkbinop1(int op, int n, node *left, node *right);\nnode\t*mkbinop2(int op, int n1, int n2, node *left, node *right);\nnode\t*mkleaf(int op, int n);\nnode\t*mkunop(int op, node *left);\nnode\t*mkunop1(int op, int n, node *left);\nnode\t*mkunop2(int op, int n1, int n2, node *left);\nvoid\tmatch(int t, char *what);\nchar\t*newfilename(char *name, int sfx);\nint\tnext(void);\nvoid\tngen(char *s, char *inst, int n);\nvoid\tngen2(char *s, char *inst, int n, int a);\nvoid\tnotvoid(int p);\nint\tobjsize(int prim, int type, int size);\nnode\t*optimize(node *n);\nvoid\topt_init(void);\nvoid\tplaymac(char *s);\nint\tpointerto(int prim);\nvoid\tpreproc(void);\nint\tprimtype(int t, char *s);\n\n\nvoid\tprogram(char *name, FILE *in, FILE *out, char *def, char *source);\nchar \tstrmanager();\n\nvoid\tputback(int t);\nvoid\tqueue_cmp(int op);\nvoid\trbrace(void);\nvoid\trbrack(void);\nvoid\treject(void);\nvoid\trexpr(void);\nvoid\trparen(void);\nint\tscan(void);\nint\tscanraw(void);\nvoid\tscnerror(char *s, int c);\nvoid\tsemi(void);\nvoid\tsgen(char *s, char *inst, char *s2);\nvoid\tsgen2(char *s, char *inst, int v, char *s2);\nint\tskip(void);\nvoid\tspill(void);\nint\tsynch(int syn);\nvoid\ttop(void);\nint\ttypematch(int p1, int p2);\n");
		}
		//**********************************************************************************************/
		Inclev++;
		oc = next();
		oline = Line;
		ofile = File;
		// Back up original Insource and replace it with a temporary source.
		//**********************************************************************************************
		main_src = Insource;
		main_src_i = Insourcei;
		main_src_len = Insourcelen;
		Insource = incsource;
		Insourcei = 0;
		Insourcelen = strlen(incsource);
		//**********************************************************************************************/
		oinfile = Infile;
		Line = 1;
		putback('\n');
		File = path;
		Infile = inc;
		Token = scan();
		while (XEOF != Token)
			top();
		Line = oline;
		File = ofile;
		Infile = oinfile;
		// Pointing Insource back to the original source that was being parsed.
		//**********************************************************************************************
		Insource = main_src;
		Insourcei = main_src_i;
		Insourcelen = main_src_len;
		//**********************************************************************************************/
		fclose(inc);
		putback(oc);
		Inclev--;
	}
}

static void ifdef(int expect) {
	char	name[NAMELEN+1];

	if (Isp >= MAXIFDEF)
		fatal("too many nested '#ifdef's");
	Token = scanraw();
	copyname(name, Text);
	if (IDENT != Token)
		error("identifier expected in '#ifdef'", NULL);
	if (frozen(1))
		Ifdefstk[Isp++] = P_IFNDEF;
	else if ((findmac(name) != 0) == expect)
		Ifdefstk[Isp++] = P_IFDEF;
	else
		Ifdefstk[Isp++] = P_IFNDEF;
}

static void p_else(void) {
	if (!Isp)
		error("'#else' without matching '#ifdef'", NULL);
	else if (frozen(2))
		;
	else if (P_IFDEF == Ifdefstk[Isp-1])
		Ifdefstk[Isp-1] = P_ELSENOT;
	else if (P_IFNDEF == Ifdefstk[Isp-1])
		Ifdefstk[Isp-1] = P_ELSE;
	else
		error("'#else' without matching '#ifdef'", NULL);
		
}

static void endif(void) {
	if (!Isp)
		error("'#endif' without matching '#ifdef'", NULL);
	else
		Isp--;
}

static void pperror(void) {
	char	buf[TEXTLEN+1];

	if ('\n' == Putback)
		buf[0] = 0;
	else
		getln(buf, TEXTLEN-1);
	error("#error: %s", buf);
	exit(1);
}

static void setline(void) {
	char	buf[TEXTLEN+1];

	if ('\n' == Putback)
		buf[0] = 0;
	else
		getln(buf, TEXTLEN-1);
	Line = atoi(buf) - 1;
}

static void junkln(void) {
	while (!feof(Infile) && fgetc(Infile) != '\n')
		;
	/**********************************************************************************************
	while( (Insourcei != Insourcelen) && (strmanager() != '\n') )
	//**********************************************************************************************/
	Line++;
}

int frozen(int depth) {
	return Isp >= depth &&
		(P_IFNDEF == Ifdefstk[Isp-depth] ||
		P_ELSENOT == Ifdefstk[Isp-depth]);
}

void preproc(void) {
	putback('#');
	Token = scanraw();
	if (	frozen(1) &&
		P_IFDEF != Token && P_IFNDEF != Token &&
		P_ELSE != Token && P_ENDIF != Token
	) {
		junkln();
		return;
	}
	switch (Token) {
	case P_DEFINE:	defmac(); break;
	case P_UNDEF:	undef(); break;
	case P_INCLUDE:	include(); break;
	case P_IFDEF:	ifdef(1); break;
	case P_IFNDEF:	ifdef(0); break;
	case P_ELSE:	p_else(); break;
	case P_ENDIF:	endif(); break;
	case P_ERROR:	pperror(); break;
	case P_LINE:	setline(); break;
	case P_PRAGMA:	junkln(); break;
	default:	junkln(); break;
			break;
	}
}
