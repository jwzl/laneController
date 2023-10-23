#if defined(_WIN32)
#define OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
#ifndef WINVER
#define WINVER 0x0501
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>
#if defined(_WIN32)
/* Already set in modbus-tcp.h but it seems order matters in VS2005 */
#include <winsock2.h>
#include <ws2tcpip.h>
#define SHUT_RDWR 2
#define close closesocket
#define strdup _strdup
#else
#include <sys/socket.h>
#include <sys/ioctl.h>

#if defined(__OpenBSD__) || (defined(__FreeBSD__) && __FreeBSD__ < 5)
#define OS_BSD
#include <netinet/in_systm.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include "log.h"

#ifdef OS_WIN32
static int __tcp_init_win32(void)
{
    /* Initialise Windows Socket API */
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        errorf("WSAStartup() returned error code %d\n",
                (unsigned int) GetLastError());
        errno = EIO;
        return -1;
    }
    return 0;
}
#endif

/*
* create socket and listen the port.
*/
int start_tcp_listen(const char* addr, int port){
	int rc = -1;
	int sockfd = -1;
	char *node = NULL;
	char service[10];
	size_t addr_len = strlen(addr);
	struct addrinfo *ai_list = NULL;
	struct addrinfo *ai_ptr =NULL;
	struct addrinfo hints;

#ifdef OS_WIN32
    if (__tcp_init_win32() == -1) {
        return -1;
    }
#endif

	if(addr[0] == '['){
		++addr;
		--addr_len;
	}
	node = malloc(addr_len + 1u);
	if(node == NULL) return -99;

	memcpy(node, addr, addr_len);
	node[addr_len] = '\0';
	snprintf(service, 10, "%d", port);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
#ifdef AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

	ai_list = NULL;
	rc = getaddrinfo(node, service, &hints, &ai_list);
	if (rc){
		errorf("Error getaddrinfo: %s.", gai_strerror(rc));
		errno = ECONNREFUSED;
		free(node);
		return -1;
	}

	sockfd = -1;
	for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        int flags = ai_ptr->ai_socktype;
        int s;

#ifdef SOCK_CLOEXEC
		flags |= SOCK_CLOEXEC;
#endif

		s = socket(ai_ptr->ai_family, flags, ai_ptr->ai_protocol);
		if (s < 0) {
			continue;
		} else {
			int enable = 1;
			rc=	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *) &enable, sizeof(enable));
			if (rc != 0) {
				close(s);
				continue;
			}
		}

		rc = bind(s, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (rc != 0) {
            close(s);
            continue;
        }

		rc = listen(s, 10);
        if (rc != 0) {
            close(s);
            continue;
        }

		sockfd = s;
		break;
	} 

	if (sockfd < 0) {
        return -1;
    }
 
	free(node);
	freeaddrinfo(ai_list);

	infof("Server listen on %s:%d \r\n", addr, port);
	
	return sockfd; 
} 

int do_tcp_accept(int sockfd){
	int sock;
	struct sockaddr_in addr;
    socklen_t addrlen;
	char buf[INET_ADDRSTRLEN];

	if(sockfd < 0) {
		errorf("invalid sockfd \r\n");
		return -1;
	}

	addrlen = sizeof(addr);
	sock = accept(sockfd, (struct sockaddr *) &addr, &addrlen);
	if(sock < 0){
		return -1;	
	}

	if (inet_ntop(AF_INET, &(addr.sin_addr), buf, INET_ADDRSTRLEN) == NULL) {
		errorf("Client connection accepted from unparsable IP.\n");
	} else {
		infof("Client connection accepted from %s.\n", buf);
	}
	
	return sock;
}

ssize_t s_send(int sock, const uint8_t *msg, int msg_length){
    return send(sock, (const char *) msg, msg_length, MSG_NOSIGNAL);
}

static ssize_t s_recv(int sock, uint8_t *msg, int msg_length){
    return recv(sock, (char *) msg, msg_length, 0);
}

