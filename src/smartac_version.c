/*
 * smartac_version.c
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */


#include "smartac_version.h"


static char _version[][3] = {
		{"000"},
		{"000"},
		{"001"}
};

static char version[32];

static void _get_version()
{
     sprintf(version,"Ver %d.%d.%d", atoi(_version[0][0]),
    		 atoi(_version[0][1]),
    		 atoi(_version[0][2]));
}

char *get_version()
{
	_get_version();
    return version;
}

