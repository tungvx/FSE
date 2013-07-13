#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "type.h"
#include "token.h"

Token tok;
Line line;

int getlineno()  { return line.no; }
int getlinepos() { return line.pos; }

void clear_lexeme() { tok.index = 0; }
void delete_prev() { --tok.index; }
void outch(int ch) { tok.text[tok.index++] = ch; }
int  prevch() { return (tok.index>0) ? (tok.text[tok.index-1]) : '\n'; }

static char *s_buf;
static char *s_ptr;
static char *s_limit;

char *insert_STR(char *s) {
    int i;
    int len = strlen(s);
    char *p;
    for (p=s_ptr, i=0; i<len && p<s_limit; i++)
        p[i] = *s++;
    p[i++] = 0;
    s_ptr += i;
    return p;
}

inline int get_STR_offset(char *p) { return p - s_buf; }

void dump_STR(FILE *fp) {
    char *s;
    int i = 0;
    fprintf(fp,"STR:cnt=%d", s_ptr - s_buf);
    for (s=s_buf; s < s_ptr; s++) {
        if (i++%16 == 0) fprintf(fp,"\n");
        fprintf(fp,"%02x ", *s);
    }
    fprintf(fp,"\n");
}

void initline() {
    line.pos = 0;
    line.no = 0;
    line.backed = 0;
    line.buf[line.pos] = '\n'; /* dummy */

    s_ptr = s_buf = malloc(MAX_STR_BUF);
    s_limit = s_buf + MAX_STR_BUF-1;
    *s_ptr = 0;
}

int nextch() {
    Line *p = &line;
    char *r;
    int ch;

    if ((ch = p->backed) != 0) {
            p->backed = 0;
    } else {
	    ch = p->buf[p->pos++];
	    if (ch == '\n') {
		p->pos = 0;
		r = fgets(p->buf,MAX_LINE,stdin);
		if (r == 0) { p->buf[0] = EOF; return EOF; }
		++p->no;
	    }
    }
    return ch;
}

void backch(int ch) {
    Line *p = &line;
    if (p->pos > 0) {
       p->buf[--p->pos] = ch;
       p->backed = 0;
    } else {
       p->backed = ch;
    }
}

static char tmp[4];

static char *tokenname[] = {
  "ID", "ILIT", "CLIT", "FLIT", "SLIT",
  "ARIOP", "RELOP", "LOGOP", "ASNOP", "CMT",
  "class", "private", "public", "static", "const", 
  "if", "else", "switch", "case", "default",
  "break", "continue", "while", "do", "for", "call", "return", 
  0
};

static char *nameof(int sym) {
    if (sym < ID) 
         { tmp[0] = sym; tmp[1] = 0; return tmp; }
    else  if (sym < tEND)
         return tokenname[sym-ID];
    else
         return ""; 
}

static int prev_error_line_no = 0;

void parse_error(const char *s) {
    if (line.no != prev_error_line_no) {
        printf("\n%4d: %s", line.no, line.buf);
        prev_error_line_no = line.no;
    }
//    printf("ERROR: %s before %s at col %d\n", s, nameof(tok.sym), line.pos );
    printf("ERROR: %s at col %d\n", s, line.pos );
}

#define SQ ('\'')
#define DQ ('\"')

static int scancode[] = {
    /* 0x20 - 0x2f */
    ' ', '!', DQ, -1, -1, '%', '&', SQ, '(', ')', '*', '+', -1, '-', '.', '/', 
    /* 0x30 - 0x3f */
    '0', '9', '9', '9', '9', '9', '9', '9', '9', '9', -1,  -1, '<', '=', '>', -1,
    /* 0x40 - 0x4f */
    '@', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',
    /* 0x50 - 0x5f */
    'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', '{', -1, '}', -1, 'Z',
    /* 0x60 - 0x6f */
    -1 , 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z',
    /* 0x70 - 0x7f */
    'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', '[', '|', ']', -1, -1
};
#undef SQ
#undef DQ

int codeof(int ch) {
    if (ch < 0x20 || ch > 0x7f) return ch;
    return scancode[ch - 0x20];
}

static kwentry kwtable[] = {
    "class", tCLASS,
    "private", tPRIVATE,
    "public", tPUBLIC,
    "static", tSTATIC,
    "const", tCONST,
    "if", tIF,
    "else", tELSE,
    "switch", tSWITCH,
    "case", tCASE,
    "default", tDEFAULT,
    "break", tBREAK,
    "continue", tCONTINUE,
    "while", tWHILE,
    "do", tDO,
    "for", tFOR,
    "call", tCALL,
    "return", tRETURN,
    "int", tINT,
    "char", tCHAR,
    "float", tFLOAT,
    "string", tSTRING,
    "struct", tSTRUCT,
    "true", tTRUE,
    "false", tFALSE,
    "void", tVOID,
    0, 0
};

kwentry *kwlookup(const char *t) {
    kwentry *e;
    for (e=kwtable;e->text;e++) 
       if (strcmp(e->text,t)==0) return e;
    return 0;
}
