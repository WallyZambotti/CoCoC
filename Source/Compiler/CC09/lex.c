/*
     Modification history for lex.c:
        24-May-83       Changed string file to use fopen() update mode.
                        Made stringlen, sfile and datstring be DIRECT.
        25-May-83       Better register variable usage.
        09-Jun-83       Changed "out of space" to "out of memory".
        29-Aug-83       Float constants in the form 1e7 were not recognized.
        24-Apr-84 LAC   Conversion for UNIX.
*/

/*************************************************
 *                                               *
 *      SCANNER OR LEXICAL ANALYZER              *
 *                                               *
 *************************************************/

#include "cj.h"

static double normaliz();

#define     isdigit(c)  (chartab[c]==DIGIT)

typedef union {
    INTTYPE *ip;
    LONGTYPE *lp;
#ifdef DOFLOATS
    FLOATYPE *fp;
    DBLETYPE *dp;
#endif
} numptrs;

#ifdef  DOFLOATS
extern double atoftbl[];
#endif
direct int stringlen;

extern symnode *lookup();
extern char *grab();


getsym()
{
    register symnode *ptr;
    register int numtype;
    register numptrs np;
    register LONGTYPE *longp;
#ifdef  DOFLOATS
    register DBLETYPE *dblp;
    DBLETYPE n;
#else
    LONGTYPE n;
#endif
    char name[NAMESIZE+1];

    blanks();
    if (cc==EOF) {sym=EOF; return; }

    symptr=lptr-1;
    symline=lineno;
    while(!(sym=chartab[cc])) {
        error("bad character");
        getch();
        symptr=lptr;
    }
    symval=valtab[cc];

    switch(sym) {
        case LETTER:
            getword(name);
            ptr=lookup(name);
            if((sym=ptr->type)==KEYWORD){
                if((symval=ptr->storage)==SIZEOF){
                    sym=SIZEOF;
                    symval = LEV_14;    /* high level unary operator */
                }
            } else {
                sym = NAME;
                symval = (int) ptr;
            }
            break;
        case DIGIT:
#ifdef  DOFLOATS
            numtype = number(INT,&n);
#else
            numtype = number(&n);
#endif
donum:
#ifdef DOFLOATS
            np.dp = &n;
#else
            np.lp = &n;
#endif
            switch(numtype) {
                case INT:
                    symval = *np.ip;
                    sym = CONST;
                    break;
                case LONG:
                    longp = (long *) grab(sizeof(long));
                    *longp = *np.lp;
                    symval = (int) longp;
#ifdef DEBUG
                    fprintf(stderr,"getsym: symval=%04X, *longp=%08lX\n",
                            symval,*longp);
#endif
                    sym = LCONST;
                    break;
#ifdef  DOFLOATS
                case DOUBLE:
                    dblp = (double *) grab(sizeof(double));
                    *dblp = *np.dp;
                    symval = (int) dblp;
                    sym = FCONST;
                    break;
#endif
                default:
                    error("constant overflow");
                    symval = 1;
                    sym = CONST;
                    break;
            }
            break;

        case PRIME: pstr(); sym=CONST; break;
        case QUOTE: qstr(); sym=STRING; break;
        default:
            getch();
            switch(sym) {
#ifdef  DOFLOATS
                case DOT:
                    if(isdigit(cc)) {
                        numtype=number(DOUBLE,&n);
                        goto donum;
                    }
                    break;
#endif
                case AMPER :
                    switch(cc) {
                        case '&':
                            sym=DBLAND;
                            getch();
                            symval = LEV_5; /* boolean "and" level */
                            break;
                        case '=':
                            sym=ASSAND;
                            symval = LEV_2; /* assignment operator level */
                            getch();
                            break;
                    }
                    break;
                case ASSIGN:
                    if (cc=='=') {
                        sym=EQ;
                        symval = LEV_9; /* boolean "equality" level */
                        getch();
                    }
                    break;
                case OR :
                    switch (cc) {
                        case '|':
                            sym=DBLOR;
                            getch();
                            symval = LEV_4; /* boolean "or" level */
                            break;
                        case '=':
                            sym=ASSOR;
                            getch();
                            symval = LEV_2; /* assignment operator level */
                            break;
                    }
                    break;
                case NOT:
                    if(cc=='=') {
                        sym=NEQ;
                        getch();
                        symval = LEV_9; /* boolean "equality" level */
                    }
                    break;
                case STAR:
                    if (cc=='=') {
                        sym=ASSMUL;
                        getch();
                        symval = LEV_2; /* assignment operator level */
                    }
                    break;
                case DIV:
                case MOD:
                case XOR:
                    if(cc=='=') {
                        sym=sym+(ASSPLUS-PLUS);
                        getch();
                        symval = LEV_2; /* assignment operator level */
                    }
                    break;
                case LT:
                    switch (cc) {
                        case '<':
                            sym=SHL;
                            symval = LEV_11;    /* shift operator level */
                            getch();
                            if (cc=='='){
                                sym=ASSSHL;
                                symval = LEV_2; /* assignment operator level */
                                getch();
                            }
                            break;
                        case '=':
                            sym=LEQ;
                            symval = LEV_10;    /* boolean "relational" level */
                            getch();
                            break;
                    }
                    break;
                case GT:
                    switch (cc) {
                        case '>':
                            sym=SHR;
                            symval = LEV_11;    /* shift operator level */
                            getch();
                            if (cc=='=') {
                                sym=ASSSHR;
                                symval = LEV_2; /* assignment operator level */
                                getch();
                            }
                            break;
                        case '=':
                            sym=GEQ;
                            symval = LEV_10;    /* boolean "relational" level */
                            getch();
                            break;
                    }
                    break;
                case PLUS :
                    switch(cc){
                        case '+':
                            sym=INCBEF;
                            symval = LEV_14;    /* high level unary operator */
                            getch();
                            break;
                        case '=':
                            sym=ASSPLUS;
                            symval = LEV_2;     /* assignment operator level */
                            getch();
                            break;
                    }
                    break;
                case NEG:
                    switch(cc){
                        case '-':
                            sym=DECBEF;
                            symval = LEV_14;    /* high level unary operator */
                            getch();
                            break;
                        case '=':
                            sym=ASSMIN;
                            symval = LEV_2;     /* assignment operator level */
                            getch();
                            break;
                        case '>':
                            sym=ARROW;
                            symval = LEV_15;    /* top level binding */
                            getch();
                            break;
                    }
                    break;
            }
    }
    return;
}

