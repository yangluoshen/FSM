#include "client_config.h"
#include "client_base.h"

const module_t ME_MDL = DVU;  /* the module type you want */

void process_yau_req(void* pmsg);

msg_driver_node g_msg_driver[] = 
{
    {YAU, process_yau_req}

};
const size_t FSM_DRIVER_SZ = sizeof(g_msg_driver)/sizeof(msg_driver_node);

fsm_reg g_fsm_reg_table[] = 
{
    {YAU_DVU_CHAT_REQ, NULL, NULL}

};

/*FSM_EPOLL_BLOCK by default */
const int FSM_CLIENT_EPOLL_TIMEOUT = FSM_EPOLL_BLOCK;

size_t get_driver_size(void)
{
    return FSM_DRIVER_SZ;
}

const msg_driver_node* get_driver_node(size_t index)
{
    if (index < FSM_DRIVER_SZ)
        return & g_msg_driver[index];

    return NULL;
}

fsm_reg* get_reginfo_by_msgtype(msg_t type)
{
    int i;
    size_t s = sizeof(g_fsm_reg_table)/sizeof(fsm_reg);
    for (i = 0; i < s; ++i){
        if (type == g_fsm_reg_table[i].type)
            return & g_fsm_reg_table[i];
    } 
    return NULL;
}


