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
    req_t* preq = (req_t*) data->data;

    printf("yau:%s\n", preq->what);
    LOG_D("yau say hello to me:%s, msg_type[%u]", preq->what, preq->msg_type);

    fsm_entity_base* entity = fsm_factory(preq->msg_type, data);
    if (!entity){
        LOG_NE("fsm_factory failed");
        return;
    }

    entity->event(entity, data);

    if (entity->is_fsm_finish){
        LOG_D("fsm[%u] finish", entity->fsmid);
        if (entity->destructor) entity->destructor(entity);

        rmv_fsm_entity(entity->fsmid);
    }

    LOG_ND("Exit.");
    return;
}

void process_yau_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    req_t* preq = (req_t*) pmsg->data;
    switch(preq->msg_type){
        case CACHE_REQ: 
            yau_chat(pmsg);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}