static int s_select(int sock, fd_set* rset, struct timeval *tv){
    int s_rc;

    while ((s_rc = select(sock + 1, rset, NULL, NULL, tv)) == -1) {
        if (errno == EINTR) {
			errorf("A non blocked signal was caught\n");
            /* Necessary after an error */
            FD_ZERO(rset);
            FD_SET(sock, rset);
        } else {
            return -1;
        }
    }

    if (s_rc == 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    return s_rc;
}

int recv_msg(int sock, uint8_t *msg, uint32_t length, int timeout){
	int rc, i;
    fd_set rset;
    struct timeval tv;
    struct timeval *p_tv;
	int msg_length = 0;
    unsigned int length_to_read = length;
	

	/* Add a file descriptor to the set */
    FD_ZERO(&rset);
    FD_SET(sock, &rset);

	tv.tv_sec = timeout/1000;
    tv.tv_usec = (timeout%1000)*1000;
    p_tv = &tv;

	while (length_to_read != 0) {
		rc = s_select(sock, &rset, p_tv);
		if (rc == -1) {
			return msg_length > 0 ? msg_length : -1;
		}

		rc = s_recv(sock, msg + msg_length, length_to_read);
		if (rc == 0) {
            errno = ECONNRESET;
            rc = -1;
        }
		if (rc < 0) {
			return msg_length > 0 ? msg_length : rc;
		}

        for(i = 0; i < rc; i++)
        	printf("<%.2X>", msg[msg_length + i]);

		/* Sums bytes received */
        msg_length += rc;
        /* Computes remaining bytes */
        length_to_read -= rc;

		if (length_to_read > 0){
			/* If there is no character in the buffer, the allowed timeout
               		 *  interval between two consecutive bytes is 1s. 
               		 */
			tv.tv_sec = 1;
    		tv.tv_usec = 0;
    		p_tv = &tv;
		}
	}

	return msg_length;
}

ssize_t send_msg(int sock, const void *msg, int msg_length){
    return send(sock, (const char *) msg, msg_length, MSG_NOSIGNAL);
}

static int __tcp_set_ipv4_options(int s)
{
    int rc;
    int option;

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const void *) &option, sizeof(int));
    if (rc == -1) return -1;

    /* If the OS does not offer SOCK_NONBLOCK, fall back to setting FIONBIO to
     * make sockets non-blocking */
    /* Do not care about the return value, this is optional */
#if !defined(SOCK_NONBLOCK) && defined(FIONBIO)
#ifdef OS_WIN32
    {
        /* Setting FIONBIO expects an unsigned long according to MSDN */
        u_long loption = 1;
        ioctlsocket(s, FIONBIO, &loption);
    }
#else
    option = 1;
    ioctl(s, FIONBIO, &option);
#endif
#endif

#ifndef OS_WIN32
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    rc = setsockopt(s, IPPROTO_IP, IP_TOS, (const void *) &option, sizeof(int));
    if (rc == -1) return -1;
#endif

    return 0;
}

/* we connect use non-block method  to connect. */
static int _connect(int sockfd, struct sockaddr *addr, 
			socklen_t addrlen, struct timeval *ro_tv)
{
    int rc = connect(sockfd, addr, addrlen);

#ifdef OS_WIN32
    int wsaError = 0;
    if (rc == -1) {
        wsaError = WSAGetLastError();
    }

    if (wsaError == WSAEWOULDBLOCK || wsaError == WSAEINPROGRESS)
#else
    if (rc == -1 && errno == EINPROGRESS)
#endif
	{
        fd_set wset;
        int optval;
        socklen_t optlen = sizeof(optval);
        struct timeval tv = *ro_tv;

        /* Wait to be available in writing */
        FD_ZERO(&wset);
        FD_SET(sockfd, &wset);
        rc = select(sockfd + 1, NULL, &wset, NULL, &tv);
        if (rc <= 0) {
            /* Timeout or fail */
            return -1;
        }

        /* The connection is established if SO_ERROR and optval are set to 0 */
        rc = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
        if (rc == 0 && optval == 0) {
            return 0;
        } else {
            errno = ECONNREFUSED;
            return -1;
        }
    }

    return rc;
}

/* tcp client connect the tcp server. */
int connect_to(char* address, int port, int timeout){
	int sock;
	int result;
	struct timeval tv;
	int flags = SOCK_STREAM;
	struct sockaddr_in remoteAddr;

#ifdef OS_WIN32
    if (__tcp_init_win32() == -1) {
        return -1;
    }
#endif

#ifdef SOCK_CLOEXEC
	flags |= SOCK_CLOEXEC;
#endif
#ifdef SOCK_NONBLOCK
	flags |= SOCK_NONBLOCK;
#endif
	tv.tv_sec = timeout/1000;	//default connect timeout.
    tv.tv_usec = timeout%1000;
	
	//create tcp client connection.
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons((unsigned short)(port & 0xFFFF));
	result = inet_pton(AF_INET, address, &(remoteAddr.sin_addr));
	if (result <= 0) {
        errorf("Invalid IP address: %s\n", address);
        return -2;
    }

	sock = socket(AF_INET, flags, 0);
	if(sock < 0) return -3;

	result = __tcp_set_ipv4_options(sock);
    if (result == -1) {
        close(sock);
        return -4;
    }

	result = _connect(sock, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr), &tv);
	if(result < 0){
		errorf("connect %s:%d error.\n", address, port);
		close(sock);
		return result;
	}

	infof("connecting to %s:%d successfully...\r\n", inet_ntoa(remoteAddr.sin_addr), port);
	return sock;
}
