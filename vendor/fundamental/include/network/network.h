#ifndef LIBRARY_NETWORK_H
#define LIBRARY_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#include "../error/error.h"

/*
 * Network Module
 *
 * Non-blocking, event-driven TCP/UDP networking for zero-stdlib applications.
 * Inspired by Netty, Boost.Asio, Java NIO, golang gnet, and Rust Tokio.
 *
 * Reactor model: epoll on Linux, IOCP on Windows.
 * All memory is caller-allocated. No internal heap allocation on the data path.
 *
 * Typical TCP client usage:
 *
 *   char loop_storage[NETWORK_LOOP_SIZE];
 *   NetworkLoop *loop = (NetworkLoop *)loop_storage;
 *   voidResult r = fun_network_loop_init(loop);
 *   if (fun_error_is_error(r.error)) { return 1; }
 *
 *   char recv_data[4096];
 *   NetworkBuffer recv_buf = { recv_data, sizeof(recv_data) };
 *   NetworkBufferVector recv_vec = { &recv_buf, 1 };
 *
 *   NetworkHandlers handlers = { .on_connect = my_connect,
 *                                .on_read    = my_read,
 *                                .user_data  = &my_state };
 *
 *   char conn_storage[NETWORK_CONNECTION_SIZE];
 *   NetworkConnection *conn = (NetworkConnection *)conn_storage;
 *
 *   NetworkAddressResult addr = fun_network_address_parse("127.0.0.1:8080");
 *   fun_network_tcp_connect(loop, conn, addr.value, recv_vec, handlers, 5000);
 *   fun_network_loop_run(loop);
 *   fun_network_loop_destroy(loop);
 */

/* ------------------------------------------------------------------
 * Size constants for caller-allocated opaque types.
 * Declare storage as: char buf[NETWORK_LOOP_SIZE]; then cast to NetworkLoop*.
 * ------------------------------------------------------------------ */
#define NETWORK_LOOP_SIZE 1024
#define NETWORK_CONNECTION_SIZE 1024

/* ------------------------------------------------------------------
 * Address family constants
 * ------------------------------------------------------------------ */
#define NETWORK_ADDRESS_IPV4 4
#define NETWORK_ADDRESS_IPV6 6
#define NETWORK_ADDRESS_MAX_BYTES 16

/* ------------------------------------------------------------------
 * NetworkAddress — transparent struct, pure data, safe to copy by value.
 * Holds a parsed numeric IP address and port in host byte order.
 * ------------------------------------------------------------------ */
typedef struct {
	uint8_t family; /* NETWORK_ADDRESS_IPV4 or NETWORK_ADDRESS_IPV6 */
	uint8_t bytes[NETWORK_ADDRESS_MAX_BYTES]; /* raw IP bytes */
	uint16_t port; /* host byte order, 0–65535 */
} NetworkAddress;

DEFINE_RESULT_TYPE(NetworkAddress);

/* ------------------------------------------------------------------
 * NetworkBuffer — single contiguous memory region.
 * Caller owns the underlying memory for the entire I/O operation.
 * Buffer must remain valid until the corresponding callback fires.
 * ------------------------------------------------------------------ */
typedef struct {
	void *data;
	size_t length;
} NetworkBuffer;

DEFINE_RESULT_TYPE(NetworkBuffer);

/* ------------------------------------------------------------------
 * NetworkBufferVector — scatter/gather array of NetworkBuffers.
 * Maps directly to struct iovec (Linux) or WSABUF[] (Windows).
 * Caller owns the array and all underlying data regions.
 * ------------------------------------------------------------------ */
typedef struct {
	NetworkBuffer *buffers;
	size_t count;
} NetworkBufferVector;

/* ------------------------------------------------------------------
 * Forward declarations for opaque types
 * ------------------------------------------------------------------ */
typedef struct NetworkLoop_s NetworkLoop;
typedef struct NetworkConnection_s NetworkConnection;

