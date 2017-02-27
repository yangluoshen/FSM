#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include "msg.h"


int main (int argc, char* argv[])
{
    int sv_fd, cl_fd, dummy_fd;
    char client_name[CL_FIFO_NAME_LEN] = {0};
    msg_t msg;
    req_t req;
    resp_t resp;

    umask(0);
    if (-1 == mkfifo(SV_FIFO, S_IRUSR|S_IWUSR|S_IWGRP) &&
            EEXIST != errno){
        perror("mkfifo error");
        return -1;
    }
    
    sv_fd = open(SV_FIFO, O_RDONLY);
    assert(sv_fd != -1);
    dummy_fd = open(SV_FIFO, O_WRONLY);
    assert(dummy_fd != -1);

    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) return -1;

    while(1){
        if (read(sv_fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
            perror("Error reading msg");
            break;
        }

        size_t data_len = msg.data_len;
        if (read(sv_fd, &req, data_len) != data_len){
            perror("Error reading req");
            break;
        }
        
        print_req_msg(&msg, &req);
        
        snprintf(client_name, CL_FIFO_NAME_LEN, CL_FIFO_TPL, msg.pid);

        cl_fd = open(client_name, O_WRONLY);
        assert(cl_fd != -1);

        resp.err_code = 1;
		resp.msg_type = 1024;
        msg.pid = getpid();
        swap(msg.sender, msg.recver);
        msg.data_len = sizeof(resp_t);
        char* resp_buf = (char*)malloc(RESP_LEN);
        assert(resp_buf);
        memcpy(resp_buf, &msg, MSG_HEAD_LEN);
        memcpy(resp_buf + MSG_HEAD_LEN, &resp, sizeof(resp_t));
        
        if (write(cl_fd, resp_buf, RESP_LEN) != RESP_LEN){
            perror("server:write error");
            break;
        } 
        if (close(cl_fd) == -1) return -1;
    }
    
    puts("server exit");
    return 0;
}

