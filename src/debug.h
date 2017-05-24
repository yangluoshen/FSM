#ifndef __DEBUG_H
#define __DEBUG_H

#include <unistd.h>

#include "clog.h"

extern const int G_LOGGER; /*define a G_LOGGER outside */
#define assert(_e)  ((_e) ? (void)0 : (fsm_assert(#_e, __FILE__, __LINE__,__func__),_exit(1)))

void fsm_assert(const char* esrt, const char* file, int line, const char* sfunc);

#define LOG_D(fmt, ...) clog_debug(CLOG(G_LOGGER), fmt, ##__VA_ARGS__); 
#define LOG_ND(fmt) LOG_D(fmt, NULL)
#define LOG_E(fmt, ...) clog_error(CLOG(G_LOGGER), fmt, ##__VA_ARGS__); 
#define LOG_NE(fmt) LOG_E(fmt, NULL) 

#endif
