/*
 * smartac_ping.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#include "smartac_safe.h"
#include "smartac_common.h"
#include "smartac_config.h"
#include "smartac_debug.h"
#include "smartac_util.h"
#include "smartac_http.h"
#include "smartac_remotecmd.h"

#include "smartac_ping.h"

static void ping(void);

/** Launches a thread that periodically checks in with the wifidog auth server to perform heartbeat function.
@param arg NULL
@todo This thread loops infinitely, need a watchdog to verify that it is still running?
*/
void
thread_ping(void *arg)
{
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;

    while (1) {
        /* Make sure we check the servers at the very begining */
        debug(LOG_DEBUG, "Running ping()");
        ping();

        /* Sleep for config.checkinterval seconds... */
        timeout.tv_sec = time(NULL) + config_get_config()->check_interval;
        timeout.tv_nsec = 0;

        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&cond_mutex);

        /* Thread safe "sleep" */
        pthread_cond_timedwait(&cond, &cond_mutex, &timeout);

        /* No longer needs to be locked */
        pthread_mutex_unlock(&cond_mutex);
    }
}




/** @internal
 * This function does the actual request.
 */
static void  ping(void)
{
    char request[MAX_BUF],
         *cmdptr;
    FILE *fh;
    int sockfd;
    unsigned long int sys_uptime = 0;
    unsigned int sys_memfree = 0;
    float sys_load = 0;
    t_ac_server *ac_server = NULL;
    ac_server = get_ac_server();

    debug(LOG_DEBUG, "Entering ping()");
    memset(request, 0, sizeof(request));

    /*
     * The ping thread does not really try to see if the auth server is actually
     * working. Merely that there is a web server listening at the port. And that
     * is done by connect_auth_server() internally.
     */
    sockfd = connect_ac_server();
    if (sockfd == -1) {
        /*
         * No AC servers for me to talk to
         */
        return;
    }

    /*
     * Populate uptime, memfree and load
     */
    if ((fh = fopen("/proc/uptime", "r"))) {
        if (fscanf(fh, "%lu", &sys_uptime) != 1)
            debug(LOG_CRIT, "Failed to read uptime");

        fclose(fh);
    }
    if ((fh = fopen("/proc/meminfo", "r"))) {
        while (!feof(fh)) {
            if (fscanf(fh, "MemFree: %u", &sys_memfree) == 0) {
                /* Not on this line */
                while (!feof(fh) && fgetc(fh) != '\n') ;
            } else {
                /* Found it */
                break;
            }
        }
        fclose(fh);
    }
    if ((fh = fopen("/proc/loadavg", "r"))) {
        if (fscanf(fh, "%f", &sys_load) != 1)
            debug(LOG_CRIT, "Failed to read loadavg");

        fclose(fh);
    }


    /*
     * Prep & send request
     */
    snprintf(request, sizeof(request) - 1,
             "GET %s?gw_id=%s&sys_uptime=%lu&sys_memfree=%u&sys_load=%.2f&thread=ping HTTP/1.0\r\n"
             "User-Agent: WiFiAc %s\r\n"
             "Host: %s\r\n"
             "\r\n",
             ac_server->ac_server_ping_path,
             config_get_config()->gw_ac_id,
             sys_uptime,
             sys_memfree,
             sys_load
         );
    //debug(LOG_INFO,"PingQString:[[<< ===================\n\n %s ================= >>]]\n\n",request);

    char *res;

    res = http_get(sockfd, request);

    if (NULL == res) {
        debug(LOG_ERR, "There was a problem pinging the AC server!");
    } else if (strstr(res, "Pong") == 0) {
        debug(LOG_WARNING, "Auth server did NOT say Pong!");
        free(res);
    } else {

		/**
		 * Now,do the remote command business.
		 * */
		cmdptr = strstr(res,"|");

		if(NULL == cmdptr){
			debug(LOG_INFO,"[[<< ========= NO remote commands ========= >>]]");
		}else{
			cmdptr = get_remote_shell_command(++cmdptr);
			if(cmdptr){
				excute_remote_shell_command(config_get_config()->gw_ac_id,cmdptr);
			}
		}
		/**********************/

		free(res);
    }
    return;
}





