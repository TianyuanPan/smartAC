/*
 * smartac_remotecmd.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_REMOTECMD_H_
#define SMARTAC_REMOTECMD_H_

char *get_remote_shell_command(char *cmdptr);

int   excute_remote_shell_command(char *gw_id, char *shellcmd);

#endif /* SMARTAC_REMOTECMD_H_ */
