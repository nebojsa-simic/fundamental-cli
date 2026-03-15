/*
 * Network module — Windows AMD64 arch layer.
 *
 * Implements an IOCP (I/O Completion Port) based proactor event loop.
 * All Windows-specific code is isolated here.
 *
 * Strategy:
 *   - One IOCP per NetworkLoop; all sockets are associated with it.
 *   - Each I/O operation (connect, send, recv) uses an OVERLAPPED struct
 *     stored inside LinuxNetConn (actually WinNetConn here).
 *   - GetQueuedCompletionStatus drives the dispatch loop.
 *   - ConnectEx (from Mswsock) is used for non-blocking connect.
 *   - WSASend / WSARecv with WSABUF arrays provide scatter/gather.
 *   - Deferred actions (close/stop from callbacks) use flags, processed
 *     after each completion packet.
 *   - Connect timeout tracked via GetTickCount64.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#include "../../../include/network/network.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------
 * Operation tags for completion key discrimination
 * ------------------------------------------------------------------ */
#define OP_CONNECT 1
#define OP_SEND 2
#define OP_RECV 3
#define OP_SEND_VEC 4

/* ------------------------------------------------------------------
 * Internal layout
 * ------------------------------------------------------------------ */

#define WIN_MAX_WSABUF 16

/* Per-operation OVERLAPPED wrapper */
typedef struct {
	OVERLAPPED ov; /* MUST be first field */
	int op_type;
	NetworkConnection *conn;
} WinOverlapped;

/* Connection state */
#define CONN_STATE_UNUSED 0
#define CONN_STATE_CONNECTING 1
#define CONN_STATE_CONNECTED 2
#define CONN_STATE_UDP 3
#define CONN_STATE_CLOSING 4

#define CONN_DEFER_CLOSE (1 << 0)
#define LOOP_DEFER_STOP (1 << 0)

typedef struct {
	SOCKET fd;
	int state;
	int defer_flags;

	NetworkHandlers handlers;
	NetworkBufferVector receive_buffers;
	NetworkBuffer udp_recv_buffer;

	/* Overlapped ops */
	WinOverlapped ov_connect;
	WinOverlapped ov_send;
	WinOverlapped ov_recv;

	/* Scatter/gather send buffers */
	WSABUF send_wsabuf[WIN_MAX_WSABUF];
	int send_wsabuf_count;
	int send_total_requested;

	/* Scatter recv buffers */
	WSABUF recv_wsabuf[WIN_MAX_WSABUF];
	int recv_wsabuf_count;

	/* UDP recv sender address */
	struct sockaddr_storage udp_sender;
	INT udp_sender_len;
	WinOverlapped ov_udp_recv;

	/* Connect timeout */
	ULONGLONG connect_deadline_ms; /* 0 = no timeout */

	NetworkConnection *self;
} WinNetConn;

typedef struct {
	HANDLE iocp;
	int stop_flag;
	int defer_flags;
	int in_dispatch;

	NetworkConnection *connections[64];
	int connection_count;
} WinNetLoop;

/* Compile-time size checks */
typedef char _loop_sz[sizeof(WinNetLoop) <= NETWORK_LOOP_SIZE ? 1 : -1];
typedef char _conn_sz[sizeof(WinNetConn) <= NETWORK_CONNECTION_SIZE ? 1 : -1];

static WinNetLoop *get_loop(NetworkLoop *loop)
{
	return (WinNetLoop *)(void *)loop->_storage;
}
static WinNetConn *get_conn(NetworkConnection *conn)
{
	return (WinNetConn *)(void *)conn->_storage;
}

/* ------------------------------------------------------------------
 * ConnectEx function pointer (loaded dynamically from Mswsock)
 * ------------------------------------------------------------------ */

static LPFN_CONNECTEX s_connect_ex = NULL;

static LPFN_CONNECTEX load_connect_ex(void)
{
	if (s_connect_ex)
		return s_connect_ex;
	SOCKET dummy = socket(AF_INET, SOCK_STREAM, 0);
	if (dummy == INVALID_SOCKET)
		return NULL;
	DWORD bytes = 0;
	GUID guid = WSAID_CONNECTEX;
	WSAIoctl(dummy, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
			 &s_connect_ex, sizeof(s_connect_ex), &bytes, NULL, NULL);
	closesocket(dummy);
	return s_connect_ex;
}

/* ------------------------------------------------------------------
 * Address helpers
 * ------------------------------------------------------------------ */

