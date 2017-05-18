#ifndef __CLIENT_BASE_H
#define __CLIENT_BASE_H
#include "fsm_base.h"

typedef void (*drive_func)(void*);

typedef struct{
    module_t mdl;
    drive_func func;

}msg_driver_node;

typedef struct {
    int timerfd;
    int timerid;
    fsm_t fsmid;

}fsm_timer;

enum TIMERTYPE{
    NO_LOOP = 0,
    LOOP = 1
};

size_t get_driver_size(void);
const msg_driver_node* get_driver_node(size_t index);
int start_timer(int timerid, fsm_t fsmid, time_t seconds);
void stop_timer(int timerfd);

int send_msg(void* m);

#define TIMEOUT_MSG (5000)
#endif 
