#ifndef __SERVER_H___
#define __SERVER_H___

#include "serial.h"

#define SERIAL_NUM		(8U)

struct server_context {
	const char* addr;
	int port;
	int sock;
	int timeout;
	serial_device_t	devs[SERIAL_NUM];
};

void listen_and_serve(const char* addr, int port);
#endif