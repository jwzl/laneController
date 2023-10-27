#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "common.h"

void do_ticket_printer(void* d, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	struct message* response;
	struct server_context* context = d;
	serial_device_t* dev = &context->devs[PRINTER_SDEV_IDX];
	
	if(dev->s < 0 ){
		res = serial_dev_open(dev);
		if(res < 0){
			dev->s = -1;
			status = 1;
			goto resp;
		}
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		status = 1;
		res = -1;
		goto resp;
	}

	//TODO: has response
	status = 0;
resp:	
	response = new_ticket_printer_message(res, status);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send railing machine response failed %d \r\n", res);
	}
	destory_message(&response);
	if(res < 0 && dev->s > 0){
		serial_dev_close(dev);
		dev->s = -1;
	}
}

void do_pos(void* d, uint8_t *buffer, uint32_t len){
	int res, status = 0;
	uint8_t data[1024]={0};
	struct message* response;
	struct server_context* context = d;
	serial_device_t* dev = &context->devs[POS_SDEV_IDX];

	if(dev->s < 0 ){
		res = serial_dev_open(dev);
		if(res < 0){
			dev->s = -1;
			status = 1;
			goto resp;
		}
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		status = 1;
		res = -1;
		goto resp;
	}

	//TODO: device  recieve the data.
	status = 0;
resp:
	response = new_pos_response(status, data, 128);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send pos machine response failed %d \r\n", res);
	}
	destory_message(&response);
	if(res < 0 && dev->s > 0){
		serial_dev_close(dev);
		dev->s = -1;
	}
}

void do_fee_indicator(void* d, uint8_t *buffer, uint32_t len){
	int res, status = 0;
	struct message* response;
	struct server_context* context = d;
	serial_device_t* dev = &context->devs[FEE_SDEV_IDX];

	if(dev->s < 0 ){
		res = serial_dev_open(dev);
		if(res < 0){
			dev->s = -1;
			status = 1;
			goto resp;
		}
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		status = 1;
		res = -1;
		goto resp;
	}

	//device no response.
	status = 0;
resp:
	response = new_fee_indicator_message(status);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send railing machine response failed %d \r\n", res);
	}
	destory_message(&response);
	if(res < 0 && dev->s > 0){
		serial_dev_close(dev);
		dev->s = -1;
	}
}

static void device_init(struct server_context* context, int idx){
	int res;
	serial_device_t* dev = &context->devs[idx];

	res = serial_dev_open(dev);
	if(res < 0){
		dev->s = -1;
		errorf("open %s failed. \r\n", dev->device);
	}
}

void serial_devices_init(struct server_context* context){
	device_init(context, FEE_SDEV_IDX);
	device_init(context, POS_SDEV_IDX);
	device_init(context, PRINTER_SDEV_IDX);
}
