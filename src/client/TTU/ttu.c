#include "fsm_base.h"
#include "ttu_yau_msg.h"
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "client_base.h"
#include "debug.h"

#include "fsm.h"
#include "cachefsm.h"

void process_yau_req(void* pmsg);

void proc_fsm_req(msg_t* pmsg)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    fsm_entity_base* entity = fsm_factory(fsm_head->msgtype, pmsg);
    if (!entity){
        LOG_NE("fsm_factory failed");
        return;
    }

    entity->event(entity, pmsg);

    if (entity->is_fsm_finish){
        LOG_D("fsm[%u] finish", entity->fsmid);
        if (entity->destructor) entity->destructor(entity);

        rmv_fsm_entity(entity->fsmid);
    }

    LOG_ND("Exit.");
    return;
}

void proc_fsm_resp(msg_t* pmsg)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    fsm_entity_base* entity = get_fsm_entity(fsm_head->fsmid);
    if (!entity){
        LOG_NE("get_fsm_entity failed");
        return;
    }

    entity->event(entity, pmsg);

    if (entity->is_fsm_finish){
        LOG_D("fsm[%u], finish", entity->fsmid);
        if (entity->destructor) entity->destructor(entity);

        rmv_fsm_entity(entity->fsmid);
    }

    LOG_ND("Exit.");
    return ;
}

void process_yau_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    fsm_msg_head* fsm_head= (fsm_msg_head*) pmsg->data;
    switch(fsm_head->msgtype){
        case CACHE_REQ: 
            proc_fsm_req(pmsg);
            break;
        case CACHE_QUERY_REQ:
            proc_fsm_resp(pmsg);
        default:
            LOG_D("unknown msg type[%d]", fsm_head->msgtype);
            break;
    }
}

void proc_ttu_internal_msg(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    switch(fsm_head->msgtype){
        case TIMEOUT_MSG:
            proc_fsm_resp(pmsg);
            break;
        default:
            LOG_D("unknown msg type[%d]", fsm_head->msgtype);
            break;
    }
}


