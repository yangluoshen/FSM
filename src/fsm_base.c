
#include "fsm_base.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int __send_request(const char* name, const void* msg, size_t len)
{
    int fd = open(name, O_WRONLY);
    if (-1 == fd) return -1;
    
    if (write(fd, msg, len) != len){
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


int fsm_send_msg(const char* fifo_name, const void* m)
{
    if (!m) return SM_NULL;

    const msg_t* pmsg = (const msg_t*)m;
    size_t msg_len = MSG_HEAD_LEN + pmsg->data_len;

    int ret = __send_request(fifo_name, pmsg, msg_len);
    if(ret != 0) return SM_FAILED;

    return SM_OK;
}

/* send a msg to server */
int send_msg(void* m)
{
    char sv_fifo_name[FIFO_NAME_LEN] = {0};

    msg_t* pmsg = (msg_t*)m;

    GEN_SV_NAME(sv_fifo_name, pmsg->s_pid);
    return fsm_send_msg(sv_fifo_name, m);
}

int proc_prcs_reg(module_t mdl)
{
    prcs_reg reg;
    reg.cmd = PRCS_REG;
    reg.pid = getpid();
    reg.mdl = mdl;

    return __send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

void proc_prcs_unreg(void)
{
    prcs_reg reg;
    reg.cmd = PRCS_UNREG;
    reg.pid = getpid();

    (void)__send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

int client_login(module_t mdl)
{
    int ret = proc_prcs_reg(mdl);
    if (ret != 0) return -1;
    atexit(proc_prcs_unreg);
    return ret;
}

