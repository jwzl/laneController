#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "iniparser.h"
#include "common.h"

int load_config_from_file(struct server_context* context, char * ini_name){
	char* tmp = NULL;
    dictionary*   ini ;
	serial_device_t* dev;

    ini = iniparser_load(ini_name);
    if( ini == NULL ) return -1 ;
    //iniparser_dump(ini, stderr);

    /* core config */
	context->keepalive_time = iniparser_getint(ini, "core:keepalive", 3000);
	if(context->keepalive_time < 1000)
		context->keepalive_time = 1000;
	context->timeout = iniparser_getint(ini, "core:response_timeout", 3000);
	if(context->timeout < 1000)
		context->timeout = 1000;
	context->debug = iniparser_getint(ini, "core:debug", 0);
	
    /*railing*/
	context->railing_ctrl_io = iniparser_getint(ini, "railing:ctrl_io", 120);

	/*canopy*/
	context->canopy_ctrl_io = iniparser_getint(ini, "canopy:ctrl_io", 130);
	context->etc_ctrl_io = iniparser_getint(ini, "canopy:etc_io", 140);

	//fee serial port settings.
	dev = &context->devs[FEE_SDEV_IDX];
	tmp = (char*)iniparser_getstring(ini, "fee:dev_node", "/dev/ttyS0");
	if(!tmp) return -1;
	snprintf(dev->device, 62, "%s",tmp);
	dev->baud = iniparser_getint(ini, "fee:baud", 9600);
	dev->data_bit = iniparser_getint(ini, "fee:data_bit", 8);
	dev->parity = iniparser_getstring(ini, "fee:parity", "N")[0];
	dev->stop_bit = iniparser_getint(ini, "fee:stop_bit", 1);
	dev->debug = 1;
	//printf("Fee usbaude serial port [%s]: %d %d%c%d \r\n",dev->device, 
	//		dev->, dev->data_bit, dev->parity, dev->stop_bit);
#if 0
    b = iniparser_getboolean(ini, "pizza:ham", -1);
    printf("Ham:       [%d]\n", b);
#endif

    iniparser_freedict(ini);
    return 0 ;
}
