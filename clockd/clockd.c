/**
 * @brief clockd main body
 *
 * @file  clockd.c
 * This is the main file for clockd.
 *
 * @copyright GNU GPLv2 or later
 */

#include "mainloop.h"

/**
 * This is clockd main pgm.
 *
 * @param  argc  refer to mainloop_run
 * @param  argv  refer to mainloop_run
 *
 * @return refer to mainloop_run
 */
int
main (int argc, char **argv)
{
  return mainloop_run(argc, argv);
}
