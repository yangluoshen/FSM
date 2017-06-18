#include "fsm.h"
#include <malloc.h>
#include <limits.h>

#include "main.h"
#include "dict.h"
#include "debug.h"

/***** fsm id management *****/

// 0 means idle, 1 means engaged
static unsigned char fsm_id_pool[SHRT_MAX] = {0};
static fsm_t fsm_id_ptr = MIN_COMMON_FSMID;

static dict* fsm_entity_pool = NULL;

dict_option fsmid_op = {hash_calc_int, NULL, NULL, NULL, NULL,NULL};

/* return: 1 means a reserved fsm id
 *         0 means a common fsm id
 */
int is_reserved_fsmid(fsm_t id)
{
    if (id < MIN_COMMON_FSMID && id >= 0) return 1;

    return 0;
}

/* @return : Return a idle and not reserved fsmid.
 * @note   : It will be dead loop when fsmid pool gets exhausted.
 */
fsm_t alloc_fsm_id()
{
    // If fsm_id_ptr is engaged or it's a resetved fsmid , continue loop.
    // when id_pool exhausted, it will cause dead loop. How to fix it elegantly?
    while(fsm_id_pool[++fsm_id_ptr] || is_reserved_fsmid(fsm_id_ptr));

    fsm_id_pool[fsm_id_ptr] = FSMID_ENGAGED;
    return fsm_id_ptr;
}

/* @Description: put fsmid back to fsmid pool.
 */
void free_fsm_id(fsm_t fsmid)
{
    fsm_id_pool[fsmid] = FSMID_IDLE;
}

/***********fsm entity pool ***************/

void* get_fsm_entity(fsm_t fsmid)
{
    return dict_find(fsm_entity_pool, (void*)(long)fsmid);
} 

void rmv_fsm_entity(fsm_t fsmid)
{
    LOG_D("remove fsm entity[%u]", fsmid);
    dict_delete(fsm_entity_pool, (void*)(long)fsmid);
    free_fsm_id(fsmid);
}

int add_fsm_entity(fsm_t fsmid, void* entity)
{
    if (!fsm_entity_pool){
        fsm_entity_pool = dict_create(&fsmid_op);
        if (!fsm_entity_pool) return -1;
    }

    if(NULL == dict_add(fsm_entity_pool, (void*)(long)fsmid, entity))
        return -1;
    
    return 0;
}

/**************** fsm factory ***************/

/* @Description: Generate a fsm entity according to 'type'
 */
fsm_reg* get_reginfo_by_msgtype(int type);

fsm_entity_base* fsm_factory(int type, void* msg)
{
    fsm_reg* reg_info = get_reginfo_by_msgtype(type);
    if (!reg_info) return NULL;

    fsm_constructor constructor = reg_info->constructor;
    if (!constructor) return NULL;

    fsm_creator creator = reg_info->creator;
    if (!creator) return NULL;

    fsm_entity_base* entity = (fsm_entity_base*)reg_info->creator();
    if (!entity) return NULL;

    fsm_t fsmid = alloc_fsm_id();
    if (-1 == fsmid) goto RELEASE;

    constructor(entity, fsmid);
    if (!entity) goto FAILED;

    // insert into fsm entity pool
    if (0 != add_fsm_entity(fsmid, entity)) goto FAILED;

    return entity;

FAILED:
    free_fsm_id(fsmid);
RELEASE:
    free(entity);
    entity = NULL;
    return NULL;
}


/************ fsm entity base functions *****/
/* @Description: Implement fsm_entity base,
 *               which like Object-Oriented.
 */

/* @Decription : A base constructor. 
 *     Do the things all fsm entities should do.
 */
void fsm_entity_base_destructor(void* entity);
void fsm_entity_timer_init(void* entity);
void fsm_base_event(void* entity, void* msg);
int fsm_entity_start_timer(void* entity, int timerid, time_t seconds);
void fsm_entity_stop_timer(void* entity, int timerid);

