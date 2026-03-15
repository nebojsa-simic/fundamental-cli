---
name: fundamental-network
description: Networking with Fundamental Library - TCP/UDP, event loop, address parse, scatter/gather I/O
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: networking
  related: fundamental-async, fundamental-memory
---

# Fundamental Library - Network Skill

I provide copy-paste examples for non-blocking TCP/UDP networking using the Fundamental Library.

---

## Quick Reference

| Task | Function | Notes |
|------|----------|-------|
| Parse address | `fun_network_address_parse()` | Numeric IP only; `"1.2.3.4:80"` or `"[::1]:9000"` |
| Format address | `fun_network_address_to_string()` | Caller supplies buffer |
| Init loop | `fun_network_loop_init()` | Caller-allocated storage |
| Run loop | `fun_network_loop_run()` | Blocking until `loop_stop` |
| Run once | `fun_network_loop_run_once()` | `0`=poll, `-1`=block, `N`=timeout ms |
| Stop loop | `fun_network_loop_stop()` | Safe from callback |
| Destroy loop | `fun_network_loop_destroy()` | After all connections closed |
| TCP connect | `fun_network_tcp_connect()` | Non-blocking; fires `on_connect` |
| TCP send | `fun_network_tcp_send()` | Single buffer; fires `on_write_complete` |
| TCP send vector | `fun_network_tcp_send_vector()` | Scatter/gather; single syscall |
| TCP close | `fun_network_tcp_close()` | Deferred-safe from callback |
| UDP bind | `fun_network_udp_bind()` | Port 0 = OS-assigned |
| UDP send | `fun_network_udp_send_to()` | Single datagram |
| UDP close | `fun_network_udp_close()` | Deferred-safe from callback |
| Buffer slice | `fun_network_buffer_slice()` | Sub-buffer without copy |
| Vector length | `fun_network_buffer_vector_total_length()` | Sum of all segment lengths |
| Get user data | `fun_network_connection_get_user_data()` | Access per-connection state in callbacks |

---

## Pattern: TCP Client

```c
#include "network/network.h"

/* Application state — stored on stack or caller-allocated heap */
typedef struct {
    int request_sent;
    char recv_data[4096];
} MyState;

static void on_connect(NetworkConnection *conn)
{
    MyState *state = (MyState *)fun_network_connection_get_user_data(conn);
    NetworkBuffer buf = { "GET / HTTP/1.0\r\n\r\n", 18 };
    fun_network_tcp_send(conn, buf);
    state->request_sent = 1;
}

static void on_read(NetworkConnection *conn, NetworkBufferVector bufs,
                    int bytes_received)
{
    (void)bufs;
    (void)bytes_received;
    fun_network_tcp_close(conn);   /* safe to call from callback */
}

static void on_close(NetworkConnection *conn)
{
    MyState *state = (MyState *)fun_network_connection_get_user_data(conn);
    (void)state;
    /* ... */
}

static void on_error(NetworkConnection *conn, ErrorResult error)
{
    (void)conn;
    (void)error;
    /* error.code is one of ERROR_CODE_NETWORK_* */
}

int main(void)
{
    /* 1. Parse address */
    NetworkAddressResult addr = fun_network_address_parse("93.184.216.34:80");
    if (fun_error_is_error(addr.error)) return 1;

    /* 2. Allocate loop and connection on the stack */
    char loop_storage[NETWORK_LOOP_SIZE];
    NetworkLoop *loop = (NetworkLoop *)loop_storage;

    char conn_storage[NETWORK_CONNECTION_SIZE];
    NetworkConnection *conn = (NetworkConnection *)conn_storage;

    /* 3. Init loop */
    voidResult r = fun_network_loop_init(loop);
    if (fun_error_is_error(r.error)) return 1;

    /* 4. Set up receive buffers (scatter across two segments) */
    MyState state = { 0 };
    NetworkBuffer recv_bufs[2] = {
        { state.recv_data,        2048 },
        { state.recv_data + 2048, 2048 },
    };
    NetworkBufferVector recv_vec = { recv_bufs, 2 };

    /* 5. Set up handlers */
    NetworkHandlers handlers = {
        .on_connect  = on_connect,
        .on_read     = on_read,
        .on_close    = on_close,
        .on_error    = on_error,
        .user_data   = &state,
    };

    /* 6. Connect (non-blocking; 5 s timeout) */
    r = fun_network_tcp_connect(loop, conn, addr.value, recv_vec, handlers,
                                5000);
    if (fun_error_is_error(r.error)) return 1;

    /* 7. Run event loop */
    fun_network_loop_run(loop);

    /* 8. Destroy */
    fun_network_loop_destroy(loop);
    return 0;
}
```

---

## Pattern: Vectored Send (scatter/gather)

