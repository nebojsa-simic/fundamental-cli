/*
 * Network module — platform-independent core.
 *
 * Implements:
 *   - NetworkAddress parse / format
 *   - Connection pool management
 *   - Async poll functions for connect, send, receive_exact
 *   - Public API: fun_network_tcp_connect/send/receive_exact/close/udp_send
 *
 * No OS-specific code lives here.  Platform logic is in arch/network/.
 */

#include "fundamental/network/network.h"
#include "fundamental/memory/memory.h"
#include "fundamental/string/string.h"

/* ------------------------------------------------------------------
 * Arch-layer declarations (implemented per platform in arch/network/)
 * ------------------------------------------------------------------ */

int fun_network_arch_tcp_connect(NetworkAddress addr, intptr_t *out_fd);
int fun_network_arch_tcp_poll_connect(intptr_t fd);
int fun_network_arch_tcp_send(intptr_t fd, const void *data, size_t len,
							  size_t *sent);
int fun_network_arch_tcp_recv(intptr_t fd, void *data, size_t len,
							  size_t *received);
void fun_network_arch_tcp_close_fd(intptr_t fd);
int fun_network_arch_udp_send(NetworkAddress addr, const void *data,
							  size_t len);

/* ------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------ */

/* Parse a decimal integer from str[0..len-1]. Returns -1 on failure. */
static int64_t parse_decimal(const char *str, size_t len)
{
	if (len == 0 || len > 20)
		return -1;
	int64_t result = 0;
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		if (c < '0' || c > '9')
			return -1;
		result = result * 10 + (c - '0');
		if (result > 99999999999LL) /* overflow guard */
			return -1;
	}
	return result;
}

/* Parse a hex nibble. Returns -1 on failure. */
static int parse_hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/*
 * Parse a colon-separated port suffix ":NNNN" at str[pos..end).
 * Returns the port (0-65535) on success, or -1 on failure.
 */
static int64_t parse_port_suffix(const char *str, size_t pos, size_t end)
{
	if (pos >= end || str[pos] != ':')
		return -1;
	pos++;
	int64_t port = parse_decimal(str + pos, end - pos);
	if (port < 0 || port > 65535)
		return -1;
	return port;
}

/* Write a decimal uint32 into buf. Returns length written (no NUL). */
static size_t write_decimal(uint32_t n, char *buf)
{
	char tmp[12];
	size_t len = 0;
	if (n == 0) {
		buf[0] = '0';
		return 1;
	}
	while (n > 0) {
		tmp[len++] = (char)('0' + n % 10);
		n /= 10;
	}
	for (size_t i = 0; i < len; i++)
		buf[i] = tmp[len - 1 - i];
	return len;
}

/* Write a hex byte "xy" into buf. Returns 2. */
static size_t write_hex_byte(uint8_t byte, char *buf)
{
	const char hex[] = "0123456789abcdef";
	buf[0] = hex[(byte >> 4) & 0xf];
	buf[1] = hex[byte & 0xf];
	return 2;
}

/* ------------------------------------------------------------------
 * NetworkAddress — parse
 * ------------------------------------------------------------------ */

