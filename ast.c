#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "type.h" /* for primtype */
#include "ast.h"
#include "token.h"

static Node *ast_buf;
static int   ast_cnt;

static int xml = 1;
static int eliminate_null_list = 1;

static void print_Node_begin(Node*);
static void print_Node_end(Node*);

static char *namestr[] = {
    "prog",
    "@classdecls", "classdecl",
    "@structs", "struct",
    "@vardecls", "vardecl", 
    "@funcdecls", "funcdecl", 
    "@argdecls", "argdecl", 
    "@args", "arg", 
    "classhead", "block", 
    "@stmts", "stmt", "asn", "if", "@exprs", "expr", "vref", "while", 
    "op0", "op1", "op2", "deref", "lval", 
    "call", "return", "break", "continue", 
    "@vars", "var", "con", "name",
    0
} ;

static char *tnamestr[] = {
    "int", "char", "float", "string", "void", 
    "prim", "arraytype", "pointer", "structtype", "functype",
    0
};

static char *nameof(int n) {
    if (n >= nPROG && n < nEND)
	return namestr[n-nPROG];
    else if (n >= tINT && n <= tFUNC)
	return tnamestr[n-tINT];
    else
	return "";
}

void init_AST() {
    int sz = (MAX_AST_NODE+1) * sizeof(Node);
    ast_buf = (Node *)malloc(sz);
    bzero(ast_buf, sz);
    ast_cnt = 0;

    make_AST_prim("int");
    make_AST_prim("char");
    make_AST_prim("float");
    make_AST_prim("string");
    make_AST_void("void");
}

AST new_AST() {
    if (ast_cnt < MAX_AST_NODE-1)
	return ++ast_cnt;
    return 0;
}

void set_node(AST a, int type, char *text, int ival) {
    Node *np;

    if (a==0) return;
    np = &ast_buf[a];
    np->type = type;
    np->text = text ;
    np->ival = ival;
}

void get_node(AST a, int *type, char *text, int *ival) {
    Node *np;

    if (a==0) return;
    np = &ast_buf[a];
    if (type) *type = np->type;
    if (text)  text = np->text;
    if (ival) *ival = np->ival;
}

void set_sons(AST a, AST s0, AST s1, AST s2, AST s3) {
    Node *np;

    if (a==0) return;
    np = &ast_buf[a];
    np->son[0] = s0; if (s0) ast_buf[s0].father = a;
    np->son[1] = s1; if (s1) ast_buf[s1].father = a;
    np->son[2] = s2; if (s2) ast_buf[s2].father = a;
    np->son[3] = s3; if (s3) ast_buf[s3].father = a;
}

void get_sons(AST a, AST *s0, AST *s1, AST *s2, AST *s3) {
    Node *np;

    if (a==0) return;
    np = &ast_buf[a];
    if (s0) *s0 = np->son[0]; 
    if (s1) *s1 = np->son[1]; 
    if (s2) *s2 = np->son[2]; 
    if (s3) *s3 = np->son[3]; 
}

void copy_AST(AST dst, AST src) {
    int ival, op;
    char *text;
    int sons[4];
    get_node(src, &ival, text, &op);
    set_node(dst, ival, strdup(text), op);
    get_sons(src,  &sons[0], &sons[1], &sons[2], &sons[3]);
    set_sons(dst,  sons[0], sons[1], sons[2], sons[3]);
}

int nodetype(AST a) {
    Node *np;
    if (a==0) return 0;
    np = &ast_buf[a];
    return np->type;
}

void set_nodetype(AST a,int n) {
    Node *np;
    if (a==0) return ;
    np = &ast_buf[a];
    np->type = n;
}

AST make_AST(int type, AST s0, AST s1, AST s2, AST s3) {
    AST a = new_AST();
    Node *np = &ast_buf[a];

    np->type = type;
    set_sons(a, s0, s1, s2, s3);
    return a;
}

AST make_AST_prim(char *text) {
    AST a = new_AST();
    Node *np;
    np = &ast_buf[a];
    np->type = tPRIM;
    np->text = text;
    np->ival = a;
    return a;
}

