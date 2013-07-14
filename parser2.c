//
// parser2.c -- parser for tinyj
//   (outside of the block)
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "sym.h"
#include "type.h"
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
funcdecl  ::= typedecl id '(' argdecls ')' block
// end

argdecls  ::= e | argdecl [ ',' argdecl ]*
argdecl   ::= typedecl var
typedecl  ::= primtype mod?
mod       ::= { '[' con ']' }*
primtype  ;;= INT | CHAR | FLOAT | STRING

---------------------

block     ::= '{' vardecls stmts '}'
stmts     ::= [ stmt ]*
stmt      ::= expr ';' | ifstmt | asnstmt  | block
ifstmt    ::= IF '(' bexpr ')' stmt ELSE stmt

// here is not LL(1)
asnstmt   ::= lval '=' expr
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
relop     ::= EQEQ | NOTEQ | GT | GTEQ | LT | LTEQ
vref      ::= ID
con       ::= LIT
 */

extern Token *gettoken();
extern void skiptoken(int);
extern void skiptoken2(int,int);

static AST program(void);
static AST classdecls(void);
static AST classdecl(void);
static AST classhead(void);
static AST structdecls(AST sdl);
static AST vardecls(AST vdl, AST fdl);
static AST funcdecls(AST fdl);
static AST structdecl(void);
static AST vardecl(void);
static AST funcdecl(void);
static AST typedecl(void);
static AST retdecl(void);
static AST mod(AST);
static AST primtype(void);
static AST rettype(void);
static bool isprimtype(int k);
static bool isrettype(int k);
static AST argdecls(void);
static AST argdecl(void);
static AST block(void);
static AST stmts(void);
static AST stmt(void);
static AST asnstmt(AST);
static AST ifstmt(void);
static AST callstmt(AST);
static AST returnstmt(void);
static AST argrefs(AST);
static AST argref(void);
static AST var(void);
static AST name(void);
static AST vName(void);
static AST vref(void);
static AST lval(void);
static AST exprs(void);
static AST con(void);
static AST expr(void);
static AST bexpr(void);

static AST ast_root;

static int ast_debug = false;

#ifdef TEST_PARSER
int main() {
    initline();
    init_AST();
    init_SYM();
    init_LOC();

    gettoken();
    ast_root = program();

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
    gettoken();
    return program();
}
#endif

static AST program() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0;

    a1 = classdecls();
    a = make_AST(nPROG, a1, 0, 0, 0);
    return a;
}

static AST classdecls() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0;

    /* at least one class is required */
    a = new_list(nCLASSDECLS);
    a1 = classdecl();
    if (a1) a = append_list(a, a1);

    while (true) {
	if (t->sym != tCLASS) break;

	a1 = classdecl();
	if (a1) a = append_list(a, a1);
    }
    return a;
}

static AST classdecl() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0, a3=0, a4 = 0;
    AST sdl = 0, vdl=0, fdl=0;  /* struct, var and func decl list */

    a1 = classhead();

    if (t->sym == '{') {
	gettoken();
	enter_block();

	sdl = new_list(nSTRUCTS);
	vdl = new_list(nVARDECLS);
	fdl = new_list(nFUNCDECLS);

	a2 = structdecls(sdl);
	a3 = vardecls(vdl, fdl);
	a4 = funcdecls(fdl);
	leave_block();

	if (t->sym == '}') {
	    gettoken();
	    a = make_AST(nCLASSDECL, a1, a2, a3, a4);
	} else {
	    parse_error("expected }");
	}

    } else {
	parse_error("expected {");
    }
    return a;
}

static AST classhead() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0;

    if (t->sym == tCLASS) {
	gettoken();
	a1 = name();
	if (a1) insert_SYM(get_text(a1), 0, tGLOBAL, 0); /* dummy */
	a  = make_AST(nCLASSHEAD, a1, 0, 0, 0);
    } else {
	parse_error("expected class");
    }
    return a;
}

static bool isprimtype(int k) {
    return (k==tINT || k==tCHAR || k==tFLOAT || k==tSTRING);
}

static bool isrettype(int k){
    return isprimtype(k) || k == tVOID;
}

static AST structdecls(AST sdl){
    Token *t = &tok;
    AST a = 0;
    while (true){
	if (t->sym != tSTRUCT) break;
	gettoken();
	a = structdecl();
	if (a) sdl = append_list(sdl, a);
    }
    return sdl;
}

static AST vardecls(AST vdl, AST fdl) {
    Token *t = &tok;
    AST a=0;
    AST a1=0;
    bool isfuncdecl = false;

    while (true) {
	if (!isprimtype(t->sym)) break;
	a1 = vardecl();
	if (a1 && nodetype(a1) == nFUNCDECL) /* expect var, but it was func */
	{ isfuncdecl = true; break; }
	if (a1) vdl = append_list(vdl, a1);

	if (t->sym == ';') gettoken();
	else parse_error("expected ;");
    }

    if (isfuncdecl) {
	fdl = append_list(fdl, a1);    /* add first elem of fdl */
    }

    return vdl;
}

