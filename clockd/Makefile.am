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
#bin_PROGRAMS = iphbd
#noinst_PROGRAMS = hbtest
lib_LTLIBRARIES = libtime.la

#iphbd_SOURCES = iphbd.c
#iphbd_CFLAGS = $(DBUS_CFLAGS)
#iphbd_LDADD = $(DBUS_LIBS)

#hbtest_SOURCES = hbtest.c
#hbtest_LDADD = libiphb.la

libtime_la_SOURCES = libtime.c codec.c
libtime_la_CFLAGS = $(DBUS_CFLAGS)
libtime_la_LIBADD = $(DBUS_LIBS)
libtime_la_LDFLAGS = $(AM_LDFLAGS) -pthread

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
