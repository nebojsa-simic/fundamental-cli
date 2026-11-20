#ifndef NETWORK_SERVER_INTERNAL_H
#define NETWORK_SERVER_INTERNAL_H

#include "fundamental/network/network.h"

#define NETWORK_SERVER_TCP 1
#define NETWORK_SERVER_UDP 2

struct NetworkServerConfig_s {
	NetworkAddress address;
	Memory server_state;
	int server_type;
	void *recv_buffer;
	size_t recv_buffer_size;
	intptr_t listen_fd;
	volatile int stop_flag;
	void *listener;
};

#endif