/* ------------------------------------------------------------------
 * NetworkHandlers — per-connection callback table.
 *
 * All callbacks receive the NetworkConnection* they are bound to.
 * Retrieve per-connection state via fun_network_connection_get_user_data().
 *
 * Callbacks may re-entrantly call:
 *   fun_network_tcp_send, fun_network_tcp_send_vector,
 *   fun_network_tcp_close, fun_network_udp_close, fun_network_loop_stop.
 * These calls are deferred and execute after the current callback returns.
 *
 * Any handler pointer may be NULL; those events are silently ignored.
 * ------------------------------------------------------------------ */
typedef struct {
	/* TCP: called once the non-blocking connect completes */
	void (*on_connect)(NetworkConnection *connection);

	/* TCP: called when data arrives, scattered across receive_buffers */
	void (*on_read)(NetworkConnection *connection,
					NetworkBufferVector receive_buffers, int bytes_received);

	/* TCP/UDP: called when all bytes from a send call have been transmitted */
	void (*on_write_complete)(NetworkConnection *connection, int bytes_sent);

	/* TCP/UDP: called on graceful or remote close */
	void (*on_close)(NetworkConnection *connection);

	/* TCP/UDP: called on socket or protocol error */
	void (*on_error)(NetworkConnection *connection, ErrorResult error);

	/* UDP: called when a datagram arrives; includes sender address */
	void (*on_datagram_read)(NetworkConnection *connection,
							 NetworkBuffer buffer, NetworkAddress sender);

	/* Caller state — retrieved via fun_network_connection_get_user_data() */
	void *user_data;
} NetworkHandlers;

/* ------------------------------------------------------------------
 * Opaque storage types.
 * Internal layout is platform-specific and defined in the arch layer.
 * Never access fields directly; always use the API functions below.
 * ------------------------------------------------------------------ */
struct NetworkLoop_s {
	char _storage[NETWORK_LOOP_SIZE];
};

struct NetworkConnection_s {
	char _storage[NETWORK_CONNECTION_SIZE];
};

/* ------------------------------------------------------------------
 * NetworkAddress — parse and format
 * ------------------------------------------------------------------ */

/*
 * Parse a numeric "IPv4:port" or "[IPv6]:port" string into a NetworkAddress.
 * Hostname strings (e.g. "example.com:80") are rejected.
 * Port 0 is accepted (OS will assign an ephemeral port on bind).
 *
 * Returns ERROR_CODE_NETWORK_ADDRESS_PARSE_FAILED on malformed input.
 */
CanReturnError(NetworkAddress) fun_network_address_parse(const char *str);

/*
 * Format a NetworkAddress into "a.b.c.d:port" or "[addr]:port".
 * Writes a null-terminated string into out_buf (size = out_buf_size).
 *
 * Returns ERROR_CODE_BUFFER_TOO_SMALL if out_buf_size is insufficient.
 */
CanReturnError(void)
	fun_network_address_to_string(NetworkAddress address, char *out_buf,
								  size_t out_buf_size);

/* ------------------------------------------------------------------
 * NetworkBuffer utilities
 * ------------------------------------------------------------------ */

/*
 * Return a sub-buffer starting at offset with the given length.
 * Returns ERROR_CODE_NETWORK_ADDRESS_PARSE_FAILED if out of bounds.
 */
CanReturnError(NetworkBuffer)
	fun_network_buffer_slice(NetworkBuffer buffer, size_t offset,
							 size_t length);

/*
 * Return the sum of all buffer lengths in the vector.
 */
size_t fun_network_buffer_vector_total_length(NetworkBufferVector vec);

/* ------------------------------------------------------------------
 * NetworkLoop — event loop lifecycle
 * ------------------------------------------------------------------ */

/*
 * Initialise a caller-allocated NetworkLoop.
 * The storage at loop must be at least NETWORK_LOOP_SIZE bytes.
 * Cast: NetworkLoop *loop = (NetworkLoop *)your_char_array;
 */
CanReturnError(void) fun_network_loop_init(NetworkLoop *loop);

/*
 * Run the event loop, blocking until fun_network_loop_stop() is called.
 * Dispatches I/O callbacks as events arrive.
 */
CanReturnError(void) fun_network_loop_run(NetworkLoop *loop);

