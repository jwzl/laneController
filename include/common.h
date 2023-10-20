#ifndef __COMMON_H___
#define __COMMON_H___

#include <stdint.h>
#include "log.h"
#include "serial.h"
#include "socket.h"
#include "protocol.h"

#define SERIAL_NUM		(8U)

struct server_context {
	const char* addr;
	int port;
	int sock;
	int timeout;
	int railing_ctrl_io;
	serial_device_t	devs[SERIAL_NUM];
};

typedef struct {
	uint8_t frame_type;
	void (*func)(struct server_context* context, uint8_t *buffer, uint32_t len);
}msg_handler;


#define FEE_SDEV_IDX	(1U)

int load_config_from_file(struct server_context* context, char * ini_name);
void railing_machine_init(struct server_context* context);

void do_railing_machine(struct server_context* context, uint8_t *buffer, uint32_t len);
void do_canopy_light(struct server_context* context, uint8_t *buffer, uint32_t len);
void do_fee_indicator(struct server_context* context, uint8_t *buffer, uint32_t len);
#endif
