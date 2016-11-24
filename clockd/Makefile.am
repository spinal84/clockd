#
# Generic options
#
AM_CFLAGS = -Wall -Wmissing-prototypes -std=c99 -Os -g -fPIC
AM_CPPFLAGS = -I$(top_srcdir)/modules -D_GNU_SOURCE
AM_LDFLAGS = -g -Wl,--as-needed

ACLOCAL_AMFLAGS = -I m4

#
# Additional dirs
#
clockdincludedir = @includedir@/clockd

#
# Build targets
#
bin_PROGRAMS = clockd
lib_LTLIBRARIES = libtime.la
lib_LIBRARIES = libtime.a

libtime_a_SOURCES = libtime.c codec.c
libtime_a_CFLAGS = $(DBUS_CFLAGS) -DMESTR="\"$(PACKAGE_NAME):\""

clockd_SOURCES = sighnd.c clockd.c mainloop.c internal_time_utils.c mcc_tz_utils.c logging.c server.c
clockd_CFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(CITYINFO_CFLAGS) -DMESTR="\"$(PACKAGE_NAME):\""
clockd_LDADD = $(DBUS_LIBS) $(GLIB_LIBS) $(CITYINFO_LIBS) libtime.a

libtime_la_SOURCES = libtime.c
libtime_la_CFLAGS = $(DBUS_CFLAGS)
libtime_la_LIBADD = $(DBUS_LIBS)
libtime_la_LDFLAGS = $(AM_LDFLAGS) -pthread --shared

clockdinclude_HEADERS = libtime.h

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = libtime.pc

#iphbdconfdefdir=$(sysconfdir)/default
#iphbdconfdef_DATA = ../etc/default/iphbd
#iphbdconfdbusdir=$(sysconfdir)/dbus-1/system.d
#iphbdconfdbus_DATA = ../etc/dbus-1/system.d/iphbd.conf
#iphbdconfudevdir=$(sysconfdir)/udev/rules.d
#iphbdconfudev_DATA = ../etc/udev/rules.d/95-iphb.rules
#iphbdconfinitddir=$(sysconfdir)/init.d
#iphbdconfinitd_DATA = ../etc/init.d/iphbd
#iphbdconfeventddir=$(sysconfdir)/event.d
#iphbdconfeventd_DATA = ../etc/event.d/iphb
