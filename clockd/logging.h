#ifndef LOGGING_H
#define LOGGING_H

#include <syslog.h>

#define DO_LOG(__level, ...) \
{                                       \
  if (__level == LOG_DEBUG && !clockd_debug_mode) ;             \
  else {                                                        \
    if (clockd_debug_mode) {                                    \
      printf(MESTR __VA_ARGS__); printf("\n"); fflush(stdout);  \
    }                                                           \
    syslog(__level, __VA_ARGS__);                               \
  }                                                             \
}

#endif // LOGGING_H