static int network_address_to_sockaddr(NetworkAddress addr,
									   struct sockaddr_storage *out,
									   int *out_len)
{
	memset(out, 0, sizeof(*out));
	if (addr.family == NETWORK_ADDRESS_IPV4) {
		struct sockaddr_in *sa = (struct sockaddr_in *)out;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(addr.port);
		memcpy(&sa->sin_addr, addr.bytes, 4);
		*out_len = sizeof(*sa);
		return 0;
	} else if (addr.family == NETWORK_ADDRESS_IPV6) {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out;
		sa->sin6_family = AF_INET6;
		sa->sin6_port = htons(addr.port);
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
		out->port = ntohs(s->sin_port);
	} else {
		const struct sockaddr_in6 *s = (const struct sockaddr_in6 *)sa;
		out->family = NETWORK_ADDRESS_IPV6;
		memcpy(out->bytes, &s->sin6_addr, 16);
		out->port = ntohs(s->sin6_port);
	}
}

/* ------------------------------------------------------------------
 * Registration helpers
 * ------------------------------------------------------------------ */

static void loop_register(WinNetLoop *wloop, NetworkConnection *conn)
{
	if (wloop->connection_count < 64)
		wloop->connections[wloop->connection_count++] = conn;
}

static void loop_unregister(WinNetLoop *wloop, NetworkConnection *conn)
{
	for (int i = 0; i < wloop->connection_count; i++) {
		if (wloop->connections[i] == conn) {
			wloop->connections[i] =
				wloop->connections[--wloop->connection_count];
			return;
		}
	}
}

/* ------------------------------------------------------------------
 * Post next receive operation
 * ------------------------------------------------------------------ */

static void post_recv(WinNetConn *wconn, NetworkConnection *conn)
{
	DWORD flags = 0;
	memset(&wconn->ov_recv, 0, sizeof(wconn->ov_recv));
	wconn->ov_recv.op_type = OP_RECV;
	wconn->ov_recv.conn = conn;

	WSARecv(wconn->fd, wconn->recv_wsabuf, (DWORD)wconn->recv_wsabuf_count,
			NULL, &flags, &wconn->ov_recv.ov, NULL);
}

static void post_udp_recv(WinNetConn *wconn, NetworkConnection *conn)
{
	DWORD flags = 0;
	memset(&wconn->ov_udp_recv, 0, sizeof(wconn->ov_udp_recv));
	wconn->ov_udp_recv.op_type = OP_RECV;
	wconn->ov_udp_recv.conn = conn;
	wconn->udp_sender_len = sizeof(wconn->udp_sender);
	WSABUF buf;
	buf.buf = (char *)wconn->udp_recv_buffer.data;
	buf.len = (ULONG)wconn->udp_recv_buffer.length;
	WSARecvFrom(wconn->fd, &buf, 1, NULL, &flags,
				(struct sockaddr *)&wconn->udp_sender, &wconn->udp_sender_len,
				&wconn->ov_udp_recv.ov, NULL);
}

/* ------------------------------------------------------------------
 * Deferred close
 * ------------------------------------------------------------------ */

static void process_deferred_close(WinNetLoop *wloop, NetworkConnection *conn)
{
	WinNetConn *wconn = get_conn(conn);
	if (wconn->fd != INVALID_SOCKET) {
		closesocket(wconn->fd);
		wconn->fd = INVALID_SOCKET;
	}
	wconn->state = CONN_STATE_UNUSED;
	loop_unregister(wloop, conn);
	if (wconn->handlers.on_close)
		wconn->handlers.on_close(conn);
}

/* ------------------------------------------------------------------
 * Connect timeout check
 * ------------------------------------------------------------------ */

static void check_connect_timeouts(WinNetLoop *wloop)
{
	ULONGLONG now = GetTickCount64();
	for (int i = 0; i < wloop->connection_count; i++) {
		NetworkConnection *conn = wloop->connections[i];
		WinNetConn *wconn = get_conn(conn);
		if (wconn->state == CONN_STATE_CONNECTING &&
			wconn->connect_deadline_ms > 0 &&
			now >= wconn->connect_deadline_ms) {
			wconn->state = CONN_STATE_CLOSING;
			if (wconn->handlers.on_error)
				wconn->handlers.on_error(conn,
										 ERROR_RESULT_NETWORK_CONNECT_TIMEOUT);
			process_deferred_close(wloop, conn);
			i--;
		}
	}
}

/* ------------------------------------------------------------------
 * Completion packet dispatch
 * ------------------------------------------------------------------ */

