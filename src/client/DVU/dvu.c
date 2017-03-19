
#include "fsm_base.h"
#include "dvu_yau_msg.h"
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "client_base.h"
#include "debug.h"


void process_yau_req(void* pmsg);
void chat_yau_resp(pid_t r_pid, module_t r_mdl);


void yau_chat(msg_t* data)
{
    printf("yau:%s\n", ((req_t*)(data->data))->what);
    LOG_D("yau say hello to me:%s", ((req_t*)(data->data))->what);
    chat_yau_resp(data->s_pid, data->s_mdl);
}

void process_yau_req(void* data)
{
    if (!data) return;
    msg_t* pmsg = (msg_t*) data;
    req_t* preq = (req_t*) pmsg->data;
    switch(preq->msg_type){
        case YAU_DVU_CHAT_REQ: 
            yau_chat(pmsg);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}

void chat_yau_resp(pid_t r_pid, module_t r_mdl)
{
    char content[] = "Nice to see you, too.";
    size_t data_len = sizeof(msg_type_t) + sizeof(content);
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    if (!req_buf) return ;

    msg_t* pmsg = (msg_t*) req_buf;
    pmsg->s_pid = getpid();
    pmsg->r_pid = r_pid;
    pmsg->s_mdl = DVU;
    pmsg->r_mdl = r_mdl;
    pmsg->data_len = data_len;

    req_t* preq = (req_t*) pmsg->data;
    preq->msg_type = DVU_YAU_CHAT_REQ;
    memcpy(preq->what, content, sizeof(content));

    if (SM_OK != send_msg(pmsg)){
        perror("send msg failed");
        return ;
    }
    LOG_ND("say hello to yau");
}

