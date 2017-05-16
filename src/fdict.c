#include "fdict.h"
#include <malloc.h>

#define MIN_HASHNUM (100)
#define HASHNUM_DIV (7)

/*@return : 0 means num is not a prime
 *          1 means num is a prime
 */
int is_prime(size_t num)
{
    size_t i;
    size_t t = num / 2;
    for (i = 2; i < t; ++i){
        if (0 == (num % i))
            return 0;
    }
    return 1;
}

size_t get_good_hashnum(size_t hash_size)
{
    if (hash_size < MIN_HASHNUM)
        return hash_size;
    size_t t = hash_size / HASHNUM_DIV;
    size_t i = t;
    for (i = t; i < hash_size; ++i){
        if (is_prime(i))
            return i;
    }
    if (0 == (i % 2)) i += 1;

    return i;
}

#if 0
size_t __hash_calc(fdict* d, fdict_key_t key)
{
    if (!d || !key) return -1;
    size_t hash = *((size_t*)key) % d->hash_size;
    return hash;
}
#endif

fdict* fdict_create(size_t hash_size, hash_match_func match, hash_calc_func calc)
{
    hash_size = get_good_hashnum(hash_size);
    if (0 == hash_size) return NULL;
    if (!match || !calc) return NULL;
    
    fdict* d = (fdict*) malloc (sizeof(fdict));
    if (!d) return NULL;
    d->hash_size = hash_size;
    d->calc = calc;
    d->match = match;

    d->hash_list = (flist**) malloc(hash_size * sizeof(flist*));
    if (!d->hash_list) {
        free (d);
        return NULL;
    }

    size_t i = 0;
    for (; i < hash_size; ++i){
        d->hash_list[i] = flist_create(d->match);
        // if create failed, release the resource alloced
        if (!d->hash_list[i]){
            int j;
            for (j=0; j < i; ++j){
                free(d->hash_list[j]);
                d->hash_list[j] = NULL;
            }
            free(d->hash_list);
            free(d);
            return NULL;
        }
    }
    
    return d;
}

void fdict_release(fdict* d)
{
    if (!d) return;
    size_t i = 0;
    for (; i < d->hash_size; ++i){
        flist_release(d->hash_list[i]);
    }

    free(d->hash_list);
    free (d);
}

/*@return: 0 success
 *         1 failed
 *         2 exist
 *@val: val is a customed pointer
 */
int fdict_insert(fdict* d, fdict_key_t key, void* val)
{
    if (!d) return FDICT_FAILED;
    if (fdict_find(d, key)) return FDICT_EXIST;

    size_t hash_slot = d->calc(d, key);
    if (-1==hash_slot || hash_slot>=d->hash_size) 
        return FDICT_FAILED;

    if (!flist_append(d->hash_list[hash_slot], val))
        return FDICT_FAILED;

    return FDICT_SUCCESS;
}

void* fdict_find(fdict* d, fdict_key_t key)
{
    if (!d) return NULL;
    fnode* n;
    size_t hash_slot = d->calc(d, key);
    if (-1 == hash_slot || hash_slot >= d->hash_size)
        n = NULL;
    n = flist_find(d->hash_list[hash_slot], key);
    return n ? n->val : NULL;
}

int fdict_remove(fdict* d, fdict_key_t key)
{
    if (!d) return FDICT_FAILED;
    fnode* n = fdict_find(d, key);
    if (!n) return FDICT_NOTEXIST;
    
    size_t hash_slot = d->calc(d, key);
    if (-1==hash_slot || hash_slot>=d->hash_size)
        return FDICT_FAILED;

    flist_delete(d->hash_list[hash_slot], key);
    return FDICT_SUCCESS;
}

#ifdef FDICT_MAIN
#include <assert.h>

typedef struct {
    int value;
    int id;
}node;

node* generate(int id, int val)
{
    node* n = (node*) malloc(sizeof(node));
    if (!n) return NULL;
    n->value = val;
    n->id = id;
    return n;
}

