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
#include "smartac_config.h"


#include "smartac_http.h"



/* Tries really hard to connect to an auth server. Returns a file descriptor, -1 on error
 */
int connect_ac_server()
{
	int sockfd;

	LOCK_CONFIG()
	;
	sockfd = _connect_ac_server(0);
	UNLOCK_CONFIG()
	;

	if (sockfd == -1) {
		debug(LOG_ERR, "Failed to connect to any of the AC servers");
		//mark_auth_offline();
	} else {
		debug(LOG_DEBUG, "Connected to AC server");
		//mark_auth_online();
	}
	return (sockfd);
}

/* Helper function called by connect_auth_server() to do the actual work including recursion
 * DO NOT CALL DIRECTLY
 @param level recursion level indicator must be 0 when not called by _connect_auth_server()
 */
int _connect_ac_server(int level)
{
	s_config *config = config_get_config();
	t_ac_server *ac_server = NULL;
	t_popular_server *popular_server = NULL;
	struct in_addr *h_addr;
	int num_servers = 0;
	char *hostname = NULL;
	char *ip;
	struct sockaddr_in their_addr;
	int sockfd;

	/* If there are no auth servers, error out, from scan-build warning. */
	if (NULL == config->ac_servers) {
		return (-1);
	}

	/* XXX level starts out at 0 and gets incremented by every iterations. */
	level++;

	/*
	 * Let's calculate the number of servers we have
	 */
	for (ac_server = config->ac_servers; ac_server; ac_server =
			ac_server->next) {
		num_servers++;
	}
	debug(LOG_DEBUG, "Level %d: Calculated %d AC servers in list", level,
			num_servers);

	if (level > num_servers) {
		/*
		 * We've called ourselves too many times
		 * This means we've cycled through all the servers in the server list
		 * at least once and none are accessible
		 */
		return (-1);
	}

	/*
	 * Let's resolve the hostname of the top server to an IP address
	 */
	ac_server = config->ac_servers;
	hostname = ac_server->ac_server_hostname;
	debug(LOG_DEBUG, "Level %d: Resolving AC server [%s]", level, hostname);
	h_addr = wd_gethostbyname(hostname);
	if (!h_addr) {
		/*
		 * DNS resolving it failed
		 */
		debug(LOG_DEBUG, "Level %d: Resolving AC server [%s] failed", level,
				hostname);

		for (popular_server = config->popular_servers; popular_server;
				popular_server = popular_server->next) {
			debug(LOG_DEBUG, "Level %d: Resolving popular server [%s]", level,
					popular_server->hostname);
			h_addr = wd_gethostbyname(popular_server->hostname);
			if (h_addr) {
				debug(LOG_DEBUG,
						"Level %d: Resolving popular server [%s] succeeded = [%s]",
						level, popular_server->hostname, inet_ntoa(*h_addr));
				break;
			} else {
				debug(LOG_DEBUG,
						"Level %d: Resolving popular server [%s] failed", level,
						popular_server->hostname);
			}
		}

		/*
		 * If we got any h_addr buffer for one of the popular servers, in other
		 * words, if one of the popular servers resolved, we'll assume the DNS
		 * works, otherwise we'll deal with net connection or DNS failure.
		 */
		if (h_addr) {
			free(h_addr);
			/*
			 * Yes
			 *
			 * The AC server's DNS server is probably dead. Try the next AC server
			 */
			debug(LOG_DEBUG,
					"Level %d: Marking auth server [%s] as bad and trying next if possible",
					level, hostname);
			if (ac_server->last_ip) {
				free(ac_server->last_ip);
				ac_server->last_ip = NULL;
			}
			mark_ac_server_bad(ac_server);
			return _connect_ac_server(level);
		} else {
			/*
			 * No
			 *
			 * It's probably safe to assume that the internet connection is malfunctioning
			 * and nothing we can do will make it work
			 */
			//mark_offline();
			debug(LOG_DEBUG,
					"Level %d: Failed to resolve auth server and all popular servers. "
							"The internet connection is probably down", level);
			return (-1);
		}
	} else {
		/*
		 * DNS resolving was successful
		 */
		// mark_online();
		ip = safe_strdup(inet_ntoa(*h_addr));
		debug(LOG_DEBUG,
				"Level %d: Resolving auth server [%s] succeeded = [%s]", level,
				hostname, ip);

		if (!ac_server->last_ip || strcmp(ac_server->last_ip, ip) != 0) {
			/*
			 * But the IP address is different from the last one we knew
			 * Update it
			 */
			debug(LOG_DEBUG,
					"Level %d: Updating last_ip IP of server [%s] to [%s]",
					level, hostname, ip);
			if (ac_server->last_ip)
				free(ac_server->last_ip);
			ac_server->last_ip = ip;

		} else {
			/*
			 * IP is the same as last time
			 */
			free(ip);
		}

		/*
		 * Connect to it
		 */
		int port = 0;

		debug(LOG_DEBUG, "Level %d: Connecting to AC server %s:%d", level,
				hostname, ac_server->ac_server_port);
		port = htons(ac_server->ac_server_port);

		their_addr.sin_port = port;
		their_addr.sin_family = AF_INET;
		their_addr.sin_addr = *h_addr;
		memset(&(their_addr.sin_zero), '\0', sizeof(their_addr.sin_zero));
		free(h_addr);

		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			debug(LOG_ERR,
					"Level %d: Failed to create a new SOCK_STREAM socket: %s",
					strerror(errno));
			return (-1);
		}

		if (connect(sockfd, (struct sockaddr *) &their_addr,
				sizeof(struct sockaddr)) == -1) {
			/*
			 * Failed to connect
			 * Mark the server as bad and try the next one
			 */
			debug(LOG_DEBUG,
					"Level %d: Failed to connect to auth server %s:%d (%s). Marking it as bad and trying next if possible",
					level, hostname, ntohs(port), strerror(errno));
			close(sockfd);
			mark_ac_server_bad(ac_server);
			return _connect_ac_server(level); /* Yay recursion! */
		} else {
			/*
			 * We have successfully connected
			 */
			debug(LOG_DEBUG,
					"Level %d: Successfully connected to auth server %s:%d",
					level, hostname, ntohs(port));
			return sockfd;
		}
	}
}



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

    debug(LOG_DEBUG, "Sending HTTP request to AC server: [%s]\n", req);
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
                debug(LOG_ERR, "An error occurred while reading from  server: %s", strerror(errno));
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
