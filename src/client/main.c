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
#include "dict.h"
#include "ae.h"
#include "anet.h"
#include "client_config.h"
#include "main.h"

#ifdef PHI_MDL
#include <curses.h>
#endif

#define MAX_EV_NUM (1024)

#define CLOG_MAIN
#include "debug.h"
const int G_LOGGER = 0;

static dict* timer_dict;
static int curr_ev_fd = -1;
static inet_addr** inet_list;

list* g_fsm_driver;
dict_option timer_op = {hash_calc_int, NULL, NULL, NULL, NULL, NULL};

/* client base essential */
int __send_request(const char* name, const void* msg, size_t len);
void custome_processing(msg_t*);

/* timer declearation */
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
    char neterr[256];
}server;


int timer_init()
{
    timer_dict = dict_create(&timer_op);
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

inet_addr* _get_addr(module_t mdl){
    inet_addr* ia = *inet_list;
    while(ia){
        if (ia->mdl == mdl){
            return ia;
        }
        ia = ia->next;
    }

    return NULL;
}

void _timeout_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;(void)mask;
    fsm_timer* t = get_timer(fd);
    if (!t) goto end;

    msg_t* m = pack_timeout_msg(t);
    if (!m) goto end;

    custome_processing(m);

    free(m);
end:
    stop_timer(fd);
    return;
}

// bug: 考虑到字节序问题，这种读socket的方式当放在网际通信是有问题的。
void _msg_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;

    msg_t msg;
    ssize_t n = read(fd, &msg, MSG_HEAD_LEN);
    if (n == 0) goto end; // read EOF, close connection
    if (n < 0 && (errno == EINTR || errno == EAGAIN))  return ;
    if (n != MSG_HEAD_LEN) goto end;
    
    size_t msg_len = MSG_HEAD_LEN + msg.data_len;
    printf("recv msg:len= %lu + %lu\n", MSG_HEAD_LEN, msg.data_len);
    char* buf = (char*) malloc(msg_len);
    if (!buf) goto end;
    memcpy (buf, &msg, MSG_HEAD_LEN);

    if (read(fd, ((msg_t*)buf)->data, msg.data_len) != msg.data_len){
        perror("read req's data failed");
        free (buf);
        goto end;
    }

    SET_CURR_EVFD(fd);
    custome_processing((msg_t*)buf);
    free(buf);
    RESET_CURR_EVFD();
end:
    free_conn(fd, mask);
    LOG_D("close connection[%d]", fd);
    return;
}

void _accept_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;(void)mask;
    int conn_fd = anetUnixAccept(server.neterr, fd); 
    if (ANET_ERR == conn_fd) return;

    anetNonBlock(NULL, conn_fd);

    if (ANET_ERR == aeCreateFileEvent(server.el, conn_fd, AE_READABLE, _msg_handle, NULL)){
        exit(0);
    }
}

int el_init()
{
    server.el = aeCreateEventLoop(MAX_EV_NUM);
    server.backlog = 100;
    inet_addr* ia = _get_addr(ME_MDL);
    if (!ia) return -1;
    unlink(ia->addr.path);
    server.sv_fd = anetUnixServer(server.neterr, ia->addr.path, 0775, server.backlog);
    if (ANET_ERR == server.sv_fd) return -1;
    
    anetNonBlock(NULL, server.sv_fd);
    
    if (ANET_ERR == aeCreateFileEvent(server.el, server.sv_fd, AE_READABLE, _accept_handle, NULL))
        return -1;

    return 0;
}

int inet_init()
{
    inet_list = (inet_addr**) malloc(sizeof(inet_addr*));
    if (!inet_list) return -1;
    *inet_list = NULL;

// the following define is for test
#if (defined TTU_MDL || defined YAU_MDL)
    inet_addr* net = (inet_addr*) malloc(sizeof(inet_addr)); 
    if (!net) return -1;
    net->mdl = TTU;
    net->port = 5701;
    net->addr.path = "/fsmunixsockttu";
    net->next = NULL;

    net->next = *inet_list;
    *inet_list = net;

    net = (inet_addr*) malloc(sizeof(inet_addr));
    if (!net) return -1;
    net->mdl = YAU;
    net->port = 5702;
    net->addr.path = "/fsmunixsockyau";
    net->next = *inet_list;
    *inet_list = net;

#endif 
    
    return 0;
}