static void dispatch_completion(WinNetLoop *wloop, WinOverlapped *wov,
								DWORD bytes_transferred, BOOL success)
{
	NetworkConnection *conn = wov->conn;
	WinNetConn *wconn = get_conn(conn);

	switch (wov->op_type) {
	case OP_CONNECT:
		if (!success) {
			wconn->state = CONN_STATE_CLOSING;
			if (wconn->handlers.on_error)
				wconn->handlers.on_error(conn,
										 ERROR_RESULT_NETWORK_CONNECT_FAILED);
			process_deferred_close(wloop, conn);
		} else {
			wconn->state = CONN_STATE_CONNECTED;
			wconn->connect_deadline_ms = 0;
			setsockopt(wconn->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL,
					   0);
			/* Post initial receive */
			post_recv(wconn, conn);
			if (wconn->handlers.on_connect)
				wconn->handlers.on_connect(conn);
		}
		break;

	case OP_SEND:
	case OP_SEND_VEC:
		if (!success || bytes_transferred == 0) {
			wconn->state = CONN_STATE_CLOSING;
			if (wconn->handlers.on_error)
				wconn->handlers.on_error(conn,
										 ERROR_RESULT_NETWORK_SEND_FAILED);
			process_deferred_close(wloop, conn);
		} else {
			if (wconn->handlers.on_write_complete)
				wconn->handlers.on_write_complete(conn, (int)bytes_transferred);
		}
		break;

	case OP_RECV:
		if (wconn->state == CONN_STATE_UDP) {
			if (success && bytes_transferred > 0 &&
				wconn->handlers.on_datagram_read) {
				NetworkAddress sender;
				sockaddr_to_network_address(&wconn->udp_sender, &sender);
				NetworkBuffer buf;
				buf.data = wconn->udp_recv_buffer.data;
				buf.length = (size_t)bytes_transferred;
				wconn->handlers.on_datagram_read(conn, buf, sender);
			}
			/* Re-post UDP recv */
			if (wconn->state == CONN_STATE_UDP)
				post_udp_recv(wconn, conn);
		} else {
			if (!success || bytes_transferred == 0) {
				/* Remote close or error */
				if (bytes_transferred == 0)
					; /* graceful close */
				else if (wconn->handlers.on_error)
					wconn->handlers.on_error(
						conn, ERROR_RESULT_NETWORK_RECEIVE_FAILED);
				process_deferred_close(wloop, conn);
			} else {
				if (wconn->handlers.on_read)
					wconn->handlers.on_read(conn, wconn->receive_buffers,
											(int)bytes_transferred);
				/* Re-post recv */
				if (wconn->state == CONN_STATE_CONNECTED)
					post_recv(wconn, conn);
			}
		}
		break;
	}
}

/* ------------------------------------------------------------------
 * Run one IOCP wait cycle
 * ------------------------------------------------------------------ */

static int run_once_internal(WinNetLoop *wloop, int timeout_ms)
{
	check_connect_timeouts(wloop);

	DWORD timeout = (timeout_ms < 0) ? INFINITE : (DWORD)timeout_ms;
	DWORD bytes = 0;
	ULONG_PTR key = 0;
	OVERLAPPED *ov = NULL;

	wloop->in_dispatch = 1;
	BOOL ok =
		GetQueuedCompletionStatus(wloop->iocp, &bytes, &key, &ov, timeout);
	wloop->in_dispatch = 0;

	if (ov == NULL)
		return 0; /* timeout or spurious */

	WinOverlapped *wov = (WinOverlapped *)ov;
	NetworkConnection *conn = wov->conn;
	WinNetConn *wconn = get_conn(conn);

	dispatch_completion(wloop, wov, bytes, ok);

	/* Deferred close */
	if (wconn->defer_flags & CONN_DEFER_CLOSE) {
		wconn->defer_flags &= ~CONN_DEFER_CLOSE;
		process_deferred_close(wloop, conn);
	}

	/* Deferred stop */
	if (wloop->defer_flags & LOOP_DEFER_STOP) {
		wloop->defer_flags &= ~LOOP_DEFER_STOP;
		wloop->stop_flag = 1;
	}

	return 0;
}

/* ------------------------------------------------------------------
 * Arch API
 * ------------------------------------------------------------------ */

int fun_network_arch_loop_init(NetworkLoop *loop)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	WinNetLoop *wloop = get_loop(loop);
	memset(wloop, 0, sizeof(*wloop));

	wloop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (wloop->iocp == NULL) {
		WSACleanup();
		return -1;
	}
	return 0;
}

