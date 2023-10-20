#ifndef GPIO_LIB_H_
#define GPIO_LIB_H_

#include <stdbool.h>
bool gpio_is_valid(int number);
int gpio_export(unsigned gpio);
int gpio_unexport(unsigned gpio);
int gpio_direction_output(unsigned gpio);
int gpio_direction_input(unsigned gpio);
int gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
	
#endif
