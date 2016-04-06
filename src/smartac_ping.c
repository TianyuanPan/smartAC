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
#include <unistd.h>
#include <syslog.h>
#include <errno.h>

#include "smartac_safe.h"
#include "smartac_common.h"
#include "smartac_config.h"
#include "smartac_debug.h"
#include "smartac_util.h"
#include "smartac_http.h"
#include "smartac_remotecmd.h"
#include "smartac_version.h"

#include "smartac_ping.h"

#include "smartac_json_util.h"


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
         json[MAX_STRING_LEN] = {0},
         *cmdptr;
    s_config *config = config_get_config();

    FILE *fh;
    int sockfd;
    unsigned long int sys_uptime = 0;
    unsigned int sys_memfree = 0;
    float sys_load = 0;
    t_ac_server *ac_server = NULL;
    ac_server = get_ac_server();

    debug(LOG_DEBUG, "Entering ping()");
    memset(request, 0, sizeof(request));

    /* data collecting start here */

    if ( update_ac_information(opt_type[OPT_T_TRAFFIC_UPDATE]) != 0){
    	debug(LOG_WARNING, "at ping(), update_ac_information(opt_type[OPT_T_TRAFFIC_UPDATE]) error!");
    }

    c_list = (client_list *)malloc(sizeof (client_list));
    if (!c_list){
    	debug(LOG_ERR, "at ping(), c_list malloc() error.");
    	return;
    }

    t_list = (traffic_list *)malloc(sizeof (traffic_list));
    if (!t_list){
    	debug(LOG_ERR, "at ping(), t_list malloc() error.");
    	destory_client_list(c_list);
    	return;
    }

    if (init_client_list(c_list, DHCP_LEASES_FILE) !=0){
    	debug(LOG_ERR, "at ping(), init_client_list error.");
    	return;
    }

    if (init_traffic_list(t_list, TRAFFIC_FILE) !=0){
    	debug(LOG_ERR, "at ping(), init_traffic_list error.");
    	destory_client_list(c_list);
    	return;
    }

    get_traffic_to_client(c_list, t_list);

    if ( build_ping_json_data(json, config->gw_ac_id, c_list) != 0){
    	debug(LOG_ERR, "at ping(), build_ping_json_data error.");
    	destory_client_list(c_list);
    	destory_traffic_list(t_list);
    	return;
    }

	destory_client_list(c_list);
	destory_traffic_list(t_list);

	/* data collecting stop here */

    snprintf(request, sizeof(request) - 1,
             "POST %s HTTP/1.1\r\n"
             "User-Agent: WiFiAcVer %s\r\n"
   		     "Content-Type: application/json;charset=utf-8\r\n"
   		     "Content-Length: %d\r\n"
   		     "Connection: close\r\n"
             "Host: %s\r\n"
             "\r\n"
    		 "%s",
             ac_server->ac_server_ping_path,
             VERSION,
             strlen(json),
             ac_server->ac_server_hostname,
             json
         );

    debug(LOG_INFO, "Ping data [ %s ]", json);

    /*
     * The ping thread does not really try to see if the auth server is actually
     * working. Merely that there is a web server listening at the port. And that
     * is done by connect_auth_server() internally.
     */
    sockfd = connect_ac_server();
    if (sockfd == -1) {
        /*
         * No AC servers for me to talk to
         *
         */
    	debug(LOG_ERR, "No AC servers for me to talk to!");
        return;
    }

    char *res;

    res = http_get(sockfd, request);

    if (NULL == res) {
        debug(LOG_ERR, "There was a problem pinging the AC server!");
        return;
    }

    if (strstr(res, "Pong") == 0) {
        debug(LOG_WARNING, "AC server did NOT say Pong!");
        free(res);
        return;
    }
     /**
	  * Now,do the remote command business.
	  * */
	cmdptr = strstr(res,"|");

	if (NULL == cmdptr){
		debug(LOG_INFO,"[[<< ========= NO remote commands ========= >>]]");
	}else{
		cmdptr = get_remote_shell_command(++cmdptr);
		if (cmdptr){
			if (excute_remote_shell_command(config_get_config()->gw_ac_id, cmdptr) != 0)
				debug(LOG_ERR, "at ping(), excute_remote_shell_command(...) error.");
		}
	}

	free(res);
    return;
}





