#define _POSIX_C_SOURCE  199309L
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>

#include <time.h>
#include <sys/timerfd.h>
#include "msg.h"
#include "adlist.h"

#include "client_base.h"


static list* timer_list;

static char client_fifo [FIFO_NAME_LEN];
const int epoll_size = 32;

static int g_client_epfd;
static int dummy_clfd;

/* client base essential */
int proc_prcs_reg(module_t type);
void proc_prcs_unreg(void);
int __send_request(const char* name, const void* msg, size_t len);
void custome_processing(int fd);

extern const module_t ME_MDL;
extern const size_t FSM_DRIVER_SZ;
extern const int FSM_CLIENT_EPOLL_TIMEOUT;


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

int client_login()
{
    int ret = proc_prcs_reg(ME_MDL);
    if (ret != 0) return -1;
    atexit(proc_prcs_unreg);
    return ret;
}

int epoll_init()
{
    snprintf(client_fifo, FIFO_NAME_LEN, CL_FIFO_TPL, getpid());

    umask(0);
    /*while read msg, client will read from client_fifo*/
    if (mkfifo(client_fifo, S_IRUSR|S_IWUSR|S_IWGRP) == -1 &&
        EEXIST != errno){
        perror("client mkfifo failed");
        return -1;
    }
    atexit(remove_cl_fifo);
    
    int cl_fd = open(client_fifo, O_RDONLY|O_NONBLOCK);
    if (-1 == cl_fd) return -1;
    /* dummy_clfd is essential. 
     * if not define dummy_clfd (in write only), 
     * read fifo will throw failed while server close its write file descriptor*/
    dummy_clfd = open(client_fifo, O_WRONLY);
    if (-1 == dummy_clfd) return -1;

    /* add cl_fd to epoll */
    g_client_epfd = epoll_create(epoll_size);
    if (-1 == g_client_epfd) return -1;
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = cl_fd; 
    if (epoll_ctl(g_client_epfd, EPOLL_CTL_ADD, cl_fd, &ev) == -1)
        return -1;

    return 0;

}

int timer_init()
{
    if ((timer_list = listCreate()) == NULL) return -1;
    return 0;
}

int initialize()
{
    if (-1 == client_login()) return -1;
    if (-1 == epoll_init()) return -1;
    if (-1 == timer_init()) return -1;

    return 0;
}

#ifdef YAU_MDL
void say_hello_to_dvu();
#endif
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
        int ready = epoll_wait(g_client_epfd, ev_list, epoll_size, FSM_CLIENT_EPOLL_TIMEOUT);
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
                return -1; }
        }
#ifdef YAU_MDL
        say_hello_to_dvu();
#endif
    }

    return 0;
}

void custome_processing(int fd)
{
    msg_t *pmsg = (msg_t*)read_fifo(fd);
    module_t type = pmsg->s_mdl;

    int i;
    for (i = 0; i < FSM_DRIVER_SZ; ++i){
        const msg_driver_node* node = get_driver_node(i);
        if (!node) continue;
        if (type == node->mdl){
            if (!node->func)
                continue;

            node->func(pmsg);
            break;
        }
    }
    
    free(pmsg);
}


/* send a msg to server */
int send_msg(void* m)
{
    char sv_fifo_name[FIFO_NAME_LEN] = {0};

    msg_t* pmsg = (msg_t*)m;

    GEN_SV_NAME(sv_fifo_name, pmsg->s_pid);
    return fsm_send_msg(sv_fifo_name, m);
}

int proc_prcs_reg(module_t type)
{
    prcs_reg reg;
    reg.cmd = PRCS_REG;
    reg.pid = getpid();
    reg.mdl = type;

    return __send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

void proc_prcs_unreg(void)
{
    prcs_reg reg;
    reg.cmd = PRCS_UNREG;
    reg.pid = getpid();

    (void)__send_request(SV_REG_FIFO, &reg, sizeof(reg));
}

int gen_real_timer(time_t sec, int isloop)
{
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (-1 == fd){
        perror("timerfd_create");
        return -1;
    }

    struct itimerspec ts;

    ts.it_value.tv_sec = sec;
    ts.it_value.tv_nsec = 0L;
    if (isloop){
        ts.it_interval.tv_sec = sec;
        ts.it_interval.tv_nsec = 0L;
    }
    else{
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
    }

    if (timerfd_settime(fd, 0, &ts, NULL) == -1){
        perror("timerfd_settime");
        close(fd);
        return -1;
    }
    
    return fd;
}

int start_timer(fsm_t fsmid, time_t seconds)
{
    int tfd = gen_real_timer(seconds, NO_LOOP);
    if (-1 == tfd){
        perror("generate timer failed");
        return -1;
    }
    
    fsm_timer* ft = (fsm_timer*) malloc(sizeof(fsm_timer));
    if (!ft){
        perror("malloc failed");
        close(tfd);
        return -1;
    }
    
    // insert into epoll and polling
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    if (epoll_ctl(g_client_epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) goto RELEASE;
    
    // append this timer to timerlist so that we can find it while it is stopped
    ft->timerfd = tfd;
    ft->fsmid = fsmid;
    if (NULL == listAddNodeTail(timer_list, ft)) goto RELEASE;

    return 0;

RELEASE:
    close(tfd);
    free(ft);
    perror("start timer failed\n");
    return -1;
}

void stop_timer(fsm_t fsmid)
{
    listNode* node;
    listIter* iter = listGetIterator(timer_list, AL_START_HEAD);
    while ((node = listNext(iter)) != NULL){
        fsm_timer* ft = (fsm_timer*) node->value;
        if (!ft) continue;
        if (fsmid == ft->fsmid){
            // remove from epoll
            (void)epoll_ctl(g_client_epfd, EPOLL_CTL_DEL, ft->timerfd, NULL);
            // remove from timer_list
            listDelNode(timer_list, node);
            break;
        }
    }
    free (iter);
}