void init_cli_events()
{
    size_t i;
    size_t ev_num = get_cli_ev_size();
    for (i=0; i<ev_num; ++i){
        ev_driver_node* ev = get_cli_ev(i);
        if (ev){
            aeCreateFileEvent(server.el, ev->fd, ev->mask, ev->proc, NULL);
        }
    }
}

int initialize()
{
    if (-1 == timer_init()) return -1;
    if (-1 == log_init()) return -1;
    if (-1 == fsm_driver_init()) return -1;
    if (-1 == inet_init()) return -1;
    if (-1 == el_init()) return -1;
    init_cli_events();
    
    if (signal(SIGINT, sig_handler) == SIG_ERR) return -1;
    if (signal(SIGQUIT, sig_handler) == SIG_ERR) return -1;
    /*ignore SIGPIPE while there is no peer reader*/
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

    if (NULL == dict_add(timer_dict, (void*)(long)ft->timerfd, ft)){
        LOG_NE("dict_insert failed");
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
    fsm_timer* ft = (fsm_timer*) DICT_GETVAL(dict_find(timer_dict, (void*)(long)timerfd));
    if (!ft) return;

    aeDeleteFileEvent(server.el, timerfd, AE_READABLE);

    close(timerfd);
    (void)dict_delete(timer_dict, (void*)(long)timerfd);

    LOG_D("stop timer[%d]", timerfd);
    return;
}

fsm_timer* get_timer(int fd)
{
    dict_entry* he = dict_find(timer_dict, (void*)(long)fd);
    return (fsm_timer*)(DICT_GETVAL(he)) ;
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



void _delaysend_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    if (!clientData) goto end; 

    msg_t* m = (msg_t*)clientData;
    size_t msg_len = MSG_HEAD_LEN + m->data_len;
    
    if (msg_len != write(fd, m, msg_len)){
        LOG_NE("send msg failed");
    }

    free (clientData);

end:
    free_conn(fd, mask);
    return;
}

int send_msg(void* m)
{
    if (!m) return SM_FAILED;
    msg_t* pmsg = (msg_t*)m;
    size_t msg_len = MSG_HEAD_LEN + pmsg->data_len;
    printf("send msg:len= %lu + %lu\n", MSG_HEAD_LEN, pmsg->data_len);
    // if curr_ev_fd!=-1, means we are solving a request, so we owe a connecttion.
    // if curr_ev_fd==-1, means we are going to send a request,so we need a new connection.
    int send_fd = curr_ev_fd;
    if (-1 == send_fd){
        inet_addr* ia = _get_addr(pmsg->r_mdl);
        if (!ia){
            LOG_E("can not connect module[%lu]", pmsg->r_mdl);
            return SM_FAILED;
        }

        char err[256] = {0};
        int conn_fd = anetUnixConnect(err, ia->addr.path);
        printf("err:%s\n", err);
        if (conn_fd == ANET_ERR && errno == EINPROGRESS){
            LOG_ND("delay and send");
            void* client_data = malloc(msg_len); //client_data will be freed in the callback func
            if (!client_data) {
                close(conn_fd);
                return SM_FAILED;
            }
            memcpy(client_data, m, msg_len);
            aeCreateFileEvent(server.el, conn_fd, AE_WRITABLE, _delaysend_handle, client_data);
            return SM_OK;
        }
        else if (conn_fd == ANET_ERR){
            perror("connect");
            LOG_NE("connect failed");
            return SM_FAILED;
        }
        else{
            if (write(conn_fd, m, msg_len) != msg_len){
                return SM_FAILED;
            }
            aeCreateFileEvent(server.el, conn_fd, AE_READABLE, _msg_handle, NULL);
            return SM_OK;
        }
    }
    else{
        if (write(send_fd, m, msg_len) != msg_len)
            return SM_FAILED;
    }

    return SM_OK;
}
