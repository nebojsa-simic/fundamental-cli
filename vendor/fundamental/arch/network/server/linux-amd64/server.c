#include "fundamental/network/server.h"
#include "fundamental/memory/memory.h"
#include "../../../../src/network/server/server_internal.h"

#define SYS_socket 41
#define SYS_bind 49
#define SYS_listen 50
#define SYS_accept4 288
#define SYS_setsockopt 54
#define SYS_recvfrom 45
#define SYS_close 3
#define SYS_poll 7
#define SYS_getsockname 51
#define SYS_fcntl 72

#define AF_INET 2
#define AF_INET6 10
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define TCP_NODELAY 1
#define O_NONBLOCK 2048
#define F_SETFL 4
#define SOMAXCONN 128
#define POLLIN 1
#define EINTR 4
#define EAGAIN 11

struct sockaddr_in {
	uint16_t sin_family;
	uint16_t sin_port;
	uint32_t sin_addr;
	uint64_t sin_zero;
};

struct sockaddr_in6 {
	uint16_t sin6_family;
	uint16_t sin6_port;
	uint32_t sin6_flowinfo;
	uint8_t sin6_addr[16];
	uint32_t sin6_scope_id;
};

struct pollfd {
	int fd;
	short events;
	short revents;
};

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5,
							long a6)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
						   "r"(r9)
						 : "rcx", "r11", "memory");
	return ret;
}

static uint16_t htons_host(uint16_t val)
{
	return (uint16_t)((val >> 8) | (val << 8));
}

static int set_nonblocking(int fd)
{
	long flags = syscall3(SYS_fcntl, fd, F_SETFL, O_NONBLOCK);
	return (flags < 0) ? -1 : 0;
}

int fun_network_server_arch_tcp_setup(struct NetworkServerConfig_s *config)
{
	int domain = config->address.family == NETWORK_ADDRESS_IPV4 ? AF_INET :
																  AF_INET6;
	long fd = syscall3(SYS_socket, domain, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return -1;

	int reuse = 1;
	syscall6(SYS_setsockopt, fd, SOL_SOCKET, SO_REUSEADDR, (long)&reuse,
			 sizeof(reuse), 0);

	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} sa;
	int sa_len;

	if (config->address.family == NETWORK_ADDRESS_IPV4) {
		sa.in.sin_family = AF_INET;
		sa.in.sin_port = htons_host(config->address.port);
		sa.in.sin_addr = *(uint32_t *)config->address.bytes;
		for (int i = 0; i < 8; i++)
			((uint8_t *)&sa.in.sin_zero)[i] = 0;
		sa_len = sizeof(sa.in);
	} else {
		sa.in6.sin6_family = AF_INET6;
		sa.in6.sin6_port = htons_host(config->address.port);
		sa.in6.sin6_flowinfo = 0;
		for (int i = 0; i < 16; i++)
			sa.in6.sin6_addr[i] = config->address.bytes[i];
		sa.in6.sin6_scope_id = 0;
		sa_len = sizeof(sa.in6);
	}

	if (syscall3(SYS_bind, fd, (long)&sa, sa_len) < 0) {
		syscall3(SYS_close, fd, 0, 0);
		return -1;
	}

	if (syscall3(SYS_listen, fd, SOMAXCONN, 0) < 0) {
		syscall3(SYS_close, fd, 0, 0);
		return -1;
	}

	set_nonblocking((int)fd);

	config->listen_fd = fd;
	return 0;
}

