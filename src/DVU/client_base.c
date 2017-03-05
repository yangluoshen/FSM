#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "msg.h"

static char client_fifo [FIFO_NAME_LEN];
//static char dfl_content[] = "hello server";
const int epoll_size = 32;

static int epfd;
static int dummy_clfd;

void process_yau_req(void* pmsg);
void custome_processing(int fd);

typedef void (*drive_func)(void*);
typedef struct{
    module_t mdl;
    drive_func func;

}msg_driver_node;

/** client must implement the following */
const module_t ME_MDL = DVU;  /* the module type you want */
// define custome entry while msg comes 
msg_driver_node g_msg_driver[] = 
{
    {YAU, process_yau_req}

};
#define DRIVER_SZ (sizeof(g_msg_driver)/sizeof(msg_driver_node))


void remove_cl_fifo()
{
    unlink(client_fifo);
}

// read_fifo do not care what the data content is
void* read_fifo(int fd)
{
    /* read msg head first */
    msg_t msg;
    if (read(fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
        perror("client read response failed");
        return NULL;
    }

    size_t msg_len = MSG_HEAD_LEN + msg.data_len;
    char* buf = (char*) malloc(msg_len);
    if (!buf) return  NULL;
    memcpy(buf, &msg, MSG_HEAD_LEN);

    if (read(fd, ((msg_t*)buf)->data, msg.data_len) != msg.data_len){
        perror("client: Error reading resp");
        return NULL;
    }

    return buf;
}

int gen_fifo(const char* name, int mode)
{
    umask(0);
    if (-1 == mkfifo(name, S_IRUSR|S_IWUSR|S_IWGRP) &&
            EEXIST != errno) goto ERR;
    
    int fd = open(name, mode);
    if(-1 == fd) goto ERR;

    return fd;
    
ERR:
    perror("generate fifo failed");
    return -1;
}

int initialize()
{
    int ret = fsm_prcs_reg(ME_MDL);
    if (ret != 0) return -1;

    umask(0);
    snprintf(client_fifo, FIFO_NAME_LEN, CL_FIFO_TPL, getpid());

    /*while read msg, client will read from client_fifo*/
    if (mkfifo(client_fifo, S_IRUSR|S_IWUSR|S_IWGRP) == -1 &&
        EEXIST != errno){
        perror("client mkfifo failed");
        return -1;
    }
    atexit(remove_cl_fifo);
    
    int cl_fd = open(client_fifo, O_RDONLY|O_NONBLOCK);
    if (-1 == cl_fd) return -1;
    dummy_clfd = open(client_fifo, O_WRONLY);
    if (-1 == dummy_clfd) return -1;

    epfd = epoll_create(epoll_size);
    if (-1 == epfd) return -1;
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cl_fd; 
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cl_fd, &ev) == -1)
        return -1;

    atexit(fsm_prcs_unreg);
    return 0;
}

int main(int argc, char* argv[])
{
    if (0 != initialize()){
        perror("init failed");
        return -1;
    }

    /*ignore SIGPIPE while there is no server reader*/
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) return -1;

    struct epoll_event ev_list[epoll_size];
    while (1){
        int ready = epoll_wait(epfd, ev_list, epoll_size, -1);
        if (-1 == ready){
            perror("epoll_wait");
            continue;
        }
        int i;
        for (i = 0; i < ready; ++i){
            if(ev_list[i].events & EPOLLIN){
                custome_processing(ev_list[i].data.fd);
                continue;
            }else if (ev_list[i].events & (EPOLLHUP | EPOLLERR)){
                perror("read ev_list");
                return -1;
            }
        }
    }

    return 0;
}

void custome_processing(int fd)
{
    msg_t *pmsg = (msg_t*)read_fifo(fd);
    module_t type = pmsg->s_mdl;

    int i;
    for (i = 0; i < DRIVER_SZ; ++i){
        if (type == g_msg_driver[i].mdl){
            if (!g_msg_driver[i].func) continue;
            g_msg_driver[i].func(pmsg->data);
        }
    }
    
    free(pmsg);
}



