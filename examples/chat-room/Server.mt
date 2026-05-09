import * from "../../mType/tests/testFiles/lib/collections/ArrayList.mt";
import * from "../../mType/tests/testFiles/lib/exceptions/Exception.mt";
import * from "../../mType/tests/testFiles/lib/net/TcpServer.mt";
import * from "../../mType/tests/testFiles/lib/net/TcpSocket.mt";
import * from "../../mType/tests/testFiles/lib/primitives/Int.mt";
import * from "../../mType/tests/testFiles/lib/primitives/String.mt";

class ChatRoom {
    private ArrayList<TcpSocket> clients;
    private ArrayList<String> names;

    public constructor() {
        this.clients = new ArrayList<TcpSocket>();
        this.names = new ArrayList<String>();
    }

    public function addClient(TcpSocket socket, string name): void {
        this.clients.add(socket);
        this.names.add(new String(name));
        print("[server] " + name + " joined");
    }

    public function removeClient(TcpSocket socket): string {
        int index = this.indexOf(socket);
        if (index == -1) {
            return "unknown";
        }

        string name = this.names.get(index).getValue();
        this.clients.removeAt(index);
        this.names.removeAt(index);
        print("[server] " + name + " left");
        return name;
    }

    public function async broadcast(string message): Promise<void> {
        print("[broadcast] " + message);

        int i = 0;
        while (i < this.clients.size()) {
            TcpSocket client = this.clients.get(i);
            try {
                if (client.isConnected()) {
                    Int sent = await client.sendAsync(message);
                    i = i + 1;
                } else {
                    this.clients.removeAt(i);
                    this.names.removeAt(i);
                }
            } catch (Exception e) {
                this.clients.removeAt(i);
                this.names.removeAt(i);
            }
        }
    }

    public function async handleClient(TcpSocket conn): Promise<void> {
        string nickname = "guest";
        conn.setTimeout(10000);

        try {
            String first = await conn.receiveAsync(1024);
            nickname = first.getValue();
            if (nickname == "") {
                nickname = "guest";
            }

            this.addClient(conn, nickname);
            await this.broadcast("* " + nickname + " joined the room");

            bool running = true;
            while (running) {
                String incoming = await conn.receiveAsync(1024);
                string body = incoming.getValue();

                if (body == "" || body == "/quit") {
                    running = false;
                } else {
                    await this.broadcast(nickname + ": " + body);
                }
            }
        } catch (Exception e) {
            print("[server] client error: " + e.getMessage());
        }

        string leftName = this.removeClient(conn);
        await this.broadcast("* " + leftName + " left the room");
        conn.close();
    }

    private function indexOf(TcpSocket socket): int {
        for (int i = 0; i < this.clients.size(); i = i + 1) {
            TcpSocket current = this.clients.get(i);
            if (current.handle == socket.handle) {
                return i;
            }
        }
        return -1;
    }
}

function async main(): Promise<void> {
    int port = 18888;
    ChatRoom room = new ChatRoom();
    TcpServer server = new TcpServer();

    server.onConnection(async conn -> {
        await room.handleClient(conn);
    });

    server.onError(async err -> {
        print("[server] " + err.getMessage());
    });

    server.listen(port);
    print("[server] chat room listening on 127.0.0.1:" + port);
    print("[server] scripted demo server will stop after 30 seconds");

    await delay(30000);
    server.stop();
    print("[server] stopped");
}

main();
