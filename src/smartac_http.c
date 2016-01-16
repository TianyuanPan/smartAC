/*
 * martac_http.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "smartac_common.h"
#include "smartac_debug.h"
#include "smartac_util.h"


#include "smartac_http.h"



/**
 * Perform an HTTP request, caller frees both request and response,
 * NULL returned on error.
 * @param sockfd Socket to use, already connected
 * @param req Request to send, fully formatted.
 * @return char Response as a string
 */
char *
http_get(const int sockfd, const char *req)
{
    ssize_t numbytes;
    int done, nfds;
    fd_set readfds;
    struct timeval timeout;
    size_t reqlen = strlen(req);
    char readbuf[MAX_BUF];
    char *retval;
    pstr_t *response = pstr_new();

    if (sockfd == -1) {
        /* Could not connect to server */
        debug(LOG_ERR, "Could not open socket to server!");
        goto error;
    }

    debug(LOG_DEBUG, "Sending HTTP request to auth server: [%s]\n", req);
    numbytes = send(sockfd, req, reqlen, 0);
    if (numbytes <= 0) {
        debug(LOG_ERR, "send failed: %s", strerror(errno));
        goto error;
    } else if ((size_t) numbytes != reqlen) {
        debug(LOG_ERR, "send failed: only %d bytes out of %d bytes sent!", numbytes, reqlen);
        goto error;
    }

    debug(LOG_DEBUG, "Reading response");
    done = 0;
    do {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 30;    /* XXX magic... 30 second is as good a timeout as any */
        timeout.tv_usec = 0;
        nfds = sockfd + 1;

        nfds = select(nfds, &readfds, NULL, NULL, &timeout);

        if (nfds > 0) {
                        /** We don't have to use FD_ISSET() because there
			 *  was only one fd. */
            memset(readbuf, 0, MAX_BUF);
            numbytes = read(sockfd, readbuf, MAX_BUF - 1);
            if (numbytes < 0) {
                debug(LOG_ERR, "An error occurred while reading from server: %s", strerror(errno));
                goto error;
            } else if (numbytes == 0) {
                done = 1;
            } else {
                readbuf[numbytes] = '\0';
                pstr_cat(response, readbuf);
                debug(LOG_DEBUG, "Read %d bytes", numbytes);
            }
        } else if (nfds == 0) {
            debug(LOG_ERR, "Timed out reading data via select() from auth server");
            goto error;
        } else if (nfds < 0) {
            debug(LOG_ERR, "Error reading data via select() from auth server: %s", strerror(errno));
            goto error;
        }
    } while (!done);

    close(sockfd);
    retval = pstr_to_string(response);
    debug(LOG_DEBUG, "HTTP Response from Server: [%s]", retval);
    return retval;

 error:
    if (sockfd >= 0) {
        close(sockfd);
    }
    retval = pstr_to_string(response);
    free(retval);
    return NULL;
}
