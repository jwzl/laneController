#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>

#ifndef __packed
#define __packed	__attribute__((packed))
#endif

#define MSG_SEQ_HEARTBEAT			(0x09)
#define MSG_SEQ_RSP_HEARTBEAT		(0x90)

/* comunicate message type.*/
struct message {
	/* frame start flag,  = 0xFFFF*/
	uint16_t stx;
	/* protocol version, currenly is 0x00 */
	uint8_t  version;
	/*
	* 1. server message sequence number is 0x0X, 
	*  X = 1, 2, 3, 4, 5, 6, 7, 8
	* 2. client message sequence number is 0xX0, 
	* X = 1, 2, 3, 4, 5, 6, 7, 8
	*/
	uint8_t  sequence;
	/*
	*  byte 0 & 1 are reserved, for version 0, 
	* byte3 & byte4 for data length.
	*/
	uint32_t length;

	/* data + BC*/
	uint8_t  data[0]; 
} __packed;

struct message* new_message(uint32_t length);
struct message* new_heartbeat_message();
void destory_message(struct message** msg);
int check_msg_xor_sum(struct message* msg);
int length(struct message* msg);
struct message* new_railing_status_message(uint8_t status);
struct message* new_fee_indicator_message(uint8_t status);
struct message* new_ticket_printer_message(uint8_t res, uint8_t status);
struct message* new_weighing_platform_message(uint8_t res, uint8_t* buffer, int len);
#endif