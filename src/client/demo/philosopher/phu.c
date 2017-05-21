#include "fsm_base.h"
#include <malloc.h>
#include <unistd.h>

#include "main.h"
#include "debug.h"

#include "fsm.h"

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

void proc_rtu(void* data)
{
}

void proc_internal_msg(void* data)
{
}