/*
 * Run one iteration of the event loop with a timeout.
 *   timeout_ms =  0  : return immediately (pure poll)
 *   timeout_ms = -1  : block until at least one event arrives
 *   timeout_ms >  0  : wait up to timeout_ms milliseconds
 *
 * Dispatches all ready callbacks before returning.
 */
CanReturnError(void)
	fun_network_loop_run_once(NetworkLoop *loop, int timeout_ms);

/*
 * Signal the loop to exit after the current dispatch cycle.
 * Safe to call from within a callback.
 */
CanReturnError(void) fun_network_loop_stop(NetworkLoop *loop);

/*
 * Release all platform resources held by the loop.
 * All connections must be closed before calling this.
 */
CanReturnError(void) fun_network_loop_destroy(NetworkLoop *loop);

/* ------------------------------------------------------------------
 * TCP client
 * ------------------------------------------------------------------ */

/*
 * Begin a non-blocking TCP connection to address, registering connection
 * with loop. connection must point to caller-allocated storage of at least
 * NETWORK_CONNECTION_SIZE bytes.
 *
 * receive_buffers : scatter-receive vector; must remain valid for the life
 *                   of the connection.
 * handlers        : callback table; copied into connection storage.
 * timeout_ms      : connect timeout in ms; 0 = no timeout.
 *
 * Returns immediately. on_connect fires on success; on_error fires on failure
 * or timeout (ERROR_CODE_NETWORK_CONNECT_TIMEOUT).
 */
CanReturnError(void)
	fun_network_tcp_connect(NetworkLoop *loop, NetworkConnection *connection,
							NetworkAddress address,
							NetworkBufferVector receive_buffers,
							NetworkHandlers handlers, int timeout_ms);

/*
 * Enqueue a single buffer for transmission.
 * buffer must remain valid until on_write_complete fires.
 *
 * Returns ERROR_CODE_NETWORK_WOULD_BLOCK if the kernel send buffer is full.
 * Caller must wait for on_write_complete before sending again.
 * Partial writes are handled internally; on_write_complete fires once only.
 */
CanReturnError(void)
	fun_network_tcp_send(NetworkConnection *connection, NetworkBuffer buffer);

/*
 * Enqueue a scatter/gather vector for transmission.
 * All buffers in vec must remain valid until on_write_complete fires.
 * Uses writev / WSASend(multi-WSABUF) for a single syscall where possible.
 *
 * Same backpressure contract as fun_network_tcp_send.
 */
CanReturnError(void) fun_network_tcp_send_vector(NetworkConnection *connection,
												 NetworkBufferVector vec);

/*
 * Initiate a graceful close. on_close fires after the close completes.
 * Safe to call from within a callback (deferred).
 */
CanReturnError(void) fun_network_tcp_close(NetworkConnection *connection);

/* ------------------------------------------------------------------
 * UDP socket
 * ------------------------------------------------------------------ */

/*
 * Create a UDP socket, bind to local_address, and register with loop.
 * connection must point to caller-allocated NETWORK_CONNECTION_SIZE storage.
 * Use port 0 in local_address for OS-assigned ephemeral port.
 */
CanReturnError(void)
	fun_network_udp_bind(NetworkLoop *loop, NetworkConnection *connection,
						 NetworkAddress local_address,
						 NetworkBuffer recv_buffer, NetworkHandlers handlers);

/*
 * Send buffer as a UDP datagram to destination.
 */
CanReturnError(void)
	fun_network_udp_send_to(NetworkConnection *connection, NetworkBuffer buffer,
							NetworkAddress destination);

/*
 * Close the UDP socket and deregister from the loop. on_close fires.
 * Safe to call from within a callback (deferred).
 */
CanReturnError(void) fun_network_udp_close(NetworkConnection *connection);

/* ------------------------------------------------------------------
 * Per-connection user data accessor
 * ------------------------------------------------------------------ */

/*
 * Retrieve the user_data pointer registered in NetworkHandlers.
 * Use this inside callbacks to access per-connection application state.
 */
void *fun_network_connection_get_user_data(NetworkConnection *connection);

#endif /* LIBRARY_NETWORK_H */
