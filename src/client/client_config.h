#ifndef __CLIENT_CONFIG_H
#define __CLIENT_CONFIG_H

#include "fsm.h"
#include "client_base.h"
#include "ttu_yau_msg.h"

enum ENUM_FSM_EPPOL_TIMEOUT{
    FSM_EPOLL_BLOCK = -1,
    FSM_EPOLL_NONBLOCK = 0,
    FSM_EPOLL_WAIT_SECONDS = 2 * 1000 /* 2 seconds polling */
    /* any new define below please*/
    
};

size_t get_driver_size(void);
const msg_driver_node* get_driver_node(size_t index);
fsm_reg* get_reginfo_by_msgtype(int type);

#endif /*__CLIENT_CONFIG_H */
