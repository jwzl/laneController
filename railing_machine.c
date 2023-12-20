#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "thread.h"
#include "socket.h"
#include "gpio.h"
#include "common.h"

#define RAILING_CTRL_IO (112U)
#define CANOPY_CTRL_IO (113U)
#define ETC_CTRL_IO (114U)

void do_railing_machine(void* data, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	uint8_t cmd = buffer[0];
	struct message* response;
	struct server_context* context = data;
	unsigned gpio = context->railing_ctrl_io;

	if(cmd == 0){ //Down
		infof("GPIO DOWN \r\n");
		gpio_set_value(gpio, 0);
	}else{//Up
		infof("GPIO UP \r\n");
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

static int canopy_set_gpio(void* data, unsigned int val){
	int ret = 0;
	struct server_context* context = data;

	switch(val){
		case 1:
			gpio_set_value(context->canopy_ctrl_io,1);
			gpio_set_value(context->etc_ctrl_io,0);
			ret = val;
			break;
		case 2:
			gpio_set_value(context->canopy_ctrl_io,0);
			gpio_set_value(context->etc_ctrl_io,0);
			ret = val;
			break;
		case 3:
            		//gpio_set_value(context->canopy_ctrl_io,1);
			gpio_set_value(context->etc_ctrl_io,1);
			ret = val;
			break;
	}

	return ret;
}

void do_canopy_light(void* data, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	int canopy = 0;
	int etc = 0;
	uint8_t cmd = buffer[0];
	struct message* response;
	struct server_context* context = data;
    unsigned int gpio_cny = context->canopy_ctrl_io;
    unsigned int gpio_etc = context->etc_ctrl_io;

	/*compare addr valu and gpio*/	
	switch(cmd){
		case 0:
			break;
		case 1:
			//canopy red up + etc down
			canopy_set_gpio(context,1);
			break;
		case 2:
			//canopy green up + etc down
			canopy_set_gpio(context,2);
			break;
		case 3:
			//etc up 
			canopy_set_gpio(context,3);
			break;
	}
	
    //get the gpio status.
    etc = gpio_get_value(gpio_etc);
	canopy = gpio_get_value(gpio_cny);
    switch(etc){
        case 1:
            status = 0x03;
			break;
        case 0:
			if(canopy) status = 0x01;
            else status = 0x02;
            break;
    }

    //build & response.
    response = new_canopy_status_message(status);
    res = send_msg(context->sock, response, length(response));
    if(res != length(response)){
        printf("send canopy machine response failed %d \r\n", res);
    }
    destory_message(&response);
}

void railing_machine_init(struct server_context* context){
	unsigned gpio;

	if(context->railing_ctrl_io <= 0)
		context->railing_ctrl_io = RAILING_CTRL_IO;
	gpio = context->railing_ctrl_io;
	gpio_direction_output(gpio);
	
	if(context->canopy_ctrl_io <= 0)
		context->canopy_ctrl_io = CANOPY_CTRL_IO;
	gpio_direction_output(gpio);

	if(context->etc_ctrl_io<= 0)
		context->etc_ctrl_io= ETC_CTRL_IO;
	gpio_direction_output(gpio);
}

