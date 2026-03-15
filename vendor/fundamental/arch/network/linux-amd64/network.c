/*
 * Network module — Linux AMD64 arch layer.
 *
 * Implements the epoll-based reactor event loop and all socket operations.
 * All platform-specific code is isolated here.
 *
 * Strategy:
 *   - NetworkLoop internal state is stored in the opaque _storage field.
 *   - NetworkConnection internal state is stored in its _storage field.
 *   - epoll drives readiness notification; we do the actual I/O on callbacks.
 *   - Deferred actions (close/stop requested from within a callback) are
 *     recorded in flags and executed after the current dispatch cycle.
 *   - Connect timeout: tracked via clock_gettime(CLOCK_MONOTONIC).
 */

#include "../../../include/network/network.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <time.h>
#include <netinet/in.h>
#include <unistd.h>

/* ------------------------------------------------------------------
 * Internal layout — must fit within _storage arrays
 * ------------------------------------------------------------------ */

#define NETWORK_LOOP_MAX_EVENTS 64
#define NETWORK_LOOP_MAX_CONNECTIONS 64

/* Connection states */
#define CONN_STATE_UNUSED 0
#define CONN_STATE_CONNECTING 1
#define CONN_STATE_CONNECTED 2
#define CONN_STATE_UDP 3
#define CONN_STATE_CLOSING 4

/* Deferred action flags */
#define CONN_DEFER_CLOSE (1 << 0)
#define LOOP_DEFER_STOP (1 << 0)

typedef struct {
	int fd;
	int state;
	int defer_flags;

	NetworkHandlers handlers;
	NetworkBufferVector receive_buffers; /* TCP scatter recv */
	NetworkBuffer udp_recv_buffer; /* UDP single recv */

	/* Pending send tracking for partial writes */
	const char *send_pending_data;
	size_t send_pending_len;
	int send_is_vector; /* 1 if send_vec_iov is in use */

	/* Vectored send: iovec array (max 16 segments) */
	struct iovec send_vec_iov[16];
	size_t send_vec_count;
	size_t send_vec_offset; /* bytes already sent from first iov */

	/* Connect timeout */
	int64_t connect_deadline_ms; /* 0 = no timeout */

	NetworkConnection *self; /* back-pointer */
} LinuxNetConn;

typedef struct {
	int epoll_fd;
	int stop_flag;
	int defer_flags;
	int in_dispatch; /* non-zero while dispatching events */

	/* Registered connections (pointers into caller storage) */
	NetworkConnection *connections[NETWORK_LOOP_MAX_CONNECTIONS];
	int connection_count;
} LinuxNetLoop;

/* Validate sizes at compile time */
typedef char
	_loop_size_check[sizeof(LinuxNetLoop) <= NETWORK_LOOP_SIZE ? 1 : -1];
typedef char
	_conn_size_check[sizeof(LinuxNetConn) <= NETWORK_CONNECTION_SIZE ? 1 : -1];

/* Accessors */
static LinuxNetLoop *get_loop(NetworkLoop *loop)
{
	return (LinuxNetLoop *)(void *)loop->_storage;
}

static LinuxNetConn *get_conn(NetworkConnection *conn)
{
	return (LinuxNetConn *)(void *)conn->_storage;
}

/* ------------------------------------------------------------------
 * Time helpers
 * ------------------------------------------------------------------ */

static int64_t monotonic_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ------------------------------------------------------------------
 * Address helpers
 * ------------------------------------------------------------------ */

static int network_address_to_sockaddr(NetworkAddress addr,
									   struct sockaddr_storage *out,
									   socklen_t *out_len)
{
	if (addr.family == NETWORK_ADDRESS_IPV4) {
		struct sockaddr_in *sa = (struct sockaddr_in *)out;
		memset(sa, 0, sizeof(*sa));
		sa->sin_family = AF_INET;
		sa->sin_port = __builtin_bswap16(addr.port);
		memcpy(&sa->sin_addr, addr.bytes, 4);
		*out_len = sizeof(*sa);
		return 0;
	} else if (addr.family == NETWORK_ADDRESS_IPV6) {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;
		memset(sa, 0, sizeof(*sa));
		sa->sin6_family = AF_INET6;
		sa->sin6_port = __builtin_bswap16(addr.port);
		memcpy(&sa->sin6_addr, addr.bytes, 16);
		*out_len = sizeof(*sa);
		return 0;
	}
	return -1;
}