size_t calc_func(fdict* d, fdict_key_t key)
{
    if (!d || !key) return -1;
    int hash = *((int*)key) % d->hash_size;
    return (size_t)hash;
}
int match_func(void* ptr, fdict_key_t key)
{
    if (!ptr || !key) return 0;
    node* n = (node*) ptr;
    return (n->id == *(int*)key) ? 1 : 0;
}

#define GENERATE_DICT(d, s) \
    do {\
        (d) = fdict_create((s), match_func, calc_func);\
        assert(d);\
    }while(0)

void print_list(flist* l)
{
    fnode* n = l->next;
    while(n){
        node* v = n->val;
        if (v){
            printf(" %4d", v->value);
        }
        n = n->next;
    }
    puts("");
}

void print_dict(fdict* d)
{
    if (!d) return;
    puts("");
    size_t i; 
    for (i = 0; i < d->hash_size; ++i){
        printf("[%2lu]:", i);
        print_list(d->hash_list[i]);
    }
    puts("");
}

void testcase1()
{
    puts("testcase1");
    fdict* d;
    GENERATE_DICT(d,10);
    fdict_release(d);
}

void testcase2()
{
    fdict* d;
    GENERATE_DICT(d,10);
    
    node* node1 = generate(1, 1);
    
    assert(FDICT_SUCCESS==fdict_insert(d, &node1->id, node1));
    print_dict(d);
    assert(FDICT_SUCCESS!=fdict_insert(d, &node1->id, node1));
    int key = 1; 
    node* s = fdict_find(d, &key);
    assert(s);
    printf ("testcase2: [%d]-[%d] found\n", s->id, s->value); 
    fdict_release(d);
}

void testcase3()
{
    puts("testcase3");
    fdict* d;
    GENERATE_DICT(d,10);
    
    node* node1 = generate(1, 1);
    node* node2 = generate(2, 2);
    node* node3 = generate(3, 3);
    node* node5 = generate(5, 5);
    node* node15 = generate(15, 15);

    assert(FDICT_SUCCESS==fdict_insert(d, &node1->id, node1));
    assert(FDICT_SUCCESS==fdict_insert(d, &node2->id, node2));
    assert(FDICT_SUCCESS==fdict_insert(d, &node3->id, node3));
    assert(FDICT_SUCCESS==fdict_insert(d, &node5->id, node5));
    assert(FDICT_SUCCESS==fdict_insert(d, &node15->id, node15));
    assert(FDICT_SUCCESS!=fdict_insert(d, &node3->id, node3));
    print_dict(d);
    assert(FDICT_SUCCESS==fdict_remove(d, &node5->id));
    assert(FDICT_SUCCESS==fdict_remove(d, &node15->id));
    assert(FDICT_SUCCESS!=fdict_remove(d, &node15->id));
    assert(FDICT_SUCCESS==fdict_remove(d, &node3->id));
    print_dict(d);
}

void testcase4()
{
    puts("testcase4");
    fdict* d;
    GENERATE_DICT(d, 106);
    const int MAX_NODE_NUM = 300;
    node** nlist = (node**) malloc(MAX_NODE_NUM*sizeof(node*));
    if (!nlist) assert(NULL);
    int i;
    for (i=0; i < MAX_NODE_NUM; ++i){
        nlist[i] = generate(i,i);
    }
    
    for (i=0; i < MAX_NODE_NUM; ++i){
        fdict_insert(d, &(nlist[i]->id), nlist[i]);
    }
    print_dict(d);

    int key = 107; 
    node* s = fdict_find(d, &key);
    assert(s);
    printf ("[%d]-[%d] found\n", s->id, s->value); 

    for (i=0; i < MAX_NODE_NUM; ++i){
        fdict_insert(d, &(nlist[i]->id), nlist[i]);
    }
    print_dict(d);

    for (i=0; i<MAX_NODE_NUM; ++i){
        fdict_remove(d, &(nlist[i]->id));
    }
    print_dict(d);

    // release resource
    fdict_release(d);
    for (i=0; i<MAX_NODE_NUM;++i){
        free(nlist[i]);
        nlist[i] = NULL;
    }
    free(nlist);
    nlist = NULL;
}

int main()
{
    testcase1();
    testcase2();
    testcase3();
    testcase4();

    return 0;
}
#endif
