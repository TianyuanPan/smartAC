/*
 * smartac_wdctl_update_list.c
 *
 *  Created on: Feb 16, 2016
 *      Author: TianyuanPan
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

#include "smartac_debug.h"

#define  DEFAULT_WDCTL_BIN   "/usr/bin/wdctl"


/**
 *
 *
 **/
static int check_paramter(int argc)
{
	if(argc < 2)
		return (0);
	return (1);
}



static int do_wdctl_reset(const char *mac)
{
	char parm[3][32] = {{0},{0},{0}};
	sprintf(parm[0], "     ");
	sprintf(parm[1], "reset");
	sprintf(parm[2], "%s", mac);

	char *argv[] = {parm[0], parm[1], parm[2], NULL};

	debug(LOG_INFO, "Update list  argv: %s %s %s %s", DEFAULT_WDCTL_BIN, parm[0], parm[1], parm[2]);

	execv(DEFAULT_WDCTL_BIN, argv);

	// never reach here otherwise execv error.
	debug(LOG_ERR, "child execv ERROR.\nError Message: %s\n", strerror(errno));
	exit(-1);
}


int main(int argc, char **argv)
{
	if(!check_paramter(argc))
		return -1;
	int i;
	pid_t pid;

	for (i = 1; i < argc; i++){
		pid = fork();
		if (pid == -1){ // fork() failure.
			debug(LOG_ERR, "Update list fork() ERROR!");
			return -1;
		}

		if ( pid == 0){ // the child.
			do_wdctl_reset(argv[i]);
		}
		// the parent do nothing.
	}
	exit(0);
}
