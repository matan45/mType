import * from "../functional/Consumer.mt";
import * from "TcpSocket.mt";
import * from "exceptions/NetworkException.mt";

// TCP listening server. listen() spawns a worker accept thread in C++; for
// each accepted client the worker posts back to the VM event loop and the
// stored onConnection consumer is invoked with a fresh TcpSocket. Errors on
// the accept loop are delivered via onError if registered. Always call stop().
class TcpServer {
    public int handle;

    public constructor() {
        this.handle = __net_tcpserver_create();
    }

    public function listen(int port): void {
        __net_tcpserver_listen(this.handle, port);
    }

    public function onConnection(Consumer<TcpSocket> callback): void {
        __net_tcpserver_onConnection(this.handle, callback);
    }

    public function onError(Consumer<NetworkException> callback): void {
        __net_tcpserver_onError(this.handle, callback);
    }

    public function stop(): void {
        __net_tcpserver_stop(this.handle);
    }
}
