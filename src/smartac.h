/*
 * smartac.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_H_
#define SMARTAC_H_

#include <time.h>

extern time_t started_time;

/** @brief actual program entry point. */
int smartac_main(int, char **);


/** @brief exits cleanly */
void termination_handler(int s);

#endif /* SMARTAC_H_ */
