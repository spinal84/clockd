#include <stdbool.h>
#include <malloc.h>
#include <glib.h>
#include <string.h>
#include <cityinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <dbus/dbus.h>

#include "logging.h"
#include "clock_dbus.h"
#include "mcc_tz_utils.h"
#include "internal_time_utils.h"

static GSList *country_tz_name_list = NULL;
static DBusConnection *system_bus = NULL;
static unsigned int mcc_cache = 0;

static GetAutosyncEnabled get_autosync_enabled = NULL;
static HandleCsdNetTimeChange handle_csd_net_time_change = NULL;
static SetOperatorTz set_operator_tz = NULL;

static bool registration_status_change_dbus_handler_installed = false;

static void
mcc_tz_destroy_country_tz_name_list()
{
  g_slist_free_full(country_tz_name_list, free);
  country_tz_name_list = NULL;
}

static gint
tz_name_compare(gconstpointer iterTZName, gconstpointer searchingTZName)
{
  return internal_tz_cmp((const char *)iterTZName,
                         (const char *)searchingTZName);
}

static void
mcc_tz_prepend_tz_name_if_not_dup(const char *tz_name)
{
  if (g_slist_find_custom(country_tz_name_list, tz_name, tz_name_compare))
    return;

  country_tz_name_list = g_slist_prepend(country_tz_name_list, g_strdup(tz_name));
}

static gboolean
mcc_tz_searching_tz_by_country_name(const Cityinfo *city, gpointer data)
{
  gchar *country;
  gchar *zone;

  if (!city)
  {
    DO_LOG(LOG_WARNING,
           "mcc_tz_for_next_country(): NULL city info passed");
    return FALSE;
  }

  if (!data)
  {
    DO_LOG(LOG_ERR,
           "mcc_tz_for_next_country(): NULL searchingCountryName passed");
    return FALSE;
  }

  country = cityinfo_get_country(city);
  if (country)
  {
    if (strcmp(country, (const char *)data))
      return TRUE;

    zone = cityinfo_get_zone(city);

    if (!zone)
    {
      DO_LOG(LOG_WARNING,
             "mcc_tz_for_next_country(): zone = NULL in city info");
      return TRUE;
    }

    mcc_tz_prepend_tz_name_if_not_dup(zone);
  }
  else
  {
    DO_LOG(LOG_WARNING,
           "mcc_tz_for_next_country(): countryName = NULL in city info, continue searching");
  }

  return TRUE;
}

static void
mcc_tz_remove_registration_change_match()
{
  DBusError error = DBUS_ERROR_INIT;

  if (registration_status_change_dbus_handler_installed)
  {
    DO_LOG(LOG_DEBUG,
           "clockd:removing dbus_bus_add_match(CSD_REGISTRATION_CHANGE_MATCH_RULE)");
    dbus_bus_remove_match(system_bus, CSD_REGISTRATION_CHANGE_MATCH_RULE, &error);

    if (dbus_error_is_set(&error))
    {
      DO_LOG(LOG_ERR, "dbus_bus_remove_match(%s) error %s",
             CSD_REGISTRATION_CHANGE_MATCH_RULE, error.message);
    }
    else
      registration_status_change_dbus_handler_installed = 0;
  }

  dbus_error_free(&error);
}

static int
mcc_tz_parse_mcc_mapping_line(const char *line, char **country)
{
  gchar *tab_ptr = g_utf8_strrchr(line, -1, '\t');

  if (tab_ptr)
  {
    const gchar *ptr;
    char mcc_str[4];
    int mcc = 0;

    *country = g_utf8_find_next_char(tab_ptr, NULL);

    if (g_utf8_strrchr(tab_ptr, strlen(tab_ptr) + 1, '\r'))
      *tab_ptr = 0;
    else
      tab_ptr[strlen(tab_ptr) - 1] = 0;

     mcc_str[0] = g_utf8_get_char(line);
     ptr = g_utf8_find_next_char(line, NULL);
     mcc_str[1] = g_utf8_get_char(ptr);
     mcc_str[2] = g_utf8_get_char(g_utf8_find_next_char(ptr, NULL));
     mcc_str[3] = 0;

     mcc = strtol(mcc_str, NULL, 10);

     if (*country && mcc)
       return mcc;
  }

  DO_LOG(LOG_WARNING,
         "mcc_tz_find_country_by_mcc(): can't parse line: %s", line);

  *country = NULL;
  return 0;
}

