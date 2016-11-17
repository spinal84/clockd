#include "codec.h"

int decode_int (DBusMessageIter *iter, int *pval);
int decode_tm (DBusMessageIter *iter, struct tm *tm);
int decode_ctm (DBusMessageIter *iter, struct tm *tm);
