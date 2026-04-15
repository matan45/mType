import * from "../primitives/String.mt";
import * from "../primitives/Int.mt";

// TCP client socket. Holds an integer handle into the C++ SocketRegistry; all
// methods delegate to __net_socket_* natives. The TcpServer's onConnection
// callback receives instances of this class wrapping accepted client FDs.
class TcpSocket {
    public int handle;

    public constructor() {
        this.handle = __net_socket_create();
    }

    public function async connectAsync(string host, int port): Promise<void> {
        await __net_socket_connectAsync(this.handle, host, port);
    }

    public function connect(string host, int port): void {
        await this.connectAsync(host, port);
    }

    public function async sendAsync(string data): Promise<Int> {
        int sent = await __net_socket_sendAsync(this.handle, data);
        return new Int(sent);
    }

    public function send(string data): int {
        Int sent = await this.sendAsync(data);
        return sent.getValue();
    }

    public function async receiveAsync(int maxBytes): Promise<String> {
        string s = await __net_socket_receiveAsync(this.handle, maxBytes);
        return new String(s);
    }

    public function receive(int maxBytes): string {
        String s = await this.receiveAsync(maxBytes);
        return s.getValue();
    }

    public function close(): void {
        __net_socket_close(this.handle);
    }

    public function isConnected(): bool {
        return __net_socket_isConnected(this.handle);
    }

    public function setTimeout(int ms): void {
        __net_socket_setTimeout(this.handle, ms);
    }
}
