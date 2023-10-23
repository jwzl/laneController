#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "common.h"

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

/* test lanectroller. */
int main(int argc, char **argv)
{
	int c, ret;
	int sockfd;
	int port = 9000;
	struct message* msg;
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
	sockfd = connect_to(address, port, 10000);
	if(sockfd < 0){
		errorf("connect %s:%d failed with %d", address, port, sockfd);
		goto retry_connect;
	}

	while(1){
		msg = recv_one_frame(sockfd, 5000);
		if(msg == NULL) continue;

		infof("msg->sequence = %d \r\n", (int)msg->sequence);
		if(msg->sequence == 0x09){
			//build response.
			struct message* resp = new_heartbeat_response();

			send_msg(sockfd, (const uint8_t *)resp, length(resp));
			destory_message(&resp);
			infof("keepalive....\r\n");
		}

		sleep(1);
	}
	
	return 0;
}
