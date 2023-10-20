#ifndef _SERIAL_H___
#define _SERIAL_H___

#include <termios.h> /* POSIX terminal control definitions */
#include <sys/time.h>

//typedef int ssize_t;
typedef unsigned char  uint8_t;

#define  DEVICE_NAME_SIZE		(16UL)
#define  MAX_RESPONSE_TIME		(500000UL)

typedef struct _serial_device {
    int debug;	
    /* Device: "/dev/ttyS0", "/dev/ttyUSB0" or "/dev/tty.USA19*" on Mac OS X. */
    char device[64];
    int s;	 /* Socket or file descriptor */
    /* Bauds: 9600, 19200, 57600, 115200, etc */
    int baud;
    /* Data bit */
    uint8_t data_bit;
    /* Stop bit */
    uint8_t stop_bit;
    /* Parity: 'N', 'O', 'E' */
    char parity;
    /* Save old termios settings */
    struct termios old_tios;
   /* reponse timeout*/	
    struct timeval  response_timeout;	
} serial_device_t;


serial_device_t  * serial_dev_init(char *device, int baud, uint8_t data_bit, uint8_t stop_bit, char parity);

void  serial_dev_close(serial_device_t  *dev);
int serial_dev_open(serial_device_t  *dev);
ssize_t  serial_dev_send(serial_device_t  *dev,  uint8_t *data,  ssize_t length);

int serial_dev_recieve(serial_device_t  *dev,  uint8_t *data, int length_to_read);

int check_stm32_status(char * cmd);

#endif /* _SERIAL_COMMON_H */
