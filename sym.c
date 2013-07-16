#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "loc.h"
#include "sym.h"
#include "ast.h"

static symentry *symtab;
static int symcnt;

static scope *scope_buf;
static int scope_cnt;

static int var_seq;
static int con_seq;
static int type_seq;
static int func_seq;
static bool obmitted[MAX_SYMENTRY] = {false};

static scope *cur = 0;
static scope *new_scope();
static int cur_depth = 0;
inline void incr_depth() { ++cur_depth; }
inline void decr_depth() { --cur_depth; }
inline int get_cur_depth () { return cur_depth; }

char *gen(int kind) {
    char buf[10];
    int *p=0;
    int c;

    switch (kind) {
	case vLOCAL: p = &var_seq;  c = 'V'; break;        
	case cLOCAL: p = &con_seq;  c = 'C'; break;        
	case fLOCAL: p = &func_seq;  c = 'F'; break;        
	case tGLOBAL: p = &type_seq;  c = 'T'; break;        
	default: break;
    }
    if (p) { 
	sprintf(buf, "$%c%04d", c, ++(*p)); 
	return strdup(buf);
    } else 
	return "$$ERROR";
}

void init_SYM() { 
    int sz;
    sz  = (MAX_SYMENTRY+1) * sizeof(symentry);
    symtab = (symentry *)malloc(sz);
    bzero(symtab, sz);
    symcnt = 0;

    sz = (MAX_SCOPEENTRY+1) * sizeof(scope);
    scope_buf = (scope *)malloc(sz);
    bzero(scope_buf, sz);
    scope_cnt = 0;

    var_seq = 0;
    con_seq = 0;
    type_seq = 0;

    cur = new_scope();
}

static scope *new_scope() {
    scope *sp = 0;
    if (++scope_cnt < MAX_SCOPEENTRY) {
	sp = &scope_buf[scope_cnt];
	sp->begin = symcnt+1;
	sp->end   = 0;
    } else {
	parse_error("too much scopes");
    }
    return sp;
}

void delete_scope(scope *sp) {
    /* do nothing */
    int i;
    for (i = cur->begin; i <= cur->end; i++) {
	obmitted[i] = true;
    }
}

void enter_block() {
    scope *sp;

    ++cur_depth;
    reset_offset();
    sp = new_scope();
    if (sp) {
	sp->next = cur;
	cur = sp;
	reset_offset();
    }
}

void leave_block() {
    scope *tmp;
    --cur_depth;
    if (cur) {
	tmp = cur->next;
	delete_scope(cur);
	cur = tmp;
    }
}

static int arg_mark = 0;
void mark_args()   { arg_mark = symcnt; }
void unmark_args() { arg_mark = cur->end; }

int insert_SYM(char *name, int type, int prop, int val) {
    int depth, offset,sz;
    symentry *ep = &symtab[++symcnt];

    if (symcnt > MAX_SYMENTRY) {
	parse_error("Too much symbols");
	return 0;
    }

    cur->end = symcnt;
    ep->name = name;
    ep->type = type;
    ep->prop = prop;
    ep->val  = val;

    depth = get_cur_depth();
    offset = 0;
    sz = get_sizeoftype(type);

    switch (prop) {
	case tGLOBAL:
	    offset = -1;
	    ep->loc = new_loc();
	    break;

	case fLOCAL: 
	    offset = 0;
	    ep->loc = new_loc();
	    break;

	case vLOCAL:
	    offset = get_var_offset();
	    incr_var_offset(sz);
	    goto do_lookup;

	case vARG:
	    decr_arg_offset(sz);
	    offset = get_arg_offset();
	    depth++;

do_lookup:
	    /* try to share entry */
	    ep->loc = lookup_loc_entry(depth, offset, sz);
	    break;
	default:
	    break;
    }

    set_loc_entry(ep->loc, depth, offset, sz);
    return symcnt;
}

int lookup_cur(scope *sp, char *name) {
    symentry *p, *bp, *ep;
    int i;

    if (sp == 0) return 0;  /* guard */
    i = sp->end;
    bp = &symtab[sp->begin];
    ep = &symtab[sp->end];

    /* search reverse order */
    for( p = ep; p >= bp; --i,--p) {
	if (p && !obmitted[i] && p->name && strcmp(p->name,name)==0) {
	    symentry *ep = &symtab[i];
	    if (ep && ep->prop != vARG) return i;

	    /* effective args must be in [arg_mark, sp->end] */
	    if (arg_mark <= i) return i;
	}
    }
    return 0;
}