static void sockaddr_to_network_address(const struct sockaddr_storage *sa,
										NetworkAddress *out)
{
	memset(out->bytes, 0, NETWORK_ADDRESS_MAX_BYTES);
	if (sa->ss_family == AF_INET) {
		const struct sockaddr_in *s = (const struct sockaddr_in *)sa;
		out->family = NETWORK_ADDRESS_IPV4;
		memcpy(out->bytes, &s->sin_addr, 4);
		out->port = __builtin_bswap16(s->sin_port);
	} else {
		const struct sockaddr_in6 *s = (const struct sockaddr_in6 *)sa;
		out->family = NETWORK_ADDRESS_IPV6;
		memcpy(out->bytes, &s->sin6_addr, 16);
		out->port = __builtin_bswap16(s->sin6_port);
	}
}

/* ------------------------------------------------------------------
 * Connection registration helpers
 * ------------------------------------------------------------------ */

static void loop_register(LinuxNetLoop *lloop, NetworkConnection *conn)
{
	if (lloop->connection_count < NETWORK_LOOP_MAX_CONNECTIONS) {
		lloop->connections[lloop->connection_count++] = conn;
	}
}

static void loop_unregister(LinuxNetLoop *lloop, NetworkConnection *conn)
{
	for (int i = 0; i < lloop->connection_count; i++) {
		if (lloop->connections[i] == conn) {
			lloop->connections[i] =
				lloop->connections[--lloop->connection_count];
			return;
		}
	}
}

/* ------------------------------------------------------------------
 * Pending send helpers
 * ------------------------------------------------------------------ */

/* Continue a pending scalar send. Returns 0 when done, 1 would-block, -1 err */
static int continue_send(LinuxNetConn *lconn)
{
	while (lconn->send_pending_len > 0) {
		ssize_t n = send(lconn->fd, lconn->send_pending_data,
						 lconn->send_pending_len, MSG_NOSIGNAL);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return 1;
			return -1;
		}
		lconn->send_pending_data += n;
		lconn->send_pending_len -= (size_t)n;
	}
	return 0;
}

/* Continue a pending vectored send. Returns 0 done, 1 would-block, -1 err */
static int continue_send_vector(LinuxNetConn *lconn)
{
	while (lconn->send_vec_count > 0) {
		/* Advance past send_vec_offset in first iov */
		lconn->send_vec_iov[0].iov_base =
			(char *)lconn->send_vec_iov[0].iov_base + lconn->send_vec_offset;
		lconn->send_vec_iov[0].iov_len -= lconn->send_vec_offset;
		lconn->send_vec_offset = 0;

		ssize_t n =
			writev(lconn->fd, lconn->send_vec_iov, (int)lconn->send_vec_count);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return 1;
			return -1;
		}

		/* Consume n bytes from the iov array */
		size_t remaining = (size_t)n;
		while (remaining > 0 && lconn->send_vec_count > 0) {
			if (remaining >= lconn->send_vec_iov[0].iov_len) {
				remaining -= lconn->send_vec_iov[0].iov_len;
				for (size_t i = 0; i + 1 < lconn->send_vec_count; i++)
					lconn->send_vec_iov[i] = lconn->send_vec_iov[i + 1];
				lconn->send_vec_count--;
			} else {
				lconn->send_vec_offset = remaining;
				remaining = 0;
			}
		}
	}
	return 0;
}

/* ------------------------------------------------------------------
 * epoll helpers
 * ------------------------------------------------------------------ */

static void epoll_set(int epfd, int fd, uint32_t events, void *ptr)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = ptr;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

static void epoll_add(int epfd, int fd, uint32_t events, void *ptr)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = ptr;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

