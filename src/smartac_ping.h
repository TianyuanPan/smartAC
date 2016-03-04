/*
 * smartac_ping.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_PING_H_
#define SMARTAC_PING_H_

#define MINIMUM_STARTED_TIME 1041379200 /* 2003-01-01 */

/** @brief Periodically checks on the auth server to see if it's alive. */
void thread_ping(void *arg);


#endif /* SMARTAC_PING_H_ */
