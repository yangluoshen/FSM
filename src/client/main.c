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
#include <stdlib.h>

#include "msg.h"
#include "fsm.h"
#include "adlist.h"
#include "fdict.h"
#include "ae.h"
#include "anet.h"

#include "main.h"

#ifdef PHI_MDL
#include <curses.h>
#endif

#define MAX_EV_NUM (1024)

#define CLOG_MAIN
#include "debug.h"
const int G_LOGGER = 0;

static fdict* timer_dict;
list* g_fsm_driver;
static int curr_ev_fd = -1;

/* client base essential */
int __send_request(const char* name, const void* msg, size_t len);
void custome_processing(msg_t*);

/* timer declearation */
int timer_hash_match(void* ptr, fdict_key_t key);
size_t timer_hash_calc(fdict* d, fdict_key_t key);
int gen_real_timer(time_t sec, int isloop);
fsm_timer* get_timer(int fd);
msg_t* pack_timeout_msg(fsm_timer* ft);

extern const module_t ME_MDL;
extern const int ME_PORT;
extern const size_t FSM_DRIVER_SZ;
extern const int FSM_CLIENT_EPOLL_TIMEOUT;


#define SET_CURR_EVFD(fd) do {curr_ev_fd = (fd);}while(0)
#define RESET_CURR_EVFD() do {curr_ev_fd = -1;}while(0)

struct {
    int sv_fd;
    int port;
    int backlog;
    aeEventLoop* el;
    char neterr[255];
}server;


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

void finish()
{
#ifdef PHI_MDL
    endwin();
#endif
}

void sig_handler(int sig)
{
    finish();
    exit(0);
}

void free_conn(int fd, int mask)
{
    aeDeleteFileEvent(server.el, fd, mask);
    close(fd);
}

void _timeout_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;(void)mask;
    fsm_timer* t = get_timer(fd);
    if (!t) goto end;

    msg_t* m = pack_timeout_msg(t);
    if (!m) goto end;

    SET_CURR_EVFD(fd);
    custome_processing(m);

    free(m);
end:
    stop_timer(fd);
    RESET_CURR_EVFD();
    return;
}

// bug: 考虑到字节序问题，这种读socket的方式当放在网际通信是有问题的。
void* _get_msg(int fd)
{
    msg_t msg;
    if (read(fd, &msg, MSG_HEAD_LEN) != MSG_HEAD_LEN){
        LOG_ND("read req head failed");
        return NULL;
    }
    msg.ev_fd = fd;

    size_t msg_len = MSG_HEAD_LEN + msg.data_len; 
    char* buf = (char*) malloc(msg_len);
    if (!buf) return NULL;
    memcpy (buf, &msg, MSG_HEAD_LEN);

    if (read(fd, ((msg_t*)buf)->data, msg.data_len)!=msg.data_len){
        perror("read req's data failed");
        free (buf);
        return NULL;
    }
    
    return buf;
}

void _msg_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;

    msg_t* msg = (msg_t*) _get_msg(fd);
    if (!msg) goto end;

    SET_CURR_EVFD(fd);
    custome_processing(msg);
    free(msg);
end:
    free_conn(fd, mask);
    RESET_CURR_EVFD();
    return;
}

void _accept_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;(void)mask;
    int conn_fd = anetTcpAccept(server.neterr, fd, NULL, 0, NULL); 
    if (ANET_ERR == conn_fd) return;

    anetNonBlock(NULL, conn_fd);

    if (ANET_ERR == aeCreateFileEvent(server.el, conn_fd, AE_READABLE, _msg_handle, NULL)){
        exit(0);
    }
}

int el_init()
{
    server.el = aeCreateEventLoop(MAX_EV_NUM);
    server.port = ME_PORT;
    server.backlog = 100;
    server.sv_fd = anetTcpServer(server.neterr, server.port, NULL, server.backlog);
    if (ANET_ERR == server.sv_fd) return -1;
    
    anetNonBlock(NULL, server.sv_fd);
    
    if (ANET_ERR == aeCreateFileEvent(server.el, server.sv_fd, AE_READABLE, _accept_handle, NULL))
        return -1;

    return 0;
}

int initialize()
{
    if (-1 == timer_init()) return -1;
    if (-1 == log_init()) return -1;
    if (-1 == fsm_driver_init()) return -1;
    if (-1 == el_init()) return -1;
    
    if (signal(SIGINT, sig_handler) == SIG_ERR) return -1;
    if (signal(SIGQUIT, sig_handler) == SIG_ERR) return -1;
    /*ignore SIGPIPE while there is no server reader*/
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) return -1;

#ifdef PHI_MDL
    initscr();
    atexit(finish);
#endif

    return 0;
}

int main(int argc, char* argv[])
{
    if (0 != initialize()){
        perror("init failed");
        return -1;
    }

    aeMain(server.el);

    return 0;
}

void custome_processing(msg_t* pmsg)
{
    if (!pmsg) return;

    int i;
    module_t type = pmsg->s_mdl;
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
    // append this timer to timerlist so that we can find it while it is stopped
    ft->timerid = timerid;
    ft->timerfd = tfd;
    ft->fsmid = fsmid;

    if (ANET_ERR == aeCreateFileEvent(server.el, tfd, AE_READABLE, _timeout_handle, NULL)) {
        close(tfd);
        free(ft);
        return -1;
    }

    if (FDICT_SUCCESS != fdict_insert(timer_dict, &ft->timerfd, ft)){
        LOG_NE("fdict_insert failed");
        close(tfd);
        free(ft);
        aeDeleteFileEvent(server.el, tfd, AE_READABLE);
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

    aeDeleteFileEvent(server.el, timerfd, AE_READABLE);

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
    m->ev_fd = ft->timerfd;
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

int send_msg(void* m)
{
    if (-1==curr_ev_fd || !m) return SM_FAILED;
    msg_t* pmsg = (msg_t*)m;
    size_t msg_len = MSG_HEAD_LEN + pmsg->data_len;
    if (write(curr_ev_fd, m, msg_len) != msg_len)
        return SM_FAILED;

    return SM_OK;
}