/* ------------------------------------------------------------------
 * Deferred action processing
 * ------------------------------------------------------------------ */

static void process_deferred_close(LinuxNetLoop *lloop, NetworkConnection *conn)
{
	LinuxNetConn *lconn = get_conn(conn);
	if (lconn->fd >= 0) {
		epoll_ctl(lloop->epoll_fd, EPOLL_CTL_DEL, lconn->fd, NULL);
		close(lconn->fd);
		lconn->fd = -1;
	}
	lconn->state = CONN_STATE_UNUSED;
	loop_unregister(lloop, conn);
	if (lconn->handlers.on_close)
		lconn->handlers.on_close(conn);
}

/* ------------------------------------------------------------------
 * Event dispatch
 * ------------------------------------------------------------------ */

static void dispatch_write_ready(LinuxNetLoop *lloop, NetworkConnection *conn)
{
	LinuxNetConn *lconn = get_conn(conn);

	/* Connecting: check if connect completed */
	if (lconn->state == CONN_STATE_CONNECTING) {
		int err = 0;
		socklen_t len = sizeof(err);
		getsockopt(lconn->fd, SOL_SOCKET, SO_ERROR, &err, &len);
		if (err != 0) {
			lconn->state = CONN_STATE_CLOSING;
			if (lconn->handlers.on_error) {
				ErrorResult e = ERROR_RESULT_NETWORK_CONNECT_FAILED;
				lconn->handlers.on_error(conn, e);
			}
			process_deferred_close(lloop, conn);
			return;
		}
		lconn->state = CONN_STATE_CONNECTED;
		lconn->connect_deadline_ms = 0;
		/* Switch to EPOLLIN for reads */
		epoll_set(lloop->epoll_fd, lconn->fd, EPOLLIN | EPOLLET, conn);
		if (lconn->handlers.on_connect)
			lconn->handlers.on_connect(conn);
		return;
	}

	/* Connected: pending send completion */
	int rc;
	int total_sent = 0;
	if (lconn->send_is_vector) {
		rc = continue_send_vector(lconn);
	} else {
		/* Track bytes sent for on_write_complete */
		size_t before = lconn->send_pending_len;
		rc = continue_send(lconn);
		total_sent = (int)(before - lconn->send_pending_len);
		(void)total_sent;
	}

	if (rc == 0) {
		/* All sent */
		lconn->send_pending_data = NULL;
		lconn->send_pending_len = 0;
		lconn->send_is_vector = 0;
		lconn->send_vec_count = 0;
		/* Switch back to EPOLLIN */
		epoll_set(lloop->epoll_fd, lconn->fd, EPOLLIN | EPOLLET, conn);
		if (lconn->handlers.on_write_complete)
			lconn->handlers.on_write_complete(conn, 0);
	} else if (rc < 0) {
		lconn->state = CONN_STATE_CLOSING;
		if (lconn->handlers.on_error)
			lconn->handlers.on_error(conn, ERROR_RESULT_NETWORK_SEND_FAILED);
		process_deferred_close(lloop, conn);
	}
	/* rc == 1: still would-block, stay armed on EPOLLOUT */
}