char chartab[] = {
    0,      0,      0,      0,      0,      0,      0,      0,      /* 00-07 */
    0,      0,      0,      0,      0,      0,      0,      0,      /* 08-0f */
    0,      0,      0,      0,      0,      0,      0,      0,      /* 10-17 */
    0,      0,      0,      0,      0,      0,      0,      0,      /* 18-1f */
    SPACE,  NOT,    QUOTE,  0,      0,      MOD,    AMPER,  PRIME,  /* 20-27 */
    LPAREN, RPAREN, STAR,   PLUS,   COMMA,  NEG,    DOT,    DIV,    /* 28-2f */
    DIGIT,  DIGIT,  DIGIT,  DIGIT,  DIGIT,  DIGIT,  DIGIT,  DIGIT,  /* 30-37 */
    DIGIT,  DIGIT,  COLON,  SEMICOL,LT,     ASSIGN, GT,     QUERY,  /* 38-3f */
    0,      LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 40-47 */
    LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 48-4f */
    LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 50-57 */
    LETTER, LETTER, LETTER, LBRACK, BSLASH, RBRACK, XOR,    LETTER, /* 58-5f */
    0,      LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 60-67 */
    LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 68-6f */
    LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, /* 70-77 */
    LETTER, LETTER, LETTER, LBRACE, OR,     RBRACE, COMPL,  0,      /* 78-7f */
};

