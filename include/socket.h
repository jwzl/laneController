#ifndef __SOCKET_H__
#define __SOCKET_H__

int start_tcp_listen(const char* addr, int port);
int do_tcp_accept(void);
ssize_t send_msg(int sock, const void *msg, int msg_length);
int recv_msg(int sock, uint8_t *msg, uint32_t length, int timeout);
#endif