AST make_AST_void(char *text) {
    AST a = new_AST();
    Node *np;
    np = &ast_buf[a];
    np->type = tVOID;
    np->text = text;
    np->ival = a;
    return a;
}

AST make_AST_asn(int op, AST s0, AST s1) {
    AST a = new_AST();
    set_node(a, nASN, 0, op);
    set_sons(a, s0, s1, 0, 0);
    return a;
}

AST make_AST_if(AST s0, AST s1, AST s2) {
    AST a = new_AST();
    set_node(a, nIF, 0, 0);
    set_sons(a, s0, s1, s2, 0);
    return a;
}

AST make_AST_while(AST s0, AST s1) {
    AST a = new_AST();
    set_node(a, nWHILE, 0, 0);
    set_sons(a, s0, s1, 0, 0);
    return a;
}

AST make_AST_break() {
    AST a = new_AST();
    set_node(a, nBREAK, 0, 0);
    return a;
}


AST make_AST_continue() {
    AST a = new_AST();
    set_node(a, nCONTINUE, 0, 0);
    return a;
}

AST make_AST_vardecl(AST name, AST type) {
    AST a = new_AST();
    Node *np = &ast_buf[a];
    set_node(a, nVARDECL, 0, 0);
    if (name) np->son[0] = name;
    if (type) np->son[1] = type;
    return a;
}

AST make_AST_argdecl(AST name, AST type) {
    AST a = new_AST();
    Node *np = &ast_buf[a];
    set_node(a, nARGDECL, 0, 0);
    if (name) np->son[0] = name;
    if (type) np->son[1] = type;
    return a;
}

AST make_AST_funcdecl(AST name, AST type, AST args, AST block) {
    AST a = new_AST();
    Node *np = &ast_buf[a];
    Node *son = &ast_buf[type];
    set_node(a, nFUNCDECL, 0, 0);
    if (name) np->son[0] = name;
    if (type) np->son[1] = type;
    if (args) np->son[2] = args;
    if (block) np->son[3] = block;
    return a;
}

AST make_AST_structdecl(AST name, AST vardecls){
    AST a = new_AST();
    set_node(a, nSTRUCT, 0, 0);
    set_sons(a, name, vardecls, 0, 0);
    return a;
}

AST make_AST_var(char *text,int val) {
    AST a = new_AST();
    set_node(a, nVAR, text, val);
    return a;
}

AST make_AST_vref(char *text,int val) {
    AST a = new_AST();
    set_node(a, nVREF, text, val);
    return a;
}

AST exists(char *text) {
    int i;
    Node *np = &ast_buf[1];

    for (i=1;i<=ast_cnt;i++,np++) {
	if ((np->type == nNAME || np->type == tPRIM || np->type == tVOID)
		&& strcmp(text, np->text)==0) return i;
    }
    return 0;
}

AST make_AST_name(char *text) {
    AST idx;
    AST a=0;

    if (idx = exists(text))
	return idx;

    a = new_AST();
    set_node(a, nNAME, text, 0);
    return a;
}

AST make_AST_con(char *text, int val) {
    AST a = new_AST();
    set_node(a, nCON, text, val);
    return a;
}

AST make_AST_op2(int op, AST s0, AST s1) {
    AST a = new_AST();
    set_node(a, nOP2, 0, op);
    set_sons(a, s0, s1, 0, 0);
    return a;
}

AST make_AST_op0(int op, AST s){
    AST a = new_AST();
    set_node(a, nOP0, 0, op);
    set_sons(a, s, 0, 0, 0);
    return a;
}

AST make_AST_op1(int op, AST s){
    AST a = new_AST();
    set_node(a, nOP1, 0, op);
    set_sons(a, s, 0, 0, 0);
    return a;
}


bool isleaf(AST a) {
    int i;
    Node *np = &ast_buf[a];

    for (i=0;i<4;i++) {
	if (np->son[i]) return false;
    } 
    return true;
}

bool islist(AST a) {
    Node *np = &ast_buf[a];

    return nameof(np->type)[0] == '@';
}

char *get_text(AST a) {
    Node *np = &ast_buf[a];
    return (a && np->text) ? np->text : "";
}

int get_ival(AST a) {
    Node *np = &ast_buf[a];
    return (a) ? np->ival : 0;
}

