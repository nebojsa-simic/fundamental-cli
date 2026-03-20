---
name: fundamental-network
description: Networking with Fundamental Library - simple async TCP/UDP client, address parse, overflow-buffered receive
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "2.0"
  category: networking
  related: fundamental-async, fundamental-memory
---

# Fundamental Library - Network Skill

I provide copy-paste examples for async TCP/UDP networking using the Fundamental Library.

---

## Quick Reference

| Task | Function | Returns |
|------|----------|---------|
| Parse address | `fun_network_address_parse(str)` | `NetworkAddressResult` |
| Format address | `fun_network_address_to_string(addr, buf, size)` | `voidResult` |
| TCP connect | `fun_network_tcp_connect(addr, &conn)` | `AsyncResult` |
| TCP send | `fun_network_tcp_send(conn, request)` | `AsyncResult` |
| TCP receive N bytes | `fun_network_tcp_receive_exact(conn, &response, n)` | `AsyncResult` |
| TCP close | `fun_network_tcp_close(conn)` | `voidResult` |
| UDP fire-and-forget | `fun_network_udp_send(addr, datagram)` | `AsyncResult` |

---

## Task: TCP Connect, Send, Receive, Close

```c
#include "network/network.h"
#include "async/async.h"
#include "memory/memory.h"

int example_tcp_request(void)
{
    /* --- parse address --- */
    NetworkAddressResult addr_r = fun_network_address_parse("127.0.0.1:8080");
    if (fun_error_is_error(addr_r.error))
        return 1;

    /* --- connect --- */
    TcpNetworkConnection conn = NULL;
    AsyncResult r = fun_network_tcp_connect(addr_r.value, &conn);
    if (fun_error_is_error(fun_async_await(&r, 5000).error))
        return 1;

    /* --- send --- */
    const char   req_str[] = "PING\r\n";
    MemoryResult req_mem   = fun_memory_allocate(sizeof(req_str) - 1);
    if (fun_error_is_error(req_mem.error))
        goto close;
    fun_memory_copy((Memory)req_str, req_mem.value, sizeof(req_str) - 1);

    NetworkBuffer req = { req_mem.value, sizeof(req_str) - 1 };
    r = fun_network_tcp_send(conn, req);
    if (fun_error_is_error(fun_async_await(&r, 5000).error))
        goto free_req;

    /* --- receive 4-byte header (e.g. length prefix) --- */
    MemoryResult hdr_mem = fun_memory_allocate(4);
    if (fun_error_is_error(hdr_mem.error))
        goto free_req;
    NetworkBuffer hdr = { hdr_mem.value, 0 };

    r = fun_network_tcp_receive_exact(conn, &hdr, 4);
    if (fun_error_is_error(fun_async_await(&r, 5000).error))
        goto free_hdr;
    /* hdr.length == 4 */

    /* --- receive body based on header length --- */
    /* ... parse body_len from hdr.data, then: ... */
    /* r = fun_network_tcp_receive_exact(conn, &body, body_len); */

free_hdr:  fun_memory_free(&hdr_mem.value);
free_req:  fun_memory_free(&req_mem.value);
close:     fun_network_tcp_close(conn);
    return 0;
}
```

---

## Task: MySQL-style Two-Phase Receive (header → body)

```c
#define MYSQL_HDR 4   /* 3-byte LE length + 1-byte sequence */

int mysql_recv_packet(TcpNetworkConnection conn,
                      Memory *out_payload, size_t *out_len)
{
    /* phase 1: read 4-byte header */
    MemoryResult hdr_mem = fun_memory_allocate(MYSQL_HDR);
    if (fun_error_is_error(hdr_mem.error)) return 1;
    NetworkBuffer hdr = { hdr_mem.value, 0 };

    AsyncResult r = fun_network_tcp_receive_exact(conn, &hdr, MYSQL_HDR);
    if (fun_error_is_error(fun_async_await(&r, 5000).error))
        goto fail_hdr;

    /* parse 3-byte little-endian length */
    uint8_t *h   = (uint8_t *)hdr_mem.value;
    size_t   len = (size_t)h[0] | ((size_t)h[1] << 8) | ((size_t)h[2] << 16);

    /* phase 2: read payload */
    MemoryResult pay_mem = fun_memory_allocate(len);
    if (fun_error_is_error(pay_mem.error)) goto fail_hdr;
    NetworkBuffer payload = { pay_mem.value, 0 };

    r = fun_network_tcp_receive_exact(conn, &payload, len);
    if (fun_error_is_error(fun_async_await(&r, 5000).error))
        goto fail_pay;

    fun_memory_free(&hdr_mem.value);
    *out_payload = pay_mem.value;
    *out_len     = len;
    return 0;

fail_pay: fun_memory_free(&pay_mem.value);
fail_hdr: fun_memory_free(&hdr_mem.value);
    return 1;
}
```

