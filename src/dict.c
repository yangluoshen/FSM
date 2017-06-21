/*  dict -- A C hash table implemented with seperate chaining.
 *
 *  Copyright (c) 2017, yangluo shen <yangluoshen at gmail dot com>

 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#define _BSD_SOURCE
#include "dict.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#define MIN_HASHNUM (100)
#define HASHNUM_DIV (5)
#define FILL_FACTOR (50)

#define dict_free_key(d, entry) \
    if ((d)->op->key_destructor) \
        (d)->op->key_destructor((entry)->key.ptr)

#define dict_free_val(d, entry) \
    if ((d)->op->val_destructor) \
        (d)->op->val_destructor((entry)->val.ptr)

#define dict_hash(size, key)  ((d)->op->hash_func(size, key))

#define dict_key_compare(d, key1, key2) \
    ((d)->op->key_comp ? (d)->op->key_comp(key1, key2) : \
        (key1) == (key2)) 

#define dict_set_key(d, entry, _key) \
    do{(entry)->key.ptr = (d)->op->key_dup ? (d)->op->key_dup(_key) : _key;}while(0)

#define dict_set_val(d, entry, _val) \
    do{(entry)->val.ptr = (d)->op->val_dup ? (d)->op->val_dup(_val) : _val;}while(0)

#define is_need_rehash(d) ((((d)->ht[d->serve].used * 100) / (_hash_size(d) * HASHNUM_DIV)) > FILL_FACTOR)

#define serve_table(d) ((d)->ht[(d)->serve].table)
#define idle_table(d) ((d)->ht[!(d)->serve].table)
#define dict_incr(d) do{((d)->ht[(d)->serve].used++);}while(0)
#define dict_decr(d) do{((d)->ht[(d)->serve].used--);}while(0)
#define table_swap(d) do{(d)->serve = !(d)->serve;}while(0)
#define _hash_size(d) ((d)->ht[(d)->serve].size)


uint32_t prime_pool[] = {13,31,61,127,251,509,1021,2039,4093,8191,16381,32749,65521,131071,262139,524287,1048573,2097143,4194301,8388593,16777213,33554393,67108859,134217689,268435399,536870909,1073741789,2147483647};

uint32_t _get_next_hashsize(uint32_t size)
{
    uint32_t i;
    uint32_t n = sizeof(prime_pool)/sizeof(prime_pool[0]);
    for (i=0; i < n; ++i)
        if (prime_pool[i] > size)
            return prime_pool[i];

    return prime_pool[n-1];
}

int  _dict_init(dict* d, const dict_option* op, uint32_t hashsize)
{
    d->op = (dict_option*) malloc(sizeof(dict_option));
    if (!d->op) return -1;

    d->serve = 0;
    serve_table(d) = (dict_entry**) malloc(hashsize * sizeof(dict_entry*));
    if (!serve_table(d)) {
        free (d->op);
        return -1;
    }
    d->ht[d->serve].size = hashsize;
    d->ht[d->serve].used = 0;
    // valgrind would compain  "uninitialised value"
    //memset(serve_table(d), 0, sizeof(hashsize * sizeof(dict_entry*)));

    int i;
    for (i = 0; i < hashsize; ++i){
        serve_table(d)[i] = NULL;
    }

    d->ht[!d->serve].table = NULL;
    
    d->op->hash_func = op->hash_func;
    d->op->key_dup = op->key_dup;
    d->op->val_dup = op->val_dup;
    d->op->key_comp = op->key_comp;
    d->op->key_destructor = op->key_destructor;
    d->op->val_destructor = op->val_destructor;

    return 0;
}

dict* _dict_create(const dict_option* op, uint32_t hashsize)
{
    dict* d = (dict*) malloc(sizeof(dict));
    if (!d) return NULL;

    if (-1 == _dict_init(d, op, hashsize)){
        free (d);
        return NULL;
    }

    return d;
}

dict* dict_create(const dict_option* op)
{
    if (!op || !op->hash_func) return NULL;

    return _dict_create(op, prime_pool[0]);
}

void _clear_hash_table(dict* d, dict_entry** table, uint32_t size)
{
    uint32_t i;
    for (i=0; i<size; ++i){
        if (table[i] == NULL) continue;

        dict_entry** indirect = &(table[i]);
        while (*indirect) {
            dict_entry* he = *indirect;

            dict_free_key(d, he);
            dict_free_val(d, he);
            free (he);
            // next loop
            indirect = &((*indirect)->next);
        }
    }
    free (table);
}

void _dict_clear(dict* d)
{
    _clear_hash_table(d, serve_table(d), _hash_size(d));
    
    d->ht[d->serve].size = 0;
    free (d->op);
}

void dict_release(dict* d)
{
    if (!d) return;

    _dict_clear(d);

    free (d);
}


dict_entry* _dict_add(dict*d, dict_ht* ht, void* key, void* val)
{
    uint32_t hash = dict_hash(ht->size, key);
    
    // invalid hash will return NULL
    if (hash >= ht->size) return NULL;

    dict_entry* he = ht->table[hash];
    while (he){
        // if already exist, return NULL
        if (dict_key_compare(d, key, he->key.ptr))
            return NULL;

        he = he->next;
    }

    he = (dict_entry*) malloc(sizeof(dict_entry));
    if (!he) return NULL;

    // push head
    he->next = ht->table[hash];
    ht->table[hash] = he;

    ht->used ++;
    dict_set_key(d, he, key);
    dict_set_val(d, he, val);

    return he;
}

void _rehash(dict* d)
{
    uint32_t new_size = _get_next_hashsize(_hash_size(d));

    idle_table(d) = (dict_entry**) malloc(new_size * sizeof(dict_entry*));
    if (!idle_table(d)) return ;
    d->ht[!d->serve].size = new_size;
    d->ht[!d->serve].used = 0;
    // valgrind would compain  "uninitialised value"
    //memset(idle_table(d), 0, sizeof(new_size * sizeof(dict_entry*)));
    uint32_t i;
    for (i=0; i < new_size; ++i){
        idle_table(d)[i] = NULL;
    }

    for (i=0; i < _hash_size(d); ++i){
        dict_entry* he = serve_table(d)[i];
        while (he){
            if (NULL == _dict_add(d, &(d->ht[!d->serve]), he->key.ptr, he->val.ptr)){
                //dict_release(new_d);
                //return d;
            }
            he = he->next;
        }
    }

    _clear_hash_table(d, serve_table(d), _hash_size(d));
    table_swap(d);
    //printf("rehash :%u\n", _hash_size(d));
}

dict_entry* dict_add(dict* d, void* key, void* val)
{
    if (!d) return NULL;

    if (is_need_rehash(d))
        _rehash(d);
    
    return _dict_add(d, &(d->ht[d->serve]), key, val);
}

dict_entry* dict_find(dict* d, void* key)
{
    if (!d) return NULL;

    uint32_t hash = dict_hash(_hash_size(d), key);
    if (hash >= _hash_size(d)) return NULL;

    dict_entry* he = serve_table(d)[hash];
    while (he){
        if (dict_key_compare(d, key, he->key.ptr))
            return he;

        he = he->next;
    }

    return NULL;
}

int dict_delete(dict* d, void* key)
{
    if (!d) return -1;

    uint32_t hash = dict_hash(_hash_size(d), key);
    if (hash > _hash_size(d)) return -1;

    dict_entry** in_he = &(serve_table(d)[hash]);
    while(*in_he){
        if (dict_key_compare(d, key, (*in_he)->key.ptr)){
            dict_entry* he = *in_he;
            dict_free_key(d, he);
            dict_free_val(d, he);

            *in_he = (*in_he)->next;
            free(he);

            dict_decr(d);
            return 0;
        }

        in_he = &((*in_he)->next);
    }

    return -1;
}

/* key为整数的散列函数
 * 一个简单的散列函数
 * 注意: 要留意原key的类型是否与uint32_t是一致的。
 * 若不一致，不应该使用此散列函数，否则可能会导致错误，
 * 尤其是key的类型字节长为比uint32_t小时（如short,char,unsigned char）
 */
