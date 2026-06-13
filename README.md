# libWeb

A cross-platform, high-performance micro HTTP web framework written in C.

---

## Features

- **Async I/O** — epoll-based event loop (Linux) with edge-triggered, one-shot events for maximum throughput
- **Multi-threaded** — spawns one worker thread per CPU core, all sharing the same port via `SO_REUSEPORT`
- **HTTP/1.1** routing — `GET`, `POST`, `HEAD`, and `OPTIONS` methods with a hash-table-backed router
- **Static file serving** — serves files from a configurable public directory using `sendfile(2)` for zero-copy transfers
- **In-memory file cache** — frequently-served files are cached in memory to avoid repeated disk reads
- **JSON parser** — built-in recursive descent parser supporting null, bool, int, float, string, array, and object types
- **StringView** — zero-copy string slicing throughout the entire stack to avoid unnecessary allocations
- **Query parameter parsing** — URL query strings are parsed into a hash table automatically
- **Keep-alive connections** — connections are reused across requests
- **Cross-platform build** — compiles on both Linux and Windows (Winsock2)

---

## Benchmark

Tested with [`oha`](https://github.com/hatoo/oha) — 1,000,000 requests at 500 concurrent connections against the example server (`http://localhost:6969/`).

```
oha -n 1M -c 500 http://localhost:6969/
```

| Metric              | Value            |
|---------------------|------------------|
| **Requests/sec**    | **716,403**      |
| Success rate        | 100.00%          |
| Total time          | 1395.86 ms       |
| Average latency     | 0.69 ms          |
| p50 latency         | 0.50 ms          |
| p99 latency         | 3.09 ms          |
| p99.9 latency       | 5.43 ms          |
| Fastest             | 0.017 ms         |
| Slowest             | 31.71 ms         |
| Throughput          | 1.08 GiB/sec     |

**Test machine:** Arch Linux (kernel 7.0.11), Intel Core i5-14450HX (16 cores @ 4.80 GHz), 30.97 GiB RAM

---

## Building

### Library

```bash
make
```

Produces `out/libWeb.a`.

### Example server

```bash
make example
```

Produces `example/main`.

### Clean

```bash
make clean
```

---

## Running the Example

```bash
cd example
./main
```

The server starts on `0.0.0.0:6969` and serves static files from `./public`. It also registers a `POST /` route that echoes back the request body as JSON.

---

## Usage

### 1. Include the header

```c
#include "main.h"
```

### 2. Define route handlers

Each handler receives a `Request*` and a `Response*`:

```c
void home(Request *req, Response *res) {
    set_response_body("{\"hello\":\"world\"}", res);
    add_response_header("content-type", "application/json", res->headers);
}

void echo(Request *req, Response *res) {
    if (req->body.count > 0)
        set_response_body_sv(req->body, res);
}
```

### 3. Register routes and start

```c
int main(void) {
    setPublicDir("./public");         // serve static files from this directory
    addRoute(GET,  "/",    home);
    addRoute(POST, "/echo", echo);
    startServer("0.0.0.0", 8080);
    cleanupRoutes();
}
```

### 4. Compile against the library

```bash
gcc -I./include main.c -L./out -lWeb -lpthread -o server
```

---

## API Reference

### Server

| Function | Description |
|----------|-------------|
| `startServer(char *addr, int port)` | Bind, listen, and start the multi-threaded event loop |
| `setPublicDir(char *path)` | Set the directory from which static files are served |
| `cleanupRoutes()` | Free the route table (call before exit) |

### Routing

| Function | Description |
|----------|-------------|
| `addRoute(Method m, char *path, callback)` | Register a route handler |

Supported methods: `GET`, `POST`, `HEAD`, `OPTIONS`

### Request

| Field / Function | Type | Description |
|-----------------|------|-------------|
| `req->method` | `Method` | Parsed HTTP method |
| `req->path` | `StringView` | Request path (e.g. `/api/users`) |
| `req->body` | `StringView` | Raw request body |
| `req->headers` | `HashTable*` | Request headers |
| `req->query_params` | `HashTable*` | Parsed URL query parameters |
| `get_request_header(name, headers)` | `StringView` | Look up a request header by name |

### Response

| Function | Description |
|----------|-------------|
| `set_response_body(char *body, res)` | Set the response body from a C string (copied) |
| `set_response_body_sv(StringView sv, res)` | Set the response body from a StringView (zero-copy) |
| `setBodyFromFile(char *path, res)` | Read a file into the response body (cached after first read) |
| `add_response_header(name, value, headers)` | Add a response header |
| `remove_response_header(name, headers)` | Remove a response header |
| `get_response_header(name, headers)` | Retrieve a response header value |
| `setStatus(int code, res)` | Set the HTTP status code (200, 404, 413, …) |

### JSON

```c
JsonValue *val = json_parse("{\"key\": 42}");
JsonValue *key = json_get(val, "key");   // key->integer == 42
json_free(val);
```

| Function | Description |
|----------|-------------|
| `json_parse(const char *src)` | Parse a JSON string into a `JsonValue` tree |
| `json_get(obj, key)` | Look up a key in a JSON object |
| `json_index(arr, i)` | Index into a JSON array |
| `json_free(val)` | Recursively free a parsed JSON value |
| `json_print(val, indent)` | Pretty-print a JSON value to stdout |

---

## Architecture

```
┌──────────────────────────────────────────────┐
│  startServer()                               │
│  Spawns N worker threads (N = CPU cores)     │
│  Each thread: SO_REUSEPORT + epoll event loop│
└───────────────┬──────────────────────────────┘
                │
        ┌───────▼────────┐
        │  accept loop   │  (edge-triggered EPOLLIN)
        └───────┬────────┘
                │ new connection
        ┌───────▼──────────────────────────────┐
        │  Connection state machine            │
        │  PARSING_HEADERS → PARSING_BODY      │
        │  → REQUEST_BUILT → SENDING_RESPONSE  │
        │  → RESPONSE_SENT (keep-alive reset)  │
        └───────┬──────────────────────────────┘
                │
      ┌─────────▼──────────────────────┐
      │  handleRequest()               │
      │  ├─ Static file? → sendfile()  │
      │  └─ Route match? → callback()  │
      └────────────────────────────────┘
```

---

## Project Structure

```
libWeb/
├── include/                  # Public headers
│   ├── connection.h
│   ├── request.h
│   ├── response.h
│   ├── routing.h
│   ├── hash_table.h
│   ├── cache_store.h
│   ├── string_view.h
│   ├── json.h
│   ├── mimeTypes.h
│   ├── helper.h
│   ├── globals.h
│   ├── errors.h
│   └── compat.h
├── setupServer_async.c       # Event loop, thread spawning, request dispatch
├── connection.c              # Connection lifecycle, HTTP parsing, send/recv
├── routing.c                 # Route registration and lookup
├── response.c                # Response building and serialisation
├── request.c                 # Request allocation and teardown
├── json.c                    # JSON parser
├── hash_table.c              # Open-chaining hash table
├── string_view.c             # Zero-copy string utilities
├── cache_store.c             # In-memory file content cache
├── mimeTypes.c               # MIME type detection by file extension
├── helper.c                  # Misc utilities
├── makefile
├── example/
│   ├── main.c                # Example server
│   ├── main.h
│   ├── string_view.h
│   └── public/               # Static files served by the example
│       ├── index.html
│       ├── css/style.css
│       └── js/script.js
└── tests/
    ├── test_hash_table.py
    └── run_tests.py
```

---

## Platform Support

| Platform | Status |
|----------|--------|
| Linux    | Full support (epoll, sendfile, pthreads) |
| Windows  | Partial (Winsock2 socket layer; epoll/ioctl TODO) |

---

## License

See [LICENSE](LICENSE) for details.
