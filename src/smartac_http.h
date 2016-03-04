/*
 * martac_http.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef MARTAC_HTTP_H_
#define MARTAC_HTTP_H_

/** @brief Tries really hard to connect to an auth server.  Returns a connected file descriptor or -1 on error */
int connect_ac_server(void);

/** @brief Helper function called by connect_auth_server() to do the actual work including recursion - DO NOT CALL DIRECTLY */
int _connect_ac_server(int level);

/** @brief do the http request */
char *http_get(const int, const char *);

#endif /* MARTAC_HTTP_H_ */
