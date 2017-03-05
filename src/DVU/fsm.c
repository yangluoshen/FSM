
#include "fsm.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int __send_request(const char* name, void* msg, size_t len)
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

int fsm_prcs_reg(module_t type)
{
    prcs_reg reg;
    reg.cmd = PRCS_REG;
    reg.pid = getpid();
    reg.mdl = type;

    return __send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

void fsm_prcs_unreg(void)
{
    prcs_reg reg;
    reg.cmd = PRCS_UNREG;
    reg.pid = getpid();

    (void)__send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

int fsm_send_msg(void* m)
{
    if (!m) return SM_NULL;

    char sv_fifo_name[FIFO_NAME_LEN];

    msg_t* pmsg = (msg_t*)m;
    GEN_SV_NAME(sv_fifo_name, pmsg->s_pid);
    size_t msg_len = MSG_HEAD_LEN + pmsg->data_len;

    int ret = __send_request(sv_fifo_name, pmsg, msg_len);
    if(ret != 0) return SM_FAILED;

    return SM_OK;
}
