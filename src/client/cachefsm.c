
#include "cachefsm.h"
#include "fsm.h"
#include "debug.h"
#include "ttu_yau_msg.h"
#include "client_base.h"
#include <malloc.h>

void chat_yau_resp(pid_t r_pid, module_t r_mdl);

void cache_fsm_destructor(void* entity);
int cache_fsm_query(void* entity, void* data);
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
    cache_en->nextjump = cache_fsm_query;
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

int cache_fsm_query(void* entity, void* msg)
{
    msg_t* data = (msg_t*) msg;
    chat_yau_resp(data->s_pid, data->s_mdl);
    fsm_set_fsm_finish(entity);
    
    return FSM_OK;
}

void cache_fsm_exception(void* entity)
{
    LOG_ND("cache fsm exception");
    fsm_set_fsm_finish(entity);
    return;
}


void chat_yau_resp(pid_t r_pid, module_t r_mdl)
{
    char content[] = "Nice to see you, too.";
    size_t data_len = sizeof(msg_type_t) + sizeof(content);
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    if (!req_buf) return ;

    msg_t* pmsg = (msg_t*) req_buf;
    pmsg->s_pid = getpid();
    pmsg->r_pid = r_pid;
    pmsg->s_mdl = TTU;
    pmsg->r_mdl = r_mdl;
    pmsg->data_len = data_len;

    req_t* preq = (req_t*) pmsg->data;
    preq->msg_type = TTU_YAU_CHAT_REQ;
    memcpy(preq->what, content, sizeof(content));

    if (SM_OK != send_msg(pmsg)){
        perror("send msg failed");
        return ;
    }
    LOG_ND("say hello to yau");
}
