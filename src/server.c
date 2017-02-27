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

	/*ignore SIGPIPE while there is no client reader*/
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) return -1;

    while(1){
		/*read msg head first*/
        if (read(sv_fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
            perror("Error reading msg");
            break; // or continue?
        }

		/*secondly ,read the custome data.
		 * data_len tell us how long the custome data is*/
        size_t data_len = msg.data_len;
        if (read(sv_fd, &req, data_len) != data_len){
            perror("Error reading req");
            break; // or continue?
        }
        
        print_req_msg(&msg, &req); puts("");
        
		/* get client fifo name */
        snprintf(client_name, CL_FIFO_NAME_LEN, CL_FIFO_TPL, msg.s_pid);

        cl_fd = open(client_name, O_WRONLY);
        assert(cl_fd != -1);

		/*pack response msg*/
        resp.err_code = 1;
		resp.msg_type = 1024;
        msg.r_pid = getpid();
		swap(msg.s_pid, msg.r_pid);
        swap(msg.s_mdl, msg.r_mdl);
        msg.data_len = sizeof(resp_t);
        char* resp_buf = (char*)malloc(RESP_LEN);
        assert(resp_buf);
        memcpy(resp_buf, &msg, MSG_HEAD_LEN);
        memcpy(resp_buf + MSG_HEAD_LEN, &resp, sizeof(resp_t));
        
		/*response */
        if (write(cl_fd, resp_buf, RESP_LEN) != RESP_LEN){
            perror("server:write error");
            break;
        } 

		free(resp_buf); resp_buf = 0;

        if (close(cl_fd) == -1) return -1;
    }
    
    puts("server exit");
    return 0;
}

