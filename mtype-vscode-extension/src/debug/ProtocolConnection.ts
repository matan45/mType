import { EventEmitter } from 'events';

/**
 * Parsed protocol message from the C++ debug backend.
 * Format: COMMAND key1=value1 key2="quoted value"
 */
export interface ProtocolMessage {
    command: string;
    parameters: Map<string, string>;
}

/**
 * Transport-agnostic base for the mType line-based debug protocol.
 *
 * Owns the sequential command queue, response/timeout bookkeeping, message
 * parsing/escaping, and async event dispatch ('stopped'/'terminated'/'output').
 * Concrete transports (process stdio, TCP socket) supply the I/O by implementing
 * writeLine()/stop()/kill() and feeding incoming lines into handleLine().
 */
export abstract class ProtocolConnection extends EventEmitter {
    // Current in-flight request
    protected pendingResolve: ((msg: ProtocolMessage) => void) | null = null;
    protected pendingReject: ((err: Error) => void) | null = null;
    protected pendingTimeout: NodeJS.Timeout | null = null;

    // Sequential command queue: ensures only one command is in-flight at a time
    private commandQueue: Array<{
        line: string;
        timeoutMs: number;
        resolve: (msg: ProtocolMessage) => void;
        reject: (err: Error) => void;
    }> = [];
    private processingCommand: boolean = false;
    protected ended: boolean = false;

    // --- Transport hooks implemented by subclasses ---

    /** Write one protocol line to the transport (newline appended by the caller pattern). */
    protected abstract writeLine(line: string): void;

    /** Graceful shutdown of the transport. */
    public abstract stop(): void;

    /** Immediate teardown of the transport. */
    public abstract kill(): void;

    /**
     * Send a command and wait for a response (OK, ERROR, or data response).
     * Commands are queued and sent sequentially to prevent response mismatches
     * when VS Code sends multiple DAP requests concurrently.
     */
    public sendCommand(command: string, params: Record<string, string | number> = {}, timeoutMs: number = 5000): Promise<ProtocolMessage> {
        // Build the protocol line
        let line = command;
        for (const [key, value] of Object.entries(params)) {
            line += ` ${key}=${this.escapeValue(String(value))}`;
        }

        return new Promise((resolve, reject) => {
            this.commandQueue.push({ line, timeoutMs, resolve, reject });
            this.processNextCommand();
        });
    }

    private processNextCommand(): void {
        if (this.processingCommand || this.commandQueue.length === 0) {
            return;
        }

        this.processingCommand = true;
        const { line, timeoutMs, resolve, reject } = this.commandQueue.shift()!;

        this.pendingResolve = (msg: ProtocolMessage) => {
            this.processingCommand = false;
            resolve(msg);
            this.processNextCommand();
        };
        this.pendingReject = (err: Error) => {
            this.processingCommand = false;
            reject(err);
            this.processNextCommand();
        };

        this.pendingTimeout = setTimeout(() => {
            const pendingReject = this.pendingReject;
            this.pendingResolve = null;
            this.pendingReject = null;
            this.pendingTimeout = null;
            this.processingCommand = false;
            if (pendingReject) {
                pendingReject(new Error(`Timeout waiting for response to: ${line}`));
            }
            this.processNextCommand();
        }, timeoutMs);

        this.writeLine(line);
    }

    /**
     * Send a command that doesn't need a response (fire-and-forget).
     * Used for CONTINUE, STEPINTO, STEPOVER, STEPOUT which return OK
     * immediately but the actual result comes as a STOPPED event later.
     */
    public async sendCommandExpectOK(command: string, params: Record<string, string | number> = {}): Promise<void> {
        const response = await this.sendCommand(command, params);
        if (response.command === 'ERROR') {
            throw new Error(response.parameters.get('message') || 'Unknown error');
        }
    }

    /**
     * Reject every in-flight and queued command. Called by transports when the
     * connection closes so no pending DAP request hangs forever.
     */
    protected drainQueue(): void {
        if (this.pendingTimeout) {
            clearTimeout(this.pendingTimeout);
            this.pendingTimeout = null;
        }
        if (this.pendingReject) {
            this.pendingReject(new Error('Connection closed'));
            this.pendingResolve = null;
            this.pendingReject = null;
        }
        const closed = new Error('Connection closed');
        for (const entry of this.commandQueue) {
            entry.reject(closed);
        }
        this.commandQueue = [];
        this.processingCommand = false;
    }

