#ifndef __FSM_H
#define __FSM_H

#include "fsm_base.h"
#include "adlist.h"

#define MAX_ENTITY_TIMER_NUM (20)
typedef void  (*fsm_constructor)(void* entity, fsm_t fsmid);
typedef void  (*fsm_destructor)(void* entity);
typedef int   (*fsm_nextjump_func)(void* entity, void* msg);
typedef void  (*fsm_exception)(void* entity);
typedef void* (*fsm_creator)(void);
typedef void  (*fsm_event)(void* entity, void* msg);

#define FSM_ENTITY_BASE \
    fsm_constructor constructor;\
    fsm_destructor destructor;\
    fsm_event event;\
    fsm_nextjump_func nextjump;\
    fsm_exception exception;\
    fsm_t fsmid;\
    int timer_list[MAX_ENTITY_TIMER_NUM];\
    char is_fsm_finish;

typedef struct __tag_fsm_entity_base{
    FSM_ENTITY_BASE
}fsm_entity_base;

/* A fsm entity will be created 
 * while the specific type of msg type comes.
 * Client should initialize all kinds fsm_regist_info before
 * process start
 */
typedef struct __tag_fsm_regist_info{
    int type;
    fsm_constructor constructor;
    fsm_creator creator;
    //fsm_destructor  destructor;
}fsm_reg;

/* Tell procedure what next jump is.
 * The fsm driver table should be global.
 */
typedef struct __tag_fsm_drive_table_unit{
    fsm_t fsmid;
    fsm_nextjump_func nextjump;
    void* entity;
}fsm_table_unit;


fsm_table_unit* fsm_factory(int type, void* msg);
void fsm_table_unit_destroy(fsm_table_unit* unit);
void fsm_entity_base_constructor(void* entity, fsm_t fsmid);
void fsm_entity_base_destructor(void* entity);
void fsm_set_fsm_finish(void* entity);


#define MIN_COMMON_FSMID (50)
#define FSM_HASH_NUM (110)

enum {
    FSMID_IDLE = 0,
    FSMID_ENGAGED = 1
};

enum FSM_RETCODE{
    FSM_OK = 0,
    FSM_FAIL
};

#define CVTTO_BASE(base, entity)\
    fsm_entity_base* (base) = (fsm_entity_base*) (entity);

#endif /*__FSM_H */
