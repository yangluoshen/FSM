#ifndef __FSM_H
#define __FSM_H

#include <stdio.h>
//#include <unistd.h>
#include <sys/types.h>

typedef void* data_t;
typedef size_t msg_type_t;
typedef ssize_t error_t;
typedef size_t module_t;

#define PRCS_INFO \

#define MSG_HEAD \
    pid_t s_pid;     /*sender pid*/\
    pid_t r_pid;     /*receiver pid*/\
    module_t s_mdl;  /*sender module type*/\
    module_t r_mdl;  /*sender module type*/\
    size_t data_len;


/* msg head */
typedef struct{
    MSG_HEAD

    char data[];    /* point to custome data */
} msg_t;

#define MSG_HEAD_LEN (sizeof(msg_t))

int fsm_send_msg(void* pmsg);
int fsm_prcs_reg(module_t type);
void fsm_prcs_unreg(void);

enum SendMsgErrNo{
    SM_OK = 0,
    SM_FAILED = -1,
    SM_NULL = -2,
    SM_WRITE_PIPE_FAILED = -3
};

enum PRCS_CMD{
    PRCS_REG = 0,
    PRCS_UNREG
};


/** process register info **/
#define PRCS_BASE \
    pid_t pid; \
    module_t mdl;

typedef struct{
    char cmd;
    PRCS_BASE
}prcs_reg;

#define SV_REG_FIFO "/tmp/fsm_sv_reg"
#define SV_FIFO_TPL "/tmp/fsm_sv_fifo_%d"
#define CL_FIFO_TPL "/tmp/fsm_cl_fifo_%d"
#define FIFO_NAME_LEN (sizeof(CL_FIFO_TPL) + 20)

#define GEN_SV_NAME(buf, pid) \
    do \
        snprintf(buf, FIFO_NAME_LEN, SV_FIFO_TPL, pid);\
    while(0)
    
#define GEN_CL_NAME(buf, pid) \
    do \
        snprintf(buf, FIFO_NAME_LEN, CL_FIFO_TPL, pid);\
    while(0)

#endif  /*__FSM_H*/
