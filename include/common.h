#ifndef __COMMON_H___
#define __COMMON_H___

#include <stdint.h>
#include "log.h"
#include "util.h"
#include "serial.h"
#include "socket.h"
#include "protocol.h"
#include "blockedqueue.h"


#define SERIAL_NUM		(8U)

typedef struct {
	uint8_t frame_type;
	void (*func)(void* data, uint8_t *buffer, uint32_t len);
}msg_handler;

struct server_context {
	const char* addr;
	int debug;
	int port;
	int sock;
	int online;
	int keepalive_time;
	int timeout;
	int railing_ctrl_io;
	blocked_queue*  resp_queue;
	serial_device_t	devs[SERIAL_NUM];
	msg_handler*	handlers;
};

#define FEE_SDEV_IDX			(1U)
#define POS_SDEV_IDX			(2U)
#define PRINTER_SDEV_IDX		(3U)


int load_config_from_file(struct server_context* context, char * ini_name);
void railing_machine_init(struct server_context* context);
void serial_devices_init(struct server_context* context);

void do_railing_machine(void* d, uint8_t *buffer, uint32_t len);
void do_canopy_light(void* d, uint8_t *buffer, uint32_t len);
void do_fee_indicator(void* d, uint8_t *buffer, uint32_t len);
void do_pos(void* d, uint8_t *buffer, uint32_t len);
void do_ticket_printer(void* d, uint8_t *buffer, uint32_t len);

#endif
