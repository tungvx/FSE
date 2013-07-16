#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "sym.h"
#include "token.h"
#include "ast.h"

/* 
   type expression : use struct Node with AST
type : PRIM | POINTER | ARRAY | STRUCT | FUNC
ival : size (used only ARRAY, STRUCT)
son[0] : element type (used only ARRAY, POINTER)
 */

inline AST elem(AST a) { return get_son0(a); }

AST make_type(char *name) {
    AST a = make_AST_name(name);
    if (a<=4) return a;
    set_node(a, tPRIM, name, 0);
    return a;
}

AST pointer_type(char *name, AST e) {
    AST a = new_AST();
    set_node(a, tPOINTER, name, 0); 
    set_typeofnode(a, e);
    return a;
}

AST func_type(char *name, AST e) {
    AST a = new_AST();
    set_node(a, tFUNC, name, 0); 
    set_typeofnode(a, e);
    return a;
}

AST array_type(char *name, AST e, int sz) {
    AST a = new_AST();
    set_node(a, tARRAY, name, sz); 
    set_typeofnode(a, e);
    return a;
}

void print_type(AST a) {
    int ty = nodetype(a); 
    int e;

    switch (ty) {
	case tPRIM:
	    printf("prim<%s>", get_text(a));
	    break;
	case tPOINTER:
	    printf("pointer<");
	    goto out_elem;
	case tFUNC:
	    printf("function<");
	    goto out_elem;
	case tARRAY:
	    printf("array(%d)<", get_ival(a));
out_elem:
	    get_sons(a, &e, 0, 0, 0);
	    print_type(e);
	    printf("> ");
	    break;
	default:
	    break;
    }
}

AST typeof_AST(AST t) {
    int idx;
    int ty=0;

    switch (nodetype(t)) {
	case nNAME:
	    idx = lookup_SYM_all(get_text(t));
	    ty = gettype_SYM(idx);
	    break;
	case nVREF: case nVAR:
	    idx = get_ival(t);
	    ty  = gettype_SYM(idx);         
	    break;

	case nCON:
	    ty = get_typeofnode(t);
	    break;

	case nOP2: case nOP0: case nOP1:  
	    ty = typeof_AST(get_typeofnode(t));
	    break;

	case nDEREF:
	    ty = typeof_AST(get_typeofnode(t));
	    if (nodetype(ty) != tPOINTER) {
		parse_error("expected pointer type");
	    }
	    ty = get_typeofnode(ty);
	    break;

	case nLVAL:
	    ty = typeof_AST(get_typeofnode(t));
	    AST exprs = 0;
	    get_sons(t, 0, &exprs, 0, 0);
	    while (get_son0(exprs)){
		if (nodetype(ty) != tARRAY) {
		    parse_error("expected array type");
		} else ty = get_son0(ty);
		get_sons(exprs, 0, &exprs, 0, 0);
	    }
	    break;

    }
    return ty;
}

static bool equal(AST x1, AST x2) {
    char *text1, *text2;
    switch (nodetype(x1)) {
	case tINT: case tCHAR: case tFLOAT: case tSTRING:
	case tPRIM: case tCLASS: case tSTRUCT:
	    if (x1 == x2) return true;
	    text1 = get_text(x1);
	    text2 = get_text(x2);
	    if ((strncmp(text1, "int")==0 || strncmp(text1, "char") == 0) && (strncmp(text2, "int")==0 || strncmp(text2, "char") == 0)) return true;
	    break;
	case tARRAY:
	    if (nodetype(x2) != tARRAY) break;
	    if (get_ival(x1) != get_ival(x2)) break;
	    return equal( elem(x1), elem(x2));
	case tPOINTER:
	    if (nodetype(x2) != tPOINTER) break;
	    return equal( elem(x1), elem(x2));
	default:
	    break;
    }

    return false;
}

bool equaltype(AST a1, AST a2) {
    int x1 = typeof_AST(a1);
    int x2 = typeof_AST(a2);

    return equal(x1,x2);
}

void checktypesingleop(AST a){
    int x1 = typeof_AST(a);
    char *text = get_text(x1);
    switch(nodetype(x1)){
	case tPRIM: 
	    if (strncmp(text, "int") == 0 || strncmp(text, "char") == 0 ||strncmp(text, "float") == 0 ||strncmp(text, "pointer") == 0) return;
	    break;
	case tINT: case tCHAR: case tFLOAT: case tPOINTER: case tARRAY:
	    return;
    }
    printf("%s\n", text);
    parse_error("Expected type int, char, float or pointer");
}

bool checkArgs(AST argsdecl, AST args){
    AST mArgsdecl = argsdecl;
    AST mArgs = args;
    AST definition = 0, checked = 0;
    while (mArgsdecl){
	if (!mArgs) return false;
	get_sons(mArgsdecl, &definition, &mArgsdecl, 0, 0);
	get_sons(mArgs, &checked, &mArgs, 0, 0);
	if (definition){
	    if (!checked) return false;
	    AST first = get_son0(definition);
	    AST second = get_son0(checked);
	    if (first){
		if (!second)return false;
		if (!equaltype(first, second)) return false;
	    }
	}
    }
    return mArgsdecl == 0 && mArgs == 0;
}

int get_sizeoftype(AST ty) {
    switch (nodetype(ty)) {
	case tFUNC: return 0;
	case tARRAY: 
		    return get_ival(ty) * get_sizeoftype(elem(ty));
	default:
		    return 1;
    }
}
