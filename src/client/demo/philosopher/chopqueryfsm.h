#ifndef __CHOP_QUERY_FSM_H
#define __CHOP_QUERY_FSM_H

#include "fsm.h"
#include "philos_msg.h"

#define CHOP_QUERY_ENTITY \
    FSM_ENTITY_BASE 

typedef struct {
    CHOP_QUERY_ENTITY
}chop_query_fsm;

void chop_fsm_constructor(void* entity, fsm_t fsmid);
void* chop_fsm_create();


#define CVTTO_PHIQRY(phiqry, entity) \
    chop_query_fsm* (phiqry) = (chop_query_fsm*) (entity);


#endif
