/*
 *	NMH's Simple C Compiler, 2011,2016
 *	Lexical analysis (scanner)
 */

#include "defs.h"
#include "data.h"
#include "decl.h"
//**********************************************************************************************
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
	char tmpstr[TEXTLEN];
	// Have some checks here that var word have enough space to replace some part of it with var replacement. such as replacement len, and rest of word len.
	// Also have check if replacement exceed \0 of var word, got to append a \0 at the end if that's the case.
	replacelen = strlen(replacement);
	wordlen = strlen(word);
	newlen = (wordlen - modlen) + replacelen;
	
	if( newlen > TEXTLEN ) {	// 513 is the max size of the char* used in scanstr()'s argument.
		return 0; 			// Error here is new word is too long to be accomadated.
	}
	//WHAT ABOUT MAKING buf SMALLER? it works. I tested.
	//printf("%s, of size: %d\n", replacement,replacelen);
	//printf("Modifying length of word is: %d\n", modlen);

	j = 0;
	k = 0;
	//appending the 2nd half of var word, right after where we will end our replace.
	for(j = i + modlen; j < wordlen; ++j, ++k) {
		tmpstr[k] = word[j];
		//printf("Appending 2nd half to tmpstr[]:'%c'\n",word[j]);
	}
	tmpstr[k] = '\0'; 		// Making tmpstr a prope| 2nd half: '%s'r c-string, to avoid confusion later on.
	//printf("word: '%s' | modification len: %d\n",word,modlen);
	//printf("2nd half: '%s'\n",tmpstr);

	j = 0;
	k = 0;
	// Starting from i, copy var replacement over var word.
	for(j = i; j <= replacelen; ++ j, ++ k) {
		//printf("Replacing each char: '%c' with '%c'\n",word[j], replacement[k]);
		word[j] = replacement[k];
	}
	word[j] = '\0'; 		// Making this a proper c-string. To avoid confusion.
	// Append the rest of the string back on.
	j = strlen(word);;
	k = 0;
	tmplen = strlen(tmpstr);
	//printf("Why so low? : %d\n",tmplen);
	for(; k < tmplen; ++j, ++k) {
		//printf("Appending the rest of the word back in: '%c'\n", tmpstr[k]);
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

//**********************************************************************************************/

int next(void) {
	int	c;

	if (Putback) {
		c = Putback;
		Putback = 0;
		return c;
	}
	if (Mp) {
		if ('\0' == *Macp[Mp-1]) {
			Macp[Mp-1] = NULL;
			return Macc[--Mp];
		}
		else {
			return *Macp[Mp-1]++;
		}
	}
	c = fgetc(Infile);
	//***
	//printf("char: %c --> ascii: %d\n",c,c );
	//***
	if ('\n' == c) Line++;
	return c;
}

void putback(int c) {
	Putback = c;
}

static int hexchar(void) {
	int	c, h, n = 0, f = 0;

	while (isxdigit(c = next())) {
		h = chrpos("0123456789abcdef", tolower(c));
		n = n * 16 + h;
		f = 1;
	}
	putback(c);
	if (!f)
		error("missing digits after '\\x'", NULL);
	if (n > 255)
		error("value out of range after '\\x'", NULL);
	return n;
}

static int scanch(void) {
	int	i, c, c2;

	c = next();
	if ('\\' == c) {
		switch (c = next()) {
		case 'a': return '\a';
		case 'b': return '\b';
		case 'f': return '\f';
		case 'n': return '\n';
		case 'r': return '\r';
		case 't': return '\t';
		case 'v': return '\v';
		case '\\': return '\\';
		case '"': return '"' | 256;
		case '\'': return '\'';
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7':
			for (i = c2 = 0; isdigit(c) && c < '8'; c = next()) {
				if (++i > 3) break;
				c2 = c2 * 8 + (c - '0');
			}
			putback(c);
			return c2;
		case 'x':
			return hexchar();
		default:
			scnerror("unknown escape sequence: %s", c);
			return ' ';
		}
	}
	else {
		return c;
	}
}

static int scanint(int c) {
	int	val, radix, k, i = 0;

	val = 0;
	radix = 10;
	if ('0' == c) {
		Text[i++] = '0';
		if ((c = next()) == 'x') {
			radix = 16;
			Text[i++] = c;
			c = next();
		}
		else {
			radix = 8;
		}
	}
	while ((k = chrpos("0123456789abcdef", tolower(c))) >= 0) {
		Text[i++] = c;
		if (k >= radix)
			scnerror("invalid digit in integer literal: %s", c);
		val = val * radix + k;
		c = next();
	}
	putback(c);
	Text[i] = 0;
	return val;
}

// This functions parse anything in double quotes.
// The return values is the size of the buf, and also what's gonna be counted in the program.
// The return values also doesn't count the ending double quotes, since it's just here for the format purpose.
static int scanstr(char *buf) {
	int	i, c;
	//***
	int modi, modl, cmpl;	// modify index, modify length, compare length.
	char replacement[TEXTLEN], subword[TEXTLEN];	// The current 513 is the max number of char that *buf will take, *buf which is global variable Text[] of size 513.
	modi = 0; cmpl = 0; 
	strcpy(replacement, "Good Bye");
	strcpy(subword, "Hello");
	modl = strlen(subword);
	//***

	buf[0] = '"';
	for (i=1; i<TEXTLEN-2; i++) {
		if ((c = scanch()) == '"') {
			buf[i++] = '"';
			buf[i] = 0;
			//***
			// Now I got to change the size of buf, to accomadate the edits I want to insert. The edits are fine, but it's cut off when this fct returns.
			if(subwordcheck(buf,subword,&modi,&cmpl)) {
				modstr(buf,modi,modl,replacement);
			}
			return Value = (strlen(buf)); 	// Was thinkg it's gonna be strlen(buf) + 1. but I did the math, it's not, it's just strlen(buf)..
			//***
			//return Value = i;
		}
		buf[i] = c;
	}
	fatal("string literal too long");
	return 0;
}

static int scanident(int c, char *buf, int lim) {
	int	i = 0;

	while (isalpha(c) || isdigit(c) || '_' == c) {
		if (lim-1 == i) {
			error("identifier too long", NULL);
			i++;
		}
		else if (i < lim-1) {
			buf[i++] = c;
		}
		c = next();
	}
	putback(c);
	buf[i] = 0;
	return i;
}

int skip(void) {
	int	c, p, nl;

	c = next();
	nl = 0;
	for (;;) {
		if (EOF == c) {
			strcpy(Text, "<EOF>");
			return EOF;
		}
		while (' ' == c || '\t' == c || '\n' == c ||
			'\r' == c || '\f' == c
		) {
			if ('\n' == c) nl = 1;
			c = next();
		}
		if (nl && c == '#') {
			preproc();
			c = next();
			continue;
		}
		nl = 0;
		if (c != '/')
			break;
		c = next();
		if (c != '*' && c != '/') {
			putback(c);
			c = '/';
			break;
		}
		if (c == '/') {
			while ((c = next()) != EOF) {
				if (c == '\n') break;
			}
                }
                else {
			p = 0;
			while ((c = next()) != EOF) {
				if ('/' == c && '*' == p) {
					c = next();
					break;
				}
				p = c;
			}
		}
	}
	return c;
}

static int keyword(char *s) {
	switch (*s) {
	case '#':
		switch (s[1]) {
		case 'd':
			if (!strcmp(s, "#define")) return P_DEFINE;
			break;
		case 'e':
			if (!strcmp(s, "#else")) return P_ELSE;
			if (!strcmp(s, "#endif")) return P_ENDIF;
			if (!strcmp(s, "#error")) return P_ERROR;
			break;
		case 'i':
			if (!strcmp(s, "#ifdef")) return P_IFDEF;
			if (!strcmp(s, "#ifndef")) return P_IFNDEF;
			if (!strcmp(s, "#include")) return P_INCLUDE;
			break;
		case 'l':
			if (!strcmp(s, "#line")) return P_LINE;
			break;
		case 'p':
			if (!strcmp(s, "#pragma")) return P_PRAGMA;
			break;
		case 'u':
			if (!strcmp(s, "#undef")) return P_UNDEF;
			break;
		}
		break;
	case 'a':
		if (!strcmp(s, "auto")) return AUTO;
		break;
	case 'b':
		if (!strcmp(s, "break")) return BREAK;
		break;
	case 'c':
		if (!strcmp(s, "case")) return CASE;
		if (!strcmp(s, "char")) return CHAR;
		if (!strcmp(s, "continue")) return CONTINUE;
		break;
	case 'd':
		if (!strcmp(s, "default")) return DEFAULT;
		if (!strcmp(s, "do")) return DO;
		break;
	case 'e':
		if (!strcmp(s, "else")) return ELSE;
		if (!strcmp(s, "enum")) return ENUM;
		if (!strcmp(s, "extern")) return EXTERN;
		break;
	case 'f':
		if (!strcmp(s, "for")) return FOR;
		break;
	case 'i':
		if (!strcmp(s, "if")) return IF;
		if (!strcmp(s, "int")) return INT;
		break;
	case 'r':
		if (!strcmp(s, "register")) return REGISTER;
		if (!strcmp(s, "return")) return RETURN;
		break;
	case 's':
		if (!strcmp(s, "sizeof")) return SIZEOF;
		if (!strcmp(s, "static")) return STATIC;
		if (!strcmp(s, "struct")) return STRUCT;
		if (!strcmp(s, "switch")) return SWITCH;
		break;
	case 'u':
		if (!strcmp(s, "union")) return UNION;
		break;
	case 'v':
		if (!strcmp(s, "void")) return VOID;
		if (!strcmp(s, "volatile")) return VOLATILE;
		break;
	case 'w':
		if (!strcmp(s, "while")) return WHILE;
		break;
	}
	return 0;
}

static int macro(char *name) {
	int	y;

	y = findmac(name);
	if (!y || Types[y] != TMACRO)
		return 0;
	playmac(Mtext[y]);
	return 1;
}

static int scanpp(void) {
	int	c, t;

	if (Rejected != -1) {
		t = Rejected;
		Rejected = -1;
		strcpy(Text, Rejtext);
		Value = Rejval;
		return t;
	}
	for (;;) {
		Value = 0;
		c = skip();
		memset(Text, 0, 4);
		Text[0] = c;
		switch (c) {
		case '!':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return NOTEQ;
			}
			else {
				putback(c);
				return XMARK;
			}
		case '%':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return ASMOD;
			}
			else {
				putback(c);
				return MOD;
			}
		case '&':
			if ((c = next()) == '&') {
				Text[1] = '&';
				return LOGAND;
			}
			else if ('=' == c) {
				Text[1] = '=';
				return ASAND;
			}
			else {
				putback(c);
				return AMPER;
			}
		case '(':
			return LPAREN;
		case ')':
			return RPAREN;
		case '*':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return ASMUL;
			}
			else {
				putback(c);
				return STAR;
			}
		case '+':
			if ((c = next()) == '+') {
				Text[1] = '+';
				return INCR;
			}
			else if ('=' == c) {
				Text[1] = '=';
				return ASPLUS;
			}
			else {
				putback(c);
				return PLUS;
			}
		case ',':
			return COMMA;
		case '-':
			if ((c = next()) == '-') {
				Text[1] = '-';
				return DECR;
			}
			else if ('=' == c) {
				Text[1] = '=';
				return ASMINUS;
			}
			else if ('>' == c) {
				Text[1] = '>';
				return ARROW;
			}
			else {
				putback(c);
				return MINUS;
			}
		case '/':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return ASDIV;
			}
			else {
				putback(c);
				return SLASH;
			}
		case ':':
			return COLON;
		case ';':
			return SEMI;
		case '<':
			if ((c = next()) == '<') {
				Text[1] = '<';
				if ((c = next()) == '=') {
					Text[2] = '=';
					return ASLSHIFT;
				}
				else {
					putback(c);
					return LSHIFT;
				}
			}
			else if ('=' == c) {
				Text[1] = '=';
				return LTEQ;
			}
			else {
				putback(c);
				return LESS;
			}
		case '=':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return EQUAL;
			}
			else {
				putback(c);
				return ASSIGN;
			}
		case '>':
			if ((c = next()) == '>') {
				Text[1] = '>';
				if ((c = next()) == '=') {
					Text[1] = '=';
					return ASRSHIFT;
				}
				else {
					putback(c);
					return RSHIFT;
				}
			}
			else if ('=' == c) {
				Text[1] = '=';
				return GTEQ;
			}
			else {
				putback(c);
				return GREATER;
			}
		case '?':
			return QMARK;
		case '[':
			return LBRACK;
		case ']':
			return RBRACK;
		case '^':
			if ((c = next()) == '=') {
				Text[1] = '=';
				return ASXOR;
			}
			else {
				putback(c);
				return CARET;
			}
		case '{':
			return LBRACE;
		case '|':
			if ((c = next()) == '|') {
				Text[1] = '|';
				return LOGOR;
			}
			else if ('=' == c) {
				Text[1] = '=';
				return ASOR;
			}
			else {
				putback(c);
				return PIPE;
			}
		case '}':
			return RBRACE;
		case '~':
			return TILDE;
		case EOF:
			strcpy(Text, "<EOF>");
			return XEOF;
		case '\'':
			Text[1] = Value = scanch();
			if ((c = next()) != '\'')
				error(
				 "expected '\\'' at end of char literal",
					NULL);
			Text[2] = '\'';
			return INTLIT;
		case '"':
			Value = scanstr(Text);
			return STRLIT;
		case '#':
			Text[0] = '#';
			scanident(next(), &Text[1], TEXTLEN-1);
			if ((t = keyword(Text)) != 0) 
				return t; 
			error("unknown preprocessor command: %s", Text);
			return IDENT;
		case '.':
			if ((c = next()) == '.') {
				Text[1] = Text[2] = '.';
				Text[3] = 0;
				if ((c = next()) == '.')
					return ELLIPSIS;
				putback(c);
				error("incomplete '...'", NULL);
				return ELLIPSIS;
			}
			putback(c);
			return DOT;
		default:
			if (isdigit(c)) {
				Value = scanint(c);
				return INTLIT;
			}
			else if (isalpha(c) || '_' == c) {
				Value = scanident(c, Text, TEXTLEN);
				if (Expandmac && macro(Text))
					break;
				if ((t = keyword(Text)) != 0)
					return t;
				return IDENT;
			}
			else {
				scnerror("funny input character: %s", c);
				break;
			}
		}
	}
}

int scan(void) {
	int	t;

	do {
		t = scanpp();
		if (!Inclev && Isp && XEOF == t)
			fatal("missing '#endif'");
	} while (frozen(1));
	if (t == Syntoken)
		Syntoken = 0;
	return t;
}

int scanraw(void) {
	int	t, oisp;

	oisp = Isp;
	Isp = 0;
	Expandmac = 0;
	t = scan();
	Expandmac = 1;
	Isp = oisp;
	return t;
}

void reject(void) {
	Rejected = Token;
	Rejval = Value;
	strcpy(Rejtext, Text);
}