void fun_network_arch_loop_destroy(NetworkLoop *loop)
{
	WinNetLoop *wloop = get_loop(loop);
	if (wloop->iocp) {
		CloseHandle(wloop->iocp);
		wloop->iocp = NULL;
	}
	WSACleanup();
}

int fun_network_arch_loop_run(NetworkLoop *loop)
{
	WinNetLoop *wloop = get_loop(loop);
	wloop->stop_flag = 0;
	while (!wloop->stop_flag)
		run_once_internal(wloop, 50);
	return 0;
}

int fun_network_arch_loop_run_once(NetworkLoop *loop, int timeout_ms)
{
	WinNetLoop *wloop = get_loop(loop);
	return run_once_internal(wloop, timeout_ms);
}

void fun_network_arch_loop_stop(NetworkLoop *loop)
{
	WinNetLoop *wloop = get_loop(loop);
	if (wloop->in_dispatch)
		wloop->defer_flags |= LOOP_DEFER_STOP;
	else
		wloop->stop_flag = 1;
}

int fun_network_arch_tcp_connect(NetworkLoop *loop,
								 NetworkConnection *connection,
								 NetworkAddress address,
								 NetworkBufferVector receive_buffers,
								 NetworkHandlers handlers, int timeout_ms)
{
	WinNetLoop *wloop = get_loop(loop);
	WinNetConn *wconn = get_conn(connection);
	memset(wconn, 0, sizeof(*wconn));
	wconn->fd = INVALID_SOCKET;
	wconn->self = connection;

	LPFN_CONNECTEX connectEx = load_connect_ex();
	if (!connectEx)
		return -1;

	int domain = (address.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 : AF_INET;
	SOCKET fd = WSASocketW(domain, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
						   WSA_FLAG_OVERLAPPED);
	if (fd == INVALID_SOCKET)
		return -1;

	/* ConnectEx requires the socket to be bound first */
	struct sockaddr_storage bind_sa;
	int bind_len = 0;
	memset(&bind_sa, 0, sizeof(bind_sa));
	if (domain == AF_INET) {
		struct sockaddr_in *ba = (struct sockaddr_in *)&bind_sa;
		ba->sin_family = AF_INET;
		bind_len = sizeof(*ba);
	} else {
		struct sockaddr_in6 *ba = (struct sockaddr_in6 *)&bind_sa;
		ba->sin6_family = AF_INET6;
		bind_len = sizeof(*ba);
	}
	if (bind(fd, (struct sockaddr *)&bind_sa, bind_len) != 0) {
		closesocket(fd);
		return -1;
	}

	/* Associate with IOCP */
	if (CreateIoCompletionPort((HANDLE)fd, wloop->iocp, 0, 0) == NULL) {
		closesocket(fd);
		return -1;
	}

	wconn->fd = fd;
	wconn->state = CONN_STATE_CONNECTING;
	wconn->handlers = handlers;
	wconn->receive_buffers = receive_buffers;

	/* Build recv WSABUF array */
	size_t rbuf_count = receive_buffers.count;
	if (rbuf_count > WIN_MAX_WSABUF)
		rbuf_count = WIN_MAX_WSABUF;
	wconn->recv_wsabuf_count = (int)rbuf_count;
	for (size_t i = 0; i < rbuf_count; i++) {
		wconn->recv_wsabuf[i].buf = (char *)receive_buffers.buffers[i].data;
		wconn->recv_wsabuf[i].len = (ULONG)receive_buffers.buffers[i].length;
	}

	if (timeout_ms > 0)
		wconn->connect_deadline_ms = GetTickCount64() + (ULONGLONG)timeout_ms;

	struct sockaddr_storage dst_sa;
	int dst_len = 0;
	if (network_address_to_sockaddr(address, &dst_sa, &dst_len) != 0) {
		closesocket(fd);
		return -1;
	}

	memset(&wconn->ov_connect, 0, sizeof(wconn->ov_connect));
	wconn->ov_connect.op_type = OP_CONNECT;
	wconn->ov_connect.conn = connection;

	BOOL rc = connectEx(fd, (struct sockaddr *)&dst_sa, dst_len, NULL, 0, NULL,
						&wconn->ov_connect.ov);
	if (!rc && WSAGetLastError() != ERROR_IO_PENDING) {
		closesocket(fd);
		return -1;
	}

	loop_register(wloop, connection);
	return 0;
}

int fun_network_arch_tcp_send(NetworkConnection *connection,
							  NetworkBuffer buffer)
{
	WinNetConn *wconn = get_conn(connection);
	if (wconn->state != CONN_STATE_CONNECTED)
		return -1;

	memset(&wconn->ov_send, 0, sizeof(wconn->ov_send));
	wconn->ov_send.op_type = OP_SEND;
	wconn->ov_send.conn = connection;

	wconn->send_wsabuf[0].buf = (char *)buffer.data;
	wconn->send_wsabuf[0].len = (ULONG)buffer.length;

	DWORD sent = 0;
	int rc = WSASend(wconn->fd, wconn->send_wsabuf, 1, &sent, 0,
					 &wconn->ov_send.ov, NULL);
	if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 1;
		return -1;
	}
	return 0;
}