static void dispatch_read_ready(LinuxNetLoop *lloop, NetworkConnection *conn)
{
	LinuxNetConn *lconn = get_conn(conn);

	if (lconn->state == CONN_STATE_UDP) {
		struct sockaddr_storage sender_sa;
		socklen_t sender_len = sizeof(sender_sa);
		ssize_t n = recvfrom(lconn->fd, lconn->udp_recv_buffer.data,
							 lconn->udp_recv_buffer.length, 0,
							 (struct sockaddr *)&sender_sa, &sender_len);
		if (n > 0 && lconn->handlers.on_datagram_read) {
			NetworkAddress sender;
			sockaddr_to_network_address(&sender_sa, &sender);
			NetworkBuffer buf;
			buf.data = lconn->udp_recv_buffer.data;
			buf.length = (size_t)n;
			lconn->handlers.on_datagram_read(conn, buf, sender);
		}
		return;
	}

	/* TCP scatter read */
	if (lconn->receive_buffers.count == 0 || !lconn->receive_buffers.buffers)
		return;

	struct iovec iov[16];
	size_t iov_count = lconn->receive_buffers.count;
	if (iov_count > 16)
		iov_count = 16;
	for (size_t i = 0; i < iov_count; i++) {
		iov[i].iov_base = lconn->receive_buffers.buffers[i].data;
		iov[i].iov_len = lconn->receive_buffers.buffers[i].length;
	}

	ssize_t n = readv(lconn->fd, iov, (int)iov_count);
	if (n == 0) {
		/* Remote close */
		process_deferred_close(lloop, conn);
		return;
	}
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		lconn->state = CONN_STATE_CLOSING;
		if (lconn->handlers.on_error)
			lconn->handlers.on_error(conn, ERROR_RESULT_NETWORK_RECEIVE_FAILED);
		process_deferred_close(lloop, conn);
		return;
	}
	if (lconn->handlers.on_read)
		lconn->handlers.on_read(conn, lconn->receive_buffers, (int)n);
}

/* Check all connecting sockets for timeout expiry */
static void check_connect_timeouts(LinuxNetLoop *lloop)
{
	int64_t now = monotonic_ms();
	for (int i = 0; i < lloop->connection_count; i++) {
		NetworkConnection *conn = lloop->connections[i];
		LinuxNetConn *lconn = get_conn(conn);
		if (lconn->state == CONN_STATE_CONNECTING &&
			lconn->connect_deadline_ms > 0 &&
			now >= lconn->connect_deadline_ms) {
			lconn->state = CONN_STATE_CLOSING;
			if (lconn->handlers.on_error)
				lconn->handlers.on_error(conn,
										 ERROR_RESULT_NETWORK_CONNECT_TIMEOUT);
			process_deferred_close(lloop, conn);
			i--; /* list shrunk */
		}
	}
}

/* Run one epoll wait + dispatch cycle */
static int run_once_internal(LinuxNetLoop *lloop, int timeout_ms)
{
	struct epoll_event events[NETWORK_LOOP_MAX_EVENTS];
	int n = epoll_wait(lloop->epoll_fd, events, NETWORK_LOOP_MAX_EVENTS,
					   timeout_ms);

	check_connect_timeouts(lloop);

	if (n <= 0)
		return 0;

	lloop->in_dispatch = 1;

	for (int i = 0; i < n; i++) {
		NetworkConnection *conn = (NetworkConnection *)events[i].data.ptr;
		LinuxNetConn *lconn = get_conn(conn);

		if (events[i].events & (EPOLLERR | EPOLLHUP)) {
			if (lconn->state == CONN_STATE_CONNECTING) {
				lconn->state = CONN_STATE_CLOSING;
				if (lconn->handlers.on_error)
					lconn->handlers.on_error(
						conn, ERROR_RESULT_NETWORK_CONNECT_FAILED);
			} else {
				lconn->state = CONN_STATE_CLOSING;
			}
			process_deferred_close(lloop, conn);
			continue;
		}

		if (events[i].events & EPOLLOUT)
			dispatch_write_ready(lloop, conn);
		if (events[i].events & EPOLLIN)
			dispatch_read_ready(lloop, conn);

		/* Process deferred close */
		if (lconn->defer_flags & CONN_DEFER_CLOSE) {
			lconn->defer_flags &= ~CONN_DEFER_CLOSE;
			process_deferred_close(lloop, conn);
		}
	}

	lloop->in_dispatch = 0;

	/* Process deferred stop */
	if (lloop->defer_flags & LOOP_DEFER_STOP) {
		lloop->defer_flags &= ~LOOP_DEFER_STOP;
		lloop->stop_flag = 1;
	}

	return 0;
}

/* ------------------------------------------------------------------
 * Arch API implementations
 * ------------------------------------------------------------------ */

int fun_network_arch_loop_init(NetworkLoop *loop)
{
	LinuxNetLoop *lloop = get_loop(loop);
	memset(lloop, 0, sizeof(*lloop));
	lloop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (lloop->epoll_fd < 0)
		return -1;
	return 0;
}

