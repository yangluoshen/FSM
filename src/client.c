#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "msg.h"

static char client_fifo [FIFO_NAME_LEN];
static char dfl_content[] = "hello server";

void remove_cl_fifo()
{
    unlink(client_fifo);
}

int read_resp()
{
    int cl_fd = open(client_fifo, O_RDONLY);
    assert(cl_fd != -1);

    /* read msg head first */
    msg_t msg;
    if (read(cl_fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
        perror("client read response failed");
        return -1;
    }
    char* resp_buf = (char*) malloc(msg.data_len);
    assert(resp_buf);
    if (read(cl_fd, resp_buf, msg.data_len) != msg.data_len){
        perror("client: Error reading resp");
        return -1;
    }
    
    req_t* preq = (req_t*) resp_buf;
    print_req_msg(&msg, preq);

    puts("client exit");
    free(resp_buf);

    return 0;
}

int main(int argc, char* argv[])
{
    int  sv_fd;
    char *content = dfl_content;
    
    if (argc > 1) content = argv[1];

    umask(0);
    snprintf(client_fifo, FIFO_NAME_LEN, CL_FIFO_TPL, getpid());

    if (mkfifo(client_fifo, S_IRUSR|S_IWUSR|S_IWGRP) == -1 &&
        EEXIST != errno){
        perror("client mkfifo failed");
        return -1;
    }
    atexit(remove_cl_fifo);

    size_t data_len = sizeof(msg_type_t) + strlen(content) + 1;
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    assert(req_buf);

    msg_t* pmsg = (msg_t*)req_buf; 
    pmsg->s_pid = getpid();
    pmsg->r_pid = pmsg->s_pid;
    pmsg->s_mdl = YAU;
    pmsg->r_mdl = DVU;
    pmsg->data_len = data_len;
    
    req_t* preq = (req_t*)pmsg->data;
    preq->msg_type = 200;
    memcpy(preq->what, content, strlen(content) + 1);

    print_req_msg(pmsg, preq); puts("");


    sv_fd = open(SV_FIFO, O_WRONLY);
    if (-1 == sv_fd){
        perror("open sv fifo failed");
        return -1;
    }
    
    if (write(sv_fd, req_buf, msg_len) != msg_len){
        perror("client: write error");
        return -1;
    }


    if(-1 == read_resp()){
        perror("read_response");
        return -1;
    }

    return 0;
}