char valtab[] = {
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      LEV_9,  0,      0,      0,      LEV_13, LEV_14, 0,
/*          NOT                             MOD     AMPER */
    0,      0,      LEV_14, LEV_12, LEV_1,  LEV_14, LEV_15, LEV_13,
    /*              STAR    PLUS    COMMA   NEG     DOT     DIV */
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      LEV_3,  0,      LEV_10, LEV_2,  LEV_10, LEV_3,
/*                  COLON           LT      ASSIGN  GT      QUERY */
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      LEV_7,  0,
/*                                                  XOR */
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      LEV_6,  0,      LEV_14, 0
/*                                  OR              COMPL */
};

lexinit()
{
     register symnode *ptr;

#ifdef  DOFLOATS
     install("double",DOUBLE);
     install("float",FLOAT);
#endif
     install("typedef",TYPEDEF);
     install("static",STATIC);
     install("sizeof",SIZEOF);
     install("int",INT);
     mosflg=1;
     install("int",INT);
#ifdef  DOFLOATS
     install("float",FLOAT);
#endif
     mosflg=0;
     install("char",CHAR);
     install("short",SHORT);
     install("auto",AUTO);
     install("extern",EXTERN);
     install("direct",DIRECT);
     install("register",REG);
     install("goto",GOTO);
     install("return",RETURN);
     install("if",IF);
     install("while",WHILE);
     install("else",ELSE);
     install("switch",SWITCH);
     install("case",CASE);
     install("break",BREAK);
     install("continue",CONTIN);
     install("do",DO);
     install("default",DEFAULT);
     install("for",FOR);
     install("struct",STRUCT);
     install("union",UNION);
     install("unsigned",UNSIGN);
     install("signed", SIGN);
     install("long",LONG);
     ptr=lookup("errno");
     ptr->type=INT;
     ptr->storage=EXTERN;
     ptr->size=2;
     ptr=lookup("lseek");
     ptr->type=LONG | FUNCTION;
     ptr->storage=EXTDEF;
     ptr->size=4;
}


getword(name)
register char name[];
{
    int count;

    for (count=1; an(cc) && count<=NAMESIZE ; ++count) {
        *name++=cc;
        getch();
    }

    if(count == 2) *name++ = UNIQUE;
    *name='\0';

    while(an(cc)) getch();
}

an(c)
int c;
{
    return (chartab[c]==LETTER || chartab[c]==DIGIT);
}

direct  symnode *freesym;       /* list of free symbol table entries */

symnode *
lookup(name)
char name[];
{
    /* return a pointer to a symbol table entry for 'name'  */
    /* if one is not found create one                       */
    register symnode *nptr,**tptr,**tab;

    /* which symbol table is it in? */
    tab = mosflg ? mostab : hashtab;

    /* point to hash table entry */
    tptr = &tab[hash(name)];

    /* chunter down the list until found or run off the end */
    for (nptr = *tptr; nptr; nptr = nptr->hlink) {
        if (nptr->sname[0] == name[0]
                    && strncmp(name,nptr->sname,NAMESIZE) == 0)
            return nptr;
    }

    /* the entry cannot be found; make a new one */
    if (nptr = freesym) freesym = nptr->hlink;
    else nptr = (symnode *) grab(SYMSIZE);

    strncpy(nptr->sname,name,NAMESIZE);
    nptr->type = UNDECL;
    nptr->storage = 0;

    /* put the new entry on the front of the list;  the chance  */
    /* that a lookup is for a recently declared variable is     */
    /* high - this should therefore speed things up             */
    nptr->hlink = *tptr;
    *tptr = nptr;

    /* prepare for release to free list by 'clear()' */
    nptr->downptr = (symnode *) tptr;

    return nptr;
}


install(word,typ)
char *word;
int typ;
{
        /* put a keyword in the symbol table */
        register symnode *cptr;
        char neword[NAMESIZE+1];

        /* cut the word down to size */
        strncpy(neword,word,NAMESIZE);

        neword[NAMESIZE] = '\0';
        cptr=lookup(neword);

        cptr->type=KEYWORD;
        cptr->storage = typ;
}


