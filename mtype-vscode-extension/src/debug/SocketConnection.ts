import * as net from 'net';
import { ProtocolConnection } from './ProtocolConnection';

const DEFAULT_HOST = 'localhost';
const DEFAULT_PORT = 5005;
const CONNECT_TIMEOUT_MS = 10000;

/**
 * TCP socket transport for the mType debug protocol (the `attach` flow).
 * Connects to an already-running mType host (e.g. an embedded engine) over a
 * port and talks the same line protocol as the spawn transport.
 *
 * Detach semantics: stop()/kill() only close our socket — the remote host keeps
 * running. Program output arrives as protocol OUTPUT messages (no side stream).
 */
export class SocketConnection extends ProtocolConnection {
    private socket: net.Socket | null = null;
    private buffer: string = '';

    /**
     * Connect to a running mType debug host.
     * Resolves on the socket 'connect' event — NOT on a STOPPED(entry) event,
     * because an attached host is already running and may emit none.
     */
    public connect(host: string = DEFAULT_HOST, port: number = DEFAULT_PORT): Promise<void> {
        return new Promise((resolve, reject) => {
            this.ended = false;
            this.buffer = '';
            let connected = false;

            const timeout = setTimeout(() => {
                if (!connected && !this.ended) {
                    reject(new Error(`Timeout connecting to ${host}:${port}`));
                    this.socket?.destroy();
                }
            }, CONNECT_TIMEOUT_MS);

            const socket = net.createConnection(port, host);
            this.socket = socket;
            socket.setEncoding('utf8');

            socket.once('connect', () => {
                connected = true;
                clearTimeout(timeout);
                resolve();
            });

            socket.on('data', (chunk: string) => this.onData(chunk));

            socket.on('error', (err: Error) => {
                clearTimeout(timeout);
                if (!connected) {
                    // Failed to establish the connection in the first place.
                    this.ended = true;
                    this.drainQueue();
                    reject(new Error(`Failed to connect to ${host}:${port}: ${err.message}`));
                } else {
                    // Mid-session error — treat as a disconnect.
                    this.onClosed();
                }
            });

            socket.on('close', () => this.onClosed());
            socket.on('end', () => this.onClosed());
        });
    }

    private onData(chunk: string): void {
        this.buffer += chunk;
        let nl: number;
        while ((nl = this.buffer.indexOf('\n')) !== -1) {
            let line = this.buffer.slice(0, nl);
            if (line.endsWith('\r')) {
                line = line.slice(0, -1);
            }
            this.buffer = this.buffer.slice(nl + 1);
            this.handleLine(line);
        }
    }

    /** Converge 'close'/'end'/mid-session 'error' onto a single terminate. */
    private onClosed(): void {
        if (this.ended) { return; }
        this.ended = true;
        this.drainQueue();
        this.emit('terminated');
        this.emit('exit', 0, null);
    }

    protected writeLine(line: string): void {
        if (this.socket && !this.socket.destroyed) {
            this.socket.write(line + '\n');
        }
    }

    /**
     * Detach: half-close our socket. The remote host keeps running.
     */
    public stop(): void {
        this.drainQueue();
        this.socket?.end();
    }

    /**
     * Hard detach: drop the socket immediately. The remote host keeps running.
     */
    public kill(): void {
        this.drainQueue();
        this.socket?.destroy();
        this.socket = null;
    }
}
