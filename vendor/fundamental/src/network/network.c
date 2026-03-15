/*
 * Network module — platform-independent core.
 *
 * Implements:
 *   - NetworkAddress parse / format
 *   - NetworkBuffer slice / vector utilities
 *   - NetworkLoop lifecycle wrappers (delegate to arch layer)
 *   - TCP / UDP API wrappers (validate args, delegate to arch layer)
 *
 * No OS-specific code lives here. Platform logic is in arch/network/.
 */

#include "../../include/network/network.h"
#include "../../include/string/string.h"

/* ------------------------------------------------------------------
 * Arch-layer declarations (implemented per platform in arch/network/)
 * ------------------------------------------------------------------ */

/* Loop */
int fun_network_arch_loop_init(NetworkLoop *loop);
void fun_network_arch_loop_destroy(NetworkLoop *loop);
int fun_network_arch_loop_run(NetworkLoop *loop);
int fun_network_arch_loop_run_once(NetworkLoop *loop, int timeout_ms);
void fun_network_arch_loop_stop(NetworkLoop *loop);

/* TCP */
int fun_network_arch_tcp_connect(NetworkLoop *loop,
								 NetworkConnection *connection,
								 NetworkAddress address,
								 NetworkBufferVector receive_buffers,
								 NetworkHandlers handlers, int timeout_ms);
int fun_network_arch_tcp_send(NetworkConnection *connection,
							  NetworkBuffer buffer);
int fun_network_arch_tcp_send_vector(NetworkConnection *connection,
									 NetworkBufferVector vec);
void fun_network_arch_tcp_close(NetworkConnection *connection);

/* UDP */
int fun_network_arch_udp_bind(NetworkLoop *loop, NetworkConnection *connection,
							  NetworkAddress local_address,
							  NetworkBuffer recv_buffer,
							  NetworkHandlers handlers);
int fun_network_arch_udp_send_to(NetworkConnection *connection,
								 NetworkBuffer buffer,
								 NetworkAddress destination);
void fun_network_arch_udp_close(NetworkConnection *connection);

/* Connection */
void *fun_network_arch_connection_get_user_data(NetworkConnection *connection);

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
 * Returns the port (0–65535) on success, or -1 on failure.
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
 * NetworkBuffer utilities
 * ------------------------------------------------------------------ */

NetworkBufferResult fun_network_buffer_slice(NetworkBuffer buffer,
											 size_t offset, size_t length)
{
	NetworkBufferResult result;
	result.error = ERROR_RESULT_NO_ERROR;

	if (offset + length > buffer.length) {
		result.error = ERROR_RESULT_NETWORK_ADDRESS_PARSE_FAILED;
		return result;
	}

	result.value.data = (char *)buffer.data + offset;
	result.value.length = length;
	return result;
}

size_t fun_network_buffer_vector_total_length(NetworkBufferVector vec)
{
	size_t total = 0;
	for (size_t i = 0; i < vec.count; i++)
		total += vec.buffers[i].length;
	return total;
}

/* ------------------------------------------------------------------
 * NetworkLoop — lifecycle (delegate to arch)
 * ------------------------------------------------------------------ */

voidResult fun_network_loop_init(NetworkLoop *loop)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (fun_network_arch_loop_init(loop) != 0)
		result.error = ERROR_RESULT_NETWORK_INIT_FAILED;
	return result;
}

voidResult fun_network_loop_run(NetworkLoop *loop)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (fun_network_arch_loop_run(loop) != 0)
		result.error =
			fun_error_result(ERROR_CODE_NETWORK_INIT_FAILED, "Loop run failed");
	return result;
}

voidResult fun_network_loop_run_once(NetworkLoop *loop, int timeout_ms)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (fun_network_arch_loop_run_once(loop, timeout_ms) != 0)
		result.error = fun_error_result(ERROR_CODE_NETWORK_INIT_FAILED,
										"Loop run_once failed");
	return result;
}

voidResult fun_network_loop_stop(NetworkLoop *loop)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	fun_network_arch_loop_stop(loop);
	return result;
}

voidResult fun_network_loop_destroy(NetworkLoop *loop)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	fun_network_arch_loop_destroy(loop);
	return result;
}

/* ------------------------------------------------------------------
 * TCP — delegate to arch
 * ------------------------------------------------------------------ */

voidResult fun_network_tcp_connect(NetworkLoop *loop,
								   NetworkConnection *connection,
								   NetworkAddress address,
								   NetworkBufferVector receive_buffers,
								   NetworkHandlers handlers, int timeout_ms)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop || !connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (receive_buffers.count > 0 && !receive_buffers.buffers) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_arch_tcp_connect(
		loop, connection, address, receive_buffers, handlers, timeout_ms);
	if (rc != 0)
		result.error = ERROR_RESULT_NETWORK_CONNECT_FAILED;
	return result;
}

voidResult fun_network_tcp_send(NetworkConnection *connection,
								NetworkBuffer buffer)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_arch_tcp_send(connection, buffer);
	if (rc == 1)
		result.error = ERROR_RESULT_NETWORK_WOULD_BLOCK;
	else if (rc != 0)
		result.error = ERROR_RESULT_NETWORK_SEND_FAILED;
	return result;
}

voidResult fun_network_tcp_send_vector(NetworkConnection *connection,
									   NetworkBufferVector vec)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	if (vec.count > 0 && !vec.buffers) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_arch_tcp_send_vector(connection, vec);
	if (rc == 1)
		result.error = ERROR_RESULT_NETWORK_WOULD_BLOCK;
	else if (rc != 0)
		result.error = ERROR_RESULT_NETWORK_SEND_FAILED;
	return result;
}

voidResult fun_network_tcp_close(NetworkConnection *connection)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	fun_network_arch_tcp_close(connection);
	return result;
}

/* ------------------------------------------------------------------
 * UDP — delegate to arch
 * ------------------------------------------------------------------ */

voidResult fun_network_udp_bind(NetworkLoop *loop,
								NetworkConnection *connection,
								NetworkAddress local_address,
								NetworkBuffer recv_buffer,
								NetworkHandlers handlers)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!loop || !connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_arch_udp_bind(loop, connection, local_address,
									   recv_buffer, handlers);
	if (rc != 0)
		result.error = ERROR_RESULT_NETWORK_BIND_FAILED;
	return result;
}

voidResult fun_network_udp_send_to(NetworkConnection *connection,
								   NetworkBuffer buffer,
								   NetworkAddress destination)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	int rc = fun_network_arch_udp_send_to(connection, buffer, destination);
	if (rc != 0)
		result.error = ERROR_RESULT_NETWORK_SEND_FAILED;
	return result;
}

voidResult fun_network_udp_close(NetworkConnection *connection)
{
	voidResult result;
	result.error = ERROR_RESULT_NO_ERROR;
	if (!connection) {
		result.error = ERROR_RESULT_NULL_POINTER;
		return result;
	}
	fun_network_arch_udp_close(connection);
	return result;
}

/* ------------------------------------------------------------------
 * Connection — user data
 * ------------------------------------------------------------------ */

void *fun_network_connection_get_user_data(NetworkConnection *connection)
{
	if (!connection)
		return NULL;
	return fun_network_arch_connection_get_user_data(connection);
}
