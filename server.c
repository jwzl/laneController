#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "thread.h"
#include "socket.h"
#include "server.h"
#include "protocol.h"
#include "common.h"
#include "server.h"


#define DCTIMEOUT	(5000U)

static struct server_context* new_server_context(const char* addr, int port, int sock){
	struct server_context* context = (struct server_context *)malloc(sizeof(struct server_context));

	if(!context) return NULL;
	memset(context, 0, sizeof(struct server_context));
	context->addr = addr;
	context->port = port;
	context->sock = sock;

	//load the config from file.
	if(load_config_from_file(context, "conf/config.ini") < 0 ){ 
		free(context);
		return NULL;
	}
	
	return context;
}

struct message* recv_one_frame(int sock, int timeout){
	int rc;
	uint8_t seq;
	uint32_t length;
	uint8_t buf[65537]={0};
	struct message* msg;

	//recieve the frame start flag
	rc = recv_msg(sock, buf, 2, DCTIMEOUT);
	if(rc < 0){
		return NULL;
	}
	if(buf[0] != 0xFF){
		if(buf[1] != 0xFF){
			return NULL;
		}else{
			rc = recv_msg(sock, &buf[1], 1, DCTIMEOUT);
			if(rc < 0) return NULL;
		}
	}
	if(buf[1] != 0xFF){
		return NULL;
	}
	

	//recieve the version.
	rc = recv_msg(sock, &buf[2], 6, DCTIMEOUT);
	if(rc < 0){
		return NULL;
	}
	if(buf[2] != 0x00){
		return NULL;
	}

	seq = buf[3];
	length = ((uint32_t)buf[6]<<8);
	length |= buf[7];

	msg = new_message(length);
	if(msg == NULL){
		errorf("new message failed \r\n");
		return NULL;
	}
	msg->sequence = seq;
	
	rc = recv_msg(sock, msg->data, length+1, DCTIMEOUT);
	if(rc != length+1){
		destory_message(&msg);
		return NULL;
	}

    if(!check_msg_xor_sum(msg)){
		errorf("invalid xor sum \r\n");
		destory_message(&msg);
		return NULL;
	}

	return msg;
}

void do_rd_wr(struct server_context* context, uint8_t *buffer, uint32_t len){
	//reader/writer
		
}
void do_antenna(struct server_context* context, uint8_t *buffer, uint32_t len){
	//antenna
		
}

void do_serial_info(struct server_context* context, uint8_t *buffer, uint32_t len){
	//antenna
	uint8_t content;
	/* device serial port setting info.*/
	content = buffer[0];
	if(content == 0){
			//get_serial_setting_info(msg);
	}else{
			//currently not  supoort.
	}	
}

#define POS_SERIAL_DEV_IDX	(2U)
void do_pos(struct server_context* context, uint8_t *buffer, uint32_t len){
}

#define PRINTER_SERIAL_DEV_IDX	(3U)
void do_ticket_printer(struct server_context* context, uint8_t *buffer, uint32_t len){
	int res = 0;
	int status = 0;
	struct message* response;
	serial_device_t* dev = &context->devs[PRINTER_SERIAL_DEV_IDX];

	dev->debug = 1;
	
	res = serial_dev_open(dev);
	if(res < 0){
		res = 1;
		goto resp;
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		res = 1;
		goto out;
	}

	//device no response, we set it as 0x00.
	status = 0;

out:
	serial_dev_close(dev);
resp:	
	response = new_ticket_printer_message(res, status);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send railing machine response failed %d \r\n", res);
	}
	destory_message(&response);
}

#define WP_SERIAL_DEV_IDX	(4U)

void do_weighing_platform(struct server_context* context, uint8_t *buffer, uint32_t len){
	int res = 0;
	uint8_t data[128]={0};
	struct message* response;
	serial_device_t* dev = &context->devs[WP_SERIAL_DEV_IDX];

	dev->debug = 1;
	
	res = serial_dev_open(dev);
	if(res < 0){
		res = 1;
		goto resp;
	}

	res = serial_dev_send(dev, buffer, len);
	if(res != len){
		res = 1;
		goto out;
	}

	//response .
	res = serial_dev_recieve(dev, data, 128);
	if(res < 0){
		res = 1;
		goto out;
	}
	len = res;
	res = 0;

out:
	serial_dev_close(dev);
resp:	
	response = new_weighing_platform_message(res, data, len);
	res = send_msg(context->sock, response, length(response));
	if(res != length(response)){
		errorf("send weighing platform response failed %d \r\n", res);
	}
	destory_message(&response);
}

void do_minions(struct server_context* context, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_license_plate(struct server_context* context, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_lane_camera(struct server_context* context, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_sense_coil(struct server_context* context, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_horse_race_lamp(struct server_context* context, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_not_support(struct server_context* context, uint8_t *buffer, uint32_t len){
	infof("Not support. \r\n");
}

/* message handler. */
static const msg_handler handlers[]={
	{0xA0, do_serial_info}, 	//device serial port setting info.
	{0xA1, do_antenna},	//antenna, run on 9001 port.
	{0xA2, do_rd_wr},	//rd_wr, run on 9002 port..
	{0xA3, do_railing_machine},	//Railing machine
	{0xA4, do_fee_indicator},	//Fee Indicator
	{0xA5, do_pos},	// POS
	{0xA6, do_ticket_printer},	//  Ticket printer
	{0xA7, do_weighing_platform},	//  Ticket printer
	{0xA8, do_minions},	//  Minions
	{0xA9, do_license_plate},	//  License plate run 9003. 
	{0xAA, do_lane_camera},	//  Lane camera run 9004. 
	{0xAB, do_canopy_light},	// Canopy light. 
	{0xAC, do_sense_coil},	// sense coil.
	{0xAD, do_horse_race_lamp},	//  Horse race lamp
	{0xE, do_not_support},	//not support.
};

void process_message(struct server_context* context, struct message* msg){
	int idx = 0;
	uint8_t cmd;
	uint32_t len = msg->length;
	uint8_t frame_type = msg->data[0];

	//handler index.
	idx = frame_type - 0xA0;
	idx = idx > 0xF ? 0x0E : idx;

	if(handlers[idx].func != NULL){
		len--;
		handlers[idx].func(context, &msg->data[1], len);
	}
}

static void server_init(struct server_context* context){
	railing_machine_init(context);
}

static void* run_server(void* p){
	int rc;
	struct message* msg;
	struct server_context* context = p;
	int sock = context->sock;
	int timeout = context->timeout;

	//init the server.
	server_init(context);
	
	while(1){
		msg = recv_one_frame(sock, timeout);
		if(msg == NULL){
			continue;
		}

		process_message(context, msg);
		destory_message(&msg);
	}
}

void listen_and_serve(const char* addr, int port){
	int rc;
	int sock;
	struct server_context* context;
	
	rc = start_tcp_listen(addr, port);
	if(rc < 0){
		errorf("listen %s:%d failed with %d \r\n", addr, port, rc);
		return;
	}

	context = new_server_context(addr, port, -1);
	if(!context){
		errorf("failed create server context, please check the conf/config.ini file \r\n");
		return;
	}
	
	while(1){
		sock = do_tcp_accept();
		if(sock < 0){
			errorf("start_tcp_accept failed with %d \r\n", sock);
			continue;
		}
		context->sock = sock;

		Thread_start(run_server, context);
		//wait 3s to avoid many connections.
		sleep(3);
	}
}