NetworkAddressResult fun_network_address_parse(const char *str)
{
	NetworkAddressResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!str) {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	size_t len = fun_string_length(str);
	if (len == 0) {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	NetworkAddress addr;
	for (size_t i = 0; i < NETWORK_ADDRESS_MAX_BYTES; i++)
		addr.bytes[i] = 0;

	/* ---- IPv6: "[addr]:port" ---- */
	if (str[0] == '[') {
		/* Find closing bracket */
		size_t close = 0;
		for (size_t i = 1; i < len; i++) {
			if (str[i] == ']') {
				close = i;
				break;
			}
		}
		if (close == 0) {
			result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
			return result;
		}

		/* Parse port */
		int64_t port = parse_port_suffix(str, close + 1, len);
		if (port < 0) {
			result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
			return result;
		}

		/* Parse IPv6 address str[1..close-1] */
		/* We support full and compressed (::) notation */
		const char *ip6 = str + 1;
		size_t ip6_len = close - 1;

		/* Expand :: into full 16-byte address */
		uint16_t groups[8];
		for (int i = 0; i < 8; i++)
			groups[i] = 0;

		/* Find :: */
		size_t double_colon = ip6_len; /* marks "not found" */
		for (size_t i = 0; i + 1 < ip6_len; i++) {
			if (ip6[i] == ':' && ip6[i + 1] == ':') {
				double_colon = i;
				break;
			}
		}

		/* Parse left side */
		int left_count = 0;
		uint16_t left[8];
		size_t pos = 0;
		size_t end = (double_colon < ip6_len) ? double_colon : ip6_len;
		while (pos <= end && left_count < 8) {
			size_t colon = end;
			for (size_t i = pos; i < end; i++) {
				if (ip6[i] == ':') {
					colon = i;
					break;
				}
			}
			if (colon == pos && pos == 0 && double_colon == 0)
				break; /* leading :: */
			if (colon > pos) {
				/* Parse hex group */
				uint32_t val = 0;
				for (size_t i = pos; i < colon; i++) {
					int nibble = parse_hex_nibble(ip6[i]);
					if (nibble < 0) {
						result.error =
							ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
						return result;
					}
					val = val * 16 + (uint32_t)nibble;
					if (val > 0xFFFF) {
						result.error =
							ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
						return result;
					}
				}
				left[left_count++] = (uint16_t)val;
			}
			if (colon >= end)
				break;
			pos = colon + 1;
		}

		/* Parse right side (after ::) */
		int right_count = 0;
		uint16_t right[8];
		if (double_colon < ip6_len) {
			pos = double_colon + 2;
			while (pos <= ip6_len && right_count < 8) {
				size_t colon = ip6_len;
				for (size_t i = pos; i < ip6_len; i++) {
					if (ip6[i] == ':') {
						colon = i;
						break;
					}
				}
				if (colon > pos) {
					uint32_t val = 0;
					for (size_t i = pos; i < colon; i++) {
						int nibble = parse_hex_nibble(ip6[i]);
						if (nibble < 0) {
							result.error =
								ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
							return result;
						}
						val = val * 16 + (uint32_t)nibble;
						if (val > 0xFFFF) {
							result.error =
								ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
							return result;
						}
					}
					right[right_count++] = (uint16_t)val;
				}
				if (colon >= ip6_len)
					break;
				pos = colon + 1;
			}
		}

		if (left_count + right_count > 8) {
			result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
			return result;
		}

		int pad = 8 - left_count - right_count;
		for (int i = 0; i < left_count; i++)
			groups[i] = left[i];
		for (int i = 0; i < pad; i++)
			groups[left_count + i] = 0;
		for (int i = 0; i < right_count; i++)
			groups[left_count + pad + i] = right[i];

		for (int i = 0; i < 8; i++) {
			addr.bytes[i * 2] = (uint8_t)(groups[i] >> 8);
			addr.bytes[i * 2 + 1] = (uint8_t)(groups[i] & 0xFF);
		}

		addr.family = NETWORK_ADDRESS_IPV6;
		addr.port = (uint16_t)port;
		result.value = addr;
		return result;
	}

	/* ---- IPv4: "a.b.c.d:port" ---- */
	/* Find last colon for port */
	size_t last_colon = len;
	for (size_t i = len; i > 0; i--) {
		if (str[i - 1] == ':') {
			last_colon = i - 1;
			break;
		}
	}
	if (last_colon == len) {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	int64_t port = parse_port_suffix(str, last_colon, len);
	if (port < 0) {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	/* Parse four octets */
	uint8_t octets[4];
	size_t pos = 0;
	for (int octet = 0; octet < 4; octet++) {
		size_t dot = last_colon;
		for (size_t i = pos; i < last_colon; i++) {
			if (str[i] == '.') {
				dot = i;
				break;
			}
		}
		if (octet < 3 && dot == last_colon) {
			/* Missing dot */
			result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
			return result;
		}
		/* Reject if any non-digit character appears (catches hostnames) */
		size_t seg_end = (octet < 3) ? dot : last_colon;
		int64_t val = parse_decimal(str + pos, seg_end - pos);
		if (val < 0 || val > 255) {
			result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
			return result;
		}
		octets[octet] = (uint8_t)val;
		pos = dot + 1;
	}

	addr.family = NETWORK_ADDRESS_IPV4;
	addr.bytes[0] = octets[0];
	addr.bytes[1] = octets[1];
	addr.bytes[2] = octets[2];
	addr.bytes[3] = octets[3];
	addr.port = (uint16_t)port;
	result.value = addr;
	return result;
}

/* ------------------------------------------------------------------
 * NetworkAddress — format
 * ------------------------------------------------------------------ */

voidResult fun_network_address_to_string(NetworkAddress address, char *out_buf,
										 size_t out_buf_size)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!out_buf || out_buf_size == 0) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	char tmp[64];
	size_t pos = 0;

	if (address.family == NETWORK_ADDRESS_IPV4) {
		for (int i = 0; i < 4; i++) {
			if (i > 0)
				tmp[pos++] = '.';
			pos += write_decimal(address.bytes[i], tmp + pos);
		}
		tmp[pos++] = ':';
		pos += write_decimal(address.port, tmp + pos);
	} else if (address.family == NETWORK_ADDRESS_IPV6) {
		tmp[pos++] = '[';
		for (int i = 0; i < 8; i++) {
			if (i > 0)
				tmp[pos++] = ':';
			uint16_t group = ((uint16_t)address.bytes[i * 2] << 8) |
							 address.bytes[i * 2 + 1];
			pos += write_hex_byte((uint8_t)(group >> 8), tmp + pos);
			pos += write_hex_byte((uint8_t)(group & 0xFF), tmp + pos);
		}
		tmp[pos++] = ']';
		tmp[pos++] = ':';
		pos += write_decimal(address.port, tmp + pos);
	} else {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	if (pos + 1 > out_buf_size) {
		result.error = ERROR_RESULT_BUFFER_TOO_SMALL;
		return result;
	}

	for (size_t i = 0; i < pos; i++)
		out_buf[i] = tmp[i];
	out_buf[pos] = '\0';
	return result;
}

/* ------------------------------------------------------------------
 * Connection pool
 * ------------------------------------------------------------------ */

#define NETWORK_POOL_SIZE 16
#define NETWORK_RX_BUF_DEFAULT 4096

#define CONN_OP_NONE 0
#define CONN_OP_CONNECT 1
#define CONN_OP_SEND 2
#define CONN_OP_RECV_EXACT 3

struct TcpNetworkConnection_s {
	intptr_t fd; /* socket fd; -1 = not connected */
	int in_use;
	Memory rx_buf; /* overflow / staging buffer */
	size_t rx_head; /* offset of first valid byte in rx_buf */
	size_t rx_len; /* bytes of valid data in rx_buf */
	size_t rx_cap; /* total capacity of rx_buf */
	int op_type; /* current op: CONN_OP_* */
	union {
		struct {
			OutputTcpNetworkConnection out_conn;
		} connect;
		struct {
			const void *data;
			size_t total;
			size_t sent;
		} send;
		struct {
			OutputNetworkBuffer response;
			size_t bytes;
			size_t received;
		} recv_exact;
	} op;
};

static struct TcpNetworkConnection_s conn_pool[NETWORK_POOL_SIZE];
static int pool_ready = 0;
static size_t g_rx_buf_size = NETWORK_RX_BUF_DEFAULT;

static void pool_init(void)
{
	if (pool_ready)
		return;
	for (int i = 0; i < NETWORK_POOL_SIZE; i++) {
		conn_pool[i].fd = -1;
		conn_pool[i].in_use = 0;
		conn_pool[i].rx_buf = (Memory)0;
		conn_pool[i].rx_head = 0;
		conn_pool[i].rx_len = 0;
		conn_pool[i].rx_cap = 0;
		conn_pool[i].op_type = CONN_OP_NONE;
	}
	pool_ready = 1;
}

static struct TcpNetworkConnection_s *pool_acquire(void)
{
	pool_init();
	for (int i = 0; i < NETWORK_POOL_SIZE; i++) {
		if (!conn_pool[i].in_use) {
			conn_pool[i].in_use = 1;
			conn_pool[i].fd = -1;
			conn_pool[i].rx_head = 0;
			conn_pool[i].rx_len = 0;
			conn_pool[i].op_type = CONN_OP_NONE;
			return &conn_pool[i];
		}
	}
	return (struct TcpNetworkConnection_s *)0;
}

static void pool_release(struct TcpNetworkConnection_s *conn)
{
	if (!conn)
		return;
	if (conn->fd != -1) {
		fun_network_arch_tcp_close_fd(conn->fd);
		conn->fd = -1;
	}
	if (conn->rx_buf) {
		fun_memory_free(&conn->rx_buf);
		conn->rx_buf = (Memory)0;
	}
	conn->rx_head = 0;
	conn->rx_len = 0;
	conn->rx_cap = 0;
	conn->op_type = CONN_OP_NONE;
	conn->in_use = 0;
}

/* ------------------------------------------------------------------
 * Poll functions
 * ------------------------------------------------------------------ */

static AsyncStatus poll_connect(AsyncResult *result)
{
	struct TcpNetworkConnection_s *conn =
		(struct TcpNetworkConnection_s *)result->state;

	int rc = fun_network_arch_tcp_poll_connect(conn->fd);
	if (rc == 0) {
		/* Still pending */
		result->status = ASYNC_PENDING;
		return ASYNC_PENDING;
	}
	if (rc == 1) {
		/* Connected — allocate rx staging buffer */
		MemoryResult mr = fun_memory_allocate(g_rx_buf_size);
		if (fun_error_is_error(mr.error)) {
			pool_release(conn);
			result->status = ASYNC_ERROR;
			result->error = ERROR_RESULT_NETWORK_CONNECT_FAILED;
			return ASYNC_ERROR;
		}
		conn->rx_buf = mr.value;
		conn->rx_cap = g_rx_buf_size;
		conn->rx_head = 0;
		conn->rx_len = 0;
		conn->op_type = CONN_OP_NONE;
		*(conn->op.connect.out_conn) = conn;
		result->status = ASYNC_COMPLETED;
		result->error = ERROR_RESULT_NO_ERROR;
		return ASYNC_COMPLETED;
	}
	/* Error */
	pool_release(conn);
	result->status = ASYNC_ERROR;
	result->error = ERROR_RESULT_NETWORK_CONNECT_FAILED;
	return ASYNC_ERROR;
}

static AsyncStatus poll_send(AsyncResult *result)
{
	struct TcpNetworkConnection_s *conn =
		(struct TcpNetworkConnection_s *)result->state;

	const uint8_t *data = (const uint8_t *)conn->op.send.data;
	size_t total = conn->op.send.total;
	size_t sent = conn->op.send.sent;

	while (sent < total) {
		size_t chunk = 0;
		int rc = fun_network_arch_tcp_send(conn->fd, data + sent, total - sent,
										   &chunk);
		if (rc == 1) {
			/* Would-block */
			conn->op.send.sent = sent;
			result->status = ASYNC_PENDING;
			return ASYNC_PENDING;
		}
		if (rc == -1) {
			conn->op_type = CONN_OP_NONE;
			result->status = ASYNC_ERROR;
			result->error = ERROR_RESULT_NETWORK_SEND_FAILED;
			return ASYNC_ERROR;
		}
		sent += chunk;
	}

	conn->op.send.sent = sent;
	conn->op_type = CONN_OP_NONE;
	result->status = ASYNC_COMPLETED;
	result->error = ERROR_RESULT_NO_ERROR;
	return ASYNC_COMPLETED;
}

static AsyncStatus poll_recv_exact(AsyncResult *result)
{
	struct TcpNetworkConnection_s *conn =
		(struct TcpNetworkConnection_s *)result->state;

	uint8_t *dest = (uint8_t *)conn->op.recv_exact.response->data;
	size_t bytes = conn->op.recv_exact.bytes;
	size_t received = conn->op.recv_exact.received;

	/* Step 1: drain from rx_buf first */
	if (conn->rx_len > 0 && received < bytes) {
		size_t take = bytes - received;
		if (take > conn->rx_len)
			take = conn->rx_len;
		voidResult cr =
			fun_memory_copy((Memory)((uint8_t *)conn->rx_buf + conn->rx_head),
							(Memory)(dest + received), take);
		(void)cr;
		conn->rx_head += take;
		conn->rx_len -= take;
		if (conn->rx_len == 0)
			conn->rx_head = 0;
		received += take;
	}

	if (received >= bytes) {
		conn->op.recv_exact.received = received;
		conn->op.recv_exact.response->length = bytes;
		conn->op_type = CONN_OP_NONE;
		result->status = ASYNC_COMPLETED;
		result->error = ERROR_RESULT_NO_ERROR;
		return ASYNC_COMPLETED;
	}

	/* Step 2: need more bytes from socket — recv into rx_buf */
	/* Compact rx_buf if needed to make space at the tail */
	size_t space = conn->rx_cap - conn->rx_head - conn->rx_len;
	if (space == 0) {
		/* Compact: move rx_len bytes to front */
		if (conn->rx_head > 0 && conn->rx_len > 0) {
			uint8_t *buf = (uint8_t *)conn->rx_buf;
			for (size_t i = 0; i < conn->rx_len; i++)
				buf[i] = buf[conn->rx_head + i];
			conn->rx_head = 0;
		}
		space = conn->rx_cap - conn->rx_len;
	}

	if (space == 0) {
		/* Buffer full, can't recv more — should not happen normally */
		result->status = ASYNC_PENDING;
		return ASYNC_PENDING;
	}

	/* Recv into the tail of rx_buf */
	uint8_t *tail = (uint8_t *)conn->rx_buf + conn->rx_head + conn->rx_len;
	size_t got = 0;
	int rc = fun_network_arch_tcp_recv(conn->fd, tail, space, &got);
	if (rc == 1) {
		/* Would-block */
		conn->op.recv_exact.received = received;
		result->status = ASYNC_PENDING;
		return ASYNC_PENDING;
	}
	if (rc == -1) {
		conn->op_type = CONN_OP_NONE;
		result->status = ASYNC_ERROR;
		result->error = ERROR_RESULT_NETWORK_RECEIVE_FAILED;
		return ASYNC_ERROR;
	}
	if (got == 0) {
		/* EOF / connection closed */
		conn->op_type = CONN_OP_NONE;
		result->status = ASYNC_ERROR;
		result->error = ERROR_RESULT_NETWORK_CLOSED;
		return ASYNC_ERROR;
	}
	conn->rx_len += got;

	/* Drain what we need from rx_buf into dest */
	size_t take = bytes - received;
	if (take > conn->rx_len)
		take = conn->rx_len;
	voidResult cr =
		fun_memory_copy((Memory)((uint8_t *)conn->rx_buf + conn->rx_head),
						(Memory)(dest + received), take);
	(void)cr;
	conn->rx_head += take;
	conn->rx_len -= take;
	if (conn->rx_len == 0)
		conn->rx_head = 0;
	received += take;
	conn->op.recv_exact.received = received;

	if (received >= bytes) {
		conn->op.recv_exact.response->length = bytes;
		conn->op_type = CONN_OP_NONE;
		result->status = ASYNC_COMPLETED;
		result->error = ERROR_RESULT_NO_ERROR;
		return ASYNC_COMPLETED;
	}

	result->status = ASYNC_PENDING;
	return ASYNC_PENDING;
}

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

AsyncResult fun_network_tcp_connect(NetworkAddress address,
									OutputTcpNetworkConnection out_conn)
{
	AsyncResult result;
	result.poll = poll_connect;
	result.state = (void *)0;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!out_conn) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	struct TcpNetworkConnection_s *conn = pool_acquire();
	if (!conn) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_CONNECT_FAILED;
		return result;
	}

	intptr_t fd = -1;
	int rc = fun_network_arch_tcp_connect(address, &fd);
	if (rc == -1) {
		pool_release(conn);
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_CONNECT_FAILED;
		return result;
	}

	conn->fd = fd;
	conn->op_type = CONN_OP_CONNECT;
	conn->op.connect.out_conn = out_conn;
	*out_conn = (TcpNetworkConnection)0;

	result.state = (void *)conn;
	result.status = ASYNC_PENDING;
	return result;
}

