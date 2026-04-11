# HTTP Server in C

## Overview

A concurrent HTTP server written in C using a process-per-client model (`fork()`). It parses HTTP/1.1-style requests but operates with HTTP/1.0-style connection handling (no persistent connections). Each incoming connection is handled by a forked child process. The server parses HTTP request lines and headers, enforces HTTP Basic Authentication, and responds with file content read from a local `www/` directory. All responses are sent unchunked with a fixed `Content-Length` header.

This is a systems and network programming project. It prioritizes implementation clarity over protocol completeness and is not intended for production use. 
The server follows a simple process-per-connection model, prioritizing clarity over scalability.
The implementation focuses on core HTTP mechanics and OS-level behavior rather than full protocol compliance.

---

## Features

- **Concurrent clients** via `fork()` — one child process per connection, with non-blocking zombie reaping using `waitpid(WNOHANG)`
- **`poll()`-based connection loop** on port 8080 with a 100 ms timeout
- **HTTP request parsing** — request line (method, path, version) and up to 50 headers
- **Query string stripping** — anything after `?` is discarded before path resolution
- **Directory traversal rejection** — any path containing `..` returns `400 Bad Request`
- **Automatic `index.html` resolution** for paths ending with `/`
- **HTTP Basic Authentication** — decodes Base64 credentials via OpenSSL's BIO API and verifies against an in-memory user table
- **GET-only** — any other method returns `405 Method Not Allowed`
- **Static file serving** from `www/` with basic MIME type detection (`.html`, `.css`, `.js`, `application/octet-stream` fallback)
- **Unchunked responses** — full `Content-Length` sent in every `200 OK`
- **Header size limit** — requests exceeding 8 KB are rejected
- **Test script** — automated test suite runnable via `make test`
- **Stateless request handling** — each connection is handled independently in a child process

---

## Project Structure

| File | Role |
|---|---|
| `server.c` | Socket setup, connection loop, request reading, parsing, authentication, file serving |
| `userpass_map.h` | Public interface for the user/password store |
| `userpass_map.c` | Dynamic array of `User` structs with `init`, `add`, `delete`, `authenticate`, and `free` operations |
| `www/` | Static files served by the server (`index.html`, `styles.css`, `script.js`, `test.txt`) |
| `test_http_server.sh` | Bash test suite using `curl`; covers auth, 404, 405, traversal, and concurrency |
| `makefile` | Build and test targets |

---

## Server Workflow

1. **Initialization** — `init_user_map()` allocates the user table; a fixed set of username/password pairs is added via `add_user()`.
2. **Socket creation** — TCP socket bound to `0.0.0.0:8080` with `SO_REUSEADDR`; `listen(SOMAXCONN)`.
3. **Connection polling** — `poll()` monitors the listening socket with a 100 ms timeout; pending zombie children are reaped each iteration.
4. **Accept** — `accept()` produces a client file descriptor.
5. **Fork** — `fork()` creates a child process. The parent closes the client fd; the child closes the listening fd.
6. **Request reading** — child reads from the client socket in 256-byte chunks into a dynamically grown buffer until `\r\n\r\n` is found or the 8 KB limit is hit.
7. **Parsing** — request line and headers are tokenized; query string is stripped.
8. **Authentication** — `Authorization` header is located, `Basic` prefix verified, credentials Base64-decoded, and the username/password pair checked against the user table.
9. **File serving** — path is validated, resolved against `www/`, stat-checked, and sent with appropriate status and MIME type.
10. **Cleanup** — all heap allocations freed, client socket closed, child exits.

---

## Request Handling Details

### Request Reading

`read_http_request()` reads into a heap-allocated buffer starting at 256 bytes, doubling capacity on each reallocation. Reading stops when `\r\n\r\n` is found in the buffer (end of headers), the peer closes the connection, or the buffer would exceed `MAX_HEADER_SIZE` (8 KB). On size overflow, the function returns `-1` and frees the buffer; on `recv` failure, it calls `exit()` from the child.

### Request Parsing

`parse_http_request()` duplicates the raw buffer and tokenizes it with `strtok_r`:

