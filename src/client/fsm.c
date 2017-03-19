#include "fsm.h"
#include <limits.h>

#define MIN_COMMON_FSMID (50)
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

fsm_t alloc_fsm_id()
{
    // If fsm_id_ptr is engaged or it's a resetved fsmid , continue loop.
    // when id_pool exhausted, it will cause dead loop. How to fix it?
    while(fsm_id_pool[++fsm_id_ptr] || is_reserved_fsmid(fsm_id_ptr));

    fsm_id_pool[fsm_id_ptr] = 1;
    return fsm_id_ptr;
}

void free_fsm_id(fsm_t fsmid)
{
    fsm_id_pool[fsmid] = 0;
}

// Generator a fsm entity according to 'type'
fsm_table_unit* fsm_factory(msg_t type, void* msg)
{
    fsm_t fsmid;
    int i;
    fsm_reg reg_info = get_reginfo_by_msgtype(type);
    if (!reg_info) goto FAILED;

    fsm_constructor constructor = reg_info->constructor;
    if (!constructor) goto FAILED;

    fsmid = alloc_fsm_id();
    if (-1 == fsmid) goto FAILED;

    fsm_table_unit* unit = (fsm_table_unit*)malloc(sizeof(fsm_table_unit));
    if (!unit) goto RELEASE;

    void* entity = constructor(msg);
    if (!entity) goto FREE;

    unit->fsmid = fsmid;
    unit->entity = entity;
    unit->nextjump = NULL; //essential

    return unit;

FREE:
    free(unit);
RELEASE:
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
