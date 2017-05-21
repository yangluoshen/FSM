#include "client_config.h"
#include "main.h"
#include "cachefsm.h"
#include "ttu_yau_msg.h"

const module_t ME_MDL = TTU;  /* the module type you want */

void process_yau_req(void* pmsg);
void proc_ttu_internal_msg(void* data);

// 模块消息路由表. 根据不同模块路由给不同的处理函数入口
msg_driver_node g_msg_driver[] = 
{
    {YAU, process_yau_req},
    {TTU, proc_ttu_internal_msg}

};

const size_t FSM_DRIVER_SZ = sizeof(g_msg_driver)/sizeof(msg_driver_node);


// 自动机注册表.可根据不同的消息类型创建指定类型的自动机
fsm_reg g_fsm_reg_table[] = 
{
    {CACHE_REQ, cache_fsm_constructor, cache_fsm_create}

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
