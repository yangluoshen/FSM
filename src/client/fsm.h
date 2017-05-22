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
void fsm_entity_base_exception(void* entity);
int fsm_entity_start_timer(void* entity, int timerid, time_t seconds);
void fsm_entity_stop_timer(void* entity, int timerfd);
void fsm_set_fsm_finish(void* entity);
void rmv_fsm_entity(fsm_t fsmid);
void* get_fsm_entity(fsm_t fsmid);

void proc_fsm_req(msg_t* pmsg);
void proc_fsm_resp(msg_t* pmsg);

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


#define GET_MSGTYPE(msg) \
    (((fsm_msg_head*)(((msg_t*)(msg))->data))->msgtype)
    
#define GET_FSMID(msg) \
    (((fsm_msg_head*)(((msg_t*)(msg))->data))->fsmid)

#define GET_DATA(msg) \
    (((fsm_msg_head*)(((msg_t*)(msg))->data))->data)

#define SET_MSGTYPE(msg, type) \
    (((fsm_msg_head*)(((msg_t*)(msg))->data))->msgtype)=(type);

#define SET_FSMID(msg, id) \
    (((fsm_msg_head*)(((msg_t*)(msg))->data))->fsmid)=(id);

#define INIT_MSG_HEAD(m, spid, rpid, smdl, rmdl, len) \
    do {\
        ((msg_t*)(m))->s_pid=(spid);\
        ((msg_t*)(m))->r_pid=(rpid);\
        ((msg_t*)(m))->s_mdl=(smdl);\
        ((msg_t*)(m))->r_mdl=(rmdl);\
        ((msg_t*)(m))->data_len=(len);\
    }while(0)


#endif /*__FSM_H */
