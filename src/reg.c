#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "msg.h"

int main(int argc, char* argv[])
{
    int  sv_fd;

    umask(0);
    size_t mdl = 2;
    if (argc > 1){
        mdl = atoi(argv[1]);
    }
    prcs_reg reg;
    reg.cmd = PRCS_REG;
    reg.pid = getpid();
    reg.mdl = mdl;

    sv_fd = open(SV_REG_FIFO, O_WRONLY);
    if (-1 == sv_fd){
        perror("open sv fifo failed");
        return -1;
    }
    
    if (write(sv_fd, &reg, sizeof(reg)) != sizeof(reg)){
        perror("client: write error");
        return -1;
    }

    sleep(5);
    reg.cmd = PRCS_UNREG;

    if (write(sv_fd, &reg, sizeof(reg)) != sizeof(reg)){
        perror("client: write error");
        return -1;
    }
    
    close(sv_fd);

    return 0;
}
