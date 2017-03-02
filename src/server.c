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
#include "adlist.h"
#include "sds.h"

static int sv_epfd;
static int sv_fd;
static int sv_reg_fd;
//static unsigned int max_conn_num = MAX_PROCESS_CONN_NUM;

list* reg_list;

typedef struct {
    PRCS_BASE

    int fd;
    int dummyfd;
}prcs_info;

enum FIFO_KIND{
    SV = 0,
    CL = 1
};

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

int prcs_reg_info_init()
{
    if ((reg_list = listCreate()) == NULL) return -1;

    return 0;
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

void process_reg(const prcs_reg* preg)
{
    int fd, dummyfd;
    char name[FIFO_NAME_LEN] = {0};
    GEN_SV_NAME(name, preg->pid);

    fd = gen_fifo(name, O_RDONLY|O_NONBLOCK);
    if (-1 == fd) goto ERR;
    dummyfd = gen_fifo(name, O_WRONLY);
    if (-1 == fd) goto ERR;

    prcs_info* pi = (prcs_info*) malloc(sizeof (prcs_info));
    if (!pi) goto CLOSE_FD;

    pi->pid = preg->pid;
    pi->mdl = preg->mdl;
    pi->fd  = fd;
    pi->dummyfd = dummyfd;

    // add to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(sv_epfd, EPOLL_CTL_ADD, fd, &ev) == -1) goto RELEASE;

    // append reg_list
    if (NULL == listAddNodeTail(reg_list, pi)) goto RELEASE ;

    printf("process[%d] register successfully\n", preg->pid);

    return;

RELEASE:
    free (pi);
    pi = 0;
CLOSE_FD:
    close(fd);
    close(dummyfd);
ERR:
    perror("process_reg failed");
    return;

} 

void process_unreg(const prcs_reg* preg)
{
    listNode* node;
    listIter* iter = listGetIterator(reg_list, AL_START_HEAD);
    while ((node = listNext(iter)) != NULL){
        prcs_info* pi = (prcs_info*)node->value;
        if(!pi) continue;
        if (pi->pid == preg->pid){
            (void)epoll_ctl(sv_epfd, EPOLL_CTL_DEL, pi->fd, NULL);
            // close fd
            close(pi->fd);
            close(pi->dummyfd);
            // remove sv_fifo
            char name[FIFO_NAME_LEN] = {0};
            GEN_SV_NAME(name, pi->pid);
            unlink(name);
            // remove from register list
            listDelNode(reg_list, node);
            break;
        }
    }
    
    printf("process[%d] unregister successfully\n", preg->pid);
}

int check_process_conn()
{
    prcs_reg reg;
    
    for (;;){
        ssize_t s = read(sv_reg_fd, &reg, sizeof(prcs_reg));
        if (s == 0 || s == EAGAIN || s != sizeof(prcs_reg)) break;
        print_reg_info(&reg);

        switch(reg.cmd){
            case PRCS_REG:
                process_reg(&reg);
                break;
            case PRCS_UNREG:
                process_unreg(&reg);
                break;
            default:
                printf("Error: unknow process cmd[%c]\n", reg.cmd);
                break;
        }
    }

    return 0;
}

int check_argv(){return 0;}

int initialize()
{
    if(prcs_reg_info_init()) return -1;
    return 0;
}

int main (int argc, char* argv[])
{
    int dummy_fd, dummy_reg_fd;

    assert(!check_argv());
    assert(!initialize());

    sv_fd = gen_fifo(SV_FIFO, O_RDONLY|O_NONBLOCK);
    assert(sv_fd != -1);
    dummy_fd = gen_fifo(SV_FIFO, O_WRONLY);
    assert(dummy_fd != -1);

    sv_reg_fd = gen_fifo(SV_REG_FIFO, O_RDONLY|O_NONBLOCK);
    assert(-1 != sv_reg_fd);
    dummy_reg_fd = gen_fifo(SV_REG_FIFO, O_WRONLY);
    assert(-1 != dummy_reg_fd);


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
        (void)check_process_conn();

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

