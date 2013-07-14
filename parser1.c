//
// parser1.c -- parser for tinyj
//   (inside of the block)
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "token.h"
#include "sym.h"
#include "ast.h"

/*
   Grammar 

   program   ::= classdecls
   classdecls ::= [ classdecl ]*
   classdecl ::= classhead '{' vardecls funcdecls '}'
   classhead ::= CLASS ID
   vardecls  ::= [ vardecl ]*
   funcdecls ::= [ funcdecl ]*

// here is not LL(1)
vardecl   ::= typedecl id ';'
funcdecl  ::= typedecl id '(' argdecllist ')' block
// end

argdecllist   ::= e | argdecl [ ',' argdecl ]*
argdecl       ::= typedecl var

typedecl   ::= primtype mod?
mod       ::= { '[' con ']' }*
primtype  ::= INT | CHAR | FLOAT | STRING

----

block     ::= '{' vardecls stmts '}'
stmts     ::= [ stmt ]*
stmt      ::= expr ';' | ifstmt | asnstmt | block
ifstmt    ::= IF '(' bexpr ')' stmt ELSE stmt  // dangling else

// here is not LL(1)
asnstmt   ::= lval asnop expr
callstmt  ::= name '(' argrefs ')
// end

argrefs   ::= argref { ',' argref }*
argref    ::= expr
lval	     ::= vref exprs?
exprs     ::= { '[' expr ']' }*
expr      ::= term   [ '+' term ]* 
term      ::= factor [ '*' factor ]*
factor    ::= lval | con | '(' expr ')' | mesexpr
bexpr     ::= TRUE | FALSE | expr relop expr
asnop     ::= '=' | PLUSEQ | MINUSEQ | STAREQ | SLASHEQ | ...
relop     ::= EQEQ | NOTEQ | GT | GTEQ | LT | LTEQ
vref      ::= ID
con       ::= LIT
 */

extern Token *gettoken();
extern void skiptoken(int);
extern void skiptoken2(int,int);
extern void skiptoken3(int,int,int);

static AST block(void);
static bool isprimtype(int k);
static AST vardecls(AST vdl, AST fdl);
static AST vardecl(void);
static AST typedecl(void);
static AST mod(AST);
static AST primtype(void);
static AST argdecllist(void);
static AST argdecl(void);
static AST vars(AST);
static AST var(AST);
static AST lval(void);
static AST vref(void);
static AST name(void);
static AST con(void);
static AST stmts(void);
static AST stmt(void);
static AST asnstmt(void);
static AST ifstmt(void);
static AST whilestmt(void);
static AST exprs(void);
static AST expr(void);
static AST bexpr(void);
static AST bexpr_sub(void);
static AST term(void);
static AST factor(void);

static AST ast_root;
static AST zero;

static int ast_debug = true;

#ifdef TEST_PARSER
int main() {
    initline();
    init_AST();
    init_SYM();
    init_LOC();

    zero = make_AST_con("0",0);
    gettoken();
    ast_root = block();  /* inside the block */

    print_AST(ast_root);
    printf("\n\n");

    if (ast_debug) {
	dump_AST(stdout);
	printf("\n");
    }

    dump_SYM(stdout);

    printf("\n");
    dump_LOC(stdout);

    printf("\n");
    dump_STR(stdout);

    return 0;
}
#else
int start_parser() {
    zero = make_AST_con("0",0);
    gettoken();
    return block(); 
}
#endif

static AST block() {
    Token *t = &tok;
    AST a=0;
    AST vdl,fdl,sts;

    if (t->sym == '{') {
	gettoken();
	enter_block();

	vdl = new_list(nVARDECLS);
	fdl = new_list(nFUNCDECLS);  /* ignored */
	vardecls(vdl,fdl);

	sts = stmts();

	if (t->sym == '}') {
	    gettoken();
	    leave_block();
	    a = make_AST(nBLOCK, vdl, sts, 0, 0);           
	} else {
	    parse_error("expected }");
	}
    } else {
	parse_error("expected {");
    }
    return a;
}

inline static bool isprimtype(int k) {
    return (k==tINT || k==tCHAR || k==tFLOAT || k==tSTRING);
}