static AST funcdecls(AST fdl) {
    Token *t = &tok;
    AST a=0;
    AST a1=0;

    while (true) {
	if ( !isrettype(t->sym)) break;
	a1 = funcdecl();
	if(a1) fdl = append_list(fdl, a1);
    }
    return fdl;
}

static AST vardecl() { /* TODO: allow vars */
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0,a3=0,a4=0;
    AST ft;
    int idx;

    a2 = typedecl();
    a1 = var();		

    if (t->sym == '(') { /* oops, it was func */
	gettoken();
	set_nodetype(a1, nNAME); /* change to NAME */
	insert_SYM(get_text(a1), ft=func_type(gen(fLOCAL),a2), fLOCAL,0 /* dummy */ );

	a3 = argdecls();
	set_argtypeofnode(ft,a3);

	if (t->sym == ')') gettoken();
	else parse_error("expected )");

	a4 = block();
	a  = make_AST_funcdecl(a1, a2, a3, a4);
    } else if (t->sym == ';') { /* vardecl */
	a = make_AST_vardecl(a1, a2, 0, 0);
	idx = insert_SYM(get_text(a1), a2, vLOCAL, a1); 
	set_ival(a1,idx);
	/*
	   } else if (t->sym == ',') {  // multiple vars 
	 *     contents must be made
	 *
	 */
    } else {
	parse_error("expected ; or (");
    }
    return a;
}

static AST structdecl(){
    Token *t = &tok;

    AST struct_name = name();
    AST a = new_list(nVARDECLS);
    if (t->sym == '{'){
	enter_block();
	gettoken();
	a = vardecls(a, 0);
	leave_block();
	if (t->sym == '}') gettoken();
	else parse_error("Expected }");

	if (t->sym == ';') gettoken();
	else parse_error("Expected ;");

	a = make_AST_structdecl(struct_name, a);
    } else {
	parse_error("Expected {");
    }
    return a;
}

