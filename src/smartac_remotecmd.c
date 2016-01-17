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


#define  GET_SETTINGS_INFO_CMD          "smartac_settings"
#define  SETTINGS_INFO_FILE             "/tmp/.apsettings"
#define  NORMAL_CMD_RESULT_FILE         "/tmp/.ac_commond_result"


static char remote_shell_cmd[MAX_CMD_BUF];


extern t_queue *cmdres_queue;


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
		 get_info_cmd[MAX_CMDNAME_LEN],
		 normal_cmd[MAX_CMD_BUF],
		 cmdresult[MAX_CMD_OUT_BUF];

	char *pos_id,
		 *pos_cmd;

	int   is_get_info = 0;

	memset(cmdresult,0,MAX_CMD_OUT_BUF);
	memset(cmd_id,0,MAX_CMDID_LEN);
	memset(get_info_cmd,0,MAX_CMDNAME_LEN);

	pos_id = shellcmd;
	pos_cmd = strstr(shellcmd,"|");

	snprintf(cmd_id,++pos_cmd - pos_id - 1,"%s",++pos_id);

	pos_cmd = ++pos_cmd;

	snprintf(get_info_cmd,30,"%s",pos_cmd);

	is_get_info = strcmp(get_info_cmd,GET_SETTINGS_INFO_CMD";");

	debug(LOG_INFO,"cmd_id:%s,get_inf_cmd:%s,is_get_info cmp:%d",cmd_id,get_info_cmd,is_get_info);

	if (0 == is_get_info){
		get_info_cmd[strlen(get_info_cmd) - 1] = 0;// delete the semicolon it at the tail
		sprintf(get_info_cmd,"%s %s %s",get_info_cmd,gw_id,cmd_id);/* add gw_id and cmd_id to the command as
		                                                               the parameter of the command */
		pft = excute_open(get_info_cmd,"r");
	}else{
		/* if the command is a normal command,just do it.
		 * */
//	  sprintf(normal_cmd,"RESULT=\"$(%s)\";echo \"$RESULT\" > "NORMAL_CMD_RESULT_FILE,pos_cmd);
	  sprintf(normal_cmd,"%s",pos_cmd);
	  pft = excute_open(normal_cmd,"r");
	}

	debug(LOG_INFO,"pos_cmd:%s",pos_cmd);

	if (NULL == pft){
		debug(LOG_WARNING,"excute_shell_command excute_open error....");
		return -1;
	}

//	excute_close(pft);
//
//	if (0 == is_get_info)
//		out_file = safe_strdup(SETTINGS_INFO_FILE);
//    else
//	    out_file = safe_strdup(NORMAL_CMD_RESULT_FILE);
//

//	out_size = get_file_length(out_file);
	out_size = get_file_length(pft->name);

	if(out_size < 0){
		debug(LOG_WARNING, "command out file size error!");
		free(out_file);
		excute_close(pft);
		return -1;
	}
	printf("======\nout_size: %d\n======\n", out_size);
	cmd_result = cmdresult_malloc(out_size);

	if(!cmd_result){

		debug(LOG_WARNING, "cmdresult_malloc error");
		free(out_file);
		excute_close(pft);
		return -1;
	}
	cmd_result->c_size = out_size;

//	outf = fopen(out_file, "r");
//
//	if (!outf){
//
//		debug(LOG_WARNING, "fopen error !");
//		free(out_file);
//		return -1;
//	}
//	fread(cmd_result->result, 1, out_size, outf);
//	fclose(outf);

	excute_read(cmd_result->result, 1, out_size, pft);

	excute_close(pft);
	if (insert_queue(&cmdres_queue, cmd_result) < 0){

		debug(LOG_WARNING, "insert command out to the queue error !");
	}

	free(out_file);

	return 0;
}
