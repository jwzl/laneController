#ifndef __SERVER_H___
#define __SERVER_H___

#include "common.h"

extern char* cfgfile;
int run_lane_controler(const char* addr, int port);
#endif