static gboolean
mcc_tz_find_country_by_mcc(int mcc, char **found_country)
{
  FILE *fp;
  char buf[1024];
  char *country = NULL;
  unsigned int mcc_found = 0;

  fp = fopen("/usr/share/operator-wizard/mcc_mapping", "r");

  if (!fp)
  {
    DO_LOG(LOG_WARNING, "mcc_tz_find_country_by_mcc(): can't open %s",
           "/usr/share/operator-wizard/mcc_mapping");
    return FALSE;
  }

  do
  {
    while (true)
    {
      if (!fgets(buf, sizeof(buf), fp))
      {
        *found_country = NULL;
        fclose(fp);

        return FALSE;
      }

      mcc_found = mcc_tz_parse_mcc_mapping_line(buf, &country);

      if (mcc_found)
        break;

      DO_LOG(LOG_WARNING,
             "mcc_tz_find_country_by_mcc(): can't parse line: %s", buf);
    }
  }
  while (mcc_found != mcc);

  DO_LOG(LOG_DEBUG, "mcc_tz_find_country_by_mcc(): country found: %s", country);

  *found_country = strdup(country);
  fclose(fp);

  return TRUE;
}

static void
mcc_tz_create_tz_name_list_by_country_name(const char *country_name)
{
  cityinfo_foreach(mcc_tz_searching_tz_by_country_name, (gpointer)country_name);
  DO_LOG_STR_SLIST(LOG_DEBUG, country_tz_name_list);
}

static void
mcc_tz_update_country_tz_name_list()
{
  char *country = NULL;

  mcc_tz_destroy_country_tz_name_list();

  if (mcc_tz_find_country_by_mcc(mcc_cache, &country))
  {
    mcc_tz_create_tz_name_list_by_country_name(country);
    free(country);
  }
}

static gboolean
mcc_tz_get_tz_if_only_for_country(const char **found_tz)
{
  if (g_slist_length(country_tz_name_list) == 1 && country_tz_name_list->data)
  {
    *found_tz = country_tz_name_list->data;
    return TRUE;
  }

  *found_tz = NULL;
  return FALSE;
}

static void
mcc_tz_set_tz_from_mcc()
{
  const char *tz;

  DO_LOG(LOG_DEBUG, "mcc_tz_set_tz_from_mcc(): mcc_cache = %d", mcc_cache);

  if (mcc_tz_get_tz_if_only_for_country(&tz))
    set_operator_tz(tz);
}

static void
mcc_tz_handle_network_timeinfo_reply(DBusMessage *msg)
{
  DBusError error = DBUS_ERROR_INIT;
  const char *err_msg = NULL;

  if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR)
  {
    if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &err_msg,
                              DBUS_TYPE_INVALID))
    {
      DO_LOG(LOG_ERR, "D-Bus call failed: %s", err_msg);
    }
    else
      DO_LOG(LOG_ERR, "Failed to get error reply argument, msg = %s",
             error.message);

    dbus_error_free(&error);
  }
  else if (get_autosync_enabled() && handle_csd_net_time_change(msg) == -1)
    mcc_tz_set_tz_from_mcc();

  dbus_message_unref(msg);
}

static void
mcc_tz_network_timeinfo_reply_dbus_cb(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *msg = dbus_pending_call_steal_reply(pending);

  if (msg)
    mcc_tz_handle_network_timeinfo_reply(msg);
  else
  {
    DO_LOG(LOG_ERR,
         "mcc_tz_handle_network_timeinfo_reply: but no pending call available");
  }

  dbus_pending_call_unref(pending);
}

static void
mcc_tz_check_if_network_timeinfo_available(void)
{
  struct DBusMessage *msg;
  DBusPendingCall *pending = NULL;

  msg = dbus_message_new_method_call(CSD_SERVICE, CSD_PATH,
                                     CSD_INTERFACE,
                                     CSD_GET_NETWORK_TIMEINFO);

  if (!dbus_connection_send_with_reply(system_bus, msg, &pending,
                                       DBUS_TIMEOUT_USE_DEFAULT))
  {
    DO_LOG(LOG_ERR,
           "dbus_connection_send_with_reply error - no memory");
  }

  dbus_connection_flush(system_bus);

  if (!dbus_pending_call_set_notify(pending,
                    mcc_tz_network_timeinfo_reply_dbus_cb, NULL, NULL))
  {
    DO_LOG(LOG_ERR,
           "dbus_connection_send_with_reply error - no memory");
  }

  dbus_message_unref(msg);
}

