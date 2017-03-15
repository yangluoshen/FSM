
#include "debug.h"
#include <stdio.h>

void fsm_assert(const char* estr, const char* file, int line)
{
    clog_error(file, line, G_LOGGER, estr); 
    *((char*)-1) = 'x'; /*make a core*/
}
