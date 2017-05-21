#ifndef __CACHE_H
#define __CACHE_H

#include "fsm.h"

typedef unsigned int cachekey_t;

#define CACHE_FSM_ENTITY \
    FSM_ENTITY_BASE \
    cachekey_t key; 
   

typedef struct {
    CACHE_FSM_ENTITY
}cache_fsm;


void cache_fsm_constructor(void* entity, fsm_t fsmid);
void* cache_fsm_create();

#define CVTTO_CACHE(cache_en, entity)\
    cache_fsm* (cache_en) = (cache_fsm*) (entity);
#endif