```c
/* Send three non-contiguous buffers in a single syscall */
char header[32]   = "HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\n";
char body[5]      = "hello";
char trailer[2]   = "\r\n";

NetworkBuffer bufs[3] = {
    { header,  38 },
    { body,     5 },
    { trailer,  2 },
};
NetworkBufferVector vec = { bufs, 3 };

voidResult r = fun_network_tcp_send_vector(conn, vec);
if (fun_error_is_error(r.error)) {
    if (r.error.code == ERROR_CODE_NETWORK_WOULD_BLOCK) {
        /* Wait for on_write_complete, then send again */
    }
}
```

---

## Pattern: UDP Loopback

```c
char loop_storage[NETWORK_LOOP_SIZE];
NetworkLoop *loop = (NetworkLoop *)loop_storage;
fun_network_loop_init(loop);

/* Sender socket */
char sender_storage[NETWORK_CONNECTION_SIZE];
NetworkConnection *sender = (NetworkConnection *)sender_storage;

NetworkAddressResult local = fun_network_address_parse("0.0.0.0:0");
char dummy_recv[1];
NetworkBuffer dummy_buf = { dummy_recv, 1 };
NetworkHandlers no_handlers = { 0 };
fun_network_udp_bind(loop, sender, local.value, dummy_buf, no_handlers);

/* Receiver socket */
char recv_storage[NETWORK_CONNECTION_SIZE];
NetworkConnection *receiver = (NetworkConnection *)recv_storage;

static void on_datagram(NetworkConnection *conn, NetworkBuffer buf,
                        NetworkAddress sender_addr)
{
    (void)conn; (void)sender_addr;
    /* buf.data contains the datagram bytes, buf.length the byte count */
    fun_network_loop_stop(/* loop ptr needed — store in user_data */);
}

char recv_buf[512];
NetworkBuffer recv_buffer = { recv_buf, 512 };
NetworkHandlers recv_handlers = { .on_datagram_read = on_datagram };

NetworkAddressResult bind_addr = fun_network_address_parse("127.0.0.1:9999");
fun_network_udp_bind(loop, receiver, bind_addr.value, recv_buffer,
                     recv_handlers);

/* Send datagram */
NetworkAddressResult dest = fun_network_address_parse("127.0.0.1:9999");
NetworkBuffer payload = { "ping", 4 };
fun_network_udp_send_to(sender, payload, dest.value);

fun_network_loop_run(loop);
fun_network_loop_destroy(loop);
```

---

## Pattern: Address Parse and Format

```c
/* Parse */
NetworkAddressResult r = fun_network_address_parse("192.168.1.1:443");
if (fun_error_is_error(r.error)) { /* bad address */ }

NetworkAddress addr = r.value;
/* addr.family == NETWORK_ADDRESS_IPV4 */
/* addr.bytes[0..3] = 192, 168, 1, 1 */
/* addr.port == 443 */

/* Format back to string */
char buf[64];
voidResult fmt = fun_network_address_to_string(addr, buf, sizeof(buf));
if (fun_error_is_error(fmt.error)) { /* buffer too small */ }
/* buf == "192.168.1.1:443" */

/* IPv6 */
NetworkAddressResult r6 = fun_network_address_parse("[::1]:9000");
/* r6.value.family == NETWORK_ADDRESS_IPV6, port == 9000 */
```

---

## Backpressure Flow

```
app calls fun_network_tcp_send()
  → kernel buffer full → returns ERROR_CODE_NETWORK_WOULD_BLOCK
  → app stops sending
  → kernel drains buffer
  → arch layer fires on_write_complete
  → app calls fun_network_tcp_send() again
```

## Size Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `NETWORK_LOOP_SIZE` | 1024 | Minimum storage for `NetworkLoop` (stack-allocate: `char buf[NETWORK_LOOP_SIZE]`) |
| `NETWORK_CONNECTION_SIZE` | 1024 | Minimum storage for `NetworkConnection` |
| `NETWORK_ADDRESS_MAX_BYTES` | 16 | Raw bytes in `NetworkAddress` (IPv6 max) |

## Error Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 230 | `ERROR_CODE_NETWORK_INIT_FAILED` | Loop initialisation failed |
| 231 | `ERROR_CODE_NETWORK_CONNECT_FAILED` | TCP connect failed |
| 232 | `ERROR_CODE_NETWORK_CONNECT_TIMEOUT` | Connect timed out |
| 233 | `ERROR_CODE_NETWORK_SEND_FAILED` | Send failed |
| 234 | `ERROR_CODE_NETWORK_WOULD_BLOCK` | Kernel send buffer full |
| 235 | `ERROR_CODE_NETWORK_RECEIVE_FAILED` | Receive failed |
| 236 | `ERROR_CODE_NETWORK_BIND_FAILED` | UDP bind failed |
| 237 | `ERROR_CODE_NETWORK_CLOSED` | Connection already closed |
| 238 | `ERROR_CODE_NETWORK_ADDRESS_PARSE_FAILED` | Address string malformed |
| 239 | `ERROR_CODE_NETWORK_INVALID_STATE` | Connection in wrong state |