hash(word)
register char *word;
{
    register int n = 0, c;

    while (c = *word++) n += c;

    return (n & 127);
}


char *
grab(size)
int size;
{
    char *oldptr;

    if ((oldptr = ALLOCATE(size)) == (char *) -1) fatal("out of memory");

    if (!lobrk) lobrk = oldptr;
    hibrk = oldptr+size;

    return oldptr;
}


#ifdef  DOFLOATS
number(type,np)
register numptrs np;
{
    long i;
    double n, scale(), normaliz();
    int exp, esign, digcount = 0;
    register char *cp;

    i = 0;
    n = 0;
    cp = (char *) &n;

    if (type == DOUBLE) goto fraction;

    if (cc == '0') {
        getch();
        if (cc == '.') {
            getch();
            goto fraction;
        }
        if (cc == 'x' || cc == 'X') {
            int c;
            getch();
            while (c = ishex(cc)) {
                i = (i << 4) + (c - (c >= 'A' ? 'A' - 10 : '0'));
                getch();
            }
        } else
            while (isoct(cc)) {
                i = (i << 3) + (cc - '0');
                getch();
            }

        if (cc == 'L' || cc == 'l') {   /* explicit long */
            getch();
            goto retlong;
        }

        if ((i & 0xFFFF0000) == 0) {    /* number is <= 16 bits */
            *np.ip = i;
            return INT;
        }

retlong:
        *np.lp = i;
        return LONG;
    }

    /* here all types still possible; collect in 8 byte integer */
    while (isdigit(cc)) {
        if (addin(&n,cc)) { /* overflow */
            getch();
            return 0;
        }
        getch();
    }

    /* if no previous dp,
     * there must be either a '.' or 'e'
     * here if it is to be double
     */
    if (cc=='.' || cc=='e' || cc=='E') { /* double */
        if (cc == '.') {
            getch();
fraction:
            while (isdigit(cc)) {
                if (addin(&n,cc)) { /* overflow */
                    getch();
                    return 0;
                }
                getch();
                ++digcount;
            }
        }

        n = normaliz(&n);

        if (cc=='E' || cc=='e') {
            esign=1;
            getch();
            if (cc=='+') getch();
            else if(cc=='-') {
                getch();
                esign=0;
            }
            exp=0;
            while(isdigit(cc)) {
                exp=exp*10 + (cc - '0');
                getch();
            }

            if (exp>=40) return 0; /* overflow */

            digcount += esign ? -exp : exp;
        }
        if (digcount<0) {
            digcount= -digcount;
            esign=1;
        } else esign=0;
        *np.dp = scale(n,digcount,esign);
        return DOUBLE;
    }

#ifdef _LIL_END
    if (*(int64_t *)&n > __INT32_MAX__) return 0;
    *np.lp = *((long*)&n);
    if (cc=='l' || cc=='L') {       /* explicitly long */
        getch();
longint:
# ifdef DEBUG
        fprintf(stderr,"number: n=%08lX\n",*((long*)&n));
# endif
        return LONG;
    } else if (*np.lp > __INT16_MAX__) {    /* implicitly long */
        goto longint;
    } else {    /*  int  */
        return INT;
    }
#else /* big endian */
    if (*((long*)&n)) return 0;     /*  overflow  */
    if (cc=='l' || cc=='L') {   /* explicitly long */
        getch();
longint:
# ifdef DEBUG
#  ifdef MWOS
        pflinit();
#  endif
        fprintf(stderr,"number: n=%08lX%08lX\n",*((long*)&n),*((long*)&n+1));
# endif	/* DEBUG */
        *np.lp = *((long*)&n+1);
        return LONG;
    } else if (*((short*)&n+2)) {
        goto longint;   /* too big for int */
    } else {
        *np.ip = *((short*)&n+3);
        return INT;
    }
#endif	/* ! _LIL_END */
}

#ifndef SPLIT

#ifndef _OS9
/*
	These functions are already in the Kreider C library, and somehow these
	versions break the native compiler.
*/
double
scale(n,power,esign)
double n;
{
	double scale0();

	if (power > 9)
		n = scale0(n,(power / 10) + 9,esign);

	return scale0(n, power % 10, esign);
}


