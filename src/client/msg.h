#ifndef __MSG_H
#define __MSG_H

#include <stdio.h>
#include <unistd.h>

#include "fsm_base.h"



/**** utilities ****/
#define swap(A, B) ({  __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a ^= __b; __b ^= __a; __a ^= __b;})

        
/**** printer ****/
#define PRINT_MSG(__msg) \
        printf("sender pid[%d],receiver pid[%d],sender module[%lu],receiver module[%lu],data_len[%lu]\n",\
            ((msg_t*)(__msg))->s_pid,((msg_t*)(__msg))->r_pid,((msg_t*)(__msg))->s_mdl,((msg_t*)(__msg))->r_mdl, ((msg_t*)(__msg))->data_len)

#define print_req_msg(__msg, __req) \
    do {\
		PRINT_MSG(__msg); \
        printf("msg_type[%lu], content[%s]\n",\
            ((req_t*)(__req))->msg_type, ((req_t*)(__req))->what);\
    }while(0)

#define print_resp_msg(__msg, __resp) \
	do{ \
		PRINT_MSG(__msg);\
		printf("msg_type[%lu], err_code[%lu]\n",\
				((resp_t*)(__resp))->msg_type, ((resp_t*)(__resp))->err_code);\
	}while(0)

#define print_reg_info(__reg) \
    do {\
        printf("cmd[%d], pid[%d], module[%lu]\n",((prcs_reg*)__reg)->cmd, ((prcs_reg*)__reg)->pid,((prcs_reg*)__reg)->mdl);\
    }while(0) 

#endif
