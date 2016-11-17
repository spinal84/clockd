#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "logging.h"
#include "mainloop.h"
#include "sighnd.h"

static void 	sighnd_terminate (void);
static void 	sighnd_handler (int sig);
void 	sighnd_setup (void);
