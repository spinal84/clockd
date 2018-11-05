#ifndef LOGGING_H
#define LOGGING_H

#include <syslog.h>
#include <stdbool.h>

#define DO_LOG(__level, ...) \
do {                                       \
  if (__level == LOG_DEBUG && !clockd_debug_mode) ;             \
  else {                                                        \
    if (clockd_debug_mode) {                                    \
      printf(MESTR __VA_ARGS__); putchar('\n'); fflush(stdout); \
    }                                                           \
    syslog(__level, __VA_ARGS__);                               \
  }                                                             \
} while(0)

extern bool clockd_debug_mode;

void log_tm(const char *tag, const struct tm *tm);
void dump_date(const char *server_tz);

#endif // LOGGING_H