int fun_network_arch_tcp_send_vector(NetworkConnection *connection,
									 NetworkBufferVector vec)
{
	WinNetConn *wconn = get_conn(connection);
	if (wconn->state != CONN_STATE_CONNECTED)
		return -1;

	size_t count = vec.count;
	if (count > WIN_MAX_WSABUF)
		count = WIN_MAX_WSABUF;

	memset(&wconn->ov_send, 0, sizeof(wconn->ov_send));
	wconn->ov_send.op_type = OP_SEND_VEC;
	wconn->ov_send.conn = connection;

	for (size_t i = 0; i < count; i++) {
		wconn->send_wsabuf[i].buf = (char *)vec.buffers[i].data;
		wconn->send_wsabuf[i].len = (ULONG)vec.buffers[i].length;
	}

	DWORD sent = 0;
	int rc = WSASend(wconn->fd, wconn->send_wsabuf, (DWORD)count, &sent, 0,
					 &wconn->ov_send.ov, NULL);
	if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 1;
		return -1;
	}
	return 0;
}

void fun_network_arch_tcp_close(NetworkConnection *connection)
{
	WinNetConn *wconn = get_conn(connection);
	wconn->defer_flags |= CONN_DEFER_CLOSE;
}

int fun_network_arch_udp_bind(NetworkLoop *loop, NetworkConnection *connection,
							  NetworkAddress local_address,
							  NetworkBuffer recv_buffer,
							  NetworkHandlers handlers)
{
	WinNetLoop *wloop = get_loop(loop);
	WinNetConn *wconn = get_conn(connection);
	memset(wconn, 0, sizeof(*wconn));
	wconn->fd = INVALID_SOCKET;
	wconn->self = connection;

	int domain = (local_address.family == NETWORK_ADDRESS_IPV6) ? AF_INET6 :
																  AF_INET;
	SOCKET fd = WSASocketW(domain, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,
						   WSA_FLAG_OVERLAPPED);
	if (fd == INVALID_SOCKET)
		return -1;

	struct sockaddr_storage sa;
	int sa_len = 0;
	if (network_address_to_sockaddr(local_address, &sa, &sa_len) != 0) {
		closesocket(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sa, sa_len) != 0) {
		closesocket(fd);
		return -1;
	}

	if (CreateIoCompletionPort((HANDLE)fd, wloop->iocp, 0, 0) == NULL) {
		closesocket(fd);
		return -1;
	}

	wconn->fd = fd;
	wconn->state = CONN_STATE_UDP;
	wconn->handlers = handlers;
	wconn->udp_recv_buffer = recv_buffer;

	loop_register(wloop, connection);
	post_udp_recv(wconn, connection);
	return 0;
}

int fun_network_arch_udp_send_to(NetworkConnection *connection,
								 NetworkBuffer buffer,
								 NetworkAddress destination)
{
	WinNetConn *wconn = get_conn(connection);
	if (wconn->state != CONN_STATE_UDP)
		return -1;

	struct sockaddr_storage sa;
	int sa_len = 0;
	if (network_address_to_sockaddr(destination, &sa, &sa_len) != 0)
		return -1;

	WSABUF buf;
	buf.buf = (char *)buffer.data;
	buf.len = (ULONG)buffer.length;
	DWORD sent = 0;
	/* Fire-and-forget send (synchronous fallback acceptable for UDP) */
	WSASendTo(wconn->fd, &buf, 1, &sent, 0, (struct sockaddr *)&sa, sa_len,
			  NULL, NULL);
	return 0;
}

void fun_network_arch_udp_close(NetworkConnection *connection)
{
	WinNetConn *wconn = get_conn(connection);
	wconn->defer_flags |= CONN_DEFER_CLOSE;
}

void *fun_network_arch_connection_get_user_data(NetworkConnection *connection)
{
	WinNetConn *wconn = get_conn(connection);
	return wconn->handlers.user_data;
}
