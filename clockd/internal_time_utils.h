#ifndef INTERNAL_TIME_UTILS_H
#define INTERNAL_TIME_UTILS_H

int internal_set_tz(const char *tz);
void internal_tz_set(char **old, const char *tz);
void internal_tz_res(char **old);
int internal_set_time(time_t t);
int internal_check_timezone(const char *zone);
int internal_setenv_tz(const char *tzname);
time_t internal_mktime_in(struct tm *tm, const char *tz);
struct tm *internal_localtime_r_in(struct tm *utc_tm, struct tm *result, const char *tz);
int internal_tz_cmp(const char *firstTZName, const char *secondTZName);
int internal_get_dst(time_t tick);
int internal_get_utc_offset (time_t tick, int dst);
time_t internal_get_time (void);

#endif // INTERNAL_TIME_UTILS_H
