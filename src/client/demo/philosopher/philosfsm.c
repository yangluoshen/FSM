#include "philosfsm.h"
#include "debug.h"
#include "main.h"

#include <malloc.h>

extern module_t ME_MDL;

/*
void philos_event(void* entity, void* msg)
{
    if (!entity || !msg){
        LOG_E("parameters entity[%p] or msg[%p] is null", entity, msg);
        return ;
    }

    CVTTO_PHILOS(phi, entity);

    int msgtype = GET_MSG_TYPE(msg);
    if (msgtype == TIMEOUT_MSG){
        LOG_D("get a time out msg [%d]", msgtype);
        int timerfd = GET_TIMERFD(msg);
        fsm_entity_stop_timer(entity, msg);
    }

    if (!phi->nextjump){
        LOG_NE("nextjump is null");
        fsm_set_fsm_finish(entity);
        if (phi->exception)
            phi->exception(entity);
        return;
    }

    int ret = phi->nextjump(entity, msg);
    if (FSM_OK != ret){
        LOG_E("next jump failed[%d]", ret);
        if (phi->exception)
            phi->exception(entity);
    }
    
    return;
}
*/


int philos_fsm_wait(void* entity, void* msg);
int proc_chopstick_query_resp(void* entity, void* msg);

int philos_think(void* entity)
{
    CVTTO_PHILOS(phi, entity);
    int ret = fsm_entity_start_timer(entity, THINK_TM, phi->think_time);
    if (0 != ret) return FSM_FAIL;
    phi->status = THINKING;
    phi->nextjump = philos_fsm_wait;

    return FSM_OK;
}

int philos_fsm_save_info(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    philos_create_req* req = (philos_create_req* )GET_DATA(msg);
    CVTTO_PHILOS(phi, entity);
    phi->think_time = req->think_time;
    phi->eat_time = req->eat_time;
    phi->whoami = req->whoami;

    return  philos_think(entity);
}

int send_chopstick_query_req(void* entity, int querywhat)
{
    if (!entity) return FSM_FAIL;

    size_t data_len = FSM_MSG_HEAD_LEN + sizeof(philos_chop_req);
    size_t msg_len = data_len + MSG_HEAD_LEN;

    msg_t* m = (msg_t*) malloc(msg_len);
    if (!m) return FSM_FAIL;

    INIT_MSG_HEAD(m, getpid(), -1, PHU, RTU, data_len);

    CVTTO_PHILOS(phi, entity);
    
    SET_MSG_TYPE(m, PHILOS_CHOP_REQ);
    SET_MSG_FSMID(m, phi->fsmid);

    philos_chop_req* req = (philos_chop_req*) GET_DATA(m);
    req->whoami = phi->whoami;
    req->querywhat = querywhat;
    
    int ret = fsm_entity_start_timer(entity, QUERY_CHOP_TM, QUERY_CHOPSTICK_TIMEOUT);
    if (0 != ret){
        LOG_NE("start timer failed");
        free (m);
        return FSM_FAIL;
    }

    if (SM_OK != send_msg(m)){
        perror("send msg failed");
        LOG_NE("send msg failed");
        return FSM_FAIL;
    }

    phi->nextjump = philos_fsm_wait;
    phi->status = BUSY;

    LOG_ND("send chopstick query req success");
    return FSM_OK;
}

int philos_eat(void* entity)
{
    CVTTO_PHILOS(phi, entity);
    int ret = fsm_entity_start_timer(entity, EAT_TM, phi->eat_time);
    if (0 != ret) return FSM_FAIL;
    phi->status = EATTING;
    phi->nextjump = philos_fsm_wait;

    return FSM_OK;
}

int proc_chopstick_query_resp(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;
    philos_chop_resp* resp = (philos_chop_resp*) GET_DATA(msg);
    LOG_D("query chopstick[%d], isok[%d]", resp->querywhat, resp->isok);
    if (1 != resp->isok){
        philos_think(entity);
        return FSM_OK;
    }
    
    if (resp->querywhat == RIGHT_CHOP){
        return philos_eat(entity);
    }
    
    return send_chopstick_query_req(entity, RIGHT_CHOP);
}

int philos_proc_timeout(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    int ret = FSM_FAIL;
    int timerid = GET_TIMERID(msg);
    switch(timerid){
        case THINK_TM:
            ret = send_chopstick_query_req(entity, LEFT_CHOP);
            break;
        case EAT_TM:
            ret = philos_think(entity);
            break;
        case QUERY_CHOP_TM:
            break;
        default:
            break;
    }

    return ret;

}

int philos_proc_chop_req(void* entity, void* msg)
{

    return FSM_OK;
}

int philos_fsm_wait(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    int msgtype = GET_MSGTYPE(msg);
    int ret = FSM_FAIL;
    switch (msgtype){
        case TIMEOUT_MSG:
            ret = philos_proc_timeout(entity, msg);
            break;
        case PHILOS_CHOP_REQ:
            ret = philos_proc_chop_req(entity, msg);
            break;
        case PHILOS_CHOP_RESP:
            ret = proc_chopstick_query_resp(entity, msg);
        default:
            LOG_E("invalid msg[%d]", msgtype);
            break;
    }

    LOG_D("exit, ret[%d]", ret);
    return ret;
}

void philos_fsm_constructor(void* entity, fsm_t fsmid)
{
    if (!entity){
        LOG_NE("philos fsm constructor failed");
        return;
    }

    fsm_entity_base_constructor(entity, fsmid);

    CVTTO_PHILOS(phi, entity);
    phi->constructor = philos_fsm_constructor;
    phi->destructor = fsm_entity_base_destructor;
    phi->exception = fsm_entity_base_exception;

    phi->nextjump = philos_fsm_save_info;

    phi->status = THINKING;
    phi->think_time = 5;
    phi->eat_time = 3;
}

void* philos_fsm_create()
{
    return malloc(sizeof(philos_fsm));
}

