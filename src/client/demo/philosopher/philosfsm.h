#ifndef __PHILOS_FSM_H
#define __PHILOS_FSM_H

#include "fsm.h"
#include "philos_msg.h"

#define PHILOS_FSM_ENTITY \
    FSM_ENTITY_BASE \
    int chops[2]; \
    philosopher* philos;

typedef struct philosfsm{
    PHILOS_FSM_ENTITY
}philos_fsm;

enum TIMER_TYPE{
    THINK_TM = 5,
    EAT_TM,
    QUERY_CHOP_TM,
};

void philos_fsm_constructor(void* entity, fsm_t fsmid);
void* philos_fsm_create();

#define CVTTO_PHILOS(phi, entity) \
    philos_fsm* (phi) = (philos_fsm*) (entity);


#endif