double
scale0(n,power,esign)
double n;
{

        if (power) {
                double *p;

                p = atoftbl;
                p += power;
                return (esign ? n * *p
                              : n / *p);
        }
        return n;
}

/*
char atoftbl[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
  0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x84,
  0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x87,
  0x7A,0x00,0x00,0x00,0x00,0x00,0x00,0x8A,
  0x1C,0x40,0x00,0x00,0x00,0x00,0x00,0x8E,
  0x43,0x50,0x00,0x00,0x00,0x00,0x00,0x91,
  0x74,0x24,0x00,0x00,0x00,0x00,0x00,0x94,
  0x18,0x96,0x80,0x00,0x00,0x00,0x00,0x98,
  0x3E,0xBC,0x20,0x00,0x00,0x00,0x00,0x9B,
  0x6E,0x6B,0x28,0x00,0x00,0x00,0x00,0x9E,
  0x15,0x02,0xF9,0x00,0x00,0x00,0x00,0xA2,
  0x2D,0x78,0xEB,0xC5,0xAC,0x62,0x00,0xC3,
  0x49,0xF2,0xC9,0xCD,0x04,0x67,0x4F,0xE4,
};
*/

double atoftbl[] = {
        1,
        10,
        100,
        1000,
        10000,
        1.000000e+05 ,
        1.000000e+06,
        1.000000e+07,
        1.000000e+08,
        1.000000e+09,
        1.000000e+10,
        1.000000e+20,
        1.000000e+30,
};
#endif /* ! _OS9 */

#endif  /* !SPLIT */
#else   /* !DOFLOATS */
number(np)
register long *np;
{
       long n;
       register char *cp;

       n=0;
       cp= &n;

       if (cc=='0') {
               long n1;
               int *ip;

               getch();

               n1=0;
               if (cc=='x' || cc=='X'){
                       int c;
                       getch();
                       while(c=ishex(cc)) {
                               n1=(n1<<4)+ (c- (c>='A' ? 'A'-10 : '0'));
                               getch();
                       }
               }
               else while(isoct(cc)) {
                       n1=(n1<<3)+(cc-'0');
                       getch();
               }

               ip= &n1;
               if(cc=='L' || cc=='l') {
                       getch();
                       goto retlong;
               }

               if(*ip==0) {
                       *(int *)np=n1;
                       return INT;
               }

       retlong:
               *np = n1;
               return LONG;
       }

       /* here all types still possible; collect in 4 byte integer */
       while(isdigit(cc)) {
                n = (n * 10) + (cc - '0');
                getch();
       }

       if(cc=='l' || cc=='L') { /* explicitly long */
               getch();
       longint:
               *np = n;
               return LONG;
       }
       else if(cp[0] || cp[1]) goto longint; /* implicitly long */

       else { /* int */
               *(int *)np = (int)n;
               return INT;
       }
}
#endif  /* !DOFLOATS */


pstr()
{
        getch();

        if (cc=='\\')
                symval=dobslash();

        else {
                symval=cc;
                getch();
        }

        if (cc!='\'')
                error("unterminated character constant");
        else
                getch();
}

static direct FILE *sfile;
extern direct int datstring;

qstr()
{
        switch(datstring) {
                case 0:
                case 2:
                        if(strfile == NULL) {
#if defined(OS9) || defined(_OSK)
                                if(!(strfile = fopen(mktemp(strname),"w+")))
#else
                                if(!(strfile = fdopen(mkstemp(strname),"w+")))
#endif
                                        fatal("can't open strings file");
                        }
                        sfile = strfile;
                        break;
                case 1:
                        sfile = code;
        }

        stringlen = 0;
        if(datstring != 1) {
#ifdef  SPLIT
                putc('l',sfile);
                putw(symval = getlabel(),sfile);
#else
                fprintf(sfile,"%c%d",UNIQUE,symval = getlabel());
#endif
        }
        getch();

#ifdef  SPLIT
        putc('s',sfile);
#endif
        while(cc!='"') {
                if ((lptr-line)==0) {
                        error("unterminated string");
                        break;
                }

                if (cc=='\\')
                        oc(dobslash());
                else {
                        oc(cc);
                        getch();
                }
        }
#ifdef  SPLIT
        putc('\0',sfile);
        ++stringlen;
#else
        oc('\0');
        putc('\n',sfile);
        scount = 0;
#endif
        getch();

}


