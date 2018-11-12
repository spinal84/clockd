#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "logging.h"
#include "internal_time_utils.h"

/* 1st of January, 1st of July and 31st of December */
static int days[3] = {1, 1, 31};
static int months[3] = {0, 6, 11};

int
internal_set_tz(const char *tz)
{
  int rv;
  char buf[512];

  snprintf(buf, sizeof(buf), "/usr/bin/rclockd clockd %s", tz);

  rv = system(buf);

  if (rv)
  {
    DO_LOG(LOG_ERR, "set_tz(), system(%s) failed (st=%d/%s)", buf, rv,
           rv == -1 ? strerror(errno) : "");
  }

  return rv;
}

void
internal_tz_set(char **old, const char *tz)
{
  char *tzname = getenv("TZ");

  free(*old);

  if (tzname)
    tzname = strdup(tzname);

  *old = tzname;

  internal_setenv_tz(tz);
}

void
internal_tz_res(char **old)
{
  if (*old)
    internal_setenv_tz(*old);
  else
  {
    unsetenv("TZ");
    tzset();
  }

  free(*old);
  *old = NULL;
}

int
internal_set_time(time_t t)
{
  int st;
  char buf[512];

  snprintf(buf, 512u, "/usr/bin/rclockd clockd %lu", t);
  st = system(buf);

  if (st)
  {
    DO_LOG(LOG_ERR, "internal_set_time(), system(%s) failed (st=%d/%s)", buf,
           st, st == -1 ? strerror(errno) : "");
  }
  else
  {
    time_t now = time(0);

    if (abs(now - t) > 2)
    {
      DO_LOG(LOG_ERR,
             "internal_set_time(), difference with intended and actual time is %ld seconds!",
             (long)t - now);
    }
  }
  return st;
}

int
internal_check_timezone(const char *zone)
{
  int i = 0;
  char c;

  while (true)
  {
    c = zone[i++];

    if (!c)
      break;

    if (isdigit(c) || c == '+' || c == '-' || c == ',')
      break;

    if (i == 3)
      return 0;
  }

  DO_LOG(LOG_ERR, "invalid time zone '%s", zone);

  return -1;
}

int
internal_setenv_tz(const char *tzname)
{
  int rv;
  char buf[256] = {0, };

  memcpy(buf, "UTC", 4);

  if (tzname)
  {
    if (tzname[0] != ':' && !isalpha(tzname[0]))
      snprintf(buf, sizeof(buf), ":%s", tzname + 1);
  }

  tzname = buf;

  rv = setenv("TZ", tzname, 1);
  if ( !rv )
    tzset();

  return rv;
}

time_t
internal_mktime_in(struct tm *tm, const char *tz)
{
  time_t tick;
  char *old_tz = NULL;

  internal_tz_set(&old_tz, tz);
  tick = mktime(tm);
  internal_tz_res(&old_tz);

  return tick;
}

struct tm *
internal_localtime_r_in(struct tm *utc_tm, struct tm *result, const char *tz)
{
  time_t tick  = internal_mktime_in(utc_tm, 0);
  char *old_tz = NULL;

  if (tick == -1)
    return NULL;
  else
  {
    internal_tz_set(&old_tz, tz);
    localtime_r(&tick, result);
    internal_tz_res(&old_tz);
  }

  return result;
}

int
internal_tz_cmp(const char *firstTZName, const char *secondTZName)
{
  int rv;
  signed int i;
  struct tm tm1;
  struct tm tm2;
  struct tm tm3;
  struct tm tm4;

  memset(&tm1, 0, sizeof(tm1));
  memset(&tm3, 0, sizeof(tm3));
  memset(&tm4, 0, sizeof(tm4));

  if (!(rv = strcmp(firstTZName, secondTZName)))
    return 0;

  if (firstTZName && *firstTZName && secondTZName && *secondTZName )
  {
    time_t tick = internal_get_time();

    gmtime_r(&tick, &tm1);
    rv = 0;

    for (i = 0; !rv && i < 3; i++)
    {
      memset(&tm2, 0, sizeof(tm2));
      tm2.tm_mday = days[i];
      tm2.tm_mon = months[i];
      tm2.tm_year = tm1.tm_year;
      internal_localtime_r_in(&tm2, &tm3, firstTZName);
      internal_localtime_r_in(&tm2, &tm4, secondTZName);
      rv = strcmp(tm3.tm_zone, tm4.tm_zone);
    }
  }

  return rv;
}

int
internal_get_dst(time_t tick)
{
  struct tm tm;

  memset(&tm, 0, sizeof(tm));

  if (!tick)
    tick = internal_get_time();

  localtime_r(&tick, &tm);

  return tm.tm_isdst > 0;
}

time_t
internal_get_time (void)
{
  return time(0);
}

int
internal_get_utc_offset(time_t timer, int dst)
{
  int rv = timezone;

  if (dst)
  {
    if (internal_get_dst(timer))
      rv -= 3600;
  }

  return rv;
}
