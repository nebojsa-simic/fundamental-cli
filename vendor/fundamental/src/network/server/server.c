#include "fundamental/network/server.h"
#include "fundamental/memory/memory.h"
#include "server_internal.h"

int fun_network_server_arch_tcp_setup(struct NetworkServerConfig_s *config);
int fun_network_server_arch_udp_setup(struct NetworkServerConfig_s *config);
int fun_network_server_arch_tcp_accept(struct NetworkServerConfig_s *config,
									   int timeout_ms, intptr_t *out_fd);
int fun_network_server_arch_udp_recv(struct NetworkServerConfig_s *config,
									 int timeout_ms, NetworkAddress *source,
									 size_t *received);
void fun_network_server_arch_close(struct NetworkServerConfig_s *config);
int fun_network_server_arch_get_port(struct NetworkServerConfig_s *config,
									 uint16_t *out_port);
int fun_network_server_arch_close_connection(intptr_t fd);

static AsyncStatus server_tcp_poll(AsyncResult *result)
{
	struct NetworkServerConfig_s *config =
		(struct NetworkServerConfig_s *)result->state;

	if (config->stop_flag) {
		fun_network_server_arch_close(config);
		result->status = ASYNC_COMPLETED;
		result->error = ERROR_RESULT_NO_ERROR;
		return ASYNC_COMPLETED;
	}

	intptr_t fd = -1;
	int rc = fun_network_server_arch_tcp_accept(config, 500, &fd);
	if (rc < 0) {
		fun_network_server_arch_close(config);
		result->status = ASYNC_ERROR;
		result->error = ERROR_RESULT_NETWORK_SERVER_BIND_FAILED;
		return ASYNC_ERROR;
	}
	if (rc > 0 && fd >= 0) {
		TcpNetworkConnection conn = fun_network_tcp_register_connection(fd);
		if (conn) {
			NetworkTcpListener l = (NetworkTcpListener)config->listener;
			l(conn, config->server_state);
		} else {
			int res_rc = fun_network_server_arch_close_connection(fd);
			(void)res_rc;
		}
	}

	result->status = ASYNC_PENDING;
	return ASYNC_PENDING;
}

static AsyncStatus server_udp_poll(AsyncResult *result)
{
	struct NetworkServerConfig_s *config =
		(struct NetworkServerConfig_s *)result->state;

	if (config->stop_flag) {
		fun_network_server_arch_close(config);
		result->status = ASYNC_COMPLETED;
		result->error = ERROR_RESULT_NO_ERROR;
		return ASYNC_COMPLETED;
	}

	NetworkAddress source;
	size_t received = 0;
	int rc = fun_network_server_arch_udp_recv(config, 500, &source, &received);
	if (rc < 0) {
		fun_network_server_arch_close(config);
		result->status = ASYNC_ERROR;
		result->error = ERROR_RESULT_NETWORK_SERVER_BIND_FAILED;
		return ASYNC_ERROR;
	}
	if (rc > 0) {
		NetworkBuffer nb = { config->recv_buffer, received };
		NetworkUdpListener l = (NetworkUdpListener)config->listener;
		l(source, nb, config->server_state);
	}

	result->status = ASYNC_PENDING;
	return ASYNC_PENDING;
}

CanReturnError(void)
	fun_network_tcp_server_config(NetworkAddress address, Memory server_state,
								  NetworkServerConfig *out_config)
{
	voidResult result;

	if (!out_config) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	MemoryResult mem =
		fun_memory_allocate(sizeof(struct NetworkServerConfig_s));
	if (fun_error_is_error(mem.error)) {
		result.error = mem.error;
		return result;
	}

	struct NetworkServerConfig_s *config =
		(struct NetworkServerConfig_s *)mem.value;
	config->address = address;
	config->server_state = server_state;
	config->server_type = NETWORK_SERVER_TCP;
	config->recv_buffer = (void *)0;
	config->recv_buffer_size = 0;
	config->listen_fd = -1;
	config->stop_flag = 0;

	*out_config = (NetworkServerConfig)config;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void)
	fun_network_udp_server_config(NetworkAddress address, Memory server_state,
								  void *buffer, size_t buffer_size,
								  NetworkServerConfig *out_config)
{
	voidResult result;

	if (!out_config) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (!buffer || buffer_size == 0) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	MemoryResult mem =
		fun_memory_allocate(sizeof(struct NetworkServerConfig_s));
	if (fun_error_is_error(mem.error)) {
		result.error = mem.error;
		return result;
	}

	struct NetworkServerConfig_s *config =
		(struct NetworkServerConfig_s *)mem.value;
	config->address = address;
	config->server_state = server_state;
	config->server_type = NETWORK_SERVER_UDP;
	config->recv_buffer = buffer;
	config->recv_buffer_size = buffer_size;
	config->listen_fd = -1;
	config->stop_flag = 0;

	*out_config = (NetworkServerConfig)config;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void) fun_network_server_config_free(NetworkServerConfig config)
{
	voidResult result;
	if (!config) {
		result.error = ERROR_RESULT_NO_ERROR;
		return result;
	}
	Memory mem = (Memory)config;
	config = (NetworkServerConfig)0;
	fun_memory_free(&mem);
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

AsyncResult fun_network_tcp_listen(NetworkServerConfig config,
								   NetworkTcpListener listener)
{
	AsyncResult result;
	result.poll = (AsyncPollFn)0;
	result.state = (void *)0;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!config || !listener) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (config->server_type != NETWORK_SERVER_TCP) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_SERVER_WRONG_CONFIG_TYPE;
		return result;
	}

	int rc = fun_network_server_arch_tcp_setup(config);
	if (rc < 0) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_SERVER_BIND_FAILED;
		return result;
	}

	config->listener = (void *)listener;
	result.poll = server_tcp_poll;
	result.state = (void *)config;
	result.status = ASYNC_PENDING;
	return result;
}

AsyncResult fun_network_udp_listen(NetworkServerConfig config,
								   NetworkUdpListener listener)
{
	AsyncResult result;
	result.poll = (AsyncPollFn)0;
	result.state = (void *)0;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!config || !listener) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (config->server_type != NETWORK_SERVER_UDP) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_SERVER_WRONG_CONFIG_TYPE;
		return result;
	}

	int rc = fun_network_server_arch_udp_setup(config);
	if (rc < 0) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_SERVER_BIND_FAILED;
		return result;
	}

	config->listener = (void *)listener;
	result.poll = server_udp_poll;
	result.state = (void *)config;
	result.status = ASYNC_PENDING;
	return result;
}

CanReturnError(void) fun_network_server_stop(NetworkServerConfig config)
{
	voidResult result;
	if (!config) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	config->stop_flag = 1;
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}

CanReturnError(void)
	fun_network_server_get_port(NetworkServerConfig config, uint16_t *out_port)
{
	voidResult result;
	if (!config || !out_port) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_server_arch_get_port(config, out_port);
	if (rc < 0) {
		result.error = ERROR_RESULT_NETWORK_INVALID_STATE;
		return result;
	}
	result.error = ERROR_RESULT_NO_ERROR;
	return result;
}
