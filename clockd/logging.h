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

#define DO_LOG_STR_SLIST(__level, __slist) \
{ \
  if(__level == LOG_DEBUG && !clockd_debug_mode) ; \
  else { \
    if(!__slist) \
    { \
      DO_LOG(__level, "GSList empty"); \
    } \
    else \
    { \
      GSList *iter = __slist; \
      DO_LOG(__level, "GSList count = %d", g_slist_length (__slist)); \
      while(iter) \
      { \
        DO_LOG(__level, "%s", (char*)iter->data); \
        iter = g_slist_next(iter); \
      } \
    } \
  } \
}

extern bool clockd_debug_mode;

void log_tm(const char *tag, const struct tm *tm);
void dump_date(const char *server_tz);

#endif // LOGGING_H