uint32_t hash_calc_int(uint32_t hash_size, const void* key)
{
    uint32_t hash = (const long)key % hash_size;
    return hash;
}

/* key 为字符串的散列函数
 * 计算简单，但是不适合key特别长的情况
 * 因为key太长的话，此散列函数会花很多时间计算
 */
uint32_t hash_calc_str0(uint32_t hash_size, const void* key)
{
    uint32_t hash = 0;
    char* k = (char*)key;
    while(*k != 0){
        hash = (hash << 5) + *k++;
    }
    return (uint32_t) (hash % hash_size);
}

uint32_t hash_calc_str1(uint32_t hash_size, const void* key)
{
    if (!key) return -1;
    
    char* name = (char*) key;
    unsigned int hash = 0;
    while(*name){
        hash = (hash << 4) + *name;
        hash ^= (hash & 0xF0000000) >> 24;
        name++;
    }
    hash &= 0x0FFFFFFF;
    return (uint32_t) (hash % hash_size);
}

void print_dict(dict* d, void (*p)(void*, void*))
{
    int i;
    for (i=0; i < _hash_size(d); ++i){
        printf ("[%4u]: ", i);
        dict_entry* he = serve_table(d)[i];
        while (he){
            p(he->key.ptr, he->val.ptr);
            he = he->next;
        }
        printf("\n");
    }
}
#ifdef FDICT_MAIN
#include <assert.h>
#include <stdio.h>

