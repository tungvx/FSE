#ifndef _TOKEN_H_
#define _TOKEN_H_

#define MAX_LEXEME 255
#define MAX_LINE   1000
#define MAX_STR_BUF 10000

enum tokentype {ID=256, ILIT, CLIT, FLIT, SLIT,
ARIOP, RELOP, LOGOP, ASNOP, DUPOP, CMT, 
tCLASS, tPRIVATE, tPUBLIC, tSTATIC, tCONST,
tIF, tELSE, tSWITCH, tCASE, tDEFAULT,
tBREAK, tCONTINUE, tWHILE, tDO, tFOR, tCALL, tRETURN,
tFALSE, tTRUE, 
tEND, ERROR=-1 };

/* value of ival when sym=OP */
#define WITHEQ(c)  ((c)+0x60)
#define WITHDUP(c) ((c)+0x80)

#define EQ	 ('=')
#define PLUSEQ   WITHEQ('+')
#define MINUSEQ  WITHEQ('-')
#define STAREQ   WITHEQ('*')
#define SLASHEQ  WITHEQ('/')
#define PERCENTEQ  WITHEQ('%')

#define GT	 ('>')
#define LT	 ('<')
#define GTEQ	 WITHEQ('>')
#define LTEQ	 WITHEQ('<')
#define EQEQ	 WITHEQ('=')
#define NOTEQ	 WITHEQ('!')

#define PLUSPLUS   WITHDUP('+')
#define MINUSMINUS WITHDUP('-')
#define ANDAND     WITHDUP('&')
#define OROR       WITHDUP('|')
#define LSHIFT	   WITHDUP('<')
#define RSHIFT	   WITHDUP('>')
#define XORXOR	   WITHDUP('^')

typedef struct Token {
    enum tokentype sym;
    char text[MAX_LEXEME+1];
    int  index;
    int  ival;	/* if sym==OP, optype is stored */
    char *sval;
} Token ;

extern Token tok;

Token *gettoken(void);
void skiptoken(int);
void skiptoken2(int,int);

typedef struct Line {
    int no;
    int pos;
    int backed;		/* ch backed by backch */
    char buf[MAX_LINE];
} Line;

extern Line line;

char *insert_STR(char *);
int   get_offset_STR(char *);

void initline(void);
int nextch(void);
int prevch(void);
void clear_lexeme(void);

void backch(int);
void outch(int);
void delete_prev(void);

int getlineno(void);
int getlinepos(void);

void parse_error(const char *);

typedef struct kwentry {
    const char *text;
    int sym;
} kwentry;

kwentry *kwlookup(const char*);

#endif