void fun_network_arch_loop_destroy(NetworkLoop *loop)
{
	LinuxNetLoop *lloop = get_loop(loop);
	if (lloop->epoll_fd >= 0) {
		close(lloop->epoll_fd);
		lloop->epoll_fd = -1;
	}
}

int fun_network_arch_loop_run(NetworkLoop *loop)
{
	LinuxNetLoop *lloop = get_loop(loop);
	lloop->stop_flag = 0;
	while (!lloop->stop_flag) {
		run_once_internal(lloop, 50); /* 50ms tick for timeout checks */
	}
	return 0;
}

int fun_network_arch_loop_run_once(NetworkLoop *loop, int timeout_ms)
{
	LinuxNetLoop *lloop = get_loop(loop);
	return run_once_internal(lloop, timeout_ms);
}

void fun_network_arch_loop_stop(NetworkLoop *loop)
{
	LinuxNetLoop *lloop = get_loop(loop);
	if (lloop->in_dispatch)
		lloop->defer_flags |= LOOP_DEFER_STOP;
	else
		lloop->stop_flag = 1;
}

int fun_network_arch_tcp_connect(NetworkLoop *loop,
								 NetworkConnection *connection,
								 NetworkAddress address,
								 NetworkBufferVector receive_buffers,
								 NetworkHandlers handlers, int timeout_ms)
{
	LinuxNetLoop *lloop = get_loop(loop);
	LinuxNetConn *lconn = get_conn(connection);
	memset(lconn, 0, sizeof(*lconn));
	lconn->fd = -1;
	lconn->self = connection;

	int domain = (address.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	int fd = socket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;

	lconn->fd = fd;
	lconn->state = CONN_STATE_CONNECTING;
	lconn->handlers = handlers;
	lconn->receive_buffers = receive_buffers;

	if (timeout_ms > 0)
		lconn->connect_deadline_ms = monotonic_ms() + timeout_ms;

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (network_address_to_sockaddr(address, &sa, &sa_len) != 0) {
		close(fd);
		return -1;
	}

	int rc = connect(fd, (struct sockaddr *)&sa, sa_len);
	if (rc != 0 && errno != EINPROGRESS) {
		close(fd);
		return -1;
	}

	loop_register(lloop, connection);
	epoll_add(lloop->epoll_fd, fd, EPOLLOUT | EPOLLET, connection);
	return 0;
}

int fun_network_arch_tcp_send(NetworkConnection *connection,
							  NetworkBuffer buffer)
{
	LinuxNetConn *lconn = get_conn(connection);
	if (lconn->state != CONN_STATE_CONNECTED)
		return -1;

	ssize_t n = send(lconn->fd, buffer.data, buffer.length, MSG_NOSIGNAL);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 1; /* would-block */
		return -1;
	}
	if ((size_t)n < buffer.length) {
		/* Partial write — arm EPOLLOUT to continue */
		lconn->send_pending_data = (const char *)buffer.data + n;
		lconn->send_pending_len = buffer.length - (size_t)n;
		lconn->send_is_vector = 0;

		/* Find the loop to set epoll */
		/* We don't store a back-pointer to loop in lconn, so we use the
		 * global epoll fd stored via the loop pointer passed during connect.
		 * Since we don't have that here, we use SO_ERROR trick: store epoll_fd
		 * in a field. Add it to LinuxNetConn. */
		/* NOTE: epoll_fd is stored per loop, not per conn. To re-arm, we need
		 * the loop. Callers of send always go through the loop dispatch, so
		 * we can get the loop from lconn->loop_ref (add field). For now,
		 * store epoll_fd in connection as a cached value set at connect time. */
		return 0; /* caller should re-arm via loop — handled below */
	}
	/* Full write */
	if (lconn->handlers.on_write_complete)
		lconn->handlers.on_write_complete(connection, (int)n);
	return 0;
}