oc(c)
int c;
{
#ifdef  SPLIT
        if(c == '\0' || c == '\\')
                putc('\\',sfile);
        putc(c,sfile);
#else
/*
        fprintf(sfile,"%s%d",scount ? "," : " fcb ",c);
        if(scount++ == 16) {
                putc('\n',sfile);
                scount = 0;
        }
*/
     if(!scount)
          fprintf(sfile," fcc \"");
     if(c < 0x20 || c == '"') {    /* speedy (c is not printable || c is " */
          fprintf(sfile,"\"\n fcb $%x\n",c);
          scount=0;
     }
     else {
          putc(c,sfile);
          if(scount++ >= 75) {
               fprintf(sfile,"\"\n");
               scount=0;
          }
     }
#endif
        ++stringlen;
}


oz(n)
{
#ifdef SPLIT
     passthru();
#endif
     fprintf(sfile," rzb %d\n",n);
}


dobslash()
{
        register int c,n;

        getch();
        c=cc;
        getch();

        switch(c) {
                case 'n': return NEWLINE;
                case 'l': return LINEFEED;
                case 't': return TAB_CHAR;
                case 'b': return BACKSPACE;
                case 'v': return VTAB_CHAR;
                case 'r': return RET_CHAR;
                case 'f': return FORMFEED;
                case '\n':return ' ';
        }

        if (c=='x') {
                int c1;

                c = n = 0;
                while((c1 = ishex(cc)) && n++ < 2) {
                        c =(c<<4)+ (c1 < 'A' ? c1-'0' : c1-'A'+10);
                        getch();
                }
        }
        else if (c=='d') {
                c=n=0;
                while(isdigit(cc) && n++<3) {
                        c= c*10 + cc - '0';
                        getch();
                }
        }

        else if (isoct(c)) {
                c -= '0';
                n=0;
                while(isoct(cc) && n++<3) {
                        c = (c << 3) + (cc - '0');
                        getch();
                }
        }

        return c;
}


isoct(c)
char c;
{
        return (c<='7' && c>='0');
}


ishex(c)
char c;
{
        return ( isdigit(c) ||  ((c &=0x5f) >='A' && c<='F')) ? c : 0;
}


static double
normaliz(n)
#ifdef _LIL_END
int64_t *n;
{
    return (double)(*n);
}
#else /* big endian */
long n[];
{
    return n[0] * 4294967296. + n[1];
}
#endif

#if defined(MWOS) && defined(_BIG_END)
static numshf(n);

addin(n,c)
register char n[];
char c;
{
    register int i,x;
    char ntemp[8];

    if (numshf(n)) return 1;
    for (i = 7; i > 0; --i) ntemp[i] = n[i];
    if (numshf(n)) return 1;
    if (numshf(n)) return 1;
    x = c - '0';
    for (i = 7; i > 0; --i) {
        n[i] = x = (n[i] & 0xFF) + (ntemp[i] & 0xFF) + x;
        x >>= 8;
    }
    return x;
}


static
numshf(n)
register char n[];
{
    register int i, x = 0;

    for (i = 7; i > 0; --i) {
        n[i] = x = ((n[i] & 0xFF) << 1) | x;
        x >>= 8;
    }
    return x;
}

#else   /* portable but needs long long integers */

addin(n,c)
int64_t *n;
char c;
{
    int64_t val = *n;

    val = val * 10 + (c - '0');

    *n = val;
    if (val < 0)    /* we wrapped around */
        return 1;

    return 0; /* assume we didn't wrap around */
}

#endif