void fsm_entity_base_constructor(void* entity, fsm_t fsmid)
{
    if (!entity) return;
    CVTTO_BASE(base_entity, entity);
    base_entity->fsmid = fsmid;
    base_entity->is_fsm_finish = 0;
    base_entity->event = fsm_base_event;
    base_entity->nextjump = NULL;
    base_entity->destructor = fsm_entity_base_destructor;

    fsm_entity_timer_init(entity);
    fsm_entity_start_timer(entity, 0, -1);
}

void fsm_base_event(void* entity, void* msg)
{
    if (!entity || !msg){
        LOG_E("parameters entity[%p] or msg[%p] is null", entity, msg);
        return ;
    }

    CVTTO_BASE(base, entity);

    int msgtype = GET_MSGTYPE(msg);
    if (msgtype == TIMEOUT_MSG){
        LOG_D("get a time out msg [%d]", msgtype);
        int timerid = GET_TIMERID(msg);
        fsm_entity_stop_timer(entity, timerid);
    }

    if (!base->nextjump){
        LOG_NE("nextjump is null");
        fsm_set_fsm_finish(entity);
        if (base->exception)
            base->exception(entity);
        return;
    }

    int ret = base->nextjump(entity, msg);
    if (FSM_OK != ret){
        LOG_E("next jump failed[%d]", ret);
        if (base->exception)
            base->exception(entity);
    }
    
    return;
}

void fsm_entity_timer_init(void* entity)
{
    if (!entity) return;
    CVTTO_BASE(be, entity);
    int i;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        be->timer_list[i].id = -1;
        be->timer_list[i].fd = -1;
    }
}

int fsm_entity_set_timer(void* entity,int timerid, int timerfd)
{
    if (!entity) return -1;
    CVTTO_BASE(be, entity);

    int i;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        if (be->timer_list[i].fd == -1){
            be->timer_list[i].fd = timerfd;
            be->timer_list[i].id = timerid;
            return 0;
        }
    }

    LOG_E("set timer[%d] failed", timerid);
    return -1;
}

void fsm_entity_base_destructor(void* entity)
{
    if (!entity) return ;
    CVTTO_BASE(be, entity);
    
    free_fsm_id(be->fsmid);
    
    // stop all timers
    int i;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        if (-1 == be->timer_list[i].fd) continue;

        stop_timer(be->timer_list[i].fd);
        be->timer_list[i].fd = -1;
        be->timer_list[i].id = -1;
    }

}

int fsm_entity_start_timer(void* entity, int timerid, time_t seconds)
{
    if (!entity) return -1;
    CVTTO_BASE(base_entity, entity);
    LOG_D("start timer,fsmid[%u],timerid[%d],second[%d]",base_entity->fsmid, timerid, seconds);
    
    int tfd = -1;
    if ((time_t)-1 != seconds){
        tfd = start_timer(timerid, base_entity->fsmid, seconds);
        if (tfd == -1){
            LOG_ND("start timer failed");
            return -1;
        }
    }

    if (-1 == fsm_entity_set_timer(entity, timerid, tfd)){
        close(tfd);
        return -1;
    }
    
    LOG_D("start timer success,timerid[%d]", timerid);
    return 0;
}

void fsm_entity_stop_timer(void* entity, int timerid)
{
    if (!entity) return ;
    CVTTO_BASE(be, entity);
    
    int i = 0;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        if (timerid == be->timer_list[i].id){
            stop_timer(be->timer_list[i].fd);
            be->timer_list[i].id = -1;
            be->timer_list[i].fd = -1;
            LOG_D("timer[%d] has been stop", timerid);
            return ;
        }
    }
    return;
}

void fsm_entity_base_exception(void* entity)
{
    if (!entity) return ;
    CVTTO_BASE(be, entity);

    be->is_fsm_finish = 1;
}

void fsm_set_fsm_finish(void* entity)
{
    if (!entity) return;
    CVTTO_BASE(base, entity);
    base->is_fsm_finish = 1;
}


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

