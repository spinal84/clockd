#include <string.h>
#include <stdio.h>

#include "codec.h"
#include "logging.h"

int
encode_int(DBusMessageIter *iter, int *pval)
{
  dbus_int32_t dbus_val = *pval;

  if (dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &dbus_val))
    return 0;

  return -1;
}

int
decode_int(DBusMessageIter *iter, int *pval)
{
  int type;
  int rv = -1;
  dbus_int32_t dbus_val = 0;

  type = dbus_message_iter_get_arg_type(iter);

  if (type == DBUS_TYPE_INT32)
  {
    dbus_message_iter_get_basic(iter, &dbus_val);
    dbus_message_iter_next(iter);
    rv = 0;
  }

  *pval = dbus_val;

  return rv;
}

int
encode_tm(DBusMessageIter *iter, struct tm *tm)
{
  int rv = 0;

  if (encode_int(iter, &tm->tm_sec) ||
      encode_int(iter, &tm->tm_min) ||
      encode_int(iter, &tm->tm_hour) ||
      encode_int(iter, &tm->tm_mday) ||
      encode_int(iter, &tm->tm_mon) ||
      encode_int(iter, &tm->tm_year) ||
      encode_int(iter, &tm->tm_wday) ||
      encode_int(iter, &tm->tm_yday) ||
      encode_int(iter, &tm->tm_isdst))
  {
    rv = -1;
  }

  return rv;
}

int
decode_tm(DBusMessageIter *iter, struct tm *tm)
{
  int rv = 0;

  if (decode_int(iter, &tm->tm_sec) ||
      decode_int(iter, &tm->tm_min) ||
      decode_int(iter, &tm->tm_hour) ||
      decode_int(iter, &tm->tm_mday) ||
      decode_int(iter, &tm->tm_mon) ||
      decode_int(iter, &tm->tm_year) ||
      decode_int(iter, &tm->tm_wday) ||
      decode_int(iter, &tm->tm_yday) ||
      decode_int(iter, &tm->tm_isdst))
  {
    rv = -1;
  }

  return rv;
}

int
decode_ctm(DBusMessageIter *iter, struct tm *tm)
{
  bool invalid;
  int is_dst;
  int tz;

  memset(tm, 0, sizeof(*tm));

  if (decode_int(iter, &tm->tm_year) ||
      decode_int(iter, &tm->tm_mon) ||
      decode_int(iter, &tm->tm_mday) ||
      decode_int(iter, &tm->tm_hour) ||
      decode_int(iter, &tm->tm_min) ||
      decode_int(iter, &tm->tm_sec) ||
      decode_int(iter, &tm->tm_yday) ||
      decode_int(iter, &tm->tm_isdst))
  {
    return -1;
  }

  DO_LOG(LOG_DEBUG, "network time %d.%d.%d %d:%d:%d tz=%d isdst=%d",
         tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
         tm->tm_sec, tm->tm_yday, tm->tm_isdst);

  if (tm->tm_year == 100 || tm->tm_mon == 100 || tm->tm_mday == 100 ||
      tm->tm_hour == 100 || tm->tm_min == 100 || tm->tm_sec == 100)
  {
    invalid = 1;
  }

  if (invalid && tm->tm_yday == 100 && tm->tm_isdst == 100)
  {
    DO_LOG(LOG_DEBUG, "operator does not support network time");
    return -1;
  }

  if (invalid)
  {
    time_t tick = time(0);
    struct tm tp = {0, };

    gmtime_r(&tick, &tp);
    tm->tm_year = tp.tm_year;
    tm->tm_mon = tp.tm_mon;
    tm->tm_mday = tp.tm_mday;
    tm->tm_hour = tp.tm_hour;
    tm->tm_min = tp.tm_min;
    tm->tm_sec = tp.tm_sec;

    DO_LOG(LOG_DEBUG, "ignoring invalid time stamp, using current time");
  }
  else
  {
    tm->tm_year = tm->tm_year + 100;
    tm->tm_mon = tm->tm_mon - 1;
  }

  if (tm->tm_isdst == 100 || tm->tm_isdst < 0 || tm->tm_isdst > 2)
    is_dst = 100;
  else
    is_dst = tm->tm_isdst;

  if (tm->tm_yday == 100)
    tz = 100;
  else
  {
    tz = tm->tm_yday & 0x3F;

    if (tm->tm_yday & 0x80)
      tz = -tz;
  }

  tm->tm_isdst = is_dst;
  tm->tm_yday = tz;

  DO_LOG(LOG_DEBUG, "network time fixed %d.%d.%d %d:%d:%d tz=%d isdst=%d",
         tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
         tm->tm_sec, tm->tm_yday, tm->tm_isdst);

  return 0;
}
