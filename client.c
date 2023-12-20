#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "common.h"

struct context {
	int sock;
	blocked_queue* resp_queue;
};

static struct option long_options[] = {
	{"address", required_argument, NULL, 'a'},
	{"port", required_argument, NULL, 'p'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0}
};

static void usage(char *programname)
{
	fprintf(stdout, "%s (compiled %s)\n", programname, __DATE__);
	fprintf(stdout, "Usage %s [OPTION]\n", programname);
	fprintf(stdout,
		" -a, --address <address>              : tcp server address.\n"
		" -p, --port <port>              : tcp listen port (9000 default)\n"
		" -h, --help                     : print this help and exit\n"
		);
}

static struct message* recv_one_frame(int sock, int timeout){
	int rc;
	uint8_t seq;
	uint32_t length;
	uint8_t buf[65537]={0};
	struct message* msg;

	//recieve the frame start flag
	rc = recv_msg(sock, buf, 2, timeout);
	if(rc < 0){
		return NULL;
	}
	if(buf[0] != 0xFF){
		if(buf[1] != 0xFF){
			return NULL;
		}else{
			rc = recv_msg(sock, &buf[1], 1, timeout);
			if(rc < 0) return NULL;
		}
	}
	if(buf[1] != 0xFF){
		return NULL;
	}
	

	//recieve the version.
	rc = recv_msg(sock, &buf[2], 6, timeout);
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
	
	rc = recv_msg(sock, msg->data, length+1, timeout);
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

static struct message* send_request(struct context* ctx, struct message* req){
	int len;
	int ret;
	int s = ctx->sock;
	struct message* resp;

	len = length(req);
	ret = send_msg(s, (const uint8_t *)req, len);
	if(ret != len){
		return NULL;
	}

	resp = blocked_queue_pop(ctx->resp_queue, 5000);
	if(resp == NULL) return NULL;

	return resp;
}
static void* test_loop(void* p){
	struct context* ctx = p;
	int s = ctx->sock;
	uint8_t data[20]={0x01, 0x02,0x03,0x04,0x05,0x66};
	struct message* resp;
	struct message* req = new_serial_info_request(0x55);

	resp = send_request(ctx, req);
	if(!resp){
		errorf("get serial info failed\r\n");
	}
	destory_message(&req);
	if(resp) destory_message(&resp);


	req = new_railing_request(1);
	resp = send_request(ctx, req);
	if(!resp){
		errorf("get railing failed\r\n");
	}
	destory_message(&req);
	infof("status = %x \r\n", resp->data[1]);
	if(resp) destory_message(&resp);

	infof("fee_indicator \r\n");
	req = new_fee_indicator_request(data, 20);
	resp = send_request(ctx, req);
	if(!resp){
		errorf("fee_indicator failed\r\n");
	}
	destory_message(&req);
	infof("fee_indicator status = %x \r\n", resp->data[1]);
	if(resp) destory_message(&resp);

	req = new_canopy_request(1);
	resp = send_request(ctx,req);
	if(!resp){
		errorf("canopy failed\r\n");
	}
	destory_message(&req);
	infof("canopy status = %x \r\n", resp->data[1]);
	if(resp) destory_message(&resp);
		
	util_sleep_v2(5000);

	
}

/* test lanectroller. */
int main(int argc, char **argv)
{
	int c, ret;
	int sockfd;
	int port = 9000;
	struct message* msg;
	struct context ctx;
	char* address = "127.0.0.1";
	char main_options[256] = {"a:p:h"};
	
	while ((c = getopt_long(argc, argv, main_options,
					long_options, NULL)) != EOF) {
		switch (c) {
		case 'p':
			port = strtoul(optarg, NULL, 10);
			break;
		case 'a':
			address = strdup(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			fprintf(stdout, "Try %s -h for usage\n", argv[0]);
			return -1;
		}
	}

retry_connect:
	printf("-----------------\n");
	sockfd = connect_to(address, port, 10000);
	if(sockfd < 0){
		errorf("connect %s:%d failed with %d", address, port, sockfd);
		goto retry_connect;
	}

	ctx.sock = sockfd;
	ctx.resp_queue = blocked_queue_init();
	Thread_start(test_loop, &ctx);
	while(1){
		msg = recv_one_frame(sockfd, 5000);
		if(msg == NULL) continue;

		if(msg->sequence == 0x09){
			//build response.
			struct message* resp = new_heartbeat_response();

			send_msg(sockfd, (const uint8_t *)resp, length(resp));
			destory_message(&resp);
			continue;
		}

	    infof("msg->sequence = %d \r\n", (int)msg->sequence);
		blocked_queue_push(ctx.resp_queue, msg, 50);
	}
	
	return 0;
}
