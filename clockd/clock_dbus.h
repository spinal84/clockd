#ifndef CLOCK_DBUS_H
#define CLOCK_DBUS_H

#define 	CLOCKD_SET_TIME   "set_time"
#define 	CLOCKD_GET_TIME   "get_time"
#define 	CLOCKD_GET_TZ   "get_tz"
#define 	CLOCKD_GET_DEFAULT_TZ   "get_default_tz"
#define 	CLOCKD_SET_TZ   "set_tz"
#define 	CLOCKD_GET_TIMEFMT   "get_timefmt"
#define 	CLOCKD_SET_TIMEFMT   "set_timefmt"
#define 	CLOCKD_SET_AUTOSYNC   "set_autosync"
#define 	CLOCKD_GET_AUTOSYNC   "get_autosync"
#define 	CLOCKD_HAVE_OPERTIME   "have_opertime"
#define 	CLOCKD_ACTIVATE_NET_TIME   "activate_net_time"
#define 	CLOCKD_NET_TIME_CHANGED   "net_time_changed"
#define 	CSD_SERVICE   "com.nokia.phone.net"
#define 	CSD_PATH   "/com/nokia/phone/net"
#define 	CSD_INTERFACE   "Phone.Net"
#define 	CSD_SET_TIME   "network_time_info_change"
#define 	CSD_MATCH_RULE   "type='signal',interface='Phone.Net',member='network_time_info_change'"
#define 	CLOCKD_GET_TIMEFMT_SIZE   32
#define 	CLOCKD_TZ_SIZE   256

#endif // CLOCK_DBUS_H
