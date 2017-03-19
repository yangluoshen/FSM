#ifndef __FSM_H
#define __FSM_H

#include "fsm_base.h"
#include "adlist.h"

typedef void* (*fsm_constructor)(void*);
typedef int (*fsm_destructor)(void);
typedef int (*fsm_nextjump_func)(void* msg, void* entity);

typedef struct __tag_fsm_entity{
    fsm_t fsmid;
    list timer_list;
    char is_fsm_finish;
}fsm_entity;


// A fsm entity will be created 
// while the specific type of msg type comes.
// Client should initialize all kinds fsm_regist_info before
// process start
typedef struct __tag_fsm_regist_info{
    msg_t type;
    fsm_constructor constructor;
    fsm_destructor  destructor;
}fsm_reg;

// Tell procedure what next jump is.
// The fsm driver table should be global.
struct __tag_fsm_drive_table_unit{
    fsm_t fsmid;
    fsm_nextjump_func nextjump;
    void* entity;
}fsm_table_uint;


#endif /*__FSM_H */