static AST funcdecl() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0,a3=0,a4=0;
    AST ftype;

    a2 = retdecl();
    a1 = name();
    ftype = func_type(gen(fLOCAL),a2);
    insert_SYM(get_text(a1), ftype, fLOCAL, 0/* dummy */);

    if (t->sym == '(') { /* must be func */
	gettoken();

	mark_args();
	a3 = argdecls();
	set_argtypeofnode(ftype,a3);

	if (t->sym == ')') gettoken();
	else parse_error("expected )");

	a4 = block();
	unmark_args();
	a  = make_AST_funcdecl(a1,a2,a3,a4);

    } else {
	parse_error("expected (");
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

static AST retdecl(){
    Token *t = &tok;
    AST a=0;
    a = rettype();
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
	a1 = con();
	sz = get_ival(a1);

	if (sz <= 0) { parse_error("size must be positive"); sz = 1; }

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

static AST rettype(){
    Token *t = &tok;
    AST a=0;

    switch (t->sym) {
	case tVOID:    a = make_AST_name("void"); break;
	default:     return primtype(); 
    }
    gettoken();
    return a;
}

static AST argdecls() {
    Token *t = &tok;
    AST a=0;
    AST a1=0;

    a = new_list(nARGDECLS);
    while (true) {
	if (t->sym == ')') break;
	if ( !isprimtype(t->sym) ) break;
	a1 = argdecl();
	if(a1) a = append_list(a, a1);

	if (t->sym == ',') gettoken();
    }
    return a;
}

static AST argdecl() {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0;
    int idx;

    a2 = typedecl();
    a1 = var();
    a  = make_AST_argdecl(a1, a2);
    idx = insert_SYM(get_text(a1), a2, vARG, a1);
    set_ival(a1,idx);
    return a;
}

static AST block() {
    Token *t = &tok;
    AST a=0;
    AST vdl,sts;

    vdl = new_list(nVARDECLS);
    if(t->sym == '{') {
	gettoken();
	enter_block();

	vardecls(vdl,0);
	sts = stmts();

	a = make_AST(nBLOCK, vdl, sts, 0, 0);
	leave_block();

	if (t->sym == '}') gettoken();
	else parse_error("expected }");

    } else {
	parse_error("expected {");
    }
    return a;
}

static AST stmts() {
    Token *t = &tok;
    AST a=0;
    AST a1=0, a2=0 ;

    a = new_list(nSTMTS);
    while (true) {
	switch(t->sym) {
	    case ID: case tIF: case tRETURN: case '{':
		break;
	    default:
		return a;
	}
	a1 = stmt();
	a = append_list(a, a1);
    }
    return a;
}

static AST stmt() {
    Token *t = &tok;
    AST a=0;
    AST a1=0;
    AST n;

    switch (t->sym) {
	case ID: /* ASN or CALL */
	    n = vName();
	    if (t->sym == ASNOP || t->sym == '[') {
		a1 = asnstmt(n);
		if (t->sym == ';') gettoken();
	    } else if (t->sym == '(') {
		a1 = callstmt(n);
		if (t->sym == ';') gettoken();
	    } else {
		parse_error("expected ASNOP or Funcall");
		skiptoken(';');
	    }
	    break;
	case tIF:
	    a1 = ifstmt();
	    break;
	case tRETURN:
	    a1 = returnstmt();
	    if (t->sym == ';') gettoken();
	    break;
	case '{':
	    a1 = block();
	    break;
	default:
	    break;
    }

    a = make_AST(nSTMT,a1,0,0,0);
    return a;
}

static AST asnstmt(AST name) {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0;
    int idx,op;

    switch (t->sym) {
	case ASNOP: /* change name to VREF */
	    set_nodetype(name, nVREF);
	    idx = lookup_SYM_all(get_text(name));
	    set_ival(name,idx);
	    a1 = name;
	    break;
	case '[': /* change name to LVAL */
	    set_nodetype(name, nVREF);
	    a2 = exprs();
	    a1 = make_AST(nLVAL, name, a2, 0, 0) ;
	    break;
	default:
	    break;
    }

    op = t->ival;
    gettoken();

    a2 = expr();
    a = make_AST_asn(op,a1,a2);
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

static AST callstmt(AST name) {
    Token *t = &tok;
    AST a=0;
    AST a1=0,a2=0;

    if (t->sym == '(') gettoken();
    else parse_error("expected (");

    //get types of arguments in the function definition.
    get_sons(typeof_AST(name), 0, &a1, 0, 0);

    a2 = argrefs(a1);

    if (t->sym == ')') gettoken();
    else parse_error("expected (");

    a = make_AST(nCALL, name, a2, 0, 0);
    return a;
}

static AST argrefs(AST definition) {
    Token *t = &tok;
    AST a,a1=0;

    a = new_list(nARGS); 

    while (true) {
	if (t->sym == ')') break;
	a1 = argref();

	//start checking types
	AST check = 0;
	get_sons(definition, &check, &definition, 0, 0);
	equaltype(get_son0(check), get_son0(a1));

	if (a1) a = append_list(a,a1);

	if (t->sym == ',') gettoken();
    }
    return a;
}

static AST argref() {
    Token *t = &tok;
    AST a,a1=0;

    a1 = expr();
    a = make_AST(nARG, a1, 0, 0, 0);
    return a;
}

static AST returnstmt() {
    Token *t = &tok;
    AST a,a1=0;

    gettoken();
    a1 = expr();
    a = make_AST(nRET, a1, 0, 0, 0);
    return a;
}

static AST var() {
    Token *t = &tok;
    AST a=0;
    int idx;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	gettoken();
	/* ival may be changed later */
	a = make_AST_var(s,0);
    } else {
	parse_error("expected ID");
    }
    return a;
}

static AST vref() {
    Token *t = &tok;
    AST a=0;
    int idx;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	gettoken();
	idx = lookup_SYM_all(s);
	if (idx == 0) parse_error("undefined variable");
	a = make_AST_vref(s,idx);
    } else {
	parse_error("expected ID");
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

static AST vName(){
    Token *t = &tok;
    AST a=0;

    if (t->sym == ID) {
	char *s = strdup(t->text);
	int idx = lookup_SYM_all(s);
	if (idx == 0) parse_error("Undefined symbol");
	gettoken();
	a = make_AST_name(s);
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

static AST exprs() {
    Token *t = &tok;
    AST a,a1=0;

    a = new_list(nEXPRS);
    while (true) {
	if (t->sym != '[') break;
	gettoken();
	a1 = expr();
	if (a1) a = append_list(a,a1);

	if (t->sym == ']') gettoken();
	else parse_error("expected ]");
    }
    return a;
}

/* simplest version */
static AST expr() {
    Token *t = &tok;
    switch (t->sym) {
	case ID: return lval();
	case ILIT: return con();
	default: return 0;
    }
}

/* dummy */
static AST bexpr() {
    Token *t = &tok;
    char *s = strdup(t->text);
    switch (t->sym) {
	case tTRUE: 
	    gettoken(); 
	    return make_AST_con(s,1);
	case tFALSE: 
	    gettoken(); 
	    return make_AST_con(s,0);
	default: 
	    return 0;
    }
}
