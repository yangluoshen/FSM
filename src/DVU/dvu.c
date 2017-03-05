
#include "fsm.h"
#include "dvu_yau_msg.h"

void yau_chat(void* data)
{
    printf("yau:%s\n", ((req_t*)data)->what);

}

void process_yau_req(void* data)
{
    if (!data) return;
    req_t* preq = (req_t*) data;
    switch(preq->msg_type){
        case YAU_DVU_CHAT_REQ: 
            yau_chat(data);
            break;
        default:
            puts("unknown msg type");
            break;
    }
}

#if 0
void pack_msg()
{
    // pack message
    size_t data_len = sizeof(msg_type_t) + strlen(content) + 1;
    size_t msg_len = MSG_HEAD_LEN + data_len;
    char* req_buf = (char*) malloc(msg_len);
    assert(req_buf);

    msg_t* pmsg = (msg_t*)req_buf; 
    pmsg->s_pid = getpid();
    pmsg->r_pid = pmsg->s_pid;
    pmsg->s_mdl = ME_MDL;
    pmsg->r_mdl = DVU;
    pmsg->data_len = data_len;
    
    req_t* preq = (req_t*)pmsg->data;
    preq->msg_type = 200;
    memcpy(preq->what, content, strlen(content) + 1);

    print_req_msg(pmsg, preq); puts("");

    /* fsm_send_msg is universal interfase,
     * which allow client send msg to serve*/
    if(SM_OK != fsm_send_msg(pmsg)){
        perror("send msg failed");
        return -1;
    }

}
#endif
