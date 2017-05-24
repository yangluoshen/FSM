#include "philosfsm.h"
#include "debug.h"
#include "main.h"

#include <malloc.h>

#define QUERY_LEFT(who) (who)

#define QUERY_RIGHT(who) \
    (((who) + 1) % philos_count)

extern module_t ME_MDL;
extern fdict* philos_dict;
extern unsigned int philos_count;

void show_addphilos(philosopher* phi);
void refresh_screen();

void philos_set_status(philosopher* philos, int status)
{
    philos->status = status;
    refresh_screen();
}

int philos_fsm_wait(void* entity, void* msg);
int proc_chopstick_query_resp(void* entity, void* msg);

int philos_think(void* entity)
{
    CVTTO_PHILOS(phi, entity);
    int ret = fsm_entity_start_timer(entity, THINK_TM, phi->philos->think_time);
    if (0 != ret) return FSM_FAIL;
    philos_set_status(phi->philos, THINKING);
    phi->nextjump = philos_fsm_wait;

    LOG_D("---%s thinking---fsmid[%d]", phi->philos->name, phi->fsmid);
    return FSM_OK;
}

int philos_fsm_save_info(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    if (!philos_dict){
        philos_dict = fdict_create(MAX_PHILOS_NUM, philos_hash_match, philos_hash_calc);
        if (!philos_dict){
            LOG_NE("create fdict failed");
            return FSM_FAIL;
        }
    }
    
    philosopher* phi = (philosopher*) malloc(sizeof (philosopher));
    if (!phi) return FSM_FAIL;

    memcpy (phi, GET_DATA(msg), sizeof (philosopher));
    if (FDICT_SUCCESS != fdict_insert(philos_dict, &phi->whoami, phi)){
        LOG_NE("insert philosopher to dict failed");
        free(phi);
        return FSM_FAIL;
    }
    CVTTO_PHILOS(phifsm, entity);

    phifsm->philos = phi;
    philos_count += 1;

    // 用于显示
    show_addphilos(phi);

    return  philos_think(entity);
}

msg_t* pack_chop_req_msg(int chop_idx, int dowhat, fsm_t fsmid)
{
    size_t data_len = FSM_MSG_HEAD_LEN + sizeof(chop_req);
    size_t msg_len = data_len + MSG_HEAD_LEN;

    msg_t* m = (msg_t*) malloc(msg_len);
    if (!m) return NULL;
    INIT_MSG_HEAD(m, getpid(), getpid(), PHU, PHU, data_len);

    SET_MSGTYPE(m, PHILOS_CHOP_REQ);
    SET_FSMID(m, fsmid);

    chop_req* req = (chop_req*) GET_DATA(m);
    req->dowhat = dowhat;
    req->chop_idx = chop_idx;

    return m;
}

int send_chopstick_query_req(void* entity)
{
    if (!entity) return FSM_FAIL;

    CVTTO_PHILOS(phi, entity);
    LOG_D("---%s need chopstick---fsmid[%d]", phi->philos->name, phi->fsmid);
    int chop_idx;
    if (phi->chops[0] == -1){
        chop_idx = QUERY_LEFT(phi->philos->whoami);
        LOG_D("philos[%d] needs left chop[%d]", phi->philos->whoami, chop_idx);
    }
    else if (phi->chops[1] == -1){
        chop_idx = QUERY_RIGHT(phi->philos->whoami);
        LOG_D("philos[%d] needs right chop[%d]", phi->philos->whoami, chop_idx);
    }
    else {
        LOG_E("No need to get chopstick,fsm[%d]", phi->fsmid);
        return FSM_FAIL;
    }
    
    msg_t * m = pack_chop_req_msg(chop_idx, GET, phi->fsmid);

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

    phi->nextjump = proc_chopstick_query_resp;
    philos_set_status(phi->philos, BUSY);

    LOG_D("send chopstick query req success. fsmid[%d]", phi->fsmid);
    return FSM_OK;
}

