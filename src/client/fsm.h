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

typedef struct fsm_msg_head{
    int msgtype;
    fsm_t fsmid;
    
    char data[];
}fsm_msg_head;

#define FSM_MSG_HEAD_LEN (sizeof(fsm_msg_head))

/* A fsm entity would be created while the specific type of msg type comes.
 * Client should initialize all kinds fsm_regist_info before process start.
 */
typedef struct __tag_fsm_regist_info{
    int type;
    fsm_constructor constructor;
    fsm_creator creator;
}fsm_reg;


fsm_entity_base* fsm_factory(int type, void* msg);
void fsm_entity_base_constructor(void* entity, fsm_t fsmid);
void fsm_entity_base_destructor(void* entity);
void fsm_set_fsm_finish(void* entity);
void rmv_fsm_entity(fsm_t fsmid);
void* get_fsm_entity(fsm_t fsmid);


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
