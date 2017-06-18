#include "ttu_yau_msg.h"
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "debug.h"

#include "fsm.h"
#include "cachefsm.h"

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
            break;
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
    LOG_D("Internal msg msgtype[%d]", fsm_head->msgtype);
    switch(fsm_head->msgtype){
        case TIMEOUT_MSG:
            proc_fsm_resp(pmsg);
            break;
        default:
            LOG_D("unknown msg type[%d]", fsm_head->msgtype);
            break;
    }
}


