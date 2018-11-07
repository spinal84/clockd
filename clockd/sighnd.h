#ifndef SIGHND_H
#define SIGHND_H

/**
 * Setup signals as follows:
 * <ul>
 * <li>SIGUP: do nothing
 * <li>SIGUSR1: toggle debug mode
 * <li>SIGINT, SIGQUIT, SIGTERM: terminate clockd
 * </ul>
 */
void sighnd_setup(void);

#endif // SIGHND_H
