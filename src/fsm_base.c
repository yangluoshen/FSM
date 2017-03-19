
#include "fsm_base.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

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

