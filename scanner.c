#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "token.h"

static Token *id(int);
static Token *cmt(int);
static Token *lit(int);
static Token *flit(int);
static Token *clit(int);
static Token *slit(int);
static Token *op(int);
static Token *op_eq(int);  /* OP end with = (+=. -=, ==) */
static Token *op_dup(int); /* OP which dups the same char (++, --) */

static Token *gettoken0(void);

// Fix 'isspace' BUG in cygwin
inline int isspace(int ch) {
   return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

Token *gettoken() {
    Token *t;

    do {
      t = gettoken0();
    } while (t && (isspace(t->sym) || t->sym == CMT));

    if (t && t->sym == ID) {
        kwentry *e = kwlookup(t->text);
        if (e) t->sym = e->sym;
    }
    return  t;
}

void skiptoken(int sym) {
    Token *t=&tok;

    while (t && t->sym != '\n') {
       if (t->sym == sym)  break;
       t = gettoken0();
    }
}

void skiptoken2(int sym1,int sym2) {
    Token *t=&tok;
    while (t && t->sym != '\n') {
       if (t->sym == sym1)  break;
       if (t->sym == sym2)  break;
       t = gettoken0();
    }
}

void skiptoken3(int sym1,int sym2,int sym3) {
    Token *t=&tok;
    while (t && t->sym != '\n') {
       if (t->sym == sym1)  break;
       if (t->sym == sym2)  break;
       if (t->sym == sym3)  break;
       t = gettoken0();
    }
}

static Token *gettoken0() {
    int ch;
    Token *t = &tok;
    t->index = 0;

    ch = nextch();
    if (ch == EOF) { return (Token*)0; }

    switch (codeof(ch)) {
        case '+': case '-': case '*': case'/': case '%':
        case '<': case '>': case '=': case '!': case '&' : case '|':
            t = op(ch); break;
        case '\'':
            t = clit(ch); break;
        case '\"':
            t = slit(ch); break;
        case 'Z':
            t = id(ch); break;
        case '0': case '9':
            t = lit(ch); break;
        case '@': 
/* printf("[line %d, pos %d]", getlineno(), getlinepos()); */
            t->sym = ' '; break;
        default:
            t->sym = ch;  break;
    }
    return t;
}

static Token *id(int ch) {
    Token *t = &tok;

    outch(ch);
    while (1) {
       ch = nextch();
       switch (codeof(ch)) {
          case 'Z': case '0': case '9':
	     outch(ch);
             continue;
          default: break;
       }
       backch(ch); 
       break;
    }
    t->sym = ID;
    outch(0);
    return t;
}

static int hexval(int ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ERROR;
}

static Token *op(int ch) {
    Token *t = &tok;
    int ch2;
    int s = 1;

    t->sym = ARIOP;
    t->ival = ch;
    switch (ch) {
       case '/':
          outch(ch);
          ch2 = nextch();
          if (ch2 == '*' || ch2 == '/') {
             return cmt(ch2);
          } else if (ch2 == '=') {
             t->sym = ASNOP;
             return op_eq(ch2);
          }
          break;

       case '-':
          s = -1;
          /* through to next case */
       case '+':
          outch(ch);
          ch2 = nextch();
          if (ch2 == ch) {
	     return op_dup(ch2);
          }

          switch (codeof(ch2)) {
             case '=': 
                  t->sym = ASNOP;
                  return op_eq(ch2);
             case '0': case '9':
                  t = lit(ch2);
                  t->ival *= s;
                  return t;
             default: 
		  break;
          }
          break;

       case '<':
       case '>':
          t->sym = RELOP;
          outch(ch);
          ch2 = nextch();
	  if (ch2 == ch) {
	     t->sym = LOGOP;
	     return op_dup(ch2);
          }
          switch (codeof(ch2)) {
             case '=':
                 return op_eq(ch2);
             default:
                 break;
          }
          break;

       case '&':
       case '|':
          t->sym = LOGOP;
          outch(ch);
          ch2 = nextch();
          if (ch2 == ch) {
	     return op_dup(ch2);
          }
          break;

       case '*':
       case '%':
          outch(ch);
          t->ival = ch;
          ch2 = nextch();
          if (ch2 == '=') {
             t->sym = ASNOP;
             return op_eq(ch2);
          }
          break;

       case '!':
       case '=':
          outch(ch);
          t->ival = ch;
          ch2 = nextch();
          if (ch2 == '=') {
             t->sym = RELOP;
             return op_eq(ch2);
          } 
          switch (ch) {
            case '!': t->sym = LOGOP; break; 
            case '=': t->sym = ASNOP; break; 
          }
          break;

       default: 
          ch2 = ch; 
          break;
    }
    backch(ch2);

    outch(0);
    return t;
}

static Token *op_eq(int ch) {
    Token *t = &tok;

    outch(ch);
    outch(0);
    t->ival = WITHEQ(t->ival);
    return t;
}

static Token *op_dup(int ch) {
    Token *t = &tok;

    outch(ch);
    outch(0);
    t->ival = WITHDUP(t->ival);
    t->sym = DUPOP;
    return t;
}

static Token *lit(int ch) {
    Token *t = &tok;
    int d = 0;
    int v = 0;
    int b = 10;

    outch(ch);
    v = hexval(ch);

    if (ch == '0') {
        b = 8;
        ch = nextch();
        if (ch == 'x' || ch == 'X') {
            b = 16; outch(ch);
        } else 
            backch(ch);
    }

    while (1) {
       ch = nextch();
       d  = hexval(ch);
       switch (codeof(ch)) {
          case '0': case '9':
             if (d < b) {
                 outch(ch);
                 v = v * b + d;
                 continue;
             }
             break;
          case 'Z':
             if (b == 16 && d < b) {
                 outch(ch);
                 v = v * b + d;
                 continue;
             }
             break;
          case '.':	/* it might be a float */
             t->ival = v;
             return flit(ch); 
       }
       backch(ch); 
       break;
    }

    t->sym = ILIT;
    outch(0);
    t->ival = v;
    return t;
}

Token *flit(int ch) {
    Token *t = &tok;
    int ival = t->ival;

    outch(ch);
    ch = nextch();
    t = lit(ch);
    t->sym  = FLIT;
    t->ival = ival;
    return t;
}

Token *clit(int ch) {
    Token *t = &tok;
    int v=0;
    while (1) {
        ch = nextch();
        if (ch == '\'') break;
        outch(ch);
        v = (v << 8) + (ch & 0xff);
    } 
    outch(0); 
    t->sym = CLIT;
    t->ival = v & 0xffff ;
    return t;
}

Token *slit(int ch) {
    Token *t = &tok;
    while (1) {
        ch = nextch();
        if (ch == '\"') break;
        outch(ch);
    } 
    outch(0); 
    t->sym = SLIT;
    t->ival = 0; /* dummy */
    return t;
}

/*
   If you keep all texts in COMMENT, it may exceed the length of MAX_LEXEME.
 */
static int keep = 0;

static Token *cmt(int ch) {
    Token *t = &tok;
    clear_lexeme();

    switch (ch) {
      case '/':
        while (1) {
          ch = nextch();
          if (ch == '\n' || ch == '\r') break;
          if (keep) outch(ch);
        }
        break;

      case '*':
        while (1) {
          ch = nextch();
          if (ch == '/' && prevch() == '*')
               { delete_prev(); break; } /* erase '*' */
          outch(ch);
        }
        break;

      default:
        break;
    }

    t->sym = CMT;
    outch(0);
    return t;
}

#ifdef TEST_SCANNER
int main() {
    Token *t;

    initline();

    while ((t = gettoken()) != 0) {
       if (t->sym < ID) {
           if (isspace(t->sym))
             printf("%c", t->sym);
	   else
             printf("SEP<%c> ", t->sym);
       } else switch (t->sym) {
          case ID : printf("ID<%s> ", t->text); break;
          case ILIT: printf("ILIT<%s>(%d) ", t->text, t->ival); break;
          case FLIT: printf("FLIT<%s>(%d) ", t->text, t->ival); break;
          case CLIT: printf("CLIT<%s>(%d) ", t->text, t->ival); break;
          case SLIT: printf("SLIT<%s>(0x%p) ", t->text, t->ival); break;
          case ARIOP:  printf("ARIOP<%s>(%d) ", t->text, t->ival); break;
          case RELOP:  printf("RELOP<%s>(%d) ", t->text, t->ival); break;
          case LOGOP:  printf("LOGOP<%s>(%d) ", t->text, t->ival); break;
          case ASNOP:  printf("ASNOP<%s>(%d) ", t->text, t->ival); break;
          case tCLASS: printf("CLASS<> "); break;
          case tPRIVATE: printf("PRIVATE<> "); break;
          case tPUBLIC: printf("PUBLIC<> "); break;
          case tSTATIC: printf("STATIC<> "); break;
          case tCONST: printf("CONST<> "); break;
          case tIF: printf("IF<> "); break;
          case tELSE: printf("ELSE<> "); break;
          case tSWITCH: printf("SWITCH<> "); break;
          case tCASE: printf("CASE<> "); break;
          case tDEFAULT: printf("DEFAULT<> "); break;
          case tBREAK: printf("BREAK<> "); break;
          case tCONTINUE: printf("CONTINUE<> "); break;
          case tWHILE: printf("WHILE<> "); break;
          case tDO: printf("DO<> "); break;
          case tFOR: printf("FOR<> "); break;
          case tCALL: printf("CALL<> "); break;
          case tRETURN: printf("RETURN<> "); break;
          case tINT: printf("INT<> "); break;
          case tCHAR: printf("CHAR<> "); break;
          case tFLOAT: printf("FLOAT<> "); break;
          case tSTRING: printf("STRING<> "); break;
          case tTRUE: printf("TRUE<> "); break;
          case tFALSE: printf("FALSE<> "); break;
          default:  printf("ERROR<%s> ", t->text); break;
       }
    }
    printf("\n");
    return 0;
}
#endif
