#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char **argv)
{
	printf("======== Start device ctl server=============== \r\n");
	listen_and_serve("0.0.0.0", 9000);

	return 0;
}