    /**
     * Feed one complete protocol line from the transport into the protocol layer.
     * Blank/garbage lines are ignored.
     */
    protected handleLine(line: string): void {
        if (!line.trim()) { return; }
        const msg = this.parseMessage(line);
        if (!msg.command) { return; }
        this.handleMessage(msg);
    }

    protected handleMessage(msg: ProtocolMessage): void {
        // Async events from the backend
        if (msg.command === 'STOPPED') {
            this.emit('stopped', msg);
            return;
        }
        if (msg.command === 'TERMINATED') {
            this.emit('terminated');
            return;
        }
        if (msg.command === 'OUTPUT') {
            const text = msg.parameters.get('text') || '';
            const category = msg.parameters.get('category') || 'stdout';
            this.emit('output', text, category);
            return;
        }

        // Synchronous responses to pending commands
        if (this.pendingResolve) {
            if (this.pendingTimeout) {
                clearTimeout(this.pendingTimeout);
                this.pendingTimeout = null;
            }
            const resolve = this.pendingResolve;
            this.pendingResolve = null;
            this.pendingReject = null;
            resolve(msg); // This triggers processNextCommand() via the wrapper
        }
    }

    /**
     * Parse a line-based protocol message.
     * Format: COMMAND key1=value1 key2="quoted value"
     */
    protected parseMessage(line: string): ProtocolMessage {
        const params = new Map<string, string>();
        const trimmed = line.trim();
        if (!trimmed) {
            return { command: '', parameters: params };
        }

        const spaceIdx = trimmed.indexOf(' ');
        if (spaceIdx === -1) {
            return { command: trimmed, parameters: params };
        }

        const command = trimmed.substring(0, spaceIdx);
        let pos = spaceIdx + 1;

        while (pos < trimmed.length) {
            // Skip whitespace
            while (pos < trimmed.length && trimmed[pos] === ' ') { pos++; }
            if (pos >= trimmed.length) { break; }

            // Find key
            const eqIdx = trimmed.indexOf('=', pos);
            if (eqIdx === -1) { break; }

            const key = trimmed.substring(pos, eqIdx);
            pos = eqIdx + 1;

            let value = '';
            if (pos < trimmed.length && trimmed[pos] === '"') {
                // Quoted value
                pos++; // skip opening quote
                let escaped = false;
                while (pos < trimmed.length) {
                    const c = trimmed[pos];
                    if (escaped) {
                        if (c === 'n') { value += '\n'; }
                        else if (c === 'r') { value += '\r'; }
                        else if (c === 't') { value += '\t'; }
                        else { value += c; }
                        escaped = false;
                    } else if (c === '\\') {
                        escaped = true;
                    } else if (c === '"') {
                        pos++; // skip closing quote
                        break;
                    } else {
                        value += c;
                    }
                    pos++;
                }
            } else {
                // Unquoted value - read until space
                const nextSpace = trimmed.indexOf(' ', pos);
                if (nextSpace === -1) {
                    value = trimmed.substring(pos);
                    pos = trimmed.length;
                } else {
                    value = trimmed.substring(pos, nextSpace);
                    pos = nextSpace;
                }
            }

            params.set(key, value);
        }

        return { command, parameters: params };
    }

    protected escapeValue(value: string): string {
        if (value.indexOf(' ') !== -1 ||
            value.indexOf('=') !== -1 ||
            value.indexOf(':') !== -1 ||
            value.indexOf('\n') !== -1 ||
            value.indexOf('\r') !== -1 ||
            value.indexOf('\t') !== -1 ||
            value.indexOf('"') !== -1 ||
            value.indexOf('\\') !== -1) {
            let escaped = '"';
            for (const c of value) {
                if (c === '\n') { escaped += '\\n'; continue; }
                if (c === '\r') { escaped += '\\r'; continue; }
                if (c === '\t') { escaped += '\\t'; continue; }
                if (c === '"' || c === '\\') { escaped += '\\'; }
                escaped += c;
            }
            escaped += '"';
            return escaped;
        }
        return value;
    }
}
