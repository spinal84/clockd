#ifndef MAINLOOP_H
#define MAINLOOP_H

/**
 * Quit the mainloop.
 * @param  force  If nonzero and no mainloop, exit the process with failure
 * @return 1 if success, 0 if fails (no mainloop)
 */
int mainloop_stop (int force);

/**
 * This is clockd mainloop.<br>
 *
 * Args:<br>
 * -d enable debug mode<br>
 *
 * <ul>
 * <li>init signals
 * <li>inits dbus (server)
 * <li>starts g_mainloop
 * </ul>
 *
 * @param  argc  argc
 * @param  argv  argv
 *
 * @return 0 if exited normally, nonzero if exit with failure.
 */
int mainloop_run (int argc, char *argv[]);

#endif // MAINLOOP_H
