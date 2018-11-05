#ifndef MCC_TZ_UTILS_H
#define MCC_TZ_UTILS_H

typedef int (*GetAutosyncEnabled)();
typedef int (*HandleCsdNetTimeChange)(DBusMessage*);
typedef void (*SetOperatorTz)(const char*);

void mcc_tz_handle_registration_status_reply(struct DBusMessage *msg);
void mcc_tz_setup_timezone_from_mcc_if_required(void);
void mcc_tz_utils_quit(void);
int mcc_tz_is_tz_name_in_country_tz_list(const char *tz_name);

void mcc_tz_guess_tz_for_country_by_dst_and_offset(struct tm *tm,
                                                   int dst,
                                                   int gmtoff,
                                                   char **tz_name);

int mcc_tz_utils_init(DBusConnection *server_system_bus,
                      GetAutosyncEnabled server_get_autosync_enabled,
                      HandleCsdNetTimeChange server_handle_csd_net_time_change,
                      SetOperatorTz server_set_operator_tz);
#endif // MCC_TZ_UTILS_H
