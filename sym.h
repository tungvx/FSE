#ifndef _SYM_H_
#define _SYM_H_

#define MAX_SYMENTRY 1000
#define MAX_SCOPEENTRY 100

enum {
    vLOCAL=2048, vGLOBAL,  /* variables */
    fLOCAL, fGLOBAL, 	  /* functions */
    cLOCAL, cGLOBAL, 	  /* constants */
    tLOCAL, tGLOBAL,	  /* datatypes (classes) */
    vARG,
    pEND
};

typedef struct {
    char *name;
    int  type;
    int  prop;
    int  val;
    int  loc;
} symentry ;

typedef struct scope {
    struct scope *next;
    int begin;
    int end;
} scope;

void init_SYM();
int insert_SYM(char*,int,int,int);
int lookup_SYM(char*);
int lookup_SYM_all(char*);

void setval_SYM(int,int);
void setprop_SYM(int,int);
int  getval_SYM(int);
int  gettype_SYM(int);

void enter_block(void);
void leave_block(void);
void mark_args(void);
void unmark_args(void);

char *gen(int);

void dump_SYM(FILE*);
int  restore_SYM(FILE*);

#endif
