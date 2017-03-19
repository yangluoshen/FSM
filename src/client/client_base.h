#ifndef __CLIENT_BASE_H
#define __CLIENT_BASE_H
#include "fsm_base.h"

typedef void (*drive_func)(void*);

typedef struct {
    int timerfd;
    fsm_t fsmid;

}fsm_timer;

typedef struct{
    module_t mdl;
    drive_func func;

}msg_driver_node;

enum TIMERTYPE{
    NO_LOOP = 0,
    LOOP = 1
};

size_t get_driver_size(void);
const msg_driver_node* get_driver_node(size_t index);

int send_msg(void* m);
int start_timer(fsm_t fsmid, time_t seconds);
void stop_timer(fsm_t fsmid);

#endif 
