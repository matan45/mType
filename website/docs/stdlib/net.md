---
title: Networking
sidebar_position: 7
---

# Networking

`lib/net/` provides HTTP and TCP primitives plus a JSON-over-HTTP convenience class. Most operations are async — use `await` to consume the resulting `Promise<T>`.

## HTTP — `Http` (static facade)

`Http` is a static facade with both sync and async variants. Sync methods internally `await` the async natives.

### Async

```mtype
import * from "lib/net/Http.mt";

function async main(): Promise<void> {
    HttpResponse resp = await Http::getAsync("https://api.example.com/users/42");
    if (resp.isOk()) {
        print(resp.body);
    }
}
```

| Method | Returns |
|---|---|
| `Http::getAsync(string url)` | `Promise<HttpResponse>` |
| `Http::postAsync(string url, string body)` | `Promise<HttpResponse>` |
| `Http::putAsync(string url, string body)` | `Promise<HttpResponse>` |
| `Http::deleteAsync(string url)` | `Promise<HttpResponse>` |

### Sync

The sync variants block (internally awaiting) and return `HttpResponse` directly:

```mtype
HttpResponse resp = Http::get("https://api.example.com/users/42");
print(resp.status);
print(resp.body);
```

| Method | Returns |
|---|---|
| `Http::get(string url)` | `HttpResponse` |
| `Http::post(string url, string body)` | `HttpResponse` |
| `Http::put(string url, string body)` | `HttpResponse` |
| `Http::delete(string url)` | `HttpResponse` |

### Builder Entry Point

For finer control over headers, method, and timeout, build an `HttpRequest`:

```mtype
HttpRequest req = Http::request("GET", "https://api.example.com/data");
// configure req (headers, timeout, etc.) then send
```

### `HttpResponse`

`HttpResponse` exposes:

- `int status` — HTTP status code
- `string body` — response body
- `isOk(): bool` — true for 2xx
- (plus headers via the standard accessor methods on the type)

## JSON-over-HTTP — `JsonApi` (instance class)

`JsonApi` is a stateful HTTP client wrapper that defaults `Accept: application/json` and offers fluent header / timeout configuration. **Construct one per base URL** — its methods are instance methods.

```mtype
import * from "lib/net/JsonApi.mt";

function async main(): Promise<void> {
    JsonApi api = new JsonApi("https://api.example.com");
    api.setHeader("Authorization", "Bearer abc123");
    api.setTimeout(10000);

    String body = await api.get("/users/42");
    print(body.getValue());
}
```

### Methods

| Method | Returns |
|---|---|
| `setHeader(string name, string value)` | `JsonApi` (fluent) |
| `setTimeout(int ms)` | `JsonApi` (fluent) |
| `async get(string path)` | `Promise<String>` (raw body) |
| `async post(string path, string jsonBody)` | `Promise<String>` |
| `async put(string path, string jsonBody)` | `Promise<String>` |
| `async delete(string path)` | `Promise<String>` |
| `async getAs<T>(string path, string className)` | `Promise<T>` (deserialized) |
| `async postAs<T>(string path, T obj, string className)` | `Promise<T>` |

Non-2xx responses raise `HttpException` carrying the status code.

### Typed GET / POST

`getAs<T>` and `postAs<T>` combine HTTP with JSON deserialization in one call:

```mtype
JsonApi api = new JsonApi("https://api.example.com");
User u = await api.<User>getAs("/users/42", "User");
print(u.name);
```

`postAs` serializes the request body, sends it, and deserializes the response — useful for typed CRUD APIs.

## TCP

```mtype
import * from "lib/net/TcpServer.mt";
import * from "lib/net/TcpSocket.mt";

function async main(): Promise<void> {
    TcpServer server = new TcpServer(8080);
    while (true) {
        TcpSocket client = await server.accept();
        await client.write("hello\n");
        await client.close();
    }
}
```

## Exceptions

`lib/net/exceptions/` defines:

- `NetworkException`
- `ConnectionException`
- `DnsException`
- `HttpException` (carries `status`)
- `TimeoutException`

See [Exceptions](exceptions.md) for catching patterns.

## See Also

- [Async / Await](../language/async-await.md)
- [JSON](json.md) — `Json` is what `JsonApi.getAs<T>` uses under the hood.
