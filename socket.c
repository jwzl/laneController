#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

static int sockfd = -1;

/*
* create socket and listen the port.
*/
int start_tcp_listen(const char* addr, int port){
	int rc = -1;
	int sock = -1;
	char *addr_mem;
	char service[10];
	size_t addr_len = strlen(addr);
	struct addrinfo *result = NULL;
	struct addrinfo *rp =NULL;
	struct addrinfo hints;

	if(addr[0] == '['){
		++addr;
		--addr_len;
	}
	addr_mem = malloc(addr_len + 1u);
	if(addr_mem == NULL){
		return -99;
	}

	memcpy(addr_mem, addr, addr_len);
	addr_mem[addr_len] = '\0';

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	snprintf(service, 10, "%d", port);
	rc = getaddrinfo(addr_mem, service, &hints, &result);
	if (rc){
		printf("Error getaddrinfo: %s.", gai_strerror(rc));
		goto free_alloc;
	}

	rp = result;
	while(rp){
		/* prefer ip4 addresses */
		if (rp->ai_family == AF_INET || rp->ai_next == NULL)
			break;
		rp = rp->ai_next;
	}

	if(rp == NULL){
		rc = -1;
		goto free_result;
	}
	
	sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if(sock == -1){
		printf("Error: socket create failed %s \r\n", strerror(errno));
		rc = -1;
		goto free_result;
	}

	rc = bind(sock, rp->ai_addr, rp->ai_addrlen);
	if(rc == -1){
		if(errno == EACCES){
			printf("If you are trying to bind to a privileged port (<1024), try using setcap and do not start the server as root:");
			printf("    sudo setcap 'CAP_NET_BIND_SERVICE=+ep /usr/sbin/devctl'");
		}
		printf("bind error %s \r\n", strerror(errno));
		goto free_socket;
	}

	rc = listen(sock, 10);
	if(rc == -1){
		printf("listen error %s \r\n", strerror(errno));
		goto free_socket;
	}

	sockfd = sock;

	free(addr_mem);
	freeaddrinfo(result);

	printf("Server listen on %s:%d \r\n", addr, port);
	
	return 0;

free_socket:
	close(sock);
free_result:
	freeaddrinfo(result);
free_alloc:
	free(addr_mem);
	return rc;
}


int set_socket_nonblock(int sock){
	int rc;
#if defined(_WIN32) || defined(_WIN64)
	u_long flag = 1L;
	
	rc = ioctl(sock, FIONBIO, &flag);
#else
	int flags;
	
	if ((flags = fcntl(sock, F_GETFL, 0)))
		flags = 0;
	rc = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
	return rc;
}

int do_tcp_accept(void){
	int sock;
	struct sockaddr_in addr;
    socklen_t addrlen;
	char buf[INET_ADDRSTRLEN];

	if(sockfd < 0) {
		printf("invalid sockfd \r\n");
		return -1;
	}

	addrlen = sizeof(addr);
	sock = accept(sockfd, (struct sockaddr *) &addr, &addrlen);
	if(sock < 0){
		return -1;	
	}

	if (inet_ntop(AF_INET, &(addr.sin_addr), buf, INET_ADDRSTRLEN) == NULL) {
		printf("Client connection accepted from unparsable IP.\n");
	} else {
		printf("Client connection accepted from %s.\n", buf);
	}

	//set socket as nonblock.
	if(set_socket_nonblock(sock) != 0){
		return -1;
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
			printf("A non blocked signal was caught\n");
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
