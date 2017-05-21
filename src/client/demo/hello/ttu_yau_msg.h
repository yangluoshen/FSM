#include "fsm_base.h"

#define MAX_CONTENT_LEN 1024
typedef struct{
    fsm_t src_fsmid;
    char what[MAX_CONTENT_LEN];
}req_t;

typedef struct{
    error_t err_code;
}resp_t;
#define RESP_LEN (MSG_HEAD_LEN + sizeof(resp_t))

enum MSG_TYPE{
    TTU_YAU_CHAT_REQ = 0,
    YAU_TTU_CHAT_REQ,

    CACHE_REQ,
    CACHE_RESP,

    CACHE_QUERY_REQ,
    CACHE_QUERY_RESP,

    TIME_OUT_MSG,
    MSG_TYPE_BUTT
};


enum E_TIMER_ID{
    INVALID_TM = -1,
    IDLE,
    COMMON_TM,

    TM_BUTT
};