typedef struct {
    char* name;
    int price;
}fruit;

fruit* gen_fruit(const char* name, int price)
{
    fruit* f = (fruit*) malloc(sizeof(fruit));
    if (!f) return NULL;

    f->name = strdup(name);
    f->price = price;
    return f;
}

void* fruit_dup (const void* val)
{
    const fruit* f = (const fruit*) val;
    return gen_fruit(f->name, f->price); 
}

void fruit_free(void* val)
{
    if (!val) return;
    free (((fruit*)val)->name);
    free(val);
    val = NULL;
}

void* name_dup(const void* key)
{
    return strdup((const char*)key);
}

int fruit_comp(const void* key1, const void* key2)
{
    return strcasecmp((const char*)key1, (const char*)key2) == 0;
}

dict_option fruit_op = {
    hash_calc_str0,
    fruit_comp,
    name_dup,
    free,
    fruit_dup,
    fruit_free
};


fruit fruit_pool[] = {
    {"apple", 5999},
    {"banana", 5},
    {"pear", 4},
    {"straberry", 10},
    {"watermelon", 3},
    {"peach", 7}
};

void print_fruit(void* key, void* val)
{
    (void)key;
    fruit* f = (fruit*) val;
    printf(" {%10s,%5d} ", f->name, f->price);
}

void testcase1()
{
    dict* d = dict_create(&fruit_op);
    assert(d != NULL);
    
    assert(NULL!=dict_add(d, fruit_pool[0].name, &fruit_pool[0]));
    dict_entry* he = dict_find(d, "apple");
    assert(he && (5999 == ((fruit*)he->val.ptr)->price));
    assert(0 == dict_delete(d, "apple"));
    assert(!dict_find(d, "apple"));

    dict_release(d);
}

void testcase2()
{
    puts("testcase3");
    dict* d = dict_create(&fruit_op);
    assert(d != NULL);

    int pool_size = sizeof (fruit_pool) /sizeof(fruit_pool[0]), i;
    for (i=0; i< pool_size; ++i){
        assert(NULL!=dict_add(d, fruit_pool[i].name, &fruit_pool[i]));
    }
    
    print_dict(d, print_fruit);

    dict_entry* he = dict_find(d, "pear");
    assert(he && (4 == ((fruit*)he->val.ptr)->price));

    he = dict_find(d, "watermelon");
    assert(he && (3 == ((fruit*)he->val.ptr)->price));

    he = dict_find(d, "unknow fruit");
    assert(NULL == he);

    dict_release(d);
}

int main()
{
    testcase1();
    testcase2();

    return 0;
}
#endif
