#ifndef __MSG_H
#define __MSG_H

#include <stdio.h>
#include <unistd.h>

#define PTR_SIZE (sizeof(void*))

#define MAX_CONTENT_LEN 1024
#define SV_FIFO "sv_fifo"
#define CL_FIFO_TPL "cl_fifo_%d"
#define CL_FIFO_NAME_LEN (sizeof(CL_FIFO_TPL) + 20)

typedef void* data_t;
typedef size_t msg_type_t;
typedef ssize_t error_t;
typedef size_t module_t;

/* msg head */
typedef struct{
    pid_t s_pid;     /*sender pid*/
	pid_t r_pid;     /*receiver pid*/
    module_t s_mdl;  /*sender module type*/
    module_t r_mdl;  /*sender module type*/
    size_t data_len;

    char data[PTR_SIZE];  /* point to real data */
} msg_t;

#define MSG_HEAD_LEN (sizeof(msg_t) - PTR_SIZE)


/** user-defined struct **/
typedef struct{
    msg_type_t msg_type;
    char what[MAX_CONTENT_LEN];
}req_t;

typedef struct{
    msg_type_t msg_type;
    error_t err_code;
}resp_t;

#define RESP_LEN (MSG_HEAD_LEN + sizeof(resp_t))

/**** utilities ****/
#define swap(__a, __b) \
    do {\
        (__a) ^= (__b);\
        (__b) ^= (__a);\
        (__a) ^= (__b);\
    }while(0)

/**** printer ****/
#define PRINT_MSG(__msg) \
        printf("sender pid[%d],receiver pid[%d],sender module[%lu],receiver module[%lu],data_len[%lu]\n",\
            (__msg)->s_pid,(__msg)->r_pid,(__msg)->s_mdl,(__msg)->r_mdl, (__msg)->data_len)

#define print_req_msg(__msg, __req) \
    do {\
		PRINT_MSG(__msg); \
        printf("msg_type[%lu], content[%s]\n",\
            (__req)->msg_type, (__req)->what);\
    }while(0)

#define print_resp_msg(__msg, __resp) \
	do{ \
		PRINT_MSG(__msg);\
		printf("msg_type[%lu], err_code[%lu]\n",\
				(__resp)->msg_type, (__resp)->err_code);\
	}while(0)

/**** enums ****/

/*each Uint(module) may indicate a client(or a micro serve)*/
enum E_MODULE_TYPE{
	DVU = 0,
	YAU,
	BYU,

	UKU    /*unknown unit*/
};

#endif

