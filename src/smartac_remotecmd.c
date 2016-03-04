/*
 * smartac_remotecmd.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "smartac_common.h"
#include "smartac_do_command.h"
#include "smartac_debug.h"
#include "smartac_util.h"
#include "smartac_resultqueue.h"


#include "smartac_remotecmd.h"


static char remote_shell_cmd[MAX_CMD_BUF];


extern t_queue *cmdrets_queue;


char *get_remote_shell_command(char *cmdptr)
{

	if (NULL == cmdptr){
		debug(LOG_WARNING,"REMOTE shell: remote shell command is null.");
		return NULL;
	}
	memset(remote_shell_cmd,0,MAX_CMD_BUF);
	sprintf(remote_shell_cmd,"%s",cmdptr);
	return remote_shell_cmd;
}



int excute_remote_shell_command(char *gw_id,char *shellcmd)
{
	FILE_T   *pft;
	FILE     *outf;
	int      out_size;
	char     *out_file;

	t_result *cmd_result = NULL;


	char cmd_id[MAX_CMDID_LEN],
		 normal_cmd[MAX_CMD_BUF],
		 cmdresult[MAX_CMD_OUT_BUF];

	char *pos_id,
		 *pos_cmd;


	memset(cmdresult,0,MAX_CMD_OUT_BUF);
	memset(cmd_id,0,MAX_CMDID_LEN);

	pos_id = shellcmd;
	pos_cmd = strstr(shellcmd,"|");

	snprintf(cmd_id,++pos_cmd - pos_id - 1,"%s",++pos_id);

	pos_cmd = ++pos_cmd;

	debug(LOG_INFO,"cmd_id:%s,cmd:%s",cmd_id, pos_cmd);

	sprintf(normal_cmd,"%s",pos_cmd);
	pft = excute_open(normal_cmd,"r");

	debug(LOG_INFO,"pos_cmd:%s",pos_cmd);

	if (NULL == pft){
		debug(LOG_WARNING,"excute_shell_command excute_open error....");
		return -1;
	}

	out_size = get_file_length(pft->name);

	if(out_size < 0){
		debug(LOG_WARNING, "command out file size error!");
		free(out_file);
		excute_close(pft);
		return -1;
	}

	debug(LOG_DEBUG, "====== out_size: %d ======", out_size);

	cmd_result = cmdresult_malloc(out_size);

	if(!cmd_result){

		debug(LOG_WARNING, "cmdresult_malloc error");
		free(out_file);
		excute_close(pft);
		return -1;
	}

	cmd_result->c_size = out_size;


	excute_read(cmd_result->result, 1, out_size, pft);

	excute_close(pft);

	if (insert_queue(&cmdrets_queue, cmd_result) < 0){

		debug(LOG_WARNING, "insert command out to the queue error !");
	}

	free(out_file);

	return 0;
}