void
mcc_tz_handle_registration_status_reply(struct DBusMessage *msg)
{
  DBusError error = DBUS_ERROR_INIT;
  int unused = 0;
  dbus_uint32_t mcc = 0;
  unsigned char registration_status = 10;

  if (dbus_message_get_args(msg, &error,
                            DBUS_TYPE_BYTE, &registration_status,
                            DBUS_TYPE_UINT16, &unused,
                            DBUS_TYPE_UINT32, &unused,
                            DBUS_TYPE_UINT32, &unused,
                            DBUS_TYPE_UINT32, &mcc,
                            DBUS_TYPE_INVALID))
  {
    DO_LOG(LOG_DEBUG, "registration_status = %d, mcc = %d", registration_status,
           mcc);

    if (registration_status > 2)
      mcc_cache = 0;
    else
    {
      DO_LOG(LOG_DEBUG, "mcc_cache = %d", mcc_cache);

      if (mcc_cache != mcc)
      {
        mcc_cache = mcc;

        DO_LOG(LOG_DEBUG, "mcc changed, mcc_cache = %d", mcc_cache);

        mcc_tz_update_country_tz_name_list();

        if (get_autosync_enabled())
          mcc_tz_check_if_network_timeinfo_available();
      }
    }
  }
  else
    DO_LOG(LOG_ERR, "Failed to parse reply, msg = %s", error.message);

  dbus_error_free(&error);
}

static void
mcc_tz_registration_status_reply_dbus_cb(DBusPendingCall *pending,
                                         void *user_data)
{
  DBusMessage *reply = dbus_pending_call_steal_reply(pending);

  if (reply)
  {
    DBusError error = DBUS_ERROR_INIT;
    const char *error_msg;

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
    {
      if ( dbus_message_get_args(reply, &error, DBUS_TYPE_STRING, &error_msg,
                                 DBUS_TYPE_INVALID))
      {
        DO_LOG(LOG_ERR, "D-Bus call failed: %s", error_msg);
      }
      else
      {
        DO_LOG(LOG_ERR, "Failed to get error reply argument, msg = %s",
               error.message);
      }

      dbus_error_free(&error);
    }
    else
      mcc_tz_handle_registration_status_reply(reply);

    dbus_message_unref(reply);
  }
  else
  {
    DO_LOG(LOG_ERR,
           "mcc_tz_registration_status_reply_dbus_cb: but no pending call available");
  }

  dbus_pending_call_unref(pending);
}

static void
mcc_tz_add_registration_change_match()
{
  if (registration_status_change_dbus_handler_installed)
    return;

  DBusError error = DBUS_ERROR_INIT;
  DO_LOG(LOG_DEBUG,
         "adding dbus_bus_add_match(CSD_REGISTRATION_CHANGE_MATCH_RULE)");

  dbus_bus_add_match(system_bus, CSD_REGISTRATION_CHANGE_MATCH_RULE, &error);

  if (dbus_error_is_set(&error))
  {
    DO_LOG(LOG_ERR, "dbus_bus_add_match(%s) error %s",
           CSD_REGISTRATION_CHANGE_MATCH_RULE, error.message);
  }
  else
    registration_status_change_dbus_handler_installed = 1;

  dbus_error_free(&error);
}

void
mcc_tz_setup_timezone_from_mcc_if_required(void)
{
  if (get_autosync_enabled())
  {
    DBusMessage *msg;
    DBusPendingCall *call = NULL;

    /* FIXME - use macros */
    msg = dbus_message_new_method_call(CSD_SERVICE, CSD_PATH, CSD_INTERFACE,
                                       CSD_GET_REGISTRATION_STATUS);

    if (!dbus_connection_send_with_reply(system_bus, msg, &call,
                                         DBUS_TIMEOUT_USE_DEFAULT))
    {
      DO_LOG(LOG_ERR, "dbus_connection_send_with_reply error - no memory");
    }

    mcc_tz_add_registration_change_match();

    dbus_connection_flush(system_bus);

    if (!dbus_pending_call_set_notify(call,
                                      mcc_tz_registration_status_reply_dbus_cb,
                                      NULL, NULL))
    {
      DO_LOG(LOG_ERR, "dbus_connection_send_with_reply error - no memory");
    }

    dbus_message_unref(msg);
  }
  else
    mcc_tz_remove_registration_change_match();
}

