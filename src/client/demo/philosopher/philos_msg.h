#ifndef __PHILOS_MSG_H
#define __PHILOS_MSG_H

#include "fdict.h"

#define MAX_NAME_LEN (30)
#define MAX_PHILOS_NUM (20)
#define MAX_CHOPSTICK_NUM (100)


typedef struct chop_req{
    int chop_idx;
    int dowhat;

}chop_req;

typedef struct chop_resp{
    int chop_idx;
    int isok;
}chop_resp;

typedef struct philosopher{
    int whoami;
    int status;
    int eat_time;
    int think_time;
    char name[MAX_NAME_LEN];
    
}philosopher;

enum PHILOS_STATUS{
    THINKING = 0,
    EATTING,
    BUSY
};

enum CHOP_DOWHAT{
    GET = 0,
    BACK
};

enum PHILOS_MSG{
    PHILOS_CREATE_REQ = 1,

    PHILOS_CHOP_REQ,
    PHILOS_CHOP_RESP,

};

enum CHOPSTICKS{
    LEFT_CHOP = 0,
    RIGHT_CHOP
};

enum CHOPSTICKS_STATUS{
    CHOP_IDLE = 0,
    CHOP_BUSY
};


#define QUERY_CHOPSTICK_TIMEOUT (2) // 2 seconds

int philos_hash_match(void* ptr, fdict_key_t key);
size_t philos_hash_calc(fdict* d, fdict_key_t key);

#endif
