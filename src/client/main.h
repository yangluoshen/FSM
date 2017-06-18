#ifndef __FSM_MAIN_H
#define __FSM_MAIN_H

#include <stdio.h>
#include <sys/types.h>

typedef void* data_t;
typedef size_t msg_type_t;
typedef ssize_t error_t;
typedef size_t module_t;
typedef unsigned short fsm_t;

#define MSG_HEAD \
    pid_t s_pid;     /*sender pid*/\
    pid_t r_pid;     /*receiver pid*/\
    module_t s_mdl;  /*sender module type*/\
    module_t r_mdl;  /*receiver module type*/\
    int ev_fd;\
    size_t data_len;

/* msg head */
typedef struct{
    MSG_HEAD

    char data[];    /* point to custome data */
} msg_t;

#define MSG_HEAD_LEN (sizeof(msg_t))


typedef void (*drive_func)(void*);

typedef struct{
    module_t mdl;
    drive_func func;

}msg_driver_node;

typedef struct {
    int timerfd;
    int timerid;
    fsm_t fsmid;

}fsm_timer;

enum TIMERTYPE{
    NO_LOOP = 0,
    LOOP = 1
};

enum SendMsgErrNo{
    SM_OK = 0,
    SM_FAILED = -1,
    SM_NULL = -2,
};

/*each Uint(module) may indicate a client(or a micro serve)*/
enum E_MODULE_TYPE{
	TTU = 0,YAU, BYU, PHU, RTU, CRU,

	UKU    /*unknown unit*/
};

size_t get_driver_size(void);
const msg_driver_node* get_driver_node(size_t index);
int start_timer(int timerid, fsm_t fsmid, time_t seconds);
void stop_timer(int timerfd);
int send_msg(void* m);

#define TIMEOUT_MSG (9999)

#define GET_TIMERFD(msg) \
    (((fsm_timer*)(((fsm_msg_head*)(((msg_t*)(msg))->data))->data))->timerfd);

#define GET_TIMERID(msg) \
    (((fsm_timer*)(((fsm_msg_head*)(((msg_t*)(msg))->data))->data))->timerid);

#endif 
