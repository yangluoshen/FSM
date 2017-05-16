#include "fsm_base.h"
#include "ttu_yau_msg.h"
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

int send_msg(void*);

void* pack_msg()
{
    // pack message
    char content[] = "nice to see you!";
    size_t data_len = sizeof(msg_type_t) + strlen(content) + 1;
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    assert(req_buf);

    msg_t* pmsg = (msg_t*)req_buf; 
    pmsg->s_pid = getpid();
    pmsg->r_pid = -1;
    pmsg->s_mdl = YAU;
    pmsg->r_mdl = TTU;
    pmsg->data_len = data_len;
    
    req_t* preq = (req_t*)pmsg->data;
    preq->msg_type = CACHE_REQ;
    memcpy(preq->what, content, strlen(content) + 1);
    
    return req_buf;
}

void say_hello_to_ttu()
{
    void * pmsg = pack_msg();
    if(SM_OK != send_msg(pmsg)){
        perror("send msg failed");
    }
}


void ttu_chat(msg_t* data)
{
    printf("ttu:%s\n", ((req_t*)(data->data))->what);
    //chat_yau_resp(data->s_pid, data->s_mdl);
    exit(1);
}

void process_ttu_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    req_t* preq = (req_t*) pmsg->data;
    switch(preq->msg_type){
        case TTU_YAU_CHAT_REQ: 
            ttu_chat(pmsg);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}
