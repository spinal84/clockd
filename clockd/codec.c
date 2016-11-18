#include "codec.h"

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