int philos_eat(void* entity)
{
    CVTTO_PHILOS(phi, entity);
    int ret = fsm_entity_start_timer(entity, EAT_TM, phi->philos->eat_time);
    if (0 != ret) return FSM_FAIL;
    philos_set_status(phi->philos, EATTING);
    phi->nextjump = philos_fsm_wait;

    LOG_D("---%s eatting---fsmid[%d]", phi->philos->name, phi->fsmid);
    return FSM_OK;
}


void send_chop_rollback_req(void* entity)
{
    if (!entity) return ;

    CVTTO_PHILOS(phi, entity);
    int i;
    for (i=0; i < 2; ++i){
        int chop_idx = phi->chops[i];
        if (-1 == chop_idx)
            break;
        msg_t* m = pack_chop_req_msg(chop_idx, BACK, phi->fsmid);
        if (SM_OK != send_msg(m)){
            LOG_NE("send msg failed");
            exit(1);
        }
        phi->chops[i] = -1;
        LOG_D("philos[%d] roll back chop[%d], fsmid[%d]", phi->philos->whoami, chop_idx, phi->fsmid);
    }
}


int proc_chopstick_query_resp(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    CVTTO_PHILOS(phi, entity);
    int msgtype = GET_MSGTYPE(msg);
    if (msgtype == TIMEOUT_MSG){
        LOG_D("query chopstick timeout. fsmid[%d]", phi->fsmid);
        send_chop_rollback_req(entity);
        philos_think(entity);
        return FSM_OK;
    }

    // stop timer
    fsm_entity_stop_timer(entity, QUERY_CHOP_TM);


    chop_resp* resp = (chop_resp*) GET_DATA(msg);
    LOG_D("query chopstick[%d], isok[%d], fsmid[%d]", resp->chop_idx, resp->isok, phi->fsmid);

    if (msgtype != PHILOS_CHOP_RESP){
        LOG_E("unknow msg[%d]" ,msgtype);
        return FSM_OK;
    }

    if (1 != resp->isok){
        LOG_D("philos[%d] get chopstick[%d] failed", phi->philos->whoami, resp->chop_idx); 
        send_chop_rollback_req(entity);
        philos_think(entity);
        return FSM_OK;
    }
    
    // 如果左筷子已经有了, 说明这是右筷子
    if (-1 != phi->chops[0]){
        //如果是右手边的筷子拿到，就可以eat了
        LOG_D("---%s get right chopstaicks[%d]---fsmid[%d]", phi->philos->name, resp->chop_idx,phi->fsmid);
        phi->chops[1] = resp->chop_idx;
        return philos_eat(entity);
    }
    
    LOG_D("---%s get a left chopstick[%d]---fsmid[%d]", phi->philos->name, resp->chop_idx, phi->fsmid);
    phi->chops[0] = resp->chop_idx;
    return send_chopstick_query_req(entity);
}

int philos_proc_timeout(void* entity, void* msg)
{
    if (!entity || !msg) return FSM_FAIL;

    int ret = FSM_FAIL;
    int timerid = GET_TIMERID(msg);
    CVTTO_PHILOS(phi, entity);
    switch(timerid){
        case THINK_TM:
            LOG_D("---%s hungry---fsmid[%d]", phi->philos->name, phi->fsmid);
            ret = send_chopstick_query_req(entity);
            break;
        case EAT_TM:
            LOG_D("---%s full---fsmid[%d]", phi->philos->name, phi->fsmid);
            // eat结束之后,需将筷子放回
            send_chop_rollback_req(entity);
            ret = philos_think(entity);
            break;
        case QUERY_CHOP_TM:
            LOG_NE("should not print this!");
            break;
        default:
            break;
    }

    return ret;

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
    phi->chops[0] = -1;
    phi->chops[1] = -1;

    phi->philos = NULL;
}

void* philos_fsm_create()
{
    return malloc(sizeof(philos_fsm));
}

