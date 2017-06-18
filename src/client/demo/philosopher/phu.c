#include <malloc.h>
#include <unistd.h>

#include "main.h"
#include "debug.h"
#include "fdict.h"

#include "fsm.h"
#include "philos_msg.h"


fdict* philos_dict = NULL;
unsigned int philos_count = 0;

void proc_cru_msg(void* data)
{
    if (!data) return;
    int msgtype = GET_MSGTYPE(data);
    LOG_D("msgtype[%d]", msgtype);
    switch(msgtype){
        case  PHILOS_CREATE_REQ:
            proc_fsm_req((msg_t*)data);
            break;
        default:
            LOG_E("unknown msg[%d]", msgtype);
            break;
    }
    return;
}

void proc_internal_msg(void* data)
{
    if (!data) return;
    int msgtype = GET_MSGTYPE(data);
    LOG_D("msgtype[%d]", msgtype);
    switch(msgtype){
        case TIMEOUT_MSG:
        case PHILOS_CHOP_RESP:
            proc_fsm_resp((msg_t*)data);
            break;
        case PHILOS_CHOP_REQ:
            proc_fsm_req((msg_t*)data);
            break;
        default:
            LOG_E("unknown msg[%d]", msgtype);
            break;
    }
    return;
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
