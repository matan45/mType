# mType TCP Chat Room

This is a scripted client/server chat-room demo for the mType TCP networking library. It uses the existing `TcpServer.onConnection` callback API, `TcpSocket.connectAsync`, `sendAsync`, `receiveAsync`, socket timeouts, and cleanup paths.

The demo is intentionally scripted because mType does not currently expose a standard `readLine` or stdin API for an interactive terminal client.

## Run

From the repository root, start the server in one terminal:

```powershell
bin\mType\Release\x64\mType.exe examples\chat-room\Server.mt
```

In another terminal, run the scripted clients:

```powershell
bin\mType\Release\x64\mType.exe examples\chat-room\Client.mt
```

`Client.mt` starts two virtual clients, `alice` and `bob`, against `127.0.0.1:18888`. The server stops after 30 seconds so the sample can be run without interactive input.

## Protocol

- The first TCP send from a client is treated as the nickname.
- Later sends are chat messages.
- `/quit` disconnects that client.
- The server broadcasts join, message, and leave notifications to currently connected clients.
