#ifndef CODEC_H
#define CODEC_H

#include <time.h>
#include <dbus/dbus.h>

int decode_int (DBusMessageIter *iter, int *pval);
int decode_tm (DBusMessageIter *iter, struct tm *tm);
int decode_ctm (DBusMessageIter *iter, struct tm *tm);
int encode_tm(DBusMessageIter *iter, struct tm *tm);
int encode_int(DBusMessageIter *iter, int *pval);

#endif // CODEC_H
