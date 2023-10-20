/*
* Qing refered the modbus project.
*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>
#include <pthread.h>
#include "serial.h"

int serial_dev_open(serial_device_t  *dev)	
{
	struct termios tios;
	speed_t speed;
	int flags;
	
	if (dev->debug) {
		printf("Opening %s at %d bauds (%c, %d, %d)\n",
			dev->device, dev->baud, dev->parity,
				   dev->data_bit, dev->stop_bit);
	}

  	/* The O_NOCTTY flag tells UNIX that this program doesn't want
       to be the "controlling terminal" for that port. If you
       don't specify this then any input (such as keyboard abort
       signals and so forth) will affect your process

       Timeouts are ignored in canonical input mode or when the
       NDELAY option is set on the file via open or fcntl */
	flags = O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
    	flags |= O_CLOEXEC;
#endif

	dev->s = open(dev->device, flags);
  	if (dev->s == -1) {
		if (dev->debug) {
		   fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
				   dev->device, strerror(errno));
	   	}
	   return -1;
   	}

	 /* Save */
	tcgetattr(dev->s, &dev->old_tios);
    memset(&tios, 0, sizeof(struct termios));

	/* C_ISPEED     Input baud (new interface)
       		C_OSPEED     Output baud (new interface)
    	*/
	switch (dev->baud) {
   	case 110:
		speed = B110;
       	break;
	case 300:
    	speed = B300;
       	break;
   	case 600:
       	speed = B600;
       	break;
   	case 1200:
       	speed = B1200;
       	break;
    case 2400:
       	speed = B2400;
       	break;
    case 4800:
       	speed = B4800;
       	break;
    case 9600:
       	speed = B9600;
       	break;
    case 19200:
       	speed = B19200;
       	break;
    case 38400:
       	speed = B38400;
       	break;
#ifdef B57600
    case 57600:
       	speed = B57600;
       	break;
#endif
#ifdef B115200
    case 115200:
       	speed = B115200;
       	break;
#endif
#ifdef B230400
    case 230400:
       	speed = B230400;
       	break;
#endif
#ifdef B460800
    case 460800:
       	speed = B460800;
       	break;
#endif
#ifdef B500000
    case 500000:
       	speed = B500000;
       	break;
#endif
#ifdef B576000
    case 576000:
       	speed = B576000;
       	break;
#endif
#ifdef B921600
    case 921600:
       	speed = B921600;
       	break;
#endif
#ifdef B1000000
    case 1000000:
       	speed = B1000000;
       	break;
#endif
#ifdef B1152000
 	case 1152000:
       	speed = B1152000;
       	break;
#endif
#ifdef B1500000
    case 1500000:
       	speed = B1500000;
       	break;
#endif
#ifdef B2500000
    case 2500000:
       	speed = B2500000;
       	break;
#endif
#ifdef B3000000
    case 3000000:
       	speed = B3000000;
       	break;
#endif
#ifdef B3500000
    case 3500000:
       	speed = B3500000;
       	break;
#endif
#ifdef B4000000
   case 4000000:
       	speed = B4000000;
       	break;
#endif
   	default:
       	speed = B9600;
       	if (dev->debug) {
           		fprintf(stderr,
                 	 "WARNING Unknown baud rate %d for %s (B9600 used)\n",
                   	dev->baud, dev->device);
     	}
    }

	/* Set the baud rate */
   	if ((cfsetispeed(&tios, speed) < 0) ||
		   (cfsetospeed(&tios, speed) < 0)) {
	   	close(dev->s);
	   	dev->s = -1;
	   	return -1;
   	}
			
	
	/* C_CFLAG		Control options
	   CLOCAL		Local line - do not change "owner" of port
	   CREAD		Enable receiver
	*/
	tios.c_cflag |= (CREAD | CLOCAL);
	/* CSIZE, HUPCL, CRTSCTS (hardware flow control) */
	/* Set data bits (5, 6, 7, 8 bits)
	   CSIZE		Bit mask for data bits
	*/
	tios.c_cflag &= ~CSIZE;
	switch (dev->data_bit) {
	case 5:
		tios.c_cflag |= CS5;
		break;
	case 6:
		tios.c_cflag |= CS6;
		break;
	case 7:
		tios.c_cflag |= CS7;
		break;
	case 8:
	default:
		tios.c_cflag |= CS8;
		break;
	}

	
	/* Stop bit (1 or 2) */
	  if (dev->stop_bit == 1)
		   tios.c_cflag &=~ CSTOPB;
	   else /* 2 */
		   tios.c_cflag |= CSTOPB;
	
	/* PARENB	   Enable parity bit
		  PARODD	   Use odd parity instead of even */
	  if (dev->parity == 'N') {
	   	/* None */
	  	tios.c_cflag &=~ PARENB;
	   } else if (dev->parity == 'E') {
		   /* Even */
		   tios.c_cflag |= PARENB;
		   tios.c_cflag &=~ PARODD;
	   } else {
		   /* Odd */
		   tios.c_cflag |= PARENB;
		   tios.c_cflag |= PARODD;
	   }

	   
	   /* Read the man page of termios if you need more information. */
	   
	   /* This field isn't used on POSIX systems
	  	tios.c_line = 0;
	   */
	   
	   /* C_LFLAG	   Line options   
	  	ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
		ICANON	   Enable canonical input (else raw)
		XCASE 	   Map uppercase \lowercase (obsolete)
		ECHO Enable echoing of input characters
		ECHOE 	   Echo erase character as BS-SP-BS
		ECHOK 	   Echo NL after kill character
		ECHONL	   Echo NL
		NOFLSH	   Disable flushing of input buffers after
					 interrupt or quit characters
		IEXTEN	   Enable extended functions
		ECHOCTL	   Echo control characters as ^char and delete as ~?
		ECHOPRT	   Echo erased character as character erased
		ECHOKE	   BS-SP-BS entire line on line kill
		FLUSHO	   Output being flushed
		PENDIN	   Retype pending input at next read or input char
		TOSTOP	   Send SIGTTOU for background output
	   
		Canonical input is line-oriented. Input characters are put
		into a buffer which can be edited interactively by the user
		until a CR (carriage return) or LF (line feed) character is
		received.
	   
		Raw input is unprocessed. Input characters are passed
		through exactly as they are received, when they are
		received. Generally you'll deselect the ICANON, ECHO,
		ECHOE, and ISIG options when using raw input
	     */
	   
	/* Raw input */
	tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	   
	/* C_IFLAG	   Input options
	   
	  Constant	   Description
	  INPCK 	   Enable parity check
	  IGNPAR	   Ignore parity errors
	  PARMRK	   Mark parity errors
	  ISTRIP	   Strip parity bits
	  IXON Enable software flow control (outgoing)
	  IXOFF 	   Enable software flow control (incoming)
	  IXANY 	   Allow any character to start flow again
	  IGNBRK	   Ignore break condition
	  BRKINT	   Send a SIGINT when a break condition is detected
	  INLCR 	   Map NL to CR
	  IGNCR 	   Ignore CR
	  ICRNL 	   Map CR to NL
	  IUCLC 	   Map uppercase to lowercase
	  IMAXBEL	   Echo BEL on input line too long
	  */
	if (dev->parity == 'N') {
		   /* None */
		tios.c_iflag &= ~INPCK;
	} else {
		tios.c_iflag |= INPCK;
	 }

	/* Software flow control is disabled */
   	tios.c_iflag &= ~(IXON | IXOFF | IXANY);

   	/* C_OFLAG	   Output options
	  OPOST 	   Postprocess output (not set = raw output)
	  ONLCR 	   Map NL to CR-NL

	  ONCLR ant others needs OPOST to be enabled
   	*/

   	/* Raw ouput */
   	tios.c_oflag &=~ OPOST;

   	/* C_CC		   Control characters
	  VMIN		   Minimum number of characters to read
	  VTIME 	   Time to wait for data (tenths of seconds)

	  UNIX serial interface drivers provide the ability to
	  specify character and packet timeouts. Two elements of the
	  c_cc array are used for timeouts: VMIN and VTIME. Timeouts
	  are ignored in canonical input mode or when the NDELAY
	  option is set on the file via open or fcntl.

	  VMIN specifies the minimum number of characters to read. If
	  it is set to 0, then the VTIME value specifies the time to
	  wait for every character read. Note that this does not mean
	  that a read call for N bytes will wait for N characters to
	  come in. Rather, the timeout will apply to the first
	  character and the read call will return the number of
	  characters immediately available (up to the number you
	  request).

	  If VMIN is non-zero, VTIME specifies the time to wait for
	  the first character read. If a character is read within the
	  time given, any read will block (wait) until all VMIN
	  characters are read. That is, once the first character is
	  read, the serial interface driver expects to receive an
	  entire packet of characters (VMIN bytes total). If no
	  character is read within the time allowed, then the call to
	  read returns 0. This method allows you to tell the serial
	  driver you need exactly N bytes and any read call will
	  return 0 or N bytes. However, the timeout only applies to
	  the first character read, so if for some reason the driver
	  misses one character inside the N byte packet then the read
	  call could block forever waiting for additional input
	  characters.

	  VTIME specifies the amount of time to wait for incoming
	  characters in tenths of seconds. If VTIME is set to 0 (the
	  default), reads will block (wait) indefinitely unless the
	  NDELAY option is set on the port with open or fcntl.
   	*/
  	 /* Unused because we use open with the NDELAY option */
   	tios.c_cc[VMIN] = 0;
   	tios.c_cc[VTIME] = 0;

	if (tcsetattr(dev->s, TCSANOW, &tios) < 0) {
		close(dev->s);
	   	dev->s = -1;
	   	return -1;
   	}

	 return 0;	   
}

