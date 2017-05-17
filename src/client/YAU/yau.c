#include "fsm_base.h"
#include "ttu_yau_msg.h"
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include "fsm.h"

int send_msg(void*);


void* pack_msg(int msgtype, fsm_t peer_fsmid, fsm_t src_fsmid, char* what)
{
    // pack message
    size_t data_len = FSM_MSG_HEAD_LEN + strlen(what) + 1;
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
    memcpy(preq->what, what, strlen(what) + 1);
    
    return req_buf;
}

void say_hello_to_ttu()
{
    void * pmsg = pack_msg(CACHE_REQ, -1, 30, "Nice to meet you!");
    if(SM_OK != send_msg(pmsg)){
        perror("send msg failed");
    }
}

void proc_cache_resp(msg_t* data)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) data->data;
    req_t* r = (req_t*) fsm_head->data;
    printf("ttu:%s\n", r->what);
    
    void* pmsg = pack_msg(CACHE_QUERY_REQ, r->src_fsmid, 31, "How are you?");
    if(SM_OK != send_msg(pmsg)){
        perror("send msg failed");
    }
    //exit(1);
}

void proc_cache_query_resp(msg_t* data)
{
    fsm_msg_head* fsm_head = (fsm_msg_head*) data->data;
    req_t* r = (req_t*) fsm_head->data;
    printf("ttu:%s\n", r->what);

    exit(1);
}

void process_ttu_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    fsm_msg_head* fsm_head = (fsm_msg_head*) pmsg->data;
    switch(fsm_head->msgtype){
        case CACHE_RESP: 
            proc_cache_resp(pmsg);
            break;
        case CACHE_QUERY_RESP:
            proc_cache_query_resp(pmsg);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}

