#define _POSIX_C_SOURCE  199309L
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>
#include <sys/timerfd.h>

#include "msg.h"
#include "fsm.h"
#include "adlist.h"
#include "fdict.h"

#include "main.h"

#define CLOG_MAIN
#include "debug.h"
const int G_LOGGER = 0;

static fdict* timer_dict;
list* g_fsm_driver;

static char client_fifo [FIFO_NAME_LEN];
const int epoll_size = 32;

int g_client_epfd;
static int dummy_clfd;

/* client base essential */
int proc_prcs_reg(module_t type);
void proc_prcs_unreg(void);
int __send_request(const char* name, const void* msg, size_t len);
void custome_processing(int fd);

/* timer declearation */
int timer_hash_match(void* ptr, fdict_key_t key);
size_t timer_hash_calc(fdict* d, fdict_key_t key);
int gen_real_timer(time_t sec, int isloop);
fsm_timer* get_timer(int fd);
msg_t* pack_timeout_msg(fsm_timer* ft);



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

/*
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
*/

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
    const size_t timer_size = 256;
    timer_dict = fdict_create(timer_size, timer_hash_match, timer_hash_calc);
    if (!timer_dict) return -1;

    return 0;
}

int fsm_driver_init()
{
    if ((g_fsm_driver = listCreate()) == NULL) return -1;
    g_fsm_driver->free = free;
    return 0;
}

int log_init()
{
    char client_log_file[32] = {0};
    snprintf(client_log_file, 32, "/tmp/fsm_cl_M%luP%d.log", ME_MDL, getpid());

    int r = clog_init_path(G_LOGGER, client_log_file);
    if (-1 == r){
        perror("init clog failed");
        return -1;
    }

    return 0;
}

int initialize()
{
    if (-1 == client_login(ME_MDL)) return -1;
    if (-1 == epoll_init()) return -1;
    if (-1 == timer_init()) return -1;
    if (-1 == log_init()) return -1;
    if (-1 == fsm_driver_init()) return -1;

    return 0;
}

#ifdef YAU_MDL
void say_hello_to_ttu();
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
                return -1; 
            }
        }
// just for debug
#ifdef YAU_MDL
        static int count = 0;
        if (count++ == 0)
            say_hello_to_ttu();
#endif
    }

    return 0;
}

void custome_processing(int fd)
{
    msg_t *pmsg;
    // 如果是超时消息, 需特殊处理
    fsm_timer* ft = get_timer(fd);
    if (ft){
        LOG_ND("get timeout msg");
        pmsg = pack_timeout_msg(ft);
    }
    else{
        pmsg = (msg_t*)read_fifo(fd);
    }

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

/************** Timer ********************/

int timer_hash_match(void* ptr, fdict_key_t key)
{
    if (!ptr || !key) return 0;
    fsm_timer* t = (fsm_timer*) ptr;
    return t->timerfd == *(int*)key;
}

size_t timer_hash_calc(fdict* d, fdict_key_t key)
{
    if (!d || !key) return -1;
    int hash = *((int*)key) % d->hash_size;
    return (size_t)hash;
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

int start_timer(int timerid, fsm_t fsmid, time_t seconds)
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
    if (epoll_ctl(g_client_epfd, EPOLL_CTL_ADD, tfd, &ev) == -1){
        perror("start timer failed\n");
        close(tfd);
        free(ft);
        return -1;
    }
    
    // append this timer to timerlist so that we can find it while it is stopped
    ft->timerid = timerid;
    ft->timerfd = tfd;
    ft->fsmid = fsmid;
    if (FDICT_SUCCESS != fdict_insert(timer_dict, &ft->timerfd, ft)){
        LOG_NE("fdict_insert failed");
        close(tfd);
        free(ft);
        epoll_ctl(g_client_epfd, EPOLL_CTL_DEL, tfd, NULL);
        return -1;
    }

    LOG_D("start timer[%d] success", tfd);
    return tfd;
}

void stop_timer(int timerfd)
{
    LOG_ND("Enter");
    fsm_timer* ft = (fsm_timer*) fdict_find(timer_dict, &timerfd);
    if (!ft) return;

    (void) epoll_ctl(g_client_epfd, EPOLL_CTL_DEL, ft->timerfd, NULL);
    close(timerfd);
    (void)fdict_remove(timer_dict, &timerfd);

    LOG_D("stop timer[%d]", timerfd);
    return;
}

fsm_timer* get_timer(int fd)
{
    return (fsm_timer*) fdict_find(timer_dict, &fd);
}

msg_t* pack_timeout_msg(fsm_timer* ft)
{
    LOG_ND("pack time out msg");
    int data_len = FSM_MSG_HEAD_LEN + sizeof (fsm_timer);
    int msg_len = MSG_HEAD_LEN + data_len;
    msg_t* m = (msg_t*) malloc(msg_len);
    if (!m) return NULL;
    m->s_pid = getpid();
    m->r_pid = getpid();
    m->s_mdl = ME_MDL;
    m->r_mdl = ME_MDL;
    m->data_len = data_len;

    fsm_msg_head* fh = (fsm_msg_head*) m->data;
    fh->fsmid = ft->fsmid;
    fh->msgtype = TIMEOUT_MSG;

    fsm_timer* t = (fsm_timer*)fh->data;
    t->timerfd = ft->timerfd;
    t->fsmid = ft->fsmid;
    t->timerid = ft->timerid;

    return m;
}

