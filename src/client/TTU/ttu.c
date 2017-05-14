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


void yau_chat(msg_t* data)
{
    printf("yau:%s\n", ((req_t*)(data->data))->what);
    LOG_D("yau say hello to me:%s", ((req_t*)(data->data))->what);

    fsm_table_unit* cache_unit = fsm_factory(CACHE_REQ, data);
    if (!cache_unit){
        LOG_NE("fsm_factory failed");
        return;
    }

    cache_fsm* entity = (cache_fsm*)(cache_unit->entity);
    if (!entity) goto FREE_TABLE_UNIT;
    
    fsm_destructor destructor = entity->destructor;

    if (!entity->nextjump){
        LOG_NE("there is no nextjump");

        goto FINISH_FSM;
    }
    
    int ret = (entity->nextjump)(entity, data);
    if (FSM_OK != ret){
        LOG_NE("proc nextjump failed");
        if (entity->exception)
            entity->exception(entity);
    }

    if (entity->is_fsm_finish){
        LOG_ND("fsm is finish");
        goto FINISH_FSM;
    }

    LOG_ND("Exit.");
    return;

FINISH_FSM:
    if (destructor)
        entity->destructor(entity);
    else{
        free(entity);
        entity = NULL;
    }

FREE_TABLE_UNIT:
    fsm_table_unit_destroy(cache_unit);
    return;
}

void process_yau_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    req_t* preq = (req_t*) pmsg->data;
    switch(preq->msg_type){
        case YAU_TTU_CHAT_REQ: 
            yau_chat(pmsg);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}


