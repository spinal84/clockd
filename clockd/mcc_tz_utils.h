#ifndef MCC_TZ_UTILS_H
#define MCC_TZ_UTILS_H

void mcc_tz_handle_registration_status_reply(struct DBusMessage *msg);
void mcc_tz_add_registration_change_match(void);
void mcc_tz_utils_quit(void);
int mcc_tz_is_tz_name_in_country_tz_list(const char *tz_name);
void mcc_tz_find_tz_in_country_tz_list(struct tm *tm, int dst, int gmtoff, char **tz_name);

#endif // MCC_TZ_UTILS_H
