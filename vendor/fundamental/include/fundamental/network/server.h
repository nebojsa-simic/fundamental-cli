#ifndef LIBRARY_NETWORK_SERVER_H
#define LIBRARY_NETWORK_SERVER_H

#include <stddef.h>

#include "../async/async.h"
#include "../error/error.h"
#include "../memory/memory.h"
#include "network.h"

struct NetworkServerConfig_s;
typedef struct NetworkServerConfig_s *NetworkServerConfig;

typedef void (*NetworkTcpListener)(TcpNetworkConnection conn,
								   Memory server_state);
typedef void (*NetworkUdpListener)(NetworkAddress source, NetworkBuffer buffer,
								   Memory server_state);

CanReturnError(void)
	fun_network_tcp_server_config(NetworkAddress address, Memory server_state,
								  NetworkServerConfig *out_config);

CanReturnError(void)
	fun_network_udp_server_config(NetworkAddress address, Memory server_state,
								  void *buffer, size_t buffer_size,
								  NetworkServerConfig *out_config);

CanReturnError(void) fun_network_server_config_free(NetworkServerConfig config);

AsyncResult fun_network_tcp_listen(NetworkServerConfig config,
								   NetworkTcpListener listener);

AsyncResult fun_network_udp_listen(NetworkServerConfig config,
								   NetworkUdpListener listener);

CanReturnError(void) fun_network_server_stop(NetworkServerConfig config);

CanReturnError(void)
	fun_network_server_get_port(NetworkServerConfig config, uint16_t *out_port);

#endif
