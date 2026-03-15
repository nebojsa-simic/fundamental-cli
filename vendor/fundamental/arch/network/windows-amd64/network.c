/*
 * Network module — Windows AMD64 arch layer.
 *
 * Implements non-blocking TCP/UDP primitives using Winsock2.
 * No event loop; all poll-based readiness is checked by the caller.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../../include/network/network.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------
 * WSAStartup guard — initialise once per process
 * ------------------------------------------------------------------ */

static int s_wsa_ready = 0;

static int ensure_wsa(void)
{
	if (s_wsa_ready)
		return 0;
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
		return -1;
	s_wsa_ready = 1;
	return 0;
}

/* ------------------------------------------------------------------
 * Address helpers
 * ------------------------------------------------------------------ */

static int addr_to_sockaddr(NetworkAddress addr, struct sockaddr_storage *out,
							int *out_len)
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
 * Returns 0 = in-progress (WSAEWOULDBLOCK), -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_connect(NetworkAddress addr, intptr_t *out_fd)
{
	if (ensure_wsa() != 0)
		return -1;

	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	SOCKET fd = socket(domain, SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET)
		return -1;

	/* Set non-blocking */
	u_long mode = 1;
	if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
		closesocket(fd);
		return -1;
	}

	struct sockaddr_storage sa;
	int sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		closesocket(fd);
		return -1;
	}

	int rc = connect(fd, (struct sockaddr *)&sa, sa_len);
	if (rc == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			closesocket(fd);
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
 *
 * Uses select() rather than WSAPoll() — WSAPoll is unreliable on some
 * Windows versions for pending non-blocking connects.
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_poll_connect(intptr_t fd)
{
	fd_set wset, eset;
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET((SOCKET)fd, &wset);
	FD_SET((SOCKET)fd, &eset);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	int rc = select(0, NULL, &wset, &eset, &tv);
	if (rc < 0)
		return -1;
	if (rc == 0)
		return 0; /* nothing ready yet */

	if (FD_ISSET((SOCKET)fd, &eset)) {
		/* Error condition */
		return -1;
	}

	if (FD_ISSET((SOCKET)fd, &wset)) {
		/* Check SO_ERROR */
		int err = 0;
		int len = sizeof(err);
		if (getsockopt((SOCKET)fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) !=
			0)
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
	int n = send((SOCKET)fd, (const char *)data, (int)len, 0);
	if (n == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK)
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
	int n = recv((SOCKET)fd, (char *)data, (int)len, 0);
	if (n == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 1;
		return -1;
	}
	if (n == 0)
		return -1; /* remote close — returns -1 same as a socket error.
		            * TODO: distinguish clean EOF (NETWORK_CLOSED) from
		            * hard errors (NETWORK_RECEIVE_FAILED) by returning a
		            * separate rc (e.g. -2) so the core can set the correct
		            * error code. Callers that need to detect graceful close
		            * vs. receive failure are affected. */
	*received = (size_t)n;
	return 0;
}

/* ------------------------------------------------------------------
 * fun_network_arch_tcp_close_fd
 * ------------------------------------------------------------------ */

void fun_network_arch_tcp_close_fd(intptr_t fd)
{
	closesocket((SOCKET)fd);
}

/* ------------------------------------------------------------------
 * fun_network_arch_udp_send
 *
 * Creates an ephemeral UDP socket, sends, closes.
 * Returns 0 = ok, -1 = error.
 * ------------------------------------------------------------------ */

int fun_network_arch_udp_send(NetworkAddress addr, const void *data, size_t len)
{
	if (ensure_wsa() != 0)
		return -1;

	int domain = (addr.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	SOCKET fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == INVALID_SOCKET)
		return -1;

	struct sockaddr_storage sa;
	int sa_len = 0;
	if (addr_to_sockaddr(addr, &sa, &sa_len) != 0) {
		closesocket(fd);
		return -1;
	}

	sendto(fd, (const char *)data, (int)len, 0, (struct sockaddr *)&sa, sa_len);

	closesocket(fd);
	return 0;
}
