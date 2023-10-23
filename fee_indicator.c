#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "common.h"


void do_fee_indicator(void* d, uint8_t *buffer, uint32_t len){
	int res;
	int status = 0;
	struct message* response;
	struct server_context* context = d;
	serial_device_t* dev = &context->devs[FEE_SDEV_IDX];
	
	res = serial_dev_open(dev);
	if(res < 0){
		status = 1;
		goto resp;
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		status = 1;
		goto out;
	}

	//device no response.
	status = 0;

out:
	serial_dev_close(dev);
resp:	
	response = new_fee_indicator_message(status);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		printf("send railing machine response failed %d \r\n", res);
	}
	destory_message(&response);
}
