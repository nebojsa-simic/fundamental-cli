#include <stddef.h>
#include <stdint.h>

#include "fundamental/network/network.h"

/* ---- Syscall numbers ---- */
#define SYS_close 3
#define SYS_poll 7
#define SYS_socket 41
#define SYS_connect 42
#define SYS_sendto 44
#define SYS_recvfrom 45
#define SYS_getsockopt 55
#define SYS_fcntl 72

/* ---- Socket constants ---- */
#define AF_INET 2
#define AF_INET6 10
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define O_NONBLOCK 2048
#define F_GETFL 3
#define F_SETFL 4
#define SOL_SOCKET 1
#define SO_ERROR 4
#define MSG_NOSIGNAL 0x4000
#define POLLOUT 4
#define POLLERR 8
#define POLLHUP 16
#define EINPROGRESS 115
#define EAGAIN 11
#define EWOULDBLOCK 11

/* ---- Types ---- */
typedef unsigned int socklen_t;

struct sockaddr_in {
	unsigned short sin_family;
	unsigned short sin_port;
	unsigned char sin_addr[4];
	unsigned char sin_zero[8];
};

struct sockaddr_in6 {
	unsigned short sin6_family;
	unsigned short sin6_port;
	unsigned int sin6_flowinfo;
	unsigned char sin6_addr[16];
	unsigned int sin6_scope_id;
};

struct sockaddr_storage {
	unsigned short ss_family;
	unsigned char _pad[126];
};

struct pollfd {
	int fd;
	short events;
	short revents;
};

/* ---- Syscall helpers ---- */
static inline long syscall1(long n, long a1)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall2(long n, long a1, long a2)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3)
{
	long ret;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
						 : "rcx", "r11", "memory");
	return ret;
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
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

/* ---- Helpers ---- */
static inline unsigned short htons_impl(unsigned short x)
{
	return (unsigned short)((x >> 8) | (x << 8));
}

static inline void zero_bytes(void *p, size_t n)
{
	unsigned char *b = (unsigned char *)p;
	for (size_t i = 0; i < n; i++)
		b[i] = 0;
}

static inline void copy_bytes(void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
}

/* ---- Address helpers ---- */
static int addr_to_sockaddr(NetworkAddress addr, struct sockaddr_storage *out,
							socklen_t *out_len)
{
	zero_bytes(out, sizeof(*out));
	if (addr.family == NETWORK_ADDRESS_IPV4) {
		struct sockaddr_in *sa = (struct sockaddr_in *)out;
		sa->sin_family = AF_INET;
		sa->sin_addr[0] = addr.bytes[0];
		sa->sin_addr[1] = addr.bytes[1];
		sa->sin_addr[2] = addr.bytes[2];
		sa->sin_addr[3] = addr.bytes[3];
		sa->sin_port = htons_impl(addr.port);
		*out_len = sizeof(*sa);
		return 0;
	} else if (addr.family == NETWORK_ADDRESS_IPV6) {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;
		sa->sin6_family = AF_INET6;
		copy_bytes(sa->sin6_addr, addr.bytes, 16);
		sa->sin6_port = htons_impl(addr.port);
		*out_len = sizeof(*sa);
		return 0;
	}
	return -1;
}

/* ---- API ---- */

int fun_network_arch_tcp_connect(NetworkAddress addr, intptr_t *out_fd)
{
	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	long fd = syscall3(SYS_socket, domain, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	long flags = syscall3(SYS_fcntl, fd, F_GETFL, 0);
	if (flags < 0 || syscall3(SYS_fcntl, fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		syscall1(SYS_close, fd);
		return -1;
	}

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		syscall1(SYS_close, fd);
		return -1;
	}

	long rc = syscall3(SYS_connect, fd, (long)&sa, sa_len);
	if (rc != 0 && rc != -EINPROGRESS) {
		syscall1(SYS_close, fd);
		return -1;
	}

	*out_fd = (intptr_t)fd;
	return 0;
}

int fun_network_arch_tcp_poll_connect(intptr_t fd)
{
	struct pollfd pfd;
	pfd.fd = (int)fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	long rc = syscall3(SYS_poll, (long)&pfd, 1, 0);
	if (rc < 0)
		return -1;
	if (rc == 0)
		return 0;

	if (pfd.revents & (POLLERR | POLLHUP))
		return -1;

	if (pfd.revents & POLLOUT) {
		int err = 0;
		socklen_t len = sizeof(err);
		if (syscall5(SYS_getsockopt, fd, SOL_SOCKET, SO_ERROR, (long)&err,
					 (long)&len) != 0)
			return -1;
		if (err != 0)
			return -1;
		return 1;
	}

	return 0;
}

int fun_network_arch_tcp_send(intptr_t fd, const void *data, size_t len,
							  size_t *sent)
{
	long n =
		syscall6(SYS_sendto, fd, (long)data, (long)len, MSG_NOSIGNAL, 0, 0);
	if (n < 0) {
		if (n == -EAGAIN || n == -EWOULDBLOCK)
			return 1;
		return -1;
	}
	*sent = (size_t)n;
	return 0;
}

int fun_network_arch_tcp_recv(intptr_t fd, void *data, size_t len,
							  size_t *received)
{
	long n = syscall6(SYS_recvfrom, fd, (long)data, (long)len, 0, 0, 0);
	if (n < 0) {
		if (n == -EAGAIN || n == -EWOULDBLOCK)
			return 1;
		return -1;
	}
	if (n == 0)
		return -1;
	*received = (size_t)n;
	return 0;
}

void fun_network_arch_tcp_close_fd(intptr_t fd)
{
	syscall1(SYS_close, fd);
}

int fun_network_arch_udp_send(NetworkAddress addr, const void *data, size_t len)
{
	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	long fd = syscall3(SYS_socket, domain, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		syscall1(SYS_close, fd);
		return -1;
	}

	syscall6(SYS_sendto, fd, (long)data, (long)len, MSG_NOSIGNAL, (long)&sa,
			 sa_len);

	syscall1(SYS_close, fd);
	return 0;
}
