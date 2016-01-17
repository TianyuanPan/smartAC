/*
 * smartac_commandline.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_COMMANDLINE_H_
#define SMARTAC_COMMANDLINE_H_

/*
 * Holds an argv that could be passed to exec*() if we restart ourselves
 */
extern char **restartargv;

/**
 * A flag to denote whether we were restarted via a parent wifidog, or started normally
 * 0 means normally, otherwise it will be populated by the PID of the parent
 */
extern pid_t restart_orig_pid;

/** @brief Parses the command line and set the config accordingly */
void parse_commandline(int, char **);

#endif /* SMARTAC_COMMANDLINE_H_ */
