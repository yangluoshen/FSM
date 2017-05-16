#ifndef __DICT_H
#define __DICT_H

#include <stdio.h>
#include "flist.h"

typedef void* fdict_key_t;

typedef struct fdict{
    size_t hash_size;
    flist** hash_list;
    int (*match)(void*, fdict_key_t);
    size_t (*calc)(struct fdict*, fdict_key_t);
}fdict;

typedef int (*hash_match_func)(void* ptr, fdict_key_t key);
typedef size_t (*hash_calc_func)(fdict* d, fdict_key_t key);

fdict* fdict_create(size_t hash_size, hash_match_func match, hash_calc_func calc);
void fdict_release(fdict* d);
int fdict_insert(fdict* d, fdict_key_t key, void* val);
int fdict_remove(fdict* d, fdict_key_t key);
void* fdict_find(fdict* d, fdict_key_t key);

enum E_FDICT_RETCODE{
    FDICT_SUCCESS = 0,
    FDICT_FAILED,
    FDICT_EXIST,
    FDICT_NOTEXIST
};


#endif
