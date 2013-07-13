#ifndef _LOC_H_
#define _LOC_H_

#define MAX_LOC 1000

typedef struct locentry {
    int depth;
    int offset;
    int size;
} locentry;

void init_LOC();
int new_loc();
void set_loc_entry(int e, int d, int o, int s);
void get_loc_entry(int e, int *d, int *o, int *s);
int lookup_loc_entry(int dep, int off, int size);

int getdepth_LOC(int e);
int getoffset_LOC(int e);
int getsize_LOC(int e);

void dump_LOC(FILE*);
int  restore_LOC(FILE*);

#endif
