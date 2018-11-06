#ifndef CLOCK_DBUS_H
#define CLOCK_DBUS_H

#define	CLOCKD_SET_TIME "set_time"
#define	CLOCKD_GET_TIME "get_time"
#define	CLOCKD_GET_TZ "get_tz"
#define	CLOCKD_SET_TZ "set_tz"
#define	CLOCKD_GET_TIMEFMT "get_timefmt"
#define	CLOCKD_GET_DEFAULT_TZ "get_default_tz"
#define	CLOCKD_SET_TIMEFMT "set_timefmt"
#define	CLOCKD_SET_AUTOSYNC "set_autosync"
#define	CLOCKD_GET_AUTOSYNC "get_autosync"
#define	CLOCKD_HAVE_OPERTIME "have_opertime"
#define	CLOCKD_ACTIVATE_NET_TIME "activate_net_time"
#define	CLOCKD_NET_TIME_CHANGED "net_time_changed"
#define	CSD_SERVICE "com.nokia.phone.net"
#define	CSD_PATH "/com/nokia/phone/net"
#define	CSD_INTERFACE "Phone.Net"
#define	CSD_NETWORK_TIMEINFO_CHANGE "network_time_info_change"
#define	CSD_REGISTRATION_STATUS_CHANGE "registration_status_change"
#define	CSD_GET_REGISTRATION_STATUS "get_registration_status"
#define	CSD_GET_NETWORK_TIMEINFO "get_network_time_info"
#define	CSD_TIMEINFO_CHANGE_MATCH_RULE \
  "type='signal',interface='Phone.Net',member='network_time_info_change'"
#define	CSD_REGISTRATION_CHANGE_MATCH_RULE \
  "type='signal',interface='Phone.Net',member='registration_status_change'"
#define	MCE_SERVICE "com.nokia.mce"
#define	MCE_PATH "/com/nokia/mce/signal"
#define	MCE_INTERFACE "com.nokia.mce.signal"
#define	MCE_MODE_CHANGE "sig_device_mode_ind"
#define	MCE_MATCH_RULE \
  "type='signal',interface='com.nokia.mce.signal',member='sig_device_mode_ind'"
#define	CLOCKD_GET_TIMEFMT_SIZE 32
#define	CLOCKD_TZ_SIZE 256

#ifndef DBUS_TIMEOUT_USE_DEFAULT
#define DBUS_TIMEOUT_USE_DEFAULT -1
#endif

#endif // CLOCK_DBUS_H
