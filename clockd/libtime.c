#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "libtime.h"
#include "codec.h"
#include <dbus/dbus.h>
#include "clock_dbus.h"
#include <pthread.h>
#include <semaphore.h>

static bool s_inited = false;
static bool s_autosync_enabled = false;
static bool s_operator_time_available = false;
static sem_t sem_time;
static DBusConnection *clockd_conn = NULL;
static char s_tz[CLOCKD_TZ_SIZE] = {0, };
static char s_default_tz[CLOCKD_TZ_SIZE] = {0, };
static char s_time_format[CLOCKD_GET_TIMEFMT_SIZE] = {0, };

#define TIME_TRY_INIT_SYNC(__ret__) \
do { \
  sem_wait(&sem_time); \
 \
  if (!s_inited) \
  { \
    if (get_synced()) \
    { \
      TIME_EXIT_SYNC; \
      return __ret__; \
    } \
 \
    s_inited = true; \
  } \
} while(0)

#define TIME_EXIT_SYNC sem_post(&sem_time)
#define TIME_INIT_ERROR "libtime_init() error\n"

__attribute__((constructor)) static void
libtime_init()
{
  if (sem_init(&sem_time, 0, 1))
    if (write(STDERR_FILENO, TIME_INIT_ERROR, strlen(TIME_INIT_ERROR))) {}
}

__attribute__((destructor)) static void
libtime_fini()
{
  if (clockd_conn)
  {
    dbus_connection_close(clockd_conn);
    dbus_connection_unref(clockd_conn);
    clockd_conn = NULL;
  }

  sem_destroy(&sem_time);
}

static DBusMessage *
client_new_req(char *method, int first_arg_type, ...)
{
  DBusMessage *req;
  va_list va;

  va_start(va, first_arg_type);
  req = dbus_message_new_method_call(CLOCKD_SERVICE, CLOCKD_PATH,
                                     CLOCKD_INTERFACE, method);

  if (req)
  {
    if ( !dbus_message_append_args_valist(req, first_arg_type, va) )
      fprintf(stderr, "FAILED: %s\n", "dbus_message_append_args_valist");
  }
  else
    fprintf(stderr, "FAILED: %s\n", "dbus_message_new_method_call");

  return req;
}

static void
time_dbus_connection_close()
{
  if (clockd_conn)
  {
    dbus_connection_close(clockd_conn);
    dbus_connection_unref(clockd_conn);
    clockd_conn = NULL;
  }
}

DBusMessage *
client_get_rsp(DBusMessage *msg)
{
  DBusMessage *rsp = NULL;
  int i = 1;
  DBusError error = DBUS_ERROR_INIT;

  do
  {
    if (clockd_conn ||
        (clockd_conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error)))
    {
      rsp = dbus_connection_send_with_reply_and_block(clockd_conn, msg, -1,
                                                      &error);
      if (rsp)
        break;

      fprintf(stderr, "FAILED: %s\n",
              "dbus_connection_send_with_reply_and_block");
      fprintf(stderr, "->\t%s: %s\n", error.name, error.message);
    }
    else
    {
      fprintf(stderr, "FAILED: %s\n", "dbus_bus_get_private");
      fprintf(stderr, "->\t%s: %s\n", error.name, error.message);
    }

    time_dbus_connection_close();
  }
  while (i--);

  dbus_error_free(&error);

  return rsp;
}

static const char *
client_get_tz()
{
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *req = client_new_req(CLOCKD_GET_TZ, DBUS_TYPE_INVALID);
  const char *rv = NULL;

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      char *s = NULL;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_STRING, &s,
                                DBUS_TYPE_INVALID) && s)
      {
        snprintf(s_tz, sizeof(s_tz), "%s", s);

        if (s_tz[0] == '/')
          s_tz[0] = ':';

        rv = s_tz;

        if (*s)
        {
          setenv("TZ", s_tz, 1);
          tzset();
        }
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  return rv;
}

static const char *
client_get_time_format()
{
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *req = client_new_req(CLOCKD_GET_TIMEFMT, DBUS_TYPE_INVALID);
  const char *rv = NULL;

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      char *s = NULL;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_STRING, &s,
                                DBUS_TYPE_INVALID) && s && *s)
      {
        snprintf(s_time_format, sizeof(s_time_format), "%s", s);
        rv = s_time_format;
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  return rv;
}

