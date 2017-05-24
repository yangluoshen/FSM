/*****
 * 查询某哲学家当前状态的状态机
 *******/

#include "chopqueryfsm.h"
#include "client_config.h"
#include "main.h"
#include "debug.h"

#include <malloc.h>

extern module_t ME_MDL;
extern fdict* philos_dict;
extern unsigned int philos_count;
int chopsticks[MAX_CHOPSTICK_NUM] = {CHOP_IDLE};


int proc_get_chop(int chop_idx, void* msg)
{
    LOG_D("get chop[%d], status[%d]", chop_idx, chopsticks[chop_idx]);

    /* pack response msg */
    size_t datalen = FSM_MSG_HEAD_LEN + sizeof(chop_resp);
    size_t msglen = MSG_HEAD_LEN + datalen;
    msg_t* m = (msg_t*) malloc(msglen);
    if (!m) return FSM_FAIL;

    INIT_MSG_HEAD(m, getpid(), getpid(), PHU, PHU, datalen);
    SET_MSGTYPE(m, PHILOS_CHOP_RESP);
    SET_FSMID(m, GET_FSMID(msg));

    chop_resp* resp = (chop_resp*) GET_DATA(m);
    resp->chop_idx = chop_idx;
    resp->isok = chopsticks[chop_idx] == CHOP_IDLE;

    if (SM_OK != send_msg(m)){
        LOG_NE("send msg failed");
        return FSM_FAIL;
    }

    chopsticks[chop_idx] = chopsticks[chop_idx] == CHOP_IDLE ? CHOP_BUSY : chopsticks[chop_idx];

    return FSM_OK;
}

void proc_return_chop(int chop_idx)
{
    chopsticks[chop_idx] = CHOP_IDLE;
}

int chop_fsm_querystatus(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;
    
    chop_req* req = (chop_req*) GET_DATA(msg);
    int chop_idx = req->chop_idx;
    if (chop_idx >= philos_count) return FSM_FAIL;
    int dowhat = req->dowhat;
    LOG_D("chop_idx[%d], dowhat[%d]", chop_idx, dowhat);

    int ret = FSM_OK;
    switch (dowhat){
        case GET:
            ret = proc_get_chop(chop_idx, msg);
        case BACK:
            proc_return_chop(chop_idx);
            break;
        default:
            LOG_E("unknown operation[%d]", dowhat);
            break;
    }

    fsm_set_fsm_finish(entity);
    return ret;
}

void chop_fsm_constructor(void* entity, fsm_t fsmid)
{
    if (!entity){
        LOG_NE("philos query fsm constructor failed");
        return;
    }

    fsm_entity_base_constructor(entity, fsmid);
    CVTTO_PHIQRY(phiqry, entity);

    phiqry->constructor = chop_fsm_constructor;
    phiqry->destructor = fsm_entity_base_destructor;
    phiqry->exception = fsm_entity_base_exception;
    phiqry->nextjump = chop_fsm_querystatus;

}

void* chop_fsm_create()
{
    return malloc(sizeof (chop_query_fsm));
}
