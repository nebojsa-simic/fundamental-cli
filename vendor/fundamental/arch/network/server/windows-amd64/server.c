#include "fundamental/network/server.h"
#include "fundamental/memory/memory.h"
#include "../../../../src/network/server/server_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

static int s_wsa_ready = 0;

static void ensure_wsa(void)
{
	if (s_wsa_ready)
		return;
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	s_wsa_ready = 1;
}

static int addr_to_sockaddr(NetworkAddress addr, struct sockaddr_storage *out,
							int *out_len)
{
	for (size_t i = 0; i < sizeof(*out); i++)
		((unsigned char *)out)[i] = 0;

	if (addr.family == NETWORK_ADDRESS_IPV4) {
		struct sockaddr_in *sa = (struct sockaddr_in *)out;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(addr.port);
		sa->sin_addr.s_addr = *(uint32_t *)addr.bytes;
		*out_len = sizeof(struct sockaddr_in);
	} else if (addr.family == NETWORK_ADDRESS_IPV6) {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;
		sa->sin6_family = AF_INET6;
		sa->sin6_port = htons(addr.port);
		for (int i = 0; i < 16; i++)
			sa->sin6_addr.u.Byte[i] = addr.bytes[i];
		*out_len = sizeof(struct sockaddr_in6);
	} else {
		return -1;
	}
	return 0;
}

static int set_nonblocking(SOCKET fd)
{
	unsigned long mode = 1;
	return ioctlsocket(fd, FIONBIO, &mode);
}

int fun_network_server_arch_tcp_setup(struct NetworkServerConfig_s *config)
{
	ensure_wsa();

	SOCKET fd = socket(
		config->address.family == NETWORK_ADDRESS_IPV4 ? AF_INET : AF_INET6,
		SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET)
		return -1;

	int reuse = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
			   sizeof(reuse));

	struct sockaddr_storage sa;
	int sa_len;
	if (addr_to_sockaddr(config->address, &sa, &sa_len) < 0) {
		closesocket(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sa, sa_len) == SOCKET_ERROR) {
		closesocket(fd);
		return -1;
	}

	if (listen(fd, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(fd);
		return -1;
	}

	set_nonblocking(fd);

	config->listen_fd = (intptr_t)fd;
	return 0;
}

int fun_network_server_arch_udp_setup(struct NetworkServerConfig_s *config)
{
	ensure_wsa();

	SOCKET fd = socket(
		config->address.family == NETWORK_ADDRESS_IPV4 ? AF_INET : AF_INET6,
		SOCK_DGRAM, IPPROTO_UDP);
	if (fd == INVALID_SOCKET)
		return -1;

	int reuse = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
			   sizeof(reuse));

	struct sockaddr_storage sa;
	int sa_len;
	if (addr_to_sockaddr(config->address, &sa, &sa_len) < 0) {
		closesocket(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sa, sa_len) == SOCKET_ERROR) {
		closesocket(fd);
		return -1;
	}

	set_nonblocking(fd);

	config->listen_fd = (intptr_t)fd;
	return 0;
}

int fun_network_server_arch_tcp_accept(struct NetworkServerConfig_s *config,
									   int timeout_ms, intptr_t *out_fd)
{
	SOCKET fd = (SOCKET)config->listen_fd;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	struct timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	int rc = select(0, &readfds, (fd_set *)0, (fd_set *)0, &tv);
	if (rc == SOCKET_ERROR)
		return -1;
	if (rc == 0)
		return 0;

	SOCKET client = accept(fd, (struct sockaddr *)0, (int *)0);
	if (client == INVALID_SOCKET) {
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;
		return -1;
	}

	int nodelay = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay,
			   sizeof(nodelay));

	set_nonblocking(client);

	*out_fd = (intptr_t)client;
	return 1;
}

int fun_network_server_arch_udp_recv(struct NetworkServerConfig_s *config,
									 int timeout_ms, NetworkAddress *source,
									 size_t *received)
{
	SOCKET fd = (SOCKET)config->listen_fd;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	struct timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	int rc = select(0, &readfds, (fd_set *)0, (fd_set *)0, &tv);
	if (rc == SOCKET_ERROR)
		return -1;
	if (rc == 0)
		return 0;

	struct sockaddr_storage from;
	int from_len = sizeof(from);
	int n = recvfrom(fd, (char *)config->recv_buffer,
					 (int)config->recv_buffer_size, 0, (struct sockaddr *)&from,
					 &from_len);
	if (n == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;
		if (err == WSAEMSGSIZE) {
			/* Datagram was truncated to buffer size */
			n = (int)config->recv_buffer_size;
		} else {
			return -1;
		}
	}

	if (from.ss_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)&from;
		source->family = NETWORK_ADDRESS_IPV4;
		*(uint32_t *)source->bytes = sin->sin_addr.s_addr;
		source->port = ntohs(sin->sin_port);
	} else if (from.ss_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&from;
		source->family = NETWORK_ADDRESS_IPV6;
		for (int i = 0; i < 16; i++)
			source->bytes[i] = sin6->sin6_addr.u.Byte[i];
		source->port = ntohs(sin6->sin6_port);
	}

	*received = (size_t)n;
	return 1;
}

void fun_network_server_arch_close(struct NetworkServerConfig_s *config)
{
	if (config->listen_fd != -1) {
		closesocket((SOCKET)config->listen_fd);
		config->listen_fd = -1;
	}
}

int fun_network_server_arch_get_port(struct NetworkServerConfig_s *config,
									 uint16_t *out_port)
{
	SOCKET fd = (SOCKET)config->listen_fd;
	if (fd == INVALID_SOCKET || fd == (SOCKET)-1)
		return -1;

	struct sockaddr_storage sa;
	int sa_len = sizeof(sa);
	if (getsockname(fd, (struct sockaddr *)&sa, &sa_len) == SOCKET_ERROR)
		return -1;

	if (sa.ss_family == AF_INET)
		*out_port = ntohs(((struct sockaddr_in *)&sa)->sin_port);
	else if (sa.ss_family == AF_INET6)
		*out_port = ntohs(((struct sockaddr_in6 *)&sa)->sin6_port);
	else
		return -1;

	return 0;
}

int fun_network_server_arch_close_connection(intptr_t fd)
{
	closesocket((SOCKET)fd);
	return 0;
}