static int
client_is_operator_time_accessible()
{
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *req = client_new_req(CLOCKD_HAVE_OPERTIME, DBUS_TYPE_INVALID);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      dbus_bool_t b = FALSE;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &b,
                                DBUS_TYPE_INVALID))
      {
        s_operator_time_available = (b != FALSE);
      }

      dbus_message_unref(rsp);
    }
    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  return s_operator_time_available;
}

static int
client_get_autosync()
{
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *req = client_new_req(CLOCKD_GET_AUTOSYNC, DBUS_TYPE_INVALID);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      dbus_bool_t b = FALSE;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &b,
                                DBUS_TYPE_INVALID))
      {
        s_autosync_enabled = (b != FALSE);
      }

      dbus_message_unref(rsp);
    }
    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  return s_autosync_enabled;
}

static const char*
client_get_default_tz()
{
  DBusMessage *req = client_new_req(CLOCKD_GET_DEFAULT_TZ, DBUS_TYPE_INVALID);
  DBusError error = DBUS_ERROR_INIT;
  const char *rv = NULL;

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      char *s = NULL;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_STRING, &s,
                                DBUS_TYPE_INVALID) && s && *s)
      {
        snprintf(s_default_tz, sizeof(s_default_tz), "%s", s);
        rv = s_default_tz;
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  return rv;
}

static int
get_synced()
{
  int rv = (client_get_tz() == NULL);

  client_get_time_format();
  client_is_operator_time_accessible();
  client_get_autosync();
  client_get_default_tz();

  return rv;
}

int
time_get_synced(void)
{
  int synced;

  sem_wait(&sem_time);
  synced = get_synced();
  sem_post(&sem_time);

  if (!synced)
    s_inited = true;

  return synced;
}

time_t
time_get_time(void)
{
  return time(NULL);
}

int
time_set_time(time_t tick)
{
  DBusMessage *msg;
  DBusError error = DBUS_ERROR_INIT;
  dbus_int32_t db_time = tick;

  TIME_TRY_INIT_SYNC(-1);

  msg = client_new_req(CLOCKD_SET_TIME, DBUS_TYPE_INT32, &db_time,
                       DBUS_TYPE_INVALID);
  if (msg)
  {
    DBusMessage *rsp = client_get_rsp(msg);

    if (rsp)
    {
      dbus_bool_t res = FALSE;

      dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &res,
                            DBUS_TYPE_INVALID);
      dbus_message_unref(rsp);
    }

    dbus_message_unref(msg);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return 0;
}

int
time_get_net_time(time_t *tick, char *s, size_t max)
{
  int rv = -1;
  DBusMessage *req;
  DBusError error = DBUS_ERROR_INIT;

  TIME_TRY_INIT_SYNC(-1);

  *tick = 0;

  req = client_new_req(CLOCKD_NET_TIME_CHANGED, DBUS_TYPE_INVALID);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);
    dbus_int32_t t;
    char *tz = NULL;

    if (rsp)
    {
      if (dbus_message_get_args(rsp, &error,
                                DBUS_TYPE_INT32, &t,
                                DBUS_TYPE_STRING, &tz,
                                DBUS_TYPE_INVALID) && t)
      {
        *tick = t;

        if (tz)
        {
          snprintf(s, max, "%s", tz);
          rv = strlen(s);
        }
        else
          rv = 1;
      }

      dbus_message_unref(rsp);
    }
    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_is_net_time_changed(time_t *tick, char *s, size_t max)
{
  return time_get_net_time(tick, s, max);
}

