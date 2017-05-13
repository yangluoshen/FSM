#include "fsm_base.h"

#define MAX_CONTENT_LEN 1024
typedef struct{
    msg_type_t msg_type;
    char what[MAX_CONTENT_LEN];
}req_t;

typedef struct{
    msg_type_t msg_type;
    error_t err_code;
}resp_t;
#define RESP_LEN (MSG_HEAD_LEN + sizeof(resp_t))

enum MSG_TYPE{
    TTU_YAU_CHAT_REQ = 0,
    YAU_TTU_CHAT_REQ,

    CACHE_REQ,
    CACHE_RESP,

    MSG_TYPE_BUTT
};
