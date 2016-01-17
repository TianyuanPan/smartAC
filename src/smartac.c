/*
 * smartac.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>

/* for strerror() */
#include <string.h>

/* for wait() */
#include <sys/wait.h>

#include "smartac_commandline.h"
#include "smartac_config.h"
#include "smartac_debug.h"

#include "smartac.h"


static pthread_t tid_ping = 0;

time_t started_time = 0;

/**@internal
 * Main execution loop
 */
static void  main_loop(void)
{
   printf("==================================\n"
		  "  main_loop:  I am here !!!!!!!!!!\n"
		  "==================================\n");
}




int smartac_main(int argc, char **argv)
{
    s_config *config = config_get_config();

    config_init();

    parse_commandline(argc, argv);

    /* Initialize the config */
    config_read(config->config_file);
    config_validate();


    if (restart_orig_pid) {

        /*
         * At this point the parent will start destroying itself and the firewall. Let it finish it's job before we continue
         */
        while (kill(restart_orig_pid, 0) != -1) {
            debug(LOG_INFO, "Waiting for parent PID %d to die before continuing loading", restart_orig_pid);
            sleep(1);
        }

        debug(LOG_INFO, "Parent PID %d seems to be dead. Continuing loading.");
    }

    if (config->daemon) {

        debug(LOG_INFO, "Forking into background");

        switch (safe_fork()) {
        case 0:                /* child */
            setsid();
            main_loop();
            break;

        default:               /* parent */
            exit(0);
            break;
        }
    } else {
        main_loop();
    }

	return (0);
}
