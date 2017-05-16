#ifndef __LIST_H
#define __LIST_H

typedef struct fnode{
    void *val;
    struct fnode* next;
}fnode;

typedef struct flist{
    int (*match)(void* ptr, void* key);
    fnode* next;
}flist;

#define FLST_SET_MATCH(l, m) ((l)->match = (m))

flist* flist_create(int (*match)(void*, void*));
void flist_init(flist* l);
void flist_release(flist *l);
fnode* flist_append(flist* l, void* val);
void flist_delete(flist* l, void* val);
fnode* flist_find(flist* l, void* val);

#endif 