static ssize_t serial_dev_recv(serial_device_t  *dev, uint8_t *buf, int length)
{
	return  read(dev->s, buf, length);
}
static int serial_dev_select(serial_device_t  *dev, fd_set *rset,  struct timeval *tv)
{
	int s_rc;

	 while ((s_rc = select(dev->s+1, rset, NULL, NULL, tv)) == -1) {
		 if (errno == EINTR) {
          		if (dev->debug) {
                		fprintf(stderr, "A non blocked signal was caught\n");
            		}
            		/* Necessary after an error */
            		FD_ZERO(rset);
            		FD_SET(dev->s, rset);
        	} else {
            		return -1;
        	}
	 }

	if (s_rc == 0) {
        	/* Timeout */
        	errno = ETIMEDOUT;
        	return -1;
    	}

	return s_rc;
}

serial_device_t  * serial_dev_init(char *device, int baud, uint8_t data_bit, uint8_t stop_bit, char parity)
{
	serial_device_t  * dev;

	dev = (serial_device_t  *)malloc(sizeof(serial_device_t));
	if(!dev)
		return NULL;
	strncpy(dev->device, device, strlen(device));
	dev->device[strlen(device)] = '\0';	
	dev->baud = baud;
	dev->data_bit = data_bit;
	dev->stop_bit = stop_bit;
	dev->parity = parity;
	dev->debug = 1;
	dev->s = -1;
	dev->response_timeout.tv_sec =0;
	dev->response_timeout.tv_usec = MAX_RESPONSE_TIME;

	if(serial_dev_open(dev) != 0){
		fprintf(stderr, " Can't open this serial device correctly\r\n");
		free(dev);
		return NULL;
	}

	return dev;
}
void  serial_dev_close(serial_device_t  *dev)
{
	close(dev->s);
}
ssize_t  serial_dev_send(serial_device_t  *dev,  uint8_t *data,  ssize_t length)
{
	 return write(dev->s, data, length);
}