void set_ival(AST a, int v) {
    Node *np = &ast_buf[a];
    if (np) np->ival = v;
}

AST get_son0(AST a) {
    Node *np = &ast_buf[a];
    return (a) ? np->son[0] : 0;
}

static void set_son0(AST a, AST s0) {
    Node *np = &ast_buf[a];
    if (s0) np->son[0] = s0;
}

static void set_son1(AST a, AST s1) {
    Node *np = &ast_buf[a];
    if (s1) np->son[1] = s1;
}

inline AST get_typeofnode(AST a) {
    return get_son0(a);
}

inline void set_typeofnode(AST a, AST t) {
    set_son0(a,t);
}

inline void set_argtypeofnode(AST a, AST t) {
    set_son1(a,t);
}

AST new_list(int type) {
    AST a = new_AST();
    Node *np = &ast_buf[a];

    np->type = type;
    np->son[0] = np->son[1] = 0;
    return a;
}

/* caution: very slow ! */
AST append_list(AST l, AST a1) {
    Node *np;
    int type;
    AST a=l, a2;

    if (l==0) return l;
    np = &ast_buf[a];
    type = np->type;
    while (np->son[1]) {
	a = np->son[1];
	np = &ast_buf[a];
    }
    a2 = make_AST(type, 0, 0, 0, 0);
    set_sons(a,a1,a2,0,0);
    return l;
}

static void abbrib(Node *np) {
    switch (np->ival) {
	case '+': printf("plus<"); break;
	case '-': printf("minus<"); break;
	case '*': printf("times<"); break;
	case '/': printf("divide<"); break;
	case '%': printf("modulo<"); break;
	default: break;
    }
}

static void print_Node_begin(Node *np)  {
    char *s;

    if (np->type == nOP2 || np->type == nOP1 || np->type == nOP0) {
	int first = 0, second = 0;
	switch(np->ival){
	    case PLUSPLUS: case MINUSMINUS: case ANDAND: case OROR: case LSHIFT: case RSHIFT:
		second = np->ival - 0x80;
		first = second;
		break;	
	    case PLUSEQ: case MINUSEQ: case STAREQ: case SLASHEQ: case PERCENTEQ: case GTEQ: case LTEQ: case EQEQ: case NOTEQ:
		second = '='; 
		first = np->ival - 0x60;
		break;
	    default:
		first = np->ival;
		break;
	}

	char a[11];
	char temp[2];
	if (xml){
	    switch (first){
		case GT:
		    strcpy(a, "&gt;");
		    break;
		case LT:
		    strcpy(a, "&lt;");
		    break;
		case '&':
		    strcpy(a, "&amp;");
		    break;
		default:
		    temp[0] = first;
		    temp[1] = '\0';
		    strcpy(a, temp);
		    break;
	    }
	    if (second){	
		switch (second){
		    case GT:
			strcat(a, "&gt;");
			break;
		    case LT:
			strcat(a, "&lt;");
			break;
		    case '&':
			strcpy(a, "&amp;");
			break;
		    default:
			temp[0] = second;
			temp[1] = '\0';
			strcat(a, temp);
			break;
		}
	    }
	}
	switch (np->type){
	    case nOP2:
		if (!xml)
		    second ? printf("op2<\'%c%c\',", first, second) : printf("op2<\'%c\',", first);
		else
		    printf("<op2 val=\"%s\">", a);
		/*second ? printf("<op2 val=\"%c%c\">", first, second) : printf("<op2 val=\"%c\">", first);*/
		return;

	    case nOP1:
		if (!xml)
		    second ? printf("op1<\'%c%c\',", first, second) : printf("op1<\'%c\',", first);
		else
		    second ? printf("<op1 val=\"%c%c\">", first, second) : printf("<op1 val=\"%c\">", first);
		return;

	    case nOP0:
		if (!xml)
		    second ? printf("op0<\'%c%c\',", first, second) : printf("op0<\'%c\',", first);
		else
		    second ? printf("<op0 val=\"%c%c\">", first, second) : printf("<op0 val=\"%c\">", first);
		return;
	}

    }

    if (np->type == tPRIM) {
	if (!xml) printf("prim<%s", np->text);
	else printf("<prim>%s", np->text);
	return;
    }

    s = nameof(np->type);

    if (xml) {
	if (*s == '@') s++;
	printf("<%s", s);
    } else {
	if (*s == '@') { printf("\n"); s++ ;}
	printf("%s<", s);
    }

    switch (np->type) {
	case tVOID:
	case nNAME:
	    if (xml) {
		printf(">%s", np->text);
	    } else {
		printf("\"%s\"", np->text);
	    }
	    break;
	case nVREF :
	case nVAR :
	    if (xml) {
		printf(" val=\"%d\"", np->ival);
		printf(">%s",     np->text);
	    } else {
		printf("\"%s\"",  np->text);
		printf("(%d)",    np->ival);
	    }
	    break;
	case nCON :
	    if (xml) {
		printf(" val=\"%d\" name=\"%s\">", np->ival, np->text);
	    } else  {
		printf("\"%s\"",  np->text);
		printf("(%d)",    np->ival);
	    }
	    break;
	    /*
	       case nASN : 
	       if (xml) {
	       printf(" op=\"%d\">", np->ival); 
	       } else {
	       printf("%d,", np->ival);
	       }
	       break;
	     */
	default: 
	    if (xml) printf(">");
	    break;
    }
}

