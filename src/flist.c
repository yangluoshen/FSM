
#include "flist.h"
#include <malloc.h>

fnode* generate_fnode(void* val)
{
    fnode* node = (fnode*) malloc(sizeof(fnode));
    if (!node) return NULL;
    node->val = val;
    node->next = NULL;
    return node;
}


static int __flist_match_default(void* ptr, void* key)
{
    return ptr == key;
}

flist* flist_create(int (*match)(void*, void*))
{
    flist* l = (flist*) malloc(sizeof(flist));
    if (!l) return NULL;
    l->next = NULL;
    l->match = match ? match : __flist_match_default;
    return l;
}

void flist_release(flist* l)
{
    if (!l) return;
    fnode* n = l->next;
    while (n){
        fnode* tmpnode = n;
        n = n->next;
        free(tmpnode);
    }
    free (l);
}

fnode* flist_append(flist* l, void* val)
{
    if (!l) return NULL;
    fnode* n = l->next;
    fnode* addn;
    if (!n) {
        addn = generate_fnode(val);
        if (!addn) return NULL;
        l->next = addn;
    } else{
        while (n->next) 
            n = n->next;
        addn = generate_fnode(val);
        if (!addn) return NULL;
        n->next = addn;
    }
    return addn;
}

#if 0
/*flist delete with "bad taste"*/
void flist_delete(flist* l, void* key)
{
    if (!l || !(l->next)) return;
    fnode* prev = l->next;
    if (l->match && l->match(prev->val, key)){
        l->next = prev->next;
        free(prev);
        return;
    }

    fnode* n = prev->next;
    while (n){
        if (l->match && l->match(n->val, key)){
            prev->next = n->next;
            free (n);
            break;
        }
        prev = n;
        n = n->next;
    }
}
#endif

/*flist delete with "good taste"*/
void flist_delete(flist* l, void* key)
{
    if (!l ||!l->next||!key) return ;

    fnode** indirect = &(l->next);
    while (*indirect && !l->match((*indirect)->val, key))
        indirect = &((*indirect)->next);
    
    // key found in list
    if (*indirect){
        fnode* t = *indirect;
        *indirect = (*indirect)->next;
        free(t);
    }
}

fnode* flist_find(flist* l, void* key)
{
    if (!l) return NULL;
    fnode* n = l->next;
    while(n){
        if (l->match && l->match(n->val, key))
            return n;
        n = n->next;
    }
    return NULL;
}


#ifdef LIST_MAIN
#include <stdio.h>

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

int match_func(void* ptr, void* key)
{
    if (!ptr || !key) return 0;
    node* n = (node*) ptr;
    return (n->id == *((int*)key)) ? 1 : 0;
}

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

void testcase1()
{
    flist* l = flist_create(match_func);
    if (!l) return ;
    //FLST_SET_MATCH(l, match_func);

    node* node1 = generate(1, 1);
    node* node2 = generate(2, 2);
    node* node3 = generate(3, 3);
    node* node5 = generate(5, 5);
    
    //flist_delete(l, node3);
    fnode* ln1 = flist_append(l, node1);
    fnode* ln3 = flist_append(l, node3);
    fnode* ln2 = flist_append(l, node2);
    fnode* ln5 = flist_append(l, node5);

    print_list(l);
    flist_delete(l, node1);
    print_list(l);
    flist_delete(l, node3);
    print_list(l);
    flist_delete(l, node5);
    print_list(l);

    int key = 5;
    fnode* searchfnode = flist_find(l, &key); 
    if (!searchfnode) {
        printf("search falied\n");
        return ;
    }
    node* searchnode = (node*) searchfnode->val;
    printf("search node:%d\n", searchnode->value);
    
    flist_release(l);

    (void)ln1;(void)ln2;(void)ln3;(void)ln5;//(void)ln6;
}

void testcase2()
{
    flist* l = flist_create(NULL);
    if (!l) return ;

    node* node1 = generate(1, 1);
    node* node2 = generate(2, 2);
    node* node3 = generate(3, 3);
    node* node5 = generate(5, 5);
    
    //flist_delete(l, node3);
    fnode* ln1 = flist_append(l, node1);
    fnode* ln3 = flist_append(l, node3);
    fnode* ln2 = flist_append(l, node2);
    fnode* ln5 = flist_append(l, node5);

    print_list(l);
    flist_delete(l, node1);
    print_list(l);
    flist_delete(l, node3);
    print_list(l);
    flist_delete(l, node5);
    print_list(l);

    fnode* searchfnode = flist_find(l, node2); 
    if (!searchfnode) {
        printf("search falied\n");
        return ;
    }
    node* searchnode = (node*) searchfnode->val;
    printf("search node:%d\n", searchnode->value);
    
    flist_release(l);

    (void)ln1;(void)ln2;(void)ln3;(void)ln5;//(void)ln6;
}

int main()
{
    testcase1();
    testcase2();
    return 0;
}
#endif