AsyncResult fun_network_tcp_send(TcpNetworkConnection conn, const void *data,
								 size_t length)
{
	AsyncResult result;
	result.poll = poll_send;
	result.state = (void *)0;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!conn || !data) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	conn->op_type = CONN_OP_SEND;
	conn->op.send.data = data;
	conn->op.send.total = length;
	conn->op.send.sent = 0;

	result.state = (void *)conn;
	result.status = ASYNC_PENDING;
	return result;
}

AsyncResult fun_network_tcp_receive_exact(TcpNetworkConnection conn,
										  OutputNetworkBuffer response,
										  size_t bytes)
{
	AsyncResult result;
	result.poll = poll_recv_exact;
	result.state = (void *)0;
	result.error = ERROR_RESULT_NO_ERROR;

	if (!conn || !response) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	conn->op_type = CONN_OP_RECV_EXACT;
	conn->op.recv_exact.response = response;
	conn->op.recv_exact.bytes = bytes;
	conn->op.recv_exact.received = 0;

	result.state = (void *)conn;
	result.status = ASYNC_PENDING;
	return result;
}

voidResult fun_network_tcp_close(TcpNetworkConnection conn)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!conn) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	pool_release(conn);
	return result;
}

AsyncResult fun_network_udp_send(NetworkAddress addr, const void *data,
								 size_t length)
{
	AsyncResult result;
	result.poll = (AsyncPollFn)0;
	result.state = (void *)0;

	if (!data) {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}

	int rc = fun_network_arch_udp_send(addr, data, length);
	if (rc == 0) {
		result.status = ASYNC_COMPLETED;
		result.error = ERROR_RESULT_NO_ERROR;
	} else {
		result.status = ASYNC_ERROR;
		result.error = ERROR_RESULT_NETWORK_SEND_FAILED;
	}
	return result;
}
