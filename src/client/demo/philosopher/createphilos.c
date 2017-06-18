#include "main.h"
#include "philos_msg.h"
#include "fsm.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static philosopher philos_pool[] = 
{
    {0,THINKING,3,5,"Allen"},
    {1,THINKING,5,5,"Fizz"},
    {2,THINKING,5,3,"Maria"},
    {3,THINKING,5,4,"Mike"},
    {4,THINKING,3,4,"Eric"},
    {5,THINKING,6,3,"Alice"},
    {6,THINKING,2,4,"Kang"},
    {7,THINKING,5,2,"Dive"}
};

msg_t* pack_msg(int idx)
{
    size_t data_len = FSM_MSG_HEAD_LEN + sizeof(philosopher);
    size_t msg_len = data_len + MSG_HEAD_LEN;

    msg_t* m = (msg_t*) malloc(msg_len);
    if (!m) return NULL;
    INIT_MSG_HEAD(m, getpid(), -1, CRU, PHU, data_len);
    SET_MSGTYPE(m, PHILOS_CREATE_REQ);
    SET_FSMID(m, -1);
    
    philosopher* philos = (philosopher*) GET_DATA(m);
    memcpy(philos, &philos_pool[idx], sizeof (philosopher));

    return m;
}

int main(int argc, char* argv[])
{
    if (argc != 2){
        printf("Usage: %s [num(2-8)]\n", argv[0]);
        exit(0);
    }
    int num = atoi(argv[1]);
    if (num < 2 || num > 8){
        printf("Usage: %s [num(2-8)]\n", argv[0]);
        exit(0);
    }
    if (-1 == client_login(CRU)) {
        printf("login failed\n");
        exit(1);
    }

    sleep(2);

    int i;
    for (i=0; i < num; ++i){
        msg_t* m = pack_msg(i);
        if (SM_OK != send_msg(m)){
            perror("sendmsg");
            printf("send msg failed\n");
            exit(1);
        }
        printf("%d:send %s\n", i, philos_pool[i].name);
        sleep(1);
    }

    printf("Done.\n");
    return 1;
}