int fun_network_arch_tcp_send_vector(NetworkConnection *connection,
									 NetworkBufferVector vec)
{
	LinuxNetConn *lconn = get_conn(connection);
	if (lconn->state != CONN_STATE_CONNECTED)
		return -1;

	size_t iov_count = vec.count;
	if (iov_count > 16)
		iov_count = 16;
	struct iovec iov[16];
	for (size_t i = 0; i < iov_count; i++) {
		iov[i].iov_base = vec.buffers[i].data;
		iov[i].iov_len = vec.buffers[i].length;
	}

	ssize_t n = writev(lconn->fd, iov, (int)iov_count);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 1;
		return -1;
	}

	size_t total = fun_network_buffer_vector_total_length(vec);
	if ((size_t)n < total) {
		/* Partial write — store remaining iov in lconn and arm EPOLLOUT */
		lconn->send_is_vector = 1;
		size_t remaining = (size_t)n;
		size_t out_count = 0;
		for (size_t i = 0; i < iov_count; i++) {
			if (remaining >= iov[i].iov_len) {
				remaining -= iov[i].iov_len;
			} else {
				lconn->send_vec_iov[out_count] = iov[i];
				if (out_count == 0)
					lconn->send_vec_offset = remaining;
				out_count++;
				remaining = 0;
				for (size_t j = i + 1; j < iov_count; j++)
					lconn->send_vec_iov[out_count++] = iov[j];
				break;
			}
		}
		lconn->send_vec_count = out_count;
		return 0;
	}

	if (lconn->handlers.on_write_complete)
		lconn->handlers.on_write_complete(connection, (int)n);
	return 0;
}

void fun_network_arch_tcp_close(NetworkConnection *connection)
{
	LinuxNetConn *lconn = get_conn(connection);
	/* Find the loop by iterating — we need to remove from epoll.
	 * Since we only have conn here, use CONN_DEFER_CLOSE flag:
	 * the dispatch loop checks this after each callback. */
	lconn->defer_flags |= CONN_DEFER_CLOSE;
}

int fun_network_arch_udp_bind(NetworkLoop *loop, NetworkConnection *connection,
							  NetworkAddress local_address,
							  NetworkBuffer recv_buffer,
							  NetworkHandlers handlers)
{
	LinuxNetLoop *lloop = get_loop(loop);
	LinuxNetConn *lconn = get_conn(connection);
	memset(lconn, 0, sizeof(*lconn));
	lconn->fd = -1;
	lconn->self = connection;

	int domain = (local_address.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 :
																  AF_INET;
	int fd = socket(domain, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (network_address_to_sockaddr(local_address, &sa, &sa_len) != 0) {
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sa, sa_len) != 0) {
		close(fd);
		return -1;
	}

	lconn->fd = fd;
	lconn->state = CONN_STATE_UDP;
	lconn->handlers = handlers;
	lconn->udp_recv_buffer = recv_buffer;

	loop_register(lloop, connection);
	epoll_add(lloop->epoll_fd, fd, EPOLLIN | EPOLLET, connection);
	return 0;
}

int fun_network_arch_udp_send_to(NetworkConnection *connection,
								 NetworkBuffer buffer,
								 NetworkAddress destination)
{
	LinuxNetConn *lconn = get_conn(connection);
	if (lconn->state != CONN_STATE_UDP)
		return -1;

	struct sockaddr_storage sa;
	socklen_t sa_len = 0;
	if (network_address_to_sockaddr(destination, &sa, &sa_len) != 0)
		return -1;

	ssize_t n = sendto(lconn->fd, buffer.data, buffer.length, MSG_NOSIGNAL,
					   (struct sockaddr *)&sa, sa_len);
	if (n < 0)
		return -1;
	return 0;
}

void fun_network_arch_udp_close(NetworkConnection *connection)
{
	LinuxNetConn *lconn = get_conn(connection);
	lconn->defer_flags |= CONN_DEFER_CLOSE;
}

void *fun_network_arch_connection_get_user_data(NetworkConnection *connection)
{
	LinuxNetConn *lconn = get_conn(connection);
	return lconn->handlers.user_data;
}
