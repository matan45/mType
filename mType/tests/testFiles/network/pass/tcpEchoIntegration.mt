// TCP loopback integration test, run as a standalone async script. mtest's v1
// runner doesn't await async test methods, so the full echo flow lives here
// instead. Run with:
//   mType C:\matan\mType\mType\tests\testFiles\network\pass\tcpEchoIntegration.mt
import * from "../../lib/net/TcpServer.mt";
import * from "../../lib/net/TcpSocket.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function async runEcho(): Promise<void> {
    int port = 18765;

    TcpServer srv = new TcpServer();
    srv.onConnection(async conn -> {
        try {
            String data = await conn.receiveAsync(1024);
            Int sent = await conn.sendAsync("ECHO:" + data.getValue());
        } catch (Exception e) {
            print("server error: " + e.getMessage());
        }
        conn.close();
    });
    srv.listen(port);

    TcpSocket client = new TcpSocket();
    client.setTimeout(2000);
    try {
        await client.connectAsync("127.0.0.1", port);
        Int sent = await client.sendAsync("hello");
        String reply = await client.receiveAsync(1024);

        if (reply.getValue() == "ECHO:hello") {
            print("OK: echo loopback succeeded");
        } else {
            print("FAIL: expected 'ECHO:hello', got '" + reply.getValue() + "'");
        }
    } catch (Exception e) {
        print("FAIL: client raised " + e.getMessage());
    }

    client.close();
    srv.stop();
}

await runEcho();
