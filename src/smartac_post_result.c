/*
 * smartac_post_result.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>


#include "smartac_version.h"
#include "smartac_common.h"
#include "smartac_resultqueue.h"
#include "smartac_http.h"
#include "smartac_config.h"
#include "smartac_debug.h"
#include "smartac_post_result.h"




void thread_post_result(void *args)
{
    char request[MAX_BUF],
         data[MAX_CMD_OUT_BUF],
         *retptr;
    int data_len;

    int sockfd;
    t_result *result;
    t_ac_server *ac_server = NULL;

    ac_server = get_ac_server();

    debug(LOG_INFO, "Entering post result()");
    memset(request, 0, sizeof(request));

    if(!args)
    	return;

    result = (t_result*)args;


    memcpy(data, result->result, result->size);

    /*
     * Prep & send request
     *
     */
    snprintf(request, sizeof(request) - 1,
             "POST %s HTTP/1.0\r\n"
             "User-Agent: WiFiAc %s\r\n"
    		 "Content-Type: text/json;charset=utf-8\r\n"
    		 "Content-Length: %d\r\n"
    		 "Connection: close\r\n"
             "Host: %s\r\n"
             "\r\n"
    		 "%s",
             ac_server->ac_server_result_path,
             VERSION,
             result->c_size,
             ac_server->ac_server_hostname,
             data
         );

    cmdresult_free(result);
    result = NULL;

    debug(LOG_DEBUG,"=== post request:\n %s  \n===========", request);
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
    	debug(LOG_ERR, "No AC servers for me to talk to!");
        return;
    }

    char *res;

    res = http_get(sockfd, request);

    if (NULL == res) {
        debug(LOG_ERR, "There was a problem posting result to the AC server!");
        return;
    }

    retptr = strstr(res, "Success");

    if(!retptr)
    	debug(LOG_ERR, "AC response is NOT Success !");

	free(res);

	return;
}


