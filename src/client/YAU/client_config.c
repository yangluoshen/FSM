#include "client_base.h"

const module_t ME_MDL = YAU;  /* the module type you want */


void process_dvu_req(void* pmsg);
msg_driver_node g_msg_driver[] = 
{
    {DVU, process_dvu_req}

};
const size_t FSM_DRIVER_SZ = sizeof(g_msg_driver)/sizeof(msg_driver_node);

enum ENUM_FSM_EPOL_TIMEOUT{
    FSM_EPOLL_BLOCK = -1,
    FSM_EPOLL_NONBLOCK = 0,
    FSM_EPOLL_WAIT_SECONDS = 2 * 1000 /* 2 seconds poling */
    /* any new define below please*/
    
};

/*FSM_EPOLL_BLOCK by default */
const int FSM_CLIENT_EPOLL_TIMEOUT = FSM_EPOLL_WAIT_SECONDS;

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

