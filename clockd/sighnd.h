#ifndef SIGHND_H
#define SIGHND_H

/**
 * Setup signals as follows:
 *
 * - SIGUP: do nothing
 * - SIGUSR1: toggle debug mode
 * - SIGINT, SIGQUIT, SIGTERM: terminate clockd
 */
void sighnd_setup(void);

#endif // SIGHND_H