---

## Task: Overflow Buffer — Sequential Receives

The connection owns an internal overflow buffer. When more bytes arrive than a single `receive_exact` call requests, the surplus is held and returned on the next call — no re-reading from the socket.

```c
/* server sends "ABCDEFGH" (8 bytes) */

NetworkBuffer first  = { mem_a, 0 };
NetworkBuffer second = { mem_b, 0 };

AsyncResult r = fun_network_tcp_receive_exact(conn, &first, 4);
fun_async_await(&r, 5000);
/* first.length == 4, "ABCD" in mem_a; "EFGH" in overflow buffer */

r = fun_network_tcp_receive_exact(conn, &second, 4);
fun_async_await(&r, 5000);
/* second.length == 4, "EFGH" from overflow buffer — no socket read */
```

---

## Task: UDP Fire-and-Forget Send

```c
int example_udp_send(void)
{
    NetworkAddressResult addr_r = fun_network_address_parse("127.0.0.1:5140");
    if (fun_error_is_error(addr_r.error)) return 1;

    const char   msg[]   = "hello";
    MemoryResult msg_mem = fun_memory_allocate(sizeof(msg) - 1);
    if (fun_error_is_error(msg_mem.error)) return 1;
    fun_memory_copy((Memory)msg, msg_mem.value, sizeof(msg) - 1);

    NetworkBuffer dgram = { msg_mem.value, sizeof(msg) - 1 };
    AsyncResult r = fun_network_udp_send(addr_r.value, dgram);
    fun_async_await(&r, 1000); /* completes immediately; no recv */

    fun_memory_free(&msg_mem.value);
    return 0;
}
```

---

## Task: Address Parse and Format

```c
/* Parse */
NetworkAddressResult ar = fun_network_address_parse("93.184.216.34:80");
if (fun_error_is_error(ar.error)) { /* bad address */ }
NetworkAddress addr = ar.value;

/* Format */
char buf[64];
voidResult fr = fun_network_address_to_string(addr, buf, sizeof(buf));
if (fun_error_is_error(fr.error)) { /* buffer too small */ }
/* buf now contains "93.184.216.34:80" */

/* IPv6 */
NetworkAddressResult ar6 = fun_network_address_parse("[::1]:8080");
```

---

## Error Handling Pattern

```c
AsyncResult r = fun_network_tcp_connect(addr, &conn);
voidResult  w = fun_async_await(&r, 5000);

if (r.status == ASYNC_ERROR) {
    /* r.error.code: ERROR_CODE_NETWORK_CONNECT_FAILED, etc. */
    /* r.error.message: human-readable description              */
}
```

---

## Key Types

```c
TcpNetworkConnection conn = NULL;   /* opaque handle; pointer-sized */
OutputTcpNetworkConnection out_conn = &conn;   /* for fun_network_tcp_connect */

NetworkBuffer buf = { mem, 0 };   /* data=Memory, length=size_t */
OutputNetworkBuffer out_buf = &buf;   /* for receive functions */

NetworkAddress addr;   /* family, bytes[16], port — copy by value */
```

---

## See Also

- `fundamental-async.md` — `fun_async_await`, `fun_async_await_all`
- `fundamental-memory.md` — `fun_memory_allocate`, `fun_memory_free`, `fun_memory_copy`
- `include/network/network.h` — complete API reference
