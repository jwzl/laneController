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

static int idx = 0;
char* cfgfile = "conf/config.ini";
static struct server_context  ctx_pools[10]={0};

static struct server_context* new_server_context(const char* addr, int port, msg_handler* handlers){
	struct server_context* context = &ctx_pools[idx++];

	if(!context) return NULL;
	memset(context, 0, sizeof(struct server_context));
	context->addr = addr;
	context->port = port;
	context->handlers= handlers;
	context->resp_queue = blocked_queue_init();

	//load the config from file.
	if(load_config_from_file(context, cfgfile) < 0 ){ 
		idx--;
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
	printf("\r\n");
	return msg;
}

void do_rd_wr(void* data, uint8_t *buffer, uint32_t len){
	//reader/writer
		
}
void do_antenna(void* data, uint8_t *buffer, uint32_t len){
	//antenna
		
}

void do_serial_info(void* data, uint8_t *buffer, uint32_t len){
	//antenna
	uint8_t content;
	/* device serial port setting info.*/
	content = buffer[0];
	if(content == 0){
			//get_serial_setting_info(msg);
	}else{
			//currently not  supoort.
	}

	infof("SSERIAL INFO  GET \r\n");
}

#define WP_SERIAL_DEV_IDX	(4U)

void do_weighing_platform(void* d, uint8_t *buffer, uint32_t len){
	int res = 0;
	uint8_t data[128]={0};
	struct message* response;
	struct server_context* context = d;
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

void do_minions(void* data, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_license_plate(void* data, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_lane_camera(void* data, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_sense_coil(void* data, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_horse_race_lamp(void* data, uint8_t *buffer, uint32_t len){
	//TODO:
}

void do_not_support(void* data, uint8_t *buffer, uint32_t len){
	infof("Not support. \r\n");
}

/* message handler. */
static msg_handler handlers[]={
	{0xA0, do_serial_info}, 	//device serial port setting info.
	{0xA3, do_railing_machine},	//Railing machine
	{0xA4, do_fee_indicator},	//Fee Indicator
	{0xA5, do_pos},	// POS
	{0xA6, do_ticket_printer},	//  Ticket printer
	{0xA7, do_weighing_platform},	//  Ticket printer
	{0xA8, do_minions},	//  Minions
	{0xAB, do_canopy_light},	// Canopy light. 
	{0xAC, do_sense_coil},	// sense coil.
	{0xAD, do_horse_race_lamp},	//  Horse race lamp
	{0xEE, do_not_support},	//not support.
};
static msg_handler antenna_handlers[]={
	{0xA1, do_antenna},	//antenna, run on 9001 port.
	{0xEE, do_not_support},	//not support.
};
static msg_handler rd_wr_handlers[]={
	{0xA2, do_rd_wr},	//rd_wr, run on 9002 port..
	{0xEE, do_not_support},	//not support.
};
static msg_handler license_plate_handlers[]={
	{0xA9, do_license_plate},	//  License plate run 9003. 
	{0xEE, do_not_support},	//not support.
};
static msg_handler lane_camera_handlers[]={
	{0xAA, do_lane_camera},	//  Lane camera run 9004. 
	{0xEE, do_not_support},	//not support.
};

void process_message(struct server_context* context, struct message* msg){
	int i = 0;
	msg_handler* h;
	uint32_t len = be2cpu(msg->length);
	uint8_t frame_type = msg->data[0];
	msg_handler* handlers = context->handlers;

	/* this is heartbeat response. */
	if(msg->sequence == 0x90){
		blocked_queue_push(context->resp_queue, msg, 50);
		return;
	}
	
	if(handlers == NULL){
		errorf("NULL handler for %s:%d server \r\n", context->addr, context->port);
		return;
	}

	i = 0;
	while(1){
		h = &handlers[i++];
		if(h->frame_type == frame_type){
			if(h->func){
				len--;
				h->func(context, &msg->data[1], len);
			}
			break;
		}else if(h->frame_type == 0xEE){
			if(h->func){
				len--;
				h->func(context, &msg->data[1], len);
			}
			break;
		}
	}	
}

int send_heartbeat(struct server_context* context){
	int ret;
	int len;
	int sock = context->sock;
	int timeout = context->timeout;
	struct message* resp = NULL;
	struct message* req = new_heartbeat_request();

	len = length(req);
	ret = send_msg(sock, (const uint8_t *)req, len);
	destory_message(&req);
	if(ret != len){
		return -1;
	}

	resp = blocked_queue_pop(context->resp_queue, timeout);
	if(resp == NULL) return -2;
	destory_message(&resp);
	return 0;
}

static void* do_heart_beat(void* p){
	int i = 0;
	struct server_context* context = p;
	int keepalive = context->keepalive_time;

	while(1){
		if(send_heartbeat(context) < 0){
			if(i < 3) {
				i++;
				util_sleep_v2(1000);
			}else{
				warningf("offline, we lost the connect, please retry to connect! \r\n");
				context->online = 0;
				break;
			}
		}else{
			i = 0;
			util_sleep_v2(keepalive);
		}
	}

	return NULL;
}

static void server_init(struct server_context* context){
	railing_machine_init(context);
	serial_devices_init(context);
	Thread_start(do_heart_beat, context);
}

static void* server_loop(void* p){
	int rc;
	struct message* msg;
	struct server_context* context = p;
	int sock = context->sock;
	int timeout = context->timeout;

	//init the server.
	server_init(context);
	
	while(1){
		if(context->online == 0) break;

		msg = recv_one_frame(sock, timeout);
		if(msg == NULL){
			util_sleep_v2(300);
			continue;
		}

		process_message(context, msg);
		destory_message(&msg);
	}
}

static int listen_and_serve(struct server_context* context){
	int rc;
	int sock;
	const char* addr = context->addr;
	int port = context->port;
	
	rc = start_tcp_listen(addr, port);
	if(rc < 0){
		errorf("listen %s:%d failed with %d \r\n", addr, port, rc);
		return -1;
	}

	while(1){
		sock = do_tcp_accept(rc);
		if(sock < 0){
			errorf("start_tcp_accept failed with %d \r\n", sock);
			continue;
		}

		context->sock = sock;
		context->online = 1;
		Thread_start(server_loop, context);
		//wait 3s to avoid many connections.
		sleep(3);
	}

	return 0;
}

static void* run_server(void* p){
	int ret;
	struct server_context* context = p;

retry_start:
	ret = listen_and_serve(context);
	if(ret < 0 ){
		goto retry_start;
	}

	return NULL;
}

int run_lane_controler(const char* addr, int port){
	struct server_context* context;

	infof("======== Start laneController =============== \r\n");
	context = new_server_context(addr, port+1, antenna_handlers);
	if(!context) return -1;
	Thread_start(run_server, context);

	context = new_server_context(addr, port+2, rd_wr_handlers);
	if(!context) return -1;
	Thread_start(run_server, context);
	
	context = new_server_context(addr, port+3, license_plate_handlers);
	if(!context) return -1;
	Thread_start(run_server, context);
	
	context = new_server_context(addr, port+4, lane_camera_handlers);
	if(!context) return -1;
	Thread_start(run_server, context);

	context = new_server_context(addr, port, handlers);
	if(!context) return -1;
	run_server(context);

	return 0;
}
