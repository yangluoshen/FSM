#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>

#include <sys/epoll.h>
#include "msg.h"

static int sv_epfd;
static int sv_fd;

int init_sv_epoll()
{
    sv_epfd = epoll_create(MAX_SV_EPOLL_NUM);
    assert(-1 != sv_epfd);
    
    return sv_epfd;
}

char* read_client_msg()
{
    /*read msg head first*/
    msg_t msg;
    if (read(sv_fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
        perror("Error reading msg");
        return 0; 
    }

    /* secondly ,read the custome data.
     * data_len tell us how long the custome data is*/
    size_t data_len = msg.data_len;
    static char* data_buf[MAX_DATA_LEN];
    if (read(sv_fd, data_buf, data_len) != data_len){
        perror("Error reading req");
        return 0;
    }
    
    size_t msg_len = data_len + MSG_HEAD_LEN;
    char* pmsg = (char*) malloc(msg_len);
    assert(pmsg);
    memcpy(pmsg, &msg, MSG_HEAD_LEN);
    memcpy(pmsg+MSG_HEAD_LEN, data_buf, data_len);

    PRINT_MSG(&msg); puts("");
    return pmsg;
}

/* send what have read*/
int send_client_msg(char* pmsg)
{
    if (!pmsg) return -1;

    msg_t* msg_head;
    msg_head = (msg_t*)pmsg;

    char client_name[FIFO_NAME_LEN] = {0};
    /* get client fifo name */
    GEN_CL_NAME(client_name, msg_head->r_pid);

    int cl_fd = open(client_name, O_WRONLY);
    if (cl_fd == -1){
        perror("open client fifo failed");
        free(pmsg); pmsg = 0;
        return -1;
    }
    
    /*response */
    size_t total_len = MSG_HEAD_LEN + msg_head->data_len;
    if (write(cl_fd, pmsg, total_len) != total_len){
        perror("server:write error");
        return -1;
    } 

    /*pmsg alloced in read_client_msg() */
    free(pmsg); pmsg = 0;

    assert(close(cl_fd) != -1);
    return 0;
}

int check_new_process(){}

int main (int argc, char* argv[])
{
    int dummy_fd;

    umask(0);
    if (-1 == mkfifo(SV_FIFO, S_IRUSR|S_IWUSR|S_IWGRP) &&
            EEXIST != errno){
        perror("mkfifo error");
        return -1;
    }
    
    sv_fd = open(SV_FIFO, O_RDONLY|O_NONBLOCK);
    assert(sv_fd != -1);
    dummy_fd = open(SV_FIFO, O_WRONLY);
    assert(dummy_fd != -1);

    /*ignore SIGPIPE while there is no client reader*/
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) return -1;

    init_sv_epoll();
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sv_fd;
    if (epoll_ctl(sv_epfd, EPOLL_CTL_ADD, sv_fd, &ev) == -1){
        perror("epoll_ctl failed");
        return -1;
    }
    
    struct epoll_event ev_list[MAX_EVENTS];
    while(1){
        check_new_process();

        int ready = epoll_wait(sv_epfd, ev_list, MAX_EVENTS, TIMEOUT_SV_EPOLL);
        assert(-1 != ready);
        
        int i;
        for (i = 0; i < ready; ++i){
            if (ev_list[i].events & EPOLLIN){
                if (ev_list[i].data.fd == sv_fd){
                    if (-1 == send_client_msg(read_client_msg())) continue;
                }
            }
            else if (ev_list[i].events & (EPOLLHUP|EPOLLERR)){
                perror("epoll wait");
                return -1;
            }
        }
    }
    
    puts("server exit");
    return 0;
}
