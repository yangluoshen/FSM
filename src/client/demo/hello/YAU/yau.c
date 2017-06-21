#include "main.h"
#include "ttu_yau_msg.h"
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include "fsm.h"
#include <errno.h>

#define MAX_BUF_SIZE (1024)

int send_msg(void*);

void* pack_msg(int msgtype, fsm_t peer_fsmid, fsm_t src_fsmid, char* what, int n)
{
    // pack message
    size_t data_len = FSM_MSG_HEAD_LEN + n;
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    assert(req_buf);

    msg_t* pmsg = (msg_t*)req_buf; 
    pmsg->s_pid = getpid();
    pmsg->r_pid = -1;
    pmsg->s_mdl = YAU;
    pmsg->r_mdl = TTU;
    pmsg->data_len = data_len;

    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    fsm_head->msgtype = msgtype;
    fsm_head->fsmid = peer_fsmid;
    
    req_t* preq = (req_t*)fsm_head->data;
    preq->src_fsmid = src_fsmid;
    memcpy(preq->what, what, n);

    printf("pack msg:len= %lu + %lu + %d\n", MSG_HEAD_LEN, FSM_MSG_HEAD_LEN, n);
    
    return req_buf;
}

void send_cache_req(char* msg, int n)
{
    void * pmsg = pack_msg(CACHE_REQ, -1, 30, msg, n);
    if(SM_OK != send_msg(pmsg)){
        perror("send msg failed");
    }
}

void stdin_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    (void)eventLoop;(void)clientData;(void)mask;
    char buf[MAX_BUF_SIZE] = {0};
    ssize_t n = read(fd, buf, MAX_BUF_SIZE);
    if (n<0){
        if (errno==EAGAIN || errno==EINTR) return;
        perror("stdin read");
        return;
    }

    send_cache_req(buf, n);
}

void proc_cache_resp(msg_t* data)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) data->data;
    req_t* r = (req_t*) fsm_head->data;
    char buf[2014] = {0};
    int what_len = data->data_len - FSM_MSG_HEAD_LEN;
    memcpy (buf, r->what, what_len);
    printf(">%s\n", buf);
    
    /*
    void* pmsg = pack_msg(CACHE_QUERY_REQ, r->src_fsmid, 31, "How are you?");
    if(SM_OK != send_msg(pmsg)){
        perror("send msg failed");
    }
    */
    //exit(1);
}

/*
void proc_cache_query_resp(msg_t* data)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) data->data;
    req_t* r = (req_t*) fsm_head->data;
    printf("ttu:%s\n", r->what);

    exit(1);
}
*/

void process_ttu_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    switch(fsm_head->msgtype){
        case CACHE_RESP: 
            proc_cache_resp(pmsg);
            break;
        /*
        case CACHE_QUERY_RESP:
            proc_cache_query_resp(pmsg);
            break;
        */
        default:
            puts("unknown msg type");
            break;
    }
}