int fun_network_server_arch_udp_setup(struct NetworkServerConfig_s *config)
{
	int domain = config->address.family == NETWORK_ADDRESS_IPV4 ? AF_INET :
																  AF_INET6;
	long fd = syscall3(SYS_socket, domain, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
		return -1;

	int reuse = 1;
	syscall6(SYS_setsockopt, fd, SOL_SOCKET, SO_REUSEADDR, (long)&reuse,
			 sizeof(reuse), 0);

	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} sa;
	int sa_len;

	if (config->address.family == NETWORK_ADDRESS_IPV4) {
		sa.in.sin_family = AF_INET;
		sa.in.sin_port = htons_host(config->address.port);
		sa.in.sin_addr = *(uint32_t *)config->address.bytes;
		for (int i = 0; i < 8; i++)
			((uint8_t *)&sa.in.sin_zero)[i] = 0;
		sa_len = sizeof(sa.in);
	} else {
		sa.in6.sin6_family = AF_INET6;
		sa.in6.sin6_port = htons_host(config->address.port);
		sa.in6.sin6_flowinfo = 0;
		for (int i = 0; i < 16; i++)
			sa.in6.sin6_addr[i] = config->address.bytes[i];
		sa.in6.sin6_scope_id = 0;
		sa_len = sizeof(sa.in6);
	}

	if (syscall3(SYS_bind, fd, (long)&sa, sa_len) < 0) {
		syscall3(SYS_close, fd, 0, 0);
		return -1;
	}

	set_nonblocking((int)fd);

	config->listen_fd = fd;
	return 0;
}

int fun_network_server_arch_tcp_accept(struct NetworkServerConfig_s *config,
									   int timeout_ms, intptr_t *out_fd)
{
	struct pollfd pfd;
	pfd.fd = (int)config->listen_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	long rc = syscall6(SYS_poll, (long)&pfd, 1, timeout_ms, 0, 0, 0);
	if (rc < 0) {
		if (rc == -EINTR)
			return 0;
		return -1;
	}
	if (rc == 0)
		return 0;

	long client =
		syscall6(SYS_accept4, config->listen_fd, 0, 0, O_NONBLOCK, 0, 0);
	if (client < 0) {
		if (client == -EAGAIN)
			return 0;
		return -1;
	}

	int nodelay = 1;
	syscall6(SYS_setsockopt, client, IPPROTO_TCP, TCP_NODELAY, (long)&nodelay,
			 sizeof(nodelay), 0);

	*out_fd = client;
	return 1;
}

int fun_network_server_arch_udp_recv(struct NetworkServerConfig_s *config,
									 int timeout_ms, NetworkAddress *source,
									 size_t *received)
{
	struct pollfd pfd;
	pfd.fd = (int)config->listen_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	long rc = syscall6(SYS_poll, (long)&pfd, 1, timeout_ms, 0, 0, 0);
	if (rc < 0) {
		if (rc == -EINTR)
			return 0;
		return -1;
	}
	if (rc == 0)
		return 0;

	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} from;
	int from_len = sizeof(from);

	long n = syscall6(SYS_recvfrom, config->listen_fd,
					  (long)config->recv_buffer, (long)config->recv_buffer_size,
					  0, (long)&from, (long)&from_len);
	if (n < 0) {
		if (n == -EAGAIN)
			return 0;
		return -1;
	}

	if (from.in.sin_family == AF_INET) {
		source->family = NETWORK_ADDRESS_IPV4;
		*(uint32_t *)source->bytes = from.in.sin_addr;
		source->port = htons_host(from.in.sin_port);
	} else {
		source->family = NETWORK_ADDRESS_IPV6;
		for (int i = 0; i < 16; i++)
			source->bytes[i] = from.in6.sin6_addr[i];
		source->port = htons_host(from.in6.sin6_port);
	}

	*received = (size_t)n;
	return 1;
}

void fun_network_server_arch_close(struct NetworkServerConfig_s *config)
{
	if (config->listen_fd != -1) {
		syscall3(SYS_close, config->listen_fd, 0, 0);
		config->listen_fd = -1;
	}
}

int fun_network_server_arch_get_port(struct NetworkServerConfig_s *config,
									 uint16_t *out_port)
{
	if (config->listen_fd < 0)
		return -1;

	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} sa;
	int sa_len = sizeof(sa);

	long rc =
		syscall3(SYS_getsockname, config->listen_fd, (long)&sa, (long)&sa_len);
	if (rc < 0)
		return -1;

	if (sa.in.sin_family == AF_INET)
		*out_port = htons_host(sa.in.sin_port);
	else
		*out_port = htons_host(sa.in6.sin6_port);

	return 0;
}

int fun_network_server_arch_close_connection(intptr_t fd)
{
	syscall3(SYS_close, fd, 0, 0);
	return 0;
}
