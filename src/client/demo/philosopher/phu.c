#include "fsm_base.h"
#include <malloc.h>
#include <unistd.h>

#include "main.h"
#include "debug.h"
#include "fdict.h"

#include "fsm.h"
#include "philos_msg.h"


fdict* philos_dict = NULL;
unsigned int philos_count = 0;

int chopsticks[MAX_CHOPSTICK_NUM] = {CHOP_IDLE};


void proc_rtu(void* data)
{
}

void proc_internal_msg(void* data)
{
}

int philos_hash_match(void* ptr, fdict_key_t key)
{
    if (!ptr || !key) return 0;
    philosopher* p = (philosopher*) ptr;
    return p->whoami == *(int*)key;
}

size_t philos_hash_calc(fdict* d, fdict_key_t key)
{
    if (!d || !key) return -1;
    int hash = *((int*)key) % d->hash_size;
    return (size_t)hash;
}
