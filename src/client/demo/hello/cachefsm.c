
#include "cachefsm.h"
#include "debug.h"
#include "ttu_yau_msg.h"
#include "main.h"
#include <malloc.h>

extern module_t ME_MDL;

void chat_yau_resp(cache_fsm* entity, fsm_t peer_fsmid, pid_t r_pid, module_t r_mdl);
void chat_yau_resp_again(cache_fsm* entity, fsm_t peer_fsmid, pid_t r_pid, module_t r_mdl);

void cache_fsm_destructor(void* entity);
int cache_fsm_req(void* entity, void* data);
int cache_fsm_query(void* entity, void* msg);
void cache_fsm_exception(void* entity);

void cache_fsm_constructor(void* entity, fsm_t fsmid)
{
    if (!entity) {
        LOG_NE("constructor falied, entity is NULL");
        return;
    }

    fsm_entity_base_constructor(entity, fsmid);

    CVTTO_CACHE(cache_en, entity);
    cache_en->constructor = cache_fsm_constructor;
    cache_en->destructor = cache_fsm_destructor;
    cache_en->nextjump = cache_fsm_req;
    cache_en->exception = cache_fsm_exception;

    cache_en->key = 1;
}

void* cache_fsm_create()
{
    return malloc(sizeof(cache_fsm));
}

void cache_fsm_destructor(void* entity)
{    
    LOG_ND("cache fsm destructor");

    fsm_entity_base_destructor(entity); 

    free(entity);
    entity = NULL;
}

int cache_fsm_req(void* entity, void* msg)
{
    msg_t* data = (msg_t*) msg;
    fsm_msg_head* fsm_head = (fsm_msg_head*)data->data;

    if (TIMEOUT_MSG == fsm_head->msgtype){
        fsm_set_fsm_finish(entity);
        LOG_NE("Time out");
        return FSM_FAIL;
    }

    req_t* preq = (req_t*) fsm_head->data;

    LOG_D("what:%s, peer fsmid[%u], my fsmid[%u], msgtype[%d]", preq->what, preq->src_fsmid, ((cache_fsm*)entity)->fsmid, fsm_head->msgtype);
    printf("yau:%s\n", preq->what);
    chat_yau_resp((cache_fsm*)entity, fsm_head->fsmid, data->s_pid, data->s_mdl);
    
    CVTTO_CACHE(cache_en, entity);
    cache_en->nextjump = cache_fsm_query;
    return FSM_OK;
}

int cache_fsm_query(void* entity, void* msg)
{
    msg_t* data = (msg_t*) msg;
    fsm_msg_head* fsm_head = (fsm_msg_head*)data->data;
    if (TIMEOUT_MSG == fsm_head->msgtype){
        fsm_set_fsm_finish(entity);
        LOG_NE("Time out");
        return FSM_FAIL;
    }

    req_t* preq = (req_t*) fsm_head->data;

    LOG_D("what:%s, peer fsmid[%u], my fsmid[%u],msg_type[%d]", preq->what, preq->src_fsmid, ((cache_fsm*)entity)->fsmid, fsm_head->msgtype);
    printf("yau:%s\n", preq->what);

    chat_yau_resp_again((cache_fsm*)entity, fsm_head->fsmid, data->s_pid, data->s_mdl);
    fsm_set_fsm_finish(entity);

    return FSM_OK;
}

void cache_fsm_exception(void* entity)
{
    LOG_ND("cache fsm exception");
    fsm_set_fsm_finish(entity);
    return;
}

void chat_yau_resp(cache_fsm* entity, fsm_t peer_fsmid, pid_t r_pid, module_t r_mdl)
{
    char content[] = "Nice to see you, too.";
    size_t data_len = FSM_MSG_HEAD_LEN + sizeof(content);
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    if (!req_buf) return ;

    msg_t* pmsg = (msg_t*) req_buf;
    pmsg->s_pid = getpid();
    pmsg->r_pid = r_pid;
    pmsg->s_mdl = ME_MDL;
    pmsg->r_mdl = r_mdl;
    pmsg->data_len = data_len;

    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    fsm_head->fsmid = peer_fsmid;
    fsm_head->msgtype = CACHE_RESP;

    req_t* preq = (req_t*) fsm_head->data;
    preq->src_fsmid = entity->fsmid;
    memcpy(preq->what, content, sizeof(content));

    if (SM_OK != send_msg(pmsg)){
        perror("send msg failed");
        return ;
    }
    LOG_D("say [%s] to yau", content);
}

void chat_yau_resp_again(cache_fsm* entity, fsm_t peer_fsmid, pid_t r_pid, module_t r_mdl)
{
    char content[] = "Find, thanks.";
    size_t data_len = FSM_MSG_HEAD_LEN + sizeof(content);
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    if (!req_buf) return ;

    msg_t* pmsg = (msg_t*) req_buf;
    pmsg->s_pid = getpid();
    pmsg->r_pid = r_pid;
    pmsg->s_mdl = ME_MDL;
    pmsg->r_mdl = r_mdl;
    pmsg->data_len = data_len;

    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    fsm_head->fsmid = peer_fsmid;
    fsm_head->msgtype = CACHE_QUERY_RESP;

    req_t* preq = (req_t*) fsm_head->data;
    preq->src_fsmid = entity->fsmid;
    memcpy(preq->what, content, sizeof(content));

    if (SM_OK != send_msg(pmsg)){
        perror("send msg failed");
        return ;
    }
    LOG_D("say [%s] to yau", content);
}