- **Request line** — split on spaces into method, path, version; path must start with `/`.
- **Query string** — truncated at the first `?`.
- **Headers** — each `name: value` line is split at the first `:`, whitespace-trimmed, and stored in parallel `header_name`/`header_value` arrays (max 50 entries).
- **Body** — the substring after `\r\n\r\n` is captured into `req->body` but is otherwise ignored.

Returns `0` on success, `-1` on malformed input.

### Authentication

`authenticate()` performs a case-insensitive linear scan of the parsed headers for `Authorization`. 

| Condition | Response |
|---|---|
| Header absent | `401 Unauthorized` + `WWW-Authenticate: Basic realm="MyServer"` |
| Value does not start with `Basic ` | `400 Bad Request` |
| Base64 decode fails or returns ≤ 0 bytes | `400 Bad Request` |
| Decoded string missing `:` separator | `400 Bad Request` |
| Username or password wrong | `403 Forbidden` |
| Credentials match | returns `true`, proceeds |

Base64 decoding uses an OpenSSL BIO chain (`BIO_f_base64` + `BIO_new_mem_buf`) with `BIO_FLAGS_BASE64_NO_NL`.

### File Serving

`send_file_response()` handles responses:

- Non-GET method → `405 Method Not Allowed` (with `Allow: GET`).
- Path contains `/..` or `../` → `400 Bad Request`.
- Path ending with `/` → `index.html` appended before `stat()`.
- `stat()` succeeds and path is a regular file → open, send header with `Content-Length` and `Content-Type`, stream file in 4 KB reads.
- `stat()` fails or path is not a regular file → `404 Not Found`.
- `open()` fails after a successful `stat()` → `500 Internal Server Error`.

MIME type is inferred from the file extension: `.html`/`.htm` → `text/html`, `.css` → `text/css`, `.js` → `application/javascript`, anything else → `application/octet-stream`.

---

## Build and Test

**Dependencies:** GCC, OpenSSL development libraries (`libssl-dev` / `openssl-devel`).

```bash
# Build
make

# Run automated tests (starts server, runs curl tests, stops server)
make test

# Manual browser test
./http_server   # then open http://127.0.0.1:8080 in a browser
```

The `www/` directory contains `index.html`, `styles.css`, `script.js`, and `test.txt` for manual verification.

---

## Current Status

### Implemented

- TCP server with `poll()`-based accept loop
- `fork()`-per-client concurrency with zombie reaping
- HTTP header parsing (request line + headers)
- HTTP Basic Authentication via OpenSSL Base64
- In-memory user/password table (dynamic array)
- GET-only static file serving with MIME detection
- Directory index resolution, query string stripping, traversal rejection
- Correct `Content-Length` in all responses (no chunked encoding)
- Automated test script covering the main response codes

### Limitations

- **GET only** — `POST`, `PUT`, `DELETE`, and other methods are rejected.
- **No persistent connections** — socket is closed after each request (HTTP/1.0 semantics despite `HTTP/1.1` in the status line).
- **No chunked transfer encoding** — requires the full file to fit within a single stat/open/read sequence.
- **No HTTPS / TLS** — all traffic is plaintext.
- **Passwords stored in plaintext** — the user table holds cleartext passwords; no hashing.
- **Hardcoded credentials** — users are populated in `main()` at compile time; no config file or runtime registration.
- **Process-per-client** — `fork()` has higher per-connection overhead than a thread pool or event loop.
- **No response abstraction** — HTTP response construction is inlined in `send_file_response()`; there is no dedicated response struct or builder.
- Header boundary detection relies on scanning the accumulated buffer for `\r\n\r\n`. While robust for typical inputs, it does not implement a fully incremental parser.

### Possible Improvements

- Add password hashing (e.g., bcrypt or SHA-256 with salt).
- Introduce a dedicated `http_response` struct to separate response construction from file I/O.
- Replace `fork()` with a thread pool or `epoll`-based event loop for lower connection overhead.
- Add `Keep-Alive` / persistent connection support.
- Expand MIME type table (images, fonts, JSON, etc.).
- Load users from a configuration file at startup.

---

## Notes

This project was written as a hands-on exercise in POSIX network programming: socket APIs, process management, HTTP protocol structure, and OpenSSL integration. The goal was a working, readable implementation — not a feature-complete or production-grade server.
