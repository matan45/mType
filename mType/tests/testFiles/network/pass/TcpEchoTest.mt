// Loopback integration test for TcpServer + TcpSocket. Brings up a server on
// a fixed high port, has a client connect and exchange one message, then
// shuts the server down. Exercises async/await, lambdas, native callback
// marshalling, and the resource-cleanup path. Requires port 18765 free.
import * from "../../lib/mtest/Mtest.mt";
import * from "../../lib/net/TcpServer.mt";
import * from "../../lib/net/TcpSocket.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

public class TcpEchoTest extends TestSuite {
    public constructor() : super() { }

    @Test
    public function async testEchoLoopback(): Promise<void> {
        int port = 18765;

        TcpServer srv = new TcpServer();
        srv.onConnection(conn -> {
            try {
                string data = conn.receive(1024);
                conn.send("ECHO:" + data);
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
            assertEqual(reply.getValue(), "ECHO:hello");
        } catch (Exception e) {
            client.close();
            srv.stop();
            throw e;
        }
        client.close();
        srv.stop();
    }

    @Test
    public function async testServerStopUnblocks(): Promise<void> {
        TcpServer srv = new TcpServer();
        srv.listen(18766);
        srv.stop();
        // Successfully reaching here means stop() joined the accept thread.
        assertTrue(true);
    }
}
