#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include "gpio.h"

#define GPIO_SYS_PATH "/sys/class/gpio"

#ifndef  ARCH_NR_GPIOS
#define ARCH_NR_GPIOS  1024
#endif

bool gpio_is_valid(int number)
{
	return number >= 0 && number < ARCH_NR_GPIOS;
}

/* Export the gpio to sysfs node */
int gpio_export(unsigned gpio)
{
    FILE *fp = NULL;
	char b_path[128]={0};
	int ret = 0;	

	if (!gpio_is_valid(gpio)) {
		ret = -EINVAL;
		goto done;
	}

    snprintf(b_path, sizeof(b_path), "%s/export",GPIO_SYS_PATH);
	fp = fopen(b_path, "w");
	if (fp == NULL) {
		return -1;
	}

    ret = fprintf(fp,"%d\n", gpio);
	fclose (fp);

done: 
	return ret;
}

/**
 * gpio_unexport - reverse effect of gpio_export()
 * @gpio: gpio to make unavailable
 *
 * This is implicit on gpio_free().
 */
int gpio_unexport(unsigned gpio)
{
    int ret = 0;
	FILE *fp = NULL;
	char b_path[128]={0};

	if (!gpio_is_valid(gpio)) {
		ret = -EINVAL;
		goto done;
	}

    snprintf(b_path, sizeof(b_path), "%s/unexport",GPIO_SYS_PATH);
	fp = fopen(b_path, "w");
	if (fp == NULL) {
		return -1;
	}

    ret = fprintf(fp,"%d\n", gpio);
	fclose (fp);

done: 
	return ret;
}

int gpio_direction_output(unsigned gpio){
	FILE *fp = NULL;
	int status = 0;
	char b_path[128]={0};

	if (!gpio_is_valid(gpio)) {
		status = -1;
		goto done;
	}

	snprintf(b_path, sizeof(b_path), "%s/gpio%d/direction",GPIO_SYS_PATH, gpio);
	fp = fopen(b_path, "w");
	if (fp == NULL) {
		return -1;
	}

	fprintf (fp,"out\n");
	fclose (fp);

done:
	return status;	
}

int gpio_direction_input(unsigned gpio){
	FILE *fp = NULL;
	int status = 0;
	char b_path[128]={0};

	if (!gpio_is_valid(gpio)) {
		status = -1;
		goto done;
	}

	snprintf(b_path, sizeof(b_path), "%s/gpio%d/direction",GPIO_SYS_PATH, gpio);
	fp = fopen(b_path, "w");
	if (fp == NULL) {
		return -1;
	}

	fprintf (fp,"in\n");
	fclose (fp);

done:
	return status;	
}

int gpio_set_value(unsigned gpio, int value)
{
	FILE *fp = NULL;
	int status = 0;
	char b_path[128]={0};

	if (!gpio_is_valid(gpio)) {
		status = -1;
		goto done;
	}

	//Set value
	snprintf(b_path, sizeof(b_path), "%s/gpio%d/value",GPIO_SYS_PATH, gpio);
	fp = fopen(b_path, "w");
	if (fp == NULL) {
		return -1;
	}

	fprintf(fp,"%d\n", value);
	fclose (fp);

done:
	return status;	
}

int gpio_get_value(unsigned gpio) {
	int value = -1;
	FILE *fp = NULL;
	int status = 0;
	char b_path[128]={0};

	if (!gpio_is_valid(gpio)) {
		return -1;
	}

	snprintf(b_path, sizeof(b_path), "%s/gpio%d/value",GPIO_SYS_PATH, gpio);
	fp = fopen(b_path, "r");
	if (fp == NULL) {
		return -1;
	}

	fscanf (fp,"%d", &value);
	fclose (fp);
	return value;
}
