#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "server.h"

static struct option long_options[] = {
	{"port", required_argument, NULL, 'p'},
	{"file", required_argument, NULL, 'f'},
	{"version", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0}
};

static void usage(char *programname)
{
	fprintf(stdout, "%s (compiled %s)\n", programname, __DATE__);
	fprintf(stdout, "Usage %s [OPTION]\n",
			programname);
	fprintf(stdout,
		" -p, --port <port>              : tcp listen port (9000 default)\n"
		" -f, --file <file path>   	: the config file (conf/config.ini default)\n"
		" -v, --version                  : program version \n"
		" -h, --help                     : print this help and exit\n"
		);
}

int main(int argc, char **argv)
{
	int c, ret;
	int port = 9000;
	char* cfg;
	char main_options[256] = {"p:f:vh"};

	while ((c = getopt_long(argc, argv, main_options,
					long_options, NULL)) != EOF) {
		switch (c) {
		case 'p':
			port = strtoul(optarg, NULL, 10);
			break;
		case 'f':
			cfg = strdup(optarg);
			if(access(cfg, F_OK)==0){ 
				cfgfile = cfg;
			}else{
				warningf("%s is not exist, we use default(conf/config.ini)! \r\n", cfg);
			}
			break;
		case 'v':
			fprintf(stdout, "version: 0.1.0\n");
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			fprintf(stdout, "Try %s -h for usage\n", argv[0]);
			return -1;
		}
	}

	
	fprintf(stdout, "============ %s (compiled %s) =========== \n", argv[0], __DATE__);
	ret = run_lane_controler("0.0.0.0", port);
	if(ret < 0){
		errorf("failed create server context, please check the conf/config.ini file \r\n");
		return -1;
	}
	
	return 0;
}
