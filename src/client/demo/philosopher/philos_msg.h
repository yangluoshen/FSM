#ifndef __PHILOS_MSG_H
#define __PHILOS_MSG_H


typedef struct philos_create_req{
    int think_time;
    int eat_time;
    int whoami;
}philos_create_req;

typedef struct philos_chop_req{
    int whoami;
    int querywhat;

}philos_chop_req;

typedef struct philos_chop_resp{
    int querywhat;
    int isok;
}philos_chop_resp;

enum PHILOS_STATUS{
    THINKING = 0,
    EATTING,
    BUSY
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

#define QUERY_CHOPSTICK_TIMEOUT (2) // 2 seconds

#endif