int
time_activate_net_time(void)
{
  DBusMessage *req;
  int rv = -1;
  DBusError error = DBUS_ERROR_INIT;
  dbus_bool_t success = false;

  TIME_TRY_INIT_SYNC(-1);

  req = client_new_req(CLOCKD_ACTIVATE_NET_TIME, DBUS_TYPE_INVALID);
  rv = 0;

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      if (dbus_message_get_args(rsp, &error,
                                DBUS_TYPE_BOOLEAN, &success,
                                DBUS_TYPE_INVALID) && success)
      {
        rv = 0;
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return rv;
}

time_t
time_mktime(struct tm *tm, const char *tz)
{
  time_t rv;

  TIME_TRY_INIT_SYNC(0);

  if (tz)
  {
    setenv("TZ", tz, 1);
    tzset();
  }

  rv = mktime(tm);

  if (tz)
  {
    setenv("TZ", s_tz, 1);
    tzset();
  }

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_timezone(char *s, size_t max)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  rv = snprintf(s, max, "%s", s_tz);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_tzname(char *s, size_t max)
{
  int rv = -1;
  struct tm tp;
  time_t t;

  TIME_TRY_INIT_SYNC(-1);

  memset(&tp, 0, sizeof(tp));
  t = time(0);

  if (localtime_r(&t, &tp))
    rv = snprintf(s, max, "%s", tzname[tp.tm_isdst ? 1 : 0]);

  TIME_EXIT_SYNC;

  return rv;
}

double
time_diff(time_t t1, time_t t2)
{
  return difftime(t1, t2);
}

int
time_set_timezone(const char *tz)
{
  DBusMessage *req;
  int rv = -1;
  DBusError error = DBUS_ERROR_INIT;

  TIME_TRY_INIT_SYNC(-1);

  req = client_new_req(CLOCKD_SET_TZ, 's', &tz, 0);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      dbus_bool_t success = false;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &success,
                                DBUS_TYPE_INVALID) && success)
      {
        snprintf(s_tz, sizeof(s_tz), "%s", tz);
        setenv("TZ", tz, 1);
        tzset();
        rv = 0;
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_utc(struct tm *tm)
{
  struct tm *tp;
  time_t timer;

  TIME_TRY_INIT_SYNC(-1);

  timer = time(0);
  tp = gmtime_r(&timer, tm);

  TIME_EXIT_SYNC;

  return tp ? 0 : -1;
}

int
time_get_utc_ex(time_t tick, struct tm *tm)
{
  struct tm *tp;

  TIME_TRY_INIT_SYNC(-1);

  tp = gmtime_r(&tick, tm);

  TIME_EXIT_SYNC;

  return tp ? 0 : -1;
}

int
time_get_local(struct tm *tm)
{
  struct tm *tp;
  time_t timer;

  TIME_TRY_INIT_SYNC(-1);

  timer = time(0);
  tp = localtime_r(&timer, tm);

  TIME_EXIT_SYNC;

  return tp ? 0 : -1;
}

int
time_get_local_ex(time_t tick, struct tm *tm)
{
  struct tm *tp;

  TIME_TRY_INIT_SYNC(-1);

  tp = localtime_r(&tick, tm);

  TIME_EXIT_SYNC;

  return tp ? 0 : -1;
}

int
time_get_remote(time_t tick, const char *tz, struct tm *tm)
{
  int rv = -1;

  TIME_TRY_INIT_SYNC(-1);

  setenv("TZ", tz, 1);
  tzset();

  if (localtime_r(&tick, tm))
    rv = 0;

  setenv("TZ", s_tz, 1);
  tzset();

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_default_timezone(char *s, size_t max)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  rv = snprintf(s, max, "%s", s_default_tz);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_time_format(char *s, size_t max)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  rv = snprintf(s, max, "%s", s_time_format);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_set_time_format(const char *fmt)
{
  DBusMessage *req;
  DBusError error = DBUS_ERROR_INIT;
  int rv = -1;

  TIME_TRY_INIT_SYNC(-1);

  req = client_new_req(CLOCKD_SET_TIMEFMT, DBUS_TYPE_STRING, &fmt,
                       DBUS_TYPE_INVALID);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      dbus_bool_t success = FALSE;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &success, 0) &&
          success)
      {
        snprintf(s_time_format, sizeof(s_time_format), "%s", fmt);
        rv = 0;
      }

      dbus_message_unref(rsp);
    }
    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_format_time(const struct tm *tm, const char *fmt, char *s, size_t max)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  if (!fmt)
    fmt = s_time_format;

  rv = strftime(s, max, fmt, tm);

  TIME_EXIT_SYNC;

  return rv;
}

static int
get_utc_offset(time_t tick)
{
  struct tm *lt;
  struct tm tp;
  int rv;

  memset(&tp, 0, sizeof(tp));
  lt = localtime_r(&tick, &tp);
  rv = -1;

  if (lt)
    rv = -tp.tm_gmtoff;

  return rv;
}

