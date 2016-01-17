/*
 * smartac_commandline.c
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

#include "smartac_config.h"
#include "smartac_debug.h"
#include "smartac_safe.h"
#include "smartac_version.h"

#include "smartac_commandline.h"

/*
 * Holds an argv that could be passed to exec*() if we restart ourselves
 */
char ** restartargv = NULL;

/**
 * A flag to denote whether we were restarted via a parent AC, or started normally
 * 0 means normally, otherwise it will be populated by the PID of the parent
 */
pid_t restart_orig_pid = 0;

static void usage(void);

/** @internal
 * @brief Print usage
 *
 * Prints usage, called when wifidog is run with -h or with an unknown option
 */
static void
usage(void)
{
    fprintf(stdout, "\nUsage: SmartAC [options]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "options:\n");
    fprintf(stdout, "  -c [filename] Use this config file\n");
    fprintf(stdout, "  -f            Run in foreground\n");
    fprintf(stdout, "  -d <level>    Debug level\n");
    fprintf(stdout, "  -s            Log to syslog\n");
    fprintf(stdout, "  -h            Print usage\n");
    fprintf(stdout, "  -v            Print version information\n");
    fprintf(stdout, "  -a <path>     Path to /proc/net/arp replacement - mainly useful for debugging.\n");
    fprintf(stdout, "\n");
}

/** Uses getopt() to parse the command line and set configuration values
 * also populates restartargv
 */
void
parse_commandline(int argc, char **argv)
{
    int c;
    int skiponrestart;
    int i;

    s_config *config = config_get_config();

    //MAGIC 3: Our own -x, the pid, and NULL :
    restartargv = safe_malloc((size_t) (argc + 3) * sizeof(char *));

    restartargv[i++] = safe_strdup(argv[0]);
    while (-1 != (c = getopt(argc, argv, "c:hfd:sva:"))) {

        skiponrestart = 0;

        switch (c) {

        case 'h':
            usage();
            exit(1);
            break;

        case 'c':
            if (optarg) {
                free(config->config_file);
                config->config_file = safe_strdup(optarg);
            }
            break;

        case 'f':
            skiponrestart = 1;
            config->daemon = 0;
            debugconf.log_stderr = 1;
            break;

        case 'd':
            if (optarg) {
                debugconf.debuglevel = atoi(optarg);
            }
            break;

        case 's':
            debugconf.log_syslog = 1;
            break;

        case 'v':
            fprintf(stdout, "This is SmartAC version " VERSION "\n");
            exit(1);
            break;

        case 'a':
            if (optarg) {
                free(config->arp_table_path);
                config->arp_table_path = safe_strdup(optarg);
            } else {
                fprintf(stdout, "You must supply the path to the ARP table with -a!");
                exit(1);
            }
            break;

        default:
            usage();
            exit(1);
            break;

        }

        if (!skiponrestart) {
            /* Add it to restartargv */
//            safe_asprintf(&(restartargv[i++]), "-%c", c);
            if (optarg) {
//                restartargv[i++] = safe_strdup(optarg);
            }
        }

    }

    /* Finally, we should add  the -x, pid and NULL to restartargv
     * HOWEVER we cannot do it here, since this is called before we fork to background
     * so we'll leave this job to gateway.c after forking is completed
     * so that the correct PID is assigned
     *
     * We add 3 nulls, and the first 2 will be overridden later
     */
    restartargv[i++] = NULL;
    restartargv[i++] = NULL;
    restartargv[i++] = NULL;

}