static void print_Node_end(Node *np) {

    char *s = nameof(np->type);

    if (xml) {
	if (*s == '@') s++;
	printf("</%s>", s);
    } else {
	printf(">");
    }
}

void print_AST(AST a) {
    Node *np = &ast_buf[a];
    int i;

    print_Node_begin(np); 
    for (i=0;i<4;i++) {
	if (np->son[i]) {
	    if (eliminate_null_list && islist(np->son[i]) && isleaf(np->son[i]))
		continue;
	    if (!xml && i) printf(",");
	    if (np->type == nIF && i >= 1) printf("\n");
	    print_AST(np->son[i]);
	} 
    }
    print_Node_end(np);
}

void dump_sons(Node *np,FILE *fp) {
    int i;
    fprintf(fp,"[");
    for (i=0;i<4;i++) {
	if (i>0) fprintf(fp," ");
	fprintf(fp,"%4d",np->son[i]);
    }
    fprintf(fp,"]");
}

void dump_AST(FILE *fp) {
    int i;
    fprintf(fp,"AST:cnt=%d\n", ast_cnt);
    for (i=5;i<=ast_cnt;i++) {
	Node *np = &ast_buf[i];
	char *s = nameof(np->type);
	if (s && s[0] == '@') s++;
	fprintf(fp,"%4d:",i);
	fprintf(fp,"<%-10s %-10s %4d %4d>",
		s, (np->text)?np->text:"@", np->ival, np->father);
	dump_sons(np,fp);
	fprintf(fp,"\n");
    }
}


static int valueof(char *text, char **list) {
    int i;
    for (i=0;list[i];i++) {
	char *s = list[i];
	if (s[0] == '@') s++;
	if (strcmp(s,text)==0) 
	    return i;
    }
    return -1;
}

static int nodetypeval(char *text) {
    int i;
    if ((i = valueof(text, namestr )) >=0)  return i+nPROG;
    if ((i = valueof(text, tnamestr)) >=0)  return i+tINT;
    return 0;
}

int restore_AST(FILE *fp) {
    int i,k;
    Node *np;
    char kind[40];
    char text[40];
    char buf[256];

    while (fgets(buf,256,fp)>0) {
	if (strncmp(buf,"AST:", 4)==0) goto top;
    }
    printf("incorrect input file\n");
    return 0;

top:
    sscanf(buf, "AST:cnt=%d\n", &ast_cnt);
    for (i=5;i<=ast_cnt;i++) {
	bzero(kind,40); bzero(text,40);

	fscanf(fp,"%4d:", &k);
	np = &ast_buf[i];

	fscanf(fp,"<%s %s %d %d>[%d %d %d %d]\n",
		kind, text,  &(np->ival), &(np->father),
		&(np->son[0]), &(np->son[1]), &(np->son[2]), &(np->son[3]) );
	np->type = nodetypeval(kind);
	np->text = strdup(text);
    }
    return ast_cnt;
}
