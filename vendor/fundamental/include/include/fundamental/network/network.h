#ifndef LIBRARY_NETWORK_H
#define LIBRARY_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#include "../async/async.h"
#include "../error/error.h"
#include "../memory/memory.h"

/*
 * Network Module — simple async TCP/UDP interface.
 *
 * TCP: connect, send, receive_exact, close.
 * UDP: fire-and-forget send.
 *
 * All operations return AsyncResult; use fun_async_await() to wait for
 * completion.
 */

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
	uint16_t port; /* host byte order, 0-65535 */
} NetworkAddress;

DEFINE_RESULT_TYPE(NetworkAddress);

/* ------------------------------------------------------------------
 * NetworkBuffer — single contiguous memory region with a length field.
 * On receive_exact completion, length is set to the number of bytes received.
 * ------------------------------------------------------------------ */
typedef struct {
	void *data;
	size_t length;
} NetworkBuffer;

DEFINE_RESULT_TYPE(NetworkBuffer);
typedef NetworkBuffer *OutputNetworkBuffer;

/* ------------------------------------------------------------------
 * TcpNetworkConnection — opaque handle to a pooled TCP connection.
 * Returned via OutputTcpNetworkConnection on successful connect.
 * ------------------------------------------------------------------ */
struct TcpNetworkConnection_s;
typedef struct TcpNetworkConnection_s *TcpNetworkConnection;
typedef TcpNetworkConnection *OutputTcpNetworkConnection;

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
 * TCP client — async interface
 * ------------------------------------------------------------------ */

/*
 * Begin a non-blocking TCP connection to address.
 * On ASYNC_COMPLETED, *out_conn is set to the pool slot handle.
 * On ASYNC_ERROR, *out_conn is unchanged.
 *
 * Call fun_async_await(&result, timeout_ms) to wait.
 */
AsyncResult fun_network_tcp_connect(NetworkAddress address,
									OutputTcpNetworkConnection out_conn);

/*
 * Asynchronously send data bytes over an established TCP connection.
 * data must remain valid until the AsyncResult reaches ASYNC_COMPLETED.
 *
 * Call fun_async_await(&result, timeout_ms) to wait.
 */
AsyncResult fun_network_tcp_send(TcpNetworkConnection conn, const void *data,
								 size_t length);

/*
 * Asynchronously receive exactly `bytes` bytes into response->data.
 * response->data must point to a buffer of at least `bytes` bytes.
 * On ASYNC_COMPLETED, response->length is set to `bytes`.
 *
 * Uses an internal staging buffer to handle TCP stream framing.
 * Surplus bytes from the underlying recv are retained for subsequent calls.
 *
 * Call fun_async_await(&result, timeout_ms) to wait.
 */
AsyncResult fun_network_tcp_receive_exact(TcpNetworkConnection conn,
										  OutputNetworkBuffer response,
										  size_t bytes);

/*
 * Close the TCP connection and return the pool slot.
 * After this call, conn must not be used.
 */
voidResult fun_network_tcp_close(TcpNetworkConnection conn);

/* ------------------------------------------------------------------
 * UDP — fire-and-forget send
 * ------------------------------------------------------------------ */

/*
 * Send a UDP datagram to addr.  Creates an ephemeral socket, sends, closes.
 * Returns a pre-completed AsyncResult (ASYNC_COMPLETED or ASYNC_ERROR).
 * No await needed; the operation is synchronous from the caller's perspective.
 */
AsyncResult fun_network_udp_send(NetworkAddress addr, const void *data,
								 size_t length);

#endif /* LIBRARY_NETWORK_H */
