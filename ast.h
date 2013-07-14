#ifndef _AST_H_
#define _AST_H_

#define MAX_AST_NODE 1000

// typedef struct Node *AST;
typedef int AST;
typedef enum { false=0, true=1 } bool;

typedef enum {
  nPROG=1024, 
  nCLASSDECLS, nCLASSDECL, 
  nSTRUCTS, nSTRUCT,
  nVARDECLS, nVARDECL, 
  nFUNCDECLS, nFUNCDECL, 
  nARGDECLS, nARGDECL, 		/* for funcdecl */
  nARGS, nARG,			/* for funcall  */
  nCLASSHEAD, nBLOCK, 
  nSTMTS, nSTMT, nASN, nIF, nEXPRS, nEXPR, nVREF, nWHILE,
  nOP0, nOP1, nOP2, nDEREF, nLVAL,
  nCALL, nRET, nBREAK, nCONTINUE,
  nVARS, nVAR, nCON, nNAME, 
  nEND, nERROR = -1
} node_type;

typedef struct Node {
  node_type type;
  char      *text;
  int       ival;
  AST       father;
  AST       son[4];
} Node ;

void set_node(AST a, int type, char *text, int ival);
void get_node(AST a, int *type, char *text, int *ival);
void set_sons(AST a, AST s0, AST s1, AST s2, AST s3);
void get_sons(AST a, AST *s0, AST *s1, AST *s2, AST *s3);

int make_AST(int,AST,AST,AST,AST); /* type, son[0..3] */

int make_AST_name(char*);
int make_AST_var(char*,int);
int make_AST_con(char*,int);
void copy_AST(AST dst, AST src);

AST new_list(int);
AST append_list(AST,AST);

void print_AST(AST);
bool isleaf(AST);
bool islist(AST);
char  *get_text(AST);
void   set_text(AST,char*);
int   get_ival(AST);
void  set_ival(AST,int);
int   nodetype(AST);
void  set_nodetype(AST,int);

/* for tFUNC */
void set_typeofnode(AST,AST);
void set_argtypeofnode(AST,AST);

void dump_AST(FILE*);
int  restore_AST(FILE*);

#endif /* _AST_H_ */
