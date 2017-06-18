#ifndef __DICT_H
#define __DICT_H

#include <stdint.h>

typedef struct dict_entry{
    union {
        void* ptr;
        uint32_t u32;
        int32_t s32;
        double d;
    } key, val;

    struct dict_entry* next;
}dict_entry;

typedef struct dict_option{
    uint32_t (*hash_func)(uint32_t hash_size, const void* key);
    int (*key_comp)(const void* key1, const void* key2);
    void* (*key_dup)(const void* key);
    void (*key_destructor)(void* key);
    void* (*val_dup)(const void* val);
    void (*val_destructor)(void* val);
}dict_option;

typedef struct dict_ht{
    uint32_t size;
    uint32_t used;
    dict_entry** table;
}dict_ht;

typedef struct dict{
    dict_ht ht[2];
    dict_option* op;
    int serve;
}dict;

dict* dict_create(const dict_option* op);
void dict_release(dict* d);
dict_entry* dict_add(dict* d, void* key, void* val);
int dict_delete(dict* d, void* key);
dict_entry* dict_find(dict* d, void* key);

uint32_t hash_calc_int(uint32_t hash_size, const void* key);
uint32_t hash_calc_str0(uint32_t hash_size, const void* key);
uint32_t hash_calc_str1(uint32_t hash_size, const void* key);
void print_dict(dict* d, void (*p)(void*, void*));

#define DICT_GETVAL(entry) ((entry)->val.ptr)
#endif
