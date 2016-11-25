#ifndef MCC_TZ_UTILS_H
#define MCC_TZ_UTILS_H

void mcc_tz_handle_registration_status_reply(struct DBusMessage *msg);
void mcc_tz_add_registration_change_match(void);
void mcc_tz_utils_quit(void);
int mcc_tz_is_tz_name_in_country_tz_list(const char *tz_name);
void mcc_tz_find_tz_in_country_tz_list(struct tm *tm, int dst, int gmtoff, char **tz_name);
int mcc_tz_utils_init(DBusConnection *server_system_bus,
                      int (*server_get_autosync_enabled)(void),
                      int (*server_handle_csd_net_time_change)(DBusMessage *),
                      void (*server_set_operator_tz)(const char *));
#endif // MCC_TZ_UTILS_H
