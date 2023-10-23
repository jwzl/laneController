#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "thread.h"
#include "socket.h"
#include "gpio.h"
#include "common.h"

#define RAILING_CTRL_IO (112U)

void do_railing_machine(void* data, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	uint8_t cmd = buffer[0];
	struct message* response;
	struct server_context* context = data;
	unsigned gpio = context->railing_ctrl_io;

	if(cmd == 0){ //Down
		gpio_set_value(gpio, 0);
	}else{//Up
		gpio_set_value(gpio, 1);
	}

	//get the gpio status.
	status = gpio_get_value(gpio);
	switch(status){
	case 1:
		status = 0x02;
		break;
	case 0:
		status = 0x03;
		break;
	default:
		status = 0x01;
		break;
	}

	//build & response.
	response = new_railing_status_message(status);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send railing machine response failed %d \r\n", res);
	}
	destory_message(&response);
}

void do_canopy_light(void* data, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	uint8_t cmd = buffer[0];
	struct message* response;
	struct server_context* context = data;
	
	/*switch(cmd){
	case 0:
		break;
	case 1:
	case 2:
	case 3:
	}*/
	
		//get the gpio status.
		status = gpio_get_value(RAILING_CTRL_IO);
		switch(status){
		case 1:
			status = 0x02;
			break;
		case 0:
			status = 0x03;
			break;
		default:
			status = 0x01;
			break;
		}
	
		//build & response.
		response = new_railing_status_message(status);
		res = send_msg(context->sock, response, length(response));
		if(res != length(response)){
			printf("send railing machine response failed %d \r\n", res);
		}
		destory_message(&response);
}

void railing_machine_init(struct server_context* context){
	unsigned gpio;

	if(context->railing_ctrl_io <= 0)
		context->railing_ctrl_io = RAILING_CTRL_IO;
	gpio = context->railing_ctrl_io;
	
	gpio_direction_output(gpio);
}
