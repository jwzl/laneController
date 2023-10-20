#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "protocol.h"

static uint8_t seq_count = 0;

struct message* new_message(uint32_t length){
	size_t size = length + sizeof(struct message)+ 1;
	struct message* msg = (struct message *)malloc(size);

	if(msg == NULL) return NULL;

	seq_count++;
	memset(msg, 0, size);
	msg->stx = 0xFFFF;
	msg->version = 0x00;
	msg->sequence = seq_count%9;
	msg->length = length &0xFFFF;
	
	return msg;	
}

int length(struct message* msg){
	return msg->length + sizeof(struct message)+ 1;
}

static uint8_t xor_sum(uint8_t *addr, uint32_t length){
	int i;
	uint8_t sum = addr[0];

	for(i = 1; i < length; i++){
		sum ^= addr[i];
	}

	return sum;
}

int check_msg_xor_sum(struct message* msg){
	uint8_t sum;
	size_t size = sizeof(struct message)-2;
	size += msg->length;

	sum = xor_sum(&msg->version, size);

	return sum == msg->data[msg->length];
}

struct message* new_heartbeat_message(){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(1);

	if(msg == NULL) return NULL;

	msg->sequence = MSG_SEQ_HEARTBEAT;
	msg->data[0] = 0xB0;
	size += msg->length;
	msg->data[1] = xor_sum(&msg->version, size);

	return msg;
}

struct message* new_serial_port_info_message(uint8_t data[20]){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(21);

	if(msg == NULL) return NULL;

	msg->data[0] = 0xA0;
	memcpy(&msg->data[1], data, 20);
	size += msg->length;
	msg->data[22] = xor_sum(&msg->version, size);

	return msg;
}

struct message* new_railing_status_message(uint8_t status){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(2);

	if(msg == NULL) return NULL;

	msg->data[0] = 0xD3;
	msg->data[1] = status;
	
	size += msg->length;
	msg->data[2] = xor_sum(&msg->version, size);

	return msg;
}

struct message* new_fee_indicator_message(uint8_t status){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(2);

	if(msg == NULL) return NULL;

	msg->data[0] = 0xD4;
	msg->data[1] = status;
	
	size += msg->length;
	msg->data[2] = xor_sum(&msg->version, size);

	return msg;
}

struct message* new_ticket_printer_message(uint8_t res, uint8_t status){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(3);

	if(msg == NULL) return NULL;

	msg->data[0] = 0xD6;
	msg->data[1] = res;
	msg->data[2] = status;
	
	size += msg->length;
	msg->data[3] = xor_sum(&msg->version, size);

	return msg;
}

struct message* new_weighing_platform_message(uint8_t res, uint8_t* buffer, int len){
	size_t size = sizeof(struct message)-2;
	struct message* msg = new_message(2+len);

	if(msg == NULL) return NULL;

	msg->data[0] = 0xD6;
	msg->data[1] = res;
	memcpy(&msg->data[2], buffer, len);
	
	size += msg->length;
	msg->data[len+1] = xor_sum(&msg->version, size);

	return msg;
}

void destory_message(struct message** msg){
	if(*msg == NULL){
		free(*msg);
		*msg = NULL;
	}
}

