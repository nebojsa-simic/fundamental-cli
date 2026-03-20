/*
 * Network module — Linux AMD64 arch layer.
 *
 * Implements non-blocking TCP/UDP primitives using POSIX sockets.
 * No event loop; all poll-based readiness is checked by the caller.
 */

#include "fundamental/network/network.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* ------------------------------------------------------------------
 * Address helpers
 * ------------------------------------------------------------------ */

static int addr_to_sockaddr(NetworkAddress addr, struct sockaddr_storage *out,
							socklen_t *out_len)
{
	memset(out, 0, sizeof(*out));
	if (addr.family == NETWORK_ADDRESS_IPV4) {
		struct sockaddr_in *sa = (struct sockaddr_in *)out;
		sa->sin_family = AF_INET;
		uint8_t *s = (uint8_t *)&sa->sin_addr;
		s[0] = addr.bytes[0];
		s[1] = addr.bytes[1];
		s[2] = addr.bytes[2];
		s[3] = addr.bytes[3];
		sa->sin_port = htons(addr.port);
		*out_len = sizeof(*sa);
		return 0;
	} else if (addr.family == NETWORK_ADDRESS_IPV6) {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;
		sa->sin6_family = AF_INET6;
		memcpy(&sa->sin6_addr, addr.bytes, 16);
		sa->sin6_port = htons(addr.port);
		*out_len = sizeof(*sa);
		return 0;
	}
	return -1;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_connect
 *
 * Creates a non-blocking socket and initiates connect.
 * Returns 0 = in-progress (EINPROGRESS), -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_connect(NetworkAddress addr, intptr_t *out_fd)
{
	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	int fd = socket(domain, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	/* Set non-blocking */
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		close(fd);
		return -1;
	}

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		close(fd);
		return -1;
	}

	int rc = connect(fd, (struct sockaddr *)&sa, sa_len);
	if (rc != 0) {
		if (errno != EINPROGRESS) {
			close(fd);
			return -1;
		}
	}

	*out_fd = (intptr_t)fd;
	return 0;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_poll_connect
 *
 * Checks if connect completed.
 * Returns 0 = pending, 1 = connected, -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_poll_connect(intptr_t fd)
{
	struct pollfd pfd;
	pfd.fd = (int)fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	int rc = poll(&pfd, 1, 0);
	if (rc < 0)
		return -1;
	if (rc == 0)
		return 0; /* still pending */

	if (pfd.revents & (POLLERR | POLLHUP))
		return -1;

	if (pfd.revents & POLLOUT) {
		int err = 0;
		socklen_t len = sizeof(err);
		if (getsockopt((int)fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0)
			return -1;
		if (err != 0)
			return -1;
		return 1;
	}

	return 0;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_send
 *
 * Non-blocking send.
 * Returns 0 = ok (*sent set), 1 = would-block, -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_send(intptr_t fd, const void *data, size_t len,
							  size_t *sent)
{
	ssize_t n = send((int)fd, data, len, MSG_NOSIGNAL);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 1;
		return -1;
	}
	*sent = (size_t)n;
	return 0;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_recv
 *
 * Non-blocking recv.
 * Returns 0 = ok (*received set), 1 = would-block, -1 = error/eof.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_recv(intptr_t fd, void *data, size_t len,
							  size_t *received)
{
	ssize_t n = recv((int)fd, data, len, 0);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 1;
		return -1;
	}
	if (n == 0) /* remote close — same TODO as Windows: returns -1 same as a
	             * hard error; callers cannot distinguish clean EOF
	             * (NETWORK_CLOSED) from NETWORK_RECEIVE_FAILED without a
	             * separate return code (e.g. -2) from the arch layer. */
		return -1; /* remote close treated as error */
	*received = (size_t)n;
	return 0;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_close_fd
 * ------------------------------------------------------------------ */

void fun_network_arch_tcp_close_fd(intptr_t fd)
{
	close((int)fd);
}

/* ------------------------------------------------------------------
 * fun_network_arch_udp_send
 *
 * Creates an ephemeral UDP socket, sends, closes.
 * Returns 0 = ok, -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_udp_send(NetworkAddress addr, const void *data, size_t len)
{
	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	int fd = socket(domain, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		close(fd);
		return -1;
	}

	sendto(fd, data, len, MSG_NOSIGNAL, (struct sockaddr *)&sa, sa_len);

	close(fd);
	return 0;
}
