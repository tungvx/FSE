#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "loc.h"

static locentry *loctab;
static int loccnt;

static int var_offset = 0;
static int arg_offset = -4;

void init_LOC() {
    int l;
    loctab = (locentry*)malloc(l = MAX_LOC * sizeof(locentry));
    bzero(loctab, l);
    loccnt = 0;
}

int new_loc() {
    return ++loccnt;
}

void set_loc_entry(int e, int dep, int off, int size) {
    locentry *ep;
    if (e==0) return;
    ep = &loctab[e];
    ep->depth = dep;
    ep->offset = off;
    ep->size = size;
}

void get_loc_entry(int e, int *dep, int *off, int *size) {
    locentry *ep;
    if (e==0) return;
    ep = &loctab[e];
    if (dep) *dep = ep->depth;
    if (off) *off = ep->offset;
    if (size) *size = ep->size;
}

int lookup_loc_entry(int dep, int off, int size) {
    int e;
    for (e=1;e<=loccnt;e++) {
        locentry *ep = &loctab[e];
        if (ep->depth == dep && ep->offset == off && ep->size == size)
            return e;
    }
    return new_loc();
}

int getdepth_LOC(int e) {
    locentry *ep;
    if (e==0) return 0;
    ep = &loctab[e];
    return ep->depth;
}

int getoffset_LOC(int e) {
    locentry *ep;
    if (e==0) return 0;
    ep = &loctab[e];
    return ep->offset;
}

int getsize_LOC(int e) {
    locentry *ep;
    if (e==0) return 0;
    ep = &loctab[e];
    return ep->size;
}

inline int  get_var_offset()        { return var_offset; }
inline int  get_arg_offset()        { return arg_offset; }
inline void reset_offset()          { var_offset = 0; arg_offset = -4;}
inline void incr_var_offset(int sz) { var_offset += sz;  }
inline void decr_arg_offset(int sz) { arg_offset -= sz;  }

void dump_LOC(FILE *fp) {
    int i;
    locentry *ep = &loctab[1];
    fprintf(fp, "LOC:cnt=%d\n", loccnt);
    for (i=1;i<=loccnt;i++, ep++) {
         fprintf(fp, "%4d: %4d %4d %4d\n", i, 
                ep->depth, ep->offset, ep->size);
    }
    fprintf(fp, "\n");
}

int restore_LOC(FILE *fp) {
    int i,k;
    locentry *ep;
    fscanf(fp, "\nLOC:cnt=%d\n", &loccnt);
    for (i=1;i<=loccnt;i++) {
         ep = &loctab[i];
         fscanf(fp, "%4d: %4d %4d %4d\n", &k, 
                &(ep->depth), &(ep->offset), &(ep->size));
    }
}