static AST vardecls(AST vdl, AST fdl) {
    Token *t = &tok;
    AST a=0;
    AST a1=0;

    /* ignore funcdecl */

    while (true) {
	if (! isprimtype(t->sym) ) break;
	a1 = vardecl();
	if (a1) vdl = append_list(vdl, a1);

	if (t->sym == ';') gettoken();
	else parse_error("expected ;");
    }
    return vdl;
}

static AST vardecl() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0,a3=0,a4=0;

    a2 = typedecl();
    a1 = var(a2);	/* TODO: change var() to vars() */

    if (t->sym == ';') { /* vardecl */
	a = make_AST_vardecl(a1, a2, 0, 0);
    } else {
	parse_error("expected ; or (");
    }
    return a;
}

static AST typedecl() {
    Token *t = &tok;
    AST a=0;
    a = primtype();
    a = mod(a);
    return a;
}

static AST mod(AST elem) {
    Token *t = &tok;
    AST a,a1=0;
    int sz;

    a = elem;
    if (t->sym == '[') {
	gettoken();
	if (t->sym == ILIT) {
	    sz = t->ival;
	} else {
	    parse_error("expected ILIT");
	}
	if (sz <= 0) { parse_error("size must be positive"); sz = 1; }

	gettoken();
	if (t->sym == ']') gettoken();
	else parse_error("expected ]");

	a = array_type(gen(tGLOBAL), elem, sz);
	a = mod(a);
    }
    return a;
}

static AST primtype() {
    Token *t = &tok;
    AST a=0;

    switch (t->sym) {
	case tINT:    a = make_AST_name("int"); break;
	case tCHAR:   a = make_AST_name("char"); break;
	case tFLOAT:  a = make_AST_name("float"); break;
	case tSTRING: a = make_AST_name("string"); break;
	default:      parse_error("expected primtype"); break;
    }
    gettoken();
    return a;
}

static AST vars(AST type) {
    Token *t = &tok;
    AST a=0;
    AST a1=0;

    a = new_list(nVARS);
    a1= var(type);
    if (a1) a = append_list(a,a1);

    while (true) {
	if (t->sym == ';')
	    break;

	if (t->sym == ',') gettoken();
	else parse_error("expected ,");

	a1 = var(type);
	if (a1) a = append_list(a,a1);
    }
    return a;
}

static AST var(AST type) {
    Token *t = &tok;
    int idx;
    AST a=0;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	gettoken();
	if (lookup_SYM(s)) parse_error("Duplicated variable declaration");
	idx = insert_SYM(s, type, vLOCAL, 0);
	a = make_AST_var(s, idx);
    } else {
	parse_error("expected ID");
    }
    return a;
}

static AST name() {
    Token *t = &tok;
    AST a=0;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	gettoken();
	a = make_AST_name(s);
    } else {
	parse_error("expected ID");
    }
    return a;
}

static AST vref()  {
    Token *t = &tok;
    int idx=0;
    AST a=0;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	gettoken();
	idx = lookup_SYM_all(s);
	if (idx == 0) parse_error("Undefined variable");
	a = make_AST_vref(s,idx);
    } else {
	parse_error("expected ID");
    }
    return a;
}

static AST con() {
    Token *t = &tok;
    int v;
    int idx;
    char *p;
    char *s;
    AST ty=0;
    AST a=0;

    v = t->ival;
    p = gen(cLOCAL);
    s = insert_STR(t->text);

    if (t->sym >= ILIT && t->sym <= SLIT)
	ty = (t->sym - ILIT) +1;

    switch (t->sym) {
	case ILIT: case CLIT: /* int, char */
	    gettoken();
	    idx = insert_SYM(p, 0, cLOCAL, v); /* immediate */
	    a = make_AST_con(p,idx);
	    break;
	case FLIT: case SLIT: /* float, string */
	    gettoken();
	    idx = insert_SYM(p, ty, cLOCAL, get_STR_offset(s));
	    a = make_AST_con(p,idx);
	    break;
	default:
	    parse_error("expected LIT");
	    break;
    }

    set_typeofnode(a,ty);
    return a;
}

