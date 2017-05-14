#include "fsm.h"
#include <malloc.h>
#include <limits.h>

#include "client_base.h"

/***** fsm id management *****/

// 0 means idle, 1 means engaged
static unsigned char fsm_id_pool[SHRT_MAX] = {0};
static fsm_t fsm_id_ptr = MIN_COMMON_FSMID;

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


/***** fsm factory *****/

/* @Description: Generate a fsm entity according to 'type'
 */
fsm_reg* get_reginfo_by_msgtype(int type);
fsm_table_unit* fsm_factory(int type, void* msg)
{
    fsm_t fsmid;
    fsm_reg* reg_info = get_reginfo_by_msgtype(type);
    if (!reg_info) goto FAILED;

    fsm_constructor constructor = reg_info->constructor;
    if (!constructor) goto FAILED;

    fsmid = alloc_fsm_id();
    if (-1 == fsmid) goto FAILED;

    fsm_creator creator = reg_info->creator;
    if (!creator) goto FAILED;

    void* entity = reg_info->creator();
    if (!entity) goto FAILED;

    fsm_table_unit* unit = (fsm_table_unit*)malloc(sizeof(fsm_table_unit));
    if (!unit) goto RELEASE;

    constructor(entity, fsmid);
    if (!entity) goto FREE;

    unit->fsmid = fsmid;
    unit->entity = entity;
    //unit->nextjump = NULL; //essential

    return unit;

FREE:
    free(unit);
RELEASE:
    free(entity);
    free_fsm_id(fsmid);
FAILED:
    return NULL;

}

void fsm_table_unit_destroy(fsm_table_unit* unit)
{
    if(!unit) return ;
    free_fsm_id(unit->fsmid);
    //unit->entity->destructor();  //how to destroy entity?
    free (unit);
}


/***** fsm entity base functions *****/
/* @Description: Implement fsm_entity base,
 *               which like Object-Oriented.
 */

/* @Decription : A base constructor. 
 *     Do the things all fsm entities should do.
 */
void fsm_entity_base_destructor(void* entity);
void fsm_entity_timer_init(void* entity);


void fsm_entity_base_constructor(void* entity, fsm_t fsmid)
{
    if (!entity) return;
    CVTTO_BASE(base_entity, entity);
    base_entity->fsmid = fsmid;
    base_entity->is_fsm_finish = 0;
    base_entity->nextjump = NULL;
    base_entity->destructor = fsm_entity_base_destructor;

    fsm_entity_timer_init(entity);
}

void fsm_entity_timer_init(void* entity)
{
    if (!entity) return;
    CVTTO_BASE(be, entity);
    int i;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i)
        be->timer_list[i] = 0;
}

int fsm_entity_set_timer(void* entity, int timerfd)
{
    if (!entity) return -1;
    CVTTO_BASE(be, entity);

    int i;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        if (be->timer_list[i] == 0){
            be->timer_list[i] = timerfd;
            return 0;
        }
    }

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
        if (!be->timer_list[i]) continue;

        stop_timer(be->timer_list[i]);
        be->timer_list[i] = 0;
    }
}

int fsm_entity_start_timer(void* entity, time_t seconds)
{
    if (!entity) return -1;
    CVTTO_BASE(base_entity, entity);
    int tfd = start_timer(base_entity->fsmid, seconds);
    if (tfd == -1){
        return -1;
    }

    if (-1 == fsm_entity_set_timer(entity, tfd)){
        return -1;
    }
    
    return 0;
}

void fsm_entity_stop_timer(void* entity, int timerfd)
{
    if (!entity) return ;
    CVTTO_BASE(be, entity);
    
    int i = 0;
    for (i = 0; i < MAX_ENTITY_TIMER_NUM; ++i){
        if (timerfd == be->timer_list[i]){
            be->timer_list[i] = 0;
            stop_timer(timerfd);
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



