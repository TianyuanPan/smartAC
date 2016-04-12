/*
 * smartac_version.c
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */


#include "smartac_version.h"


static char _version[][3] = {
		{0x01},
		{0x00},
		{0x02}
};

static char version[32];

static void _get_version()
{
     sprintf(version,"Verion %d.%d.%d", atoi(_version[0][0]),
    		 atoi(_version[0][1]),
    		 atoi(_version[0][2]));
}

char *get_version()
{
	_get_version();
    return version;
}