static AST stmts() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0 ;

    a = new_list(nSTMTS);
    while (true) {
	if (t->sym != ID && t->sym != tIF && t->sym != '{' && t->sym != tWHILE) 
	    break;

	a1 = stmt();
	a = append_list(a, a1);
    }
    return a;
}

static AST stmt() {
    Token *t = &tok;
    AST a=0;
    AST a1=0;

    switch (t->sym) {
	case ID: /* TODO: extend to support call */
	    a1 = asnstmt();
	    if (t->sym == ';') gettoken();
	    else parse_error("expected ;");
	    break;
	case tIF:
	    a1 = ifstmt();
	    break;
	case tWHILE:
	    a1 = whilestmt();
	    break;
	case '{': 
	    a1 = block();
	    break;

	default:
	    parse_error("expected ID, IF or block");
	    skiptoken(';');
	    break;
    }
    /*a = make_AST(nSTMT, a1, 0, 0, 0);*/
    /*return a;*/
    return a1;
}

static AST asnstmt() {
    Token *t = &tok;
    int ival;
    AST a=0;
    AST a1=0, a2=0;
    int op;

    a1 = expr();
    switch (t->sym) {
	case ASNOP:
	    if (nodetype(a1) == nLVAL || nodetype(a1) == nVREF) {
		op = t->ival;
	    } else parse_error("Left hand side of the assignment must be a variable or an array element");
	    gettoken();
	    break;
	case ';':
	    return a1;
	default:
	    parse_error("asnop expected or missing \';\'");
	    gettoken();
	    break;
    }

    a2 = expr();
    equaltype(a1, a2);
    a = make_AST_asn(op, a1, a2);
    return a;
}

static AST ifstmt() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0,a3=0;

    if (t->sym == tIF) {
	gettoken();
	if (t->sym == '(') {
	    gettoken();
	    a1 = bexpr(); 

	    if (t->sym == ')') gettoken();
	    else parse_error("expected )");

	    a2 = stmt();
	    if (t->sym == tELSE) {   /* else is optional */
		gettoken();
		a3 = stmt();
	    } 

	} else {
	    parse_error("expected (");
	}
	a = make_AST_if(a1,a2,a3);
    } else {
	parse_error("expected if");
	skiptoken(';');
    }
    return a;
}

static AST whilestmt() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0,a3=0;

    if (t->sym == tWHILE) {
	gettoken();
	if (t->sym == '(') {
	    gettoken();
	    a1 = bexpr(); 

	    if (t->sym == ')') gettoken();
	    else parse_error("expected )");

	    a2 = stmt();
	} else {
	    parse_error("expected (");
	}
	a = make_AST_while(a1,a2,a3);
    } else {
	parse_error("expected while");
	skiptoken(';');
    }
    return a;
}
static AST lval() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0;

    a1 = vref();

    if (t->sym == '[') {
	a2 = exprs();
	a = make_AST(nLVAL, a1, a2, 0, 0);
    } else
	a = a1;
    return a;
}

static AST exprs() {
    Token *t = &tok;
    AST a,a1=0;

    a = new_list(nEXPRS);
    while (true) {
	if (t->sym != '[') break;
	gettoken();
	a1 = expr();
	//check type of index: it must be integer:
	if (typeof_AST(a1) != make_AST_name("int")) parse_error("Index of array must be integer");
	if (a1) a = append_list(a,a1);

	if (t->sym == ']') gettoken();
	else parse_error("expected ]");
    }
    return a;
}

static AST expr() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0;
    int op;

    /* when missing both ID/LIT and ; FOLLOW(stmt) may appear */
    if (t->sym == tIF || t->sym == tELSE) {
	return zero;
    }

    a = term();

    /* FOLLOW(term) = { ';', ',', ')' ,']' } */
    while (true) {
	if (t->sym == ';' || t->sym == ',' || t->sym == ')' || t->sym == ']')
	    break;
	switch (t->sym) {
	    case ASNOP: case RELOP: case LOGOP:
		// this should be an assignment: 
		return a;
	    case ARIOP:
		if (t->ival != '+' && t->ival != '-') {
		    parse_error("expected + or -");
		    skiptoken(';');
		    return a;
		}
		op = t->ival;
		gettoken();
		break;
	    case ILIT:
		/* when LIT does not begin with +/-, it is missing */
		if (t->text[0] != '+' && t->text[0] != '-')
		    parse_error("expected + or -");
		op = '+';
		break;

	    case ID: case tIF: case tELSE: /* FOLLOW(expr) when ; is missing */
		parse_error("purhaps missing ;");
		return a;

	    default:
		parse_error("exepected op"); 
		skiptoken2(';',ARIOP);
		return a;
	}
	a1 = a;
	a2 = term();
	a = make_AST_op2(op, a1, a2); /* left associative */
    }
    return a;
}