bool checkFuncExistCur(scope *sp, char *name, AST args){
    symentry *p, *bp, *ep;
    int i;

    if (sp == 0) return 0;  /* guard */
    i = sp->end;
    bp = &symtab[sp->begin];
    ep = &symtab[sp->end];

    /* search reverse order */
    for( p = ep; p >= bp; --i,--p) {
	if (p && !obmitted[i] && p->name && strcmp(p->name,name)==0) {
	    symentry *ep = &symtab[i];
	    if (ep && ep->prop == fLOCAL){
		AST argsDef = 0;
		get_sons(gettype_SYM(i), 0, &argsDef, 0, 0);
		if (checkArgs(argsDef, args)) return true;
	    }
	}
    }
    return false;
}

bool checkFuncExist(char *name, AST args){
    return checkFuncExistCur(cur, name, args);
}

bool checkFuncExistAll(char *name, AST args){
    scope *sp;
    for (sp = cur; sp; sp = sp->next){
	if (checkFuncExistCur(sp, name, args)) return true;
    }
    return false;
}

bool checkFuncExistClass(AST class, char *name, AST args){
    AST classDecl = get_father(get_father(class));
    AST classBody = 0;
    get_sons(classDecl, 0, &classBody, 0, 0);
    if (classBody){
    	AST funcdecls = 0;
	get_sons(classBody, 0, 0, 0, &funcdecls);
	while  (funcdecls){
	    AST funcdecl = 0;
	    get_sons(funcdecls, &funcdecl, &funcdecls, 0, 0);
	    if (funcdecl){
		AST namedecl = 0, argdecls = 0;
		get_sons(funcdecl, &namedecl, 0, &argdecls, 0);
		if (strncmp(get_text(namedecl), name, 100) == 0 && checkArgs(argdecls, args, 100)) return true;
	    }
	}
    }
    return false;
}

int lookup_SYM_all(char *name) {
    scope *sp;
    int idx;
    for (sp = cur; sp; sp = sp->next) {
	if (idx = lookup_cur(sp,name)) 
	    return idx;
    }
    return 0;
}

int lookup_SYM(char *name) {
    int idx;
    if (idx = lookup_cur(cur,name)) return idx;
    return 0;
}

int getval_SYM(int k) {
    symentry *ep = &symtab[k];
    return (k) ? ep->val : 0;
}

int getloc_SYM(int k) {
    symentry *ep = &symtab[k];
    return (k) ? ep->loc : 0;
}

int getdepth_SYM(int k) {
    symentry *ep = &symtab[k];
    int loc= (k) ? ep->loc : 0;
    return getdepth_LOC(loc); 
}

int getoffset_SYM(int k) {
    symentry *ep = &symtab[k];
    int loc= (k) ? ep->loc : 0;
    return getoffset_LOC(loc); 
}

void setval_SYM(int k, int val) {
    symentry *ep = &symtab[k];
    if (k) ep->val = val;
}

void setprop_SYM(int k, int prop) {
    symentry *ep = &symtab[k];
    if (k) ep->prop = prop;
}

int gettype_SYM(int k) {
    symentry *ep = &symtab[k];
    return (k)? ep->type : 0;
}

static char *namestr[] = {
    "vLOCAL", "vGLOBAL", 
    "fLOCAL", "fGLOBAL",
    "cLOCAL", "cGLOBAL",
    "tLOCAL", "tGLOBAL",
    "vARG",
    0
};

static char *propname(int prop) {
    if (prop < vLOCAL || prop >= pEND) return "";
    return namestr[prop-vLOCAL];
}

void dump_SYM(FILE *fp) {
    int i;
    symentry *ep = &symtab[1];
    fprintf(fp, "SYM:cnt=%d\n", symcnt);
    for (i=1;i<=symcnt;i++, ep++) {
	fprintf(fp, "%4d:%-10s %4d %8s %4d %4d\n", i, ep->name, ep->type, 
		propname(ep->prop), ep->val, ep->loc);
    }
    fprintf(fp, "\n");
}

int propval(char *text) {
    int i;
    for (i=0;namestr[i];i++) {
	if (strcmp(namestr[i],text)==0) return i+vLOCAL;
    }
    return 0;
}

int restore_SYM(FILE *fp) {
    int i,k;
    char text[40];
    char prop[10];
    symentry *ep = &symtab[1];
    fscanf(fp, "\nSYM:cnt=%d\n", &symcnt);
    for (i=1;i<=symcnt;i++, ep++) {
	bzero(text,40); bzero(prop,10);
	fscanf(fp, "%d:%s %d %s %d %d\n", &k, 
		text, &(ep->type), prop, &(ep->val),&(ep->loc));
	ep->prop = propval(prop);
	ep->name = strdup(text);
    }
    return symcnt;
}