int
mcc_tz_utils_init(DBusConnection *server_system_bus,
                  GetAutosyncEnabled server_get_autosync_enabled,
                  HandleCsdNetTimeChange server_handle_csd_net_time_change,
                  SetOperatorTz server_set_operator_tz)
{
  if (!server_system_bus || !server_get_autosync_enabled ||
      !server_handle_csd_net_time_change || !server_set_operator_tz)
  {
    return -1;
  }

  system_bus = server_system_bus;
  get_autosync_enabled = server_get_autosync_enabled;
  handle_csd_net_time_change = server_handle_csd_net_time_change;
  set_operator_tz = server_set_operator_tz;
  mcc_tz_setup_timezone_from_mcc_if_required();

  return 0;
}

int
mcc_tz_is_tz_name_in_country_tz_list(const char *tz_name)
{
  const char *p;
  GSList *l;

  p = tz_name;
  l = country_tz_name_list;

  while (*p)
  {
    if (isalpha(*p))
    {
      while (l)
      {
        if (!strcmp(p, (const char *)l->data))
          return 1;

        l = l->next;
      }

      return 0;
    }

    p++;
  }

  return 0;
}

static int
mcc_tz_find_tz_in_country_tz_list(struct tm *utc_tm,
                                  int dst,
                                  int gmtoff,
                                  char **tz_name)
{
  GSList *l = country_tz_name_list;
  int count = 0;
  struct tm iter_time;

  memset(&iter_time, 0, sizeof(iter_time));
  *tz_name = 0;

  DO_LOG(LOG_DEBUG, "mcc_tz_find_tz_in_country_tz_list");
  log_tm("UTC time", utc_tm);
  DO_LOG(LOG_DEBUG, "gmtoff = %d, dst = %d", gmtoff, dst);

  while (l)
  {
    char *tz = (char *)l->data;
    DO_LOG(LOG_DEBUG, "iter: %s", tz);

    if (internal_localtime_r_in(utc_tm, &iter_time, tz))
    {
      log_tm("iter time", &iter_time);

      if (iter_time.tm_gmtoff == gmtoff &&
          (dst == 100 || iter_time.tm_isdst == (dst != 0)) )
      {
        DO_LOG(LOG_DEBUG, "TZ found: %s", tz);

        if (!*tz_name)
          *tz_name = tz;

        count++;
      }
    }
    else
      DO_LOG(LOG_ERR, "localtime_r_in() failed");

    l = l->next;
  }

  return count;
}

static void
mcc_tz_correct_tz_choice(int found_tz_count, char **tz_name)
{
  if (found_tz_count == 1)
    DO_LOG(LOG_DEBUG, "Good TZ found!");
  else if (g_slist_length(country_tz_name_list) == 1)
  {
    DO_LOG(LOG_DEBUG, "Only TZ for country, so it shall be used");
    *tz_name = (char *)country_tz_name_list->data;
  }
  else if (found_tz_count <= 0)
  {
    DO_LOG(LOG_WARNING, "Can't guess anything, so do nothing");
    *tz_name = NULL;
  }
  else
    DO_LOG(LOG_WARNING,
           "First found TZ will be used as current TZ. Yes, it is bad but what can we do?");
}

void
mcc_tz_guess_tz_for_country_by_dst_and_offset(struct tm *utc_tm,
                                              int dst,
                                              int gmtoff,
                                              char **tz_name)
{
  int tz_count = mcc_tz_find_tz_in_country_tz_list(utc_tm, dst, gmtoff, tz_name);
  mcc_tz_correct_tz_choice(tz_count, tz_name);
  DO_LOG(LOG_DEBUG, "tzname = %s", *tz_name ? *tz_name : "NULL");
}

void
mcc_tz_utils_quit(void)
{
  mcc_tz_destroy_country_tz_name_list();
  mcc_tz_remove_registration_change_match();
  set_operator_tz = NULL;
  handle_csd_net_time_change = NULL;
  get_autosync_enabled = NULL;
  system_bus = NULL;
}