static AST term() {
    Token *t = &tok;
    AST a =0;
    AST a1=0, a2=0;
    int op;

    a = factor();

    /* FOLLOW(factor) = { ';', ',', ')' , ']', '+', '-' } */
    while (true) {
	if (t->sym == ';' || t->sym == ',' || t->sym == ')' || t->sym == ']')
	    break;
	if (t->sym == ILIT) /* a+1 => ID<a> ILIT<+1> , same as OP<+> */
	    break;

	switch (t->sym) {
	    case ASNOP: case RELOP: case LOGOP:
		return a;
	    case DUPOP:
		if (nodetype(a) == nLVAL || nodetype(a) == nVREF) return a; 
	    case ARIOP:
		if (t->ival == '+' || t->ival == '-')
		    return a; 
		if (t->ival != '*' && t->ival != '/') {
		    parse_error("expected * or /");
		    skiptoken(';');
		    return a; 
		}
		op = t->ival;
		gettoken();
		break;

	    case '(':
		parse_error("purhaps missing * or /");
		op = '*';
		break;

	    case ID: case tIF: case tELSE: /* FOLLOW(expr) when ; is missing */
		parse_error("purhaps missing ;");
		return a;

	    default:
		parse_error("expected op"); 
		skiptoken2(';',ARIOP);
		return a;
	}
	a1 = a;
	a2 = factor();
	a  = make_AST_op2(op, a1, a2); /* left associative */
    }
    return a;
}

/* returns 'zero' if parse_error occurs */
static AST factor() {
    Token *t = &tok;
    AST a=0;
    int op;

    switch (t->sym) {
	case DUPOP:
	    op = t->ival;
	    gettoken();
	    a = lval();
	    return make_AST_op1(op, a);
	    break;
	case ID :
	    a = lval();
	    if (t->sym == DUPOP) {
		a = make_AST_op0(t->ival, a);
		gettoken();
		return a;
	    }
	    break;
	case ILIT: case CLIT: case FLIT: case SLIT: 
	    a = con();
	    break;
	case '(': 
	    gettoken();
	    a = expr();
	    if (t->sym == ')') gettoken();
	    else parse_error("expected )");
	    break;
	default: 
	    parse_error("expected ID or LIT");
	    skiptoken3(';',')',ARIOP);
	    a = zero;
	    break;
    }
    return a;
}

/* good */
static AST bexpr() {
    Token *t = &tok;
    char *s = strdup(t->text);
    AST a, a1;
    a = bexpr_sub();
    int op;
    while (true){
	switch (t->sym) {
	    case ')':
		return a;
	    case LOGOP:
		op = t->ival;
		gettoken();
		a1 = bexpr_sub();
		a = make_AST_op2(op, a, a1);
		break;
	    default:
		parse_error("Expected \')\' or a logical operator");
		return 0;
	}
    }
}

static AST bexpr_sub() {
    Token *t = &tok;
    char *s = strdup(t->text);
    AST a, a1;
    int op;
    switch (t->sym){
	case tTRUE: 
	    gettoken(); 
	    return make_AST_con(s,1);
	case tFALSE: 
	    gettoken(); 
	    return make_AST_con(s,0);
	case '(':
	    gettoken();
	    a = bexpr();
	    if (t->sym == ')'){
		gettoken();
	    } else {
		parse_error(") expected");
	    }
	    return a;
	default:
	    a = expr();
	    switch (t->sym){
		case RELOP:
		    op = t->ival;
		    gettoken();
		    a1 = expr();
		    return make_AST_op2(op, a, a1);
		default:
		    return a;
	    }
    }
}
