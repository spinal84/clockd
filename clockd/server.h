#ifndef SERVER_H
#define SERVER_H

/**
 * @brief Prototypes for clockd 'server'
 * @file  server.h
 * @copyright GNU GPLv2 or later
 */

/**
 * Init D-Bus server
 *
 * The following is set up:
 * <ul>
 * <li>settings from environment (/etc/clockd/clockd-settings.default)
 * <li>settings from configuration file (/home/user/.clockd.conf)
 * <li>D-Bus connections
 * <li>libosso init
 * <li>timezone from /etc/localtime or /home/user/.clockd.conf
 * </ul>
 *
 * @return 0 if OK, -1 if error
 */
int server_init(void);

/** Deinitialize D-Bus server */
void server_quit(void);

#endif // SERVER_H
