---
title: Networking
sidebar_position: 7
---

# Networking

`lib/net/` provides HTTP and TCP primitives plus a JSON-over-HTTP convenience API. Most operations are async — use `await` to consume the resulting `Promise<T>`.

## HTTP

```mtype
import * from "lib/net/Http.mt";

function async main(): Promise<void> {
    HttpResponse resp = await Http.get("https://api.example.com/users/42");
    if (resp.getStatus() == 200) {
        print(resp.getBody());
    }
}
```

`Http` exposes:

- `get(string url): Promise<HttpResponse>`
- `post(string url, string body): Promise<HttpResponse>`
- `put(string url, string body): Promise<HttpResponse>`
- `delete(string url): Promise<HttpResponse>`

`HttpRequest` and `HttpResponse` give finer control (headers, status, body).

## JSON-over-HTTP

`lib/net/JsonApi.mt` wraps `Http` with JSON encoding/decoding:

```mtype
import * from "lib/net/JsonApi.mt";

function async loadUser(): Promise<void> {
    Json data = await JsonApi.get("https://api.example.com/users/42");
    print(data.getString("name"));
}
```

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
- `HttpException`
- `TimeoutException`

See [Exceptions](exceptions.md) for how to handle them.

## See Also

- [Async / Await](../language/async-await.md)
- [JSON](json.md)