int
time_get_utc_offset(const char *tz)
{
  int rv;
  time_t tick;

  TIME_TRY_INIT_SYNC(-1);

  if (tz)
  {
    setenv("TZ", tz, 1);
    tzset();
  }

  tick = time(0);
  rv = get_utc_offset(tick);

  if (tz)
  {
    setenv("TZ", s_tz, 1);
    tzset();
  }

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_dst_usage(time_t tick, const char *tz)
{
  int rv = -1;
  int timediff;
  int gmt_off;
  struct tm tp;

  TIME_TRY_INIT_SYNC(-1);

  if (tz)
  {
    setenv("TZ", tz, 1);
    tzset();
  }

  memset(&tp, 0, sizeof(tp));

  if (localtime_r(&tick, &tp) && tp.tm_isdst > 0)
  {
    time_t t1, t2;

    t1 = mktime(&tp);
    tp.tm_isdst = 0;
    t2 = mktime(&tp);
    timediff = t2 - t1;

    if (!timediff)
    {
      gmt_off = tp.tm_gmtoff;
      tp.tm_mday = 31;
      tp.tm_mon = 11;
      t2 = mktime(&tp);
      memset(&tp, 0, sizeof(tp));
      timediff = gmt_off;

      if (localtime_r(&t2, &tp))
        timediff = gmt_off - tp.tm_gmtoff;
    }

    rv = !!timediff;
  }

  if (tz)
  {
    setenv("TZ", s_tz, 1);
    tzset();
  }

  TIME_EXIT_SYNC;

  return rv;
}

int
time_set_autosync(int enable)
{
  DBusMessage *req;
  DBusError error = DBUS_ERROR_INIT;
  dbus_bool_t dbenable = !!enable;
  int rv = -1;

  TIME_TRY_INIT_SYNC(-1);

  req = client_new_req(CLOCKD_SET_AUTOSYNC, DBUS_TYPE_BOOLEAN, &dbenable,
                       DBUS_TYPE_INVALID);

  if (req)
  {
    DBusMessage *rsp = client_get_rsp(req);

    if (rsp)
    {
      dbus_bool_t success = FALSE;

      if (dbus_message_get_args(rsp, &error, DBUS_TYPE_BOOLEAN, &success,
                                DBUS_TYPE_INVALID) && success)
      {
        rv = !!success;

        if (success)
          s_autosync_enabled = enable;
      }

      dbus_message_unref(rsp);
    }

    dbus_message_unref(req);
  }

  dbus_error_free(&error);

  TIME_EXIT_SYNC;

  return rv;
}

int
time_get_autosync(void)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  rv = s_autosync_enabled;

  TIME_EXIT_SYNC;

  return rv;
}

int time_is_operator_time_accessible(void)
{
  int rv;

  TIME_TRY_INIT_SYNC(-1);

  rv = s_operator_time_available;

  TIME_EXIT_SYNC;

  return rv;
}

static const char *
fix_tz(const char *tz, char *tz_fixed)
{
  if (tz[0] && isalpha(tz[0]) && tz[1] && isalpha(tz[1]))
  {
    if (tz[2] && (tz[2] == '-' || tz[2] == '+') && isdigit(tz[3]))
    {
      int offset = atoi(tz + 2);

      if (offset)
      {
        setenv("TZ", tz, 1);
        tzset();

        if (timezone != offset)
        {
          snprintf(tz_fixed, 24u, "GMT%s%d", offset < 0 ? "+" : "-",
                   abs(offset));
          tz = tz_fixed;
        }
      }
    }
  }

  return tz;
}

int
time_get_time_diff(time_t tick, const char *tz1, const char *tz2)
{
  const char *tz1_fixed, *tz2_fixed;
  time_t t1, t2;
  struct tm tp;
  char tz1_buf[24], tz2_buf[24];

  TIME_TRY_INIT_SYNC(0);

  tz1_fixed = fix_tz(tz1, tz1_buf);
  tz2_fixed = fix_tz(tz2, tz2_buf);

  setenv("TZ", tz1_fixed, 1);
  tzset();
  localtime_r(&tick, &tp);
  t1 = mktime(&tp) - get_utc_offset(tick);

  setenv("TZ", tz2_fixed, 1);
  tzset();
  localtime_r(&tick, &tp);
  t2 = mktime(&tp) - get_utc_offset(tick);

  setenv("TZ", s_tz, 1);
  tzset();

  TIME_EXIT_SYNC;

  return t1 - t2;
}
