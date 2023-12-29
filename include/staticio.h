#ifndef __STATICIO_H___
#define __STATICIO_H___

#include "bdaqctrl.h"
#include "compatibility.h"

int io_set_value(int32_t port, uint8 data);
int io_get_value(int32 portStart, int32 portCount, uint8 data[]);

#endif

