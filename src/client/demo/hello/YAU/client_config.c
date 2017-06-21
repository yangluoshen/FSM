#include "client_config.h"
#include "main.h"
#include <unistd.h>

const module_t ME_MDL = YAU;  /* the module type you want */
const int ME_PORT = 5702;


void process_ttu_req(void* pmsg);
void stdin_handle(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

msg_driver_node g_msg_driver[] = 
{
    {TTU, process_ttu_req}

};
const size_t FSM_DRIVER_SZ = sizeof(g_msg_driver)/sizeof(msg_driver_node);


fsm_reg g_fsm_reg_table[] = 
{
    {0, NULL, NULL}

};

ev_driver_node cli_ev_list[] = 
{
    {STDIN_FILENO, AE_READABLE, stdin_handle}
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

// 根据对端消息类型获取注册表中对应的自动机生成器
fsm_reg* get_reginfo_by_msgtype(int type)
{
    int i;
    size_t s = sizeof(g_fsm_reg_table)/sizeof(fsm_reg);
    for (i = 0; i < s; ++i){
        if (type == g_fsm_reg_table[i].type)
            return & g_fsm_reg_table[i];
    } 
    return NULL;
}

size_t get_cli_ev_size(void)
{
    return sizeof (cli_ev_list) /sizeof(cli_ev_list[0]);
}

ev_driver_node* get_cli_ev(size_t i)
{
    size_t n = sizeof (cli_ev_list) /sizeof(cli_ev_list[0]);
    if (i < n)
        return &cli_ev_list[i];

    return NULL;
}