int serial_dev_recieve(serial_device_t  *dev,  uint8_t *data, int length)
{
	fd_set rset;
	struct timeval tv;
    struct timeval *p_tv;
	ssize_t msg_len = 0;	
	int length_to_read = length;
	int rc;	
	 
	/* Add a file descriptor to the set */
	FD_ZERO(&rset);
	FD_SET(dev->s, &rset);

	if(dev->response_timeout.tv_sec==0 && dev->response_timeout.tv_usec ==0){
		p_tv = NULL;
	}else{
		tv.tv_sec = dev->response_timeout.tv_sec;
	    tv.tv_usec = dev->response_timeout.tv_usec;
        p_tv = &tv;
	}

	/*Read all required bytes.*/
	while (length_to_read != 0) {
		rc = serial_dev_select(dev, &rset, p_tv);
		if(rc == -1){
			fprintf(stderr, " select timeout/other error\r\n");
			return msg_len > 0 ? msg_len : -1;
		}

		rc = serial_dev_recv(dev, data+msg_len,  length_to_read);
		if (rc == 0) {
            errno = ECONNRESET;
            rc = -1;
        }
		if (rc < 0) {
			return msg_len > 0 ? msg_len : rc;
		}
		
		msg_len +=rc;
		length_to_read  -=rc;

		if(dev->response_timeout.tv_sec==0 && dev->response_timeout.tv_usec ==0){
			p_tv = NULL;
		}else{
			tv.tv_sec = dev->response_timeout.tv_sec;
        	tv.tv_usec = dev->response_timeout.tv_usec;
       		p_tv = &tv;
		}
	}

	return msg_len;
}
