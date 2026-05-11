import { ChildProcess, spawn } from 'child_process';
import { EventEmitter } from 'events';
import * as readline from 'readline';

/**
 * Parsed protocol message from the C++ debug backend.
 * Format: COMMAND key1=value1 key2="quoted value"
 */
export interface ProtocolMessage {
    command: string;
    parameters: Map<string, string>;
}

/**
 * Manages the stdin/stdout connection to the mType interpreter process
 * running in --debug mode. Sends line-based protocol commands and
 * parses incoming messages/events.
 */
export class InterpreterConnection extends EventEmitter {
    private process: ChildProcess | null = null;
    private stdoutReader: readline.Interface | null = null;
    private stderrReader: readline.Interface | null = null;

    // Current in-flight request
    private pendingResolve: ((msg: ProtocolMessage) => void) | null = null;
    private pendingReject: ((err: Error) => void) | null = null;
    private pendingTimeout: NodeJS.Timeout | null = null;

    // Sequential command queue: ensures only one command is in-flight at a time
    private commandQueue: Array<{
        line: string;
        timeoutMs: number;
        resolve: (msg: ProtocolMessage) => void;
        reject: (err: Error) => void;
    }> = [];
    private processingCommand: boolean = false;
    private ended: boolean = false;

    /**
     * Spawn the mType interpreter in debug mode.
     * @param interpreterPath Path to the mType executable
     * @param scriptPath Path to the .mt script to debug
     * @param args Additional arguments
     * @param cwd Working directory
     * @returns Promise that resolves when the first STOPPED(entry) event is received
     */
    public start(interpreterPath: string, scriptPath: string, args: string[] = [], cwd?: string): Promise<ProtocolMessage> {
        return new Promise((resolve, reject) => {
            try {
                this.ended = false;
                this.process = spawn(interpreterPath, ['--debug', scriptPath, ...args], {
                    cwd: cwd,
                    stdio: ['pipe', 'pipe', 'pipe']
                });

                let entryReceived = false;
                const startTimeout = setTimeout(() => {
                    if (!entryReceived && !this.ended) {
                        reject(new Error('Timeout waiting for debugger to start'));
                    }
                }, 10000);

                this.process.on('error', (err) => {
                    clearTimeout(startTimeout);
                    this.drainQueue();
                    reject(new Error(`Failed to start interpreter: ${err.message}`));
                });

                this.process.on('exit', (code, signal) => {
                    this.ended = true;
                    clearTimeout(startTimeout);
                    this.drainQueue();
                    this.emit('exit', code, signal);
                });

                // Read protocol messages from stdout
                if (this.process.stdout) {
                    this.stdoutReader = readline.createInterface({ input: this.process.stdout });

                    this.stdoutReader.on('line', (line: string) => {
                        if (!line.trim()) { return; }

                        const msg = this.parseMessage(line);
                        if (!msg.command) { return; }

                        // The first STOPPED event (reason=entry) resolves the start() promise
                        if (!entryReceived && msg.command === 'STOPPED' && msg.parameters.get('reason') === 'entry') {
                            entryReceived = true;
                            clearTimeout(startTimeout);
                            resolve(msg);
                            return;
                        }

                        this.handleMessage(msg);
                    });
                }

                // Read program output from stderr (print() goes to stderr in debug mode)
                if (this.process.stderr) {
                    this.stderrReader = readline.createInterface({ input: this.process.stderr });
                    this.stderrReader.on('line', (line: string) => {
                        this.emit('output', line, 'stdout');
                    });
                }

            } catch (err: any) {
                reject(new Error(`Failed to spawn interpreter: ${err.message}`));
            }
        });
    }

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
     * Stop the interpreter process.
     */
    public stop(): void {
        this.drainQueue();
        this.writeLine('STOP');

        setTimeout(() => {
            if (this.process && !this.process.killed) {
                this.process.kill();
            }
        }, 1000);
    }

    /**
     * Kill the process immediately.
     */
    public kill(): void {
        this.drainQueue();
        if (this.stdoutReader) {
            this.stdoutReader.close();
        }
        if (this.stderrReader) {
            this.stderrReader.close();
        }
        if (this.process && !this.process.killed) {
            this.process.kill('SIGKILL');
        }
        this.process = null;
    }

    private drainQueue(): void {
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

    private handleMessage(msg: ProtocolMessage): void {
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

    private writeLine(line: string): void {
        if (this.process && this.process.stdin && !this.process.stdin.destroyed) {
            this.process.stdin.write(line + '\n');
        }
    }

    /**
     * Parse a line-based protocol message.
     * Format: COMMAND key1=value1 key2="quoted value"
     */
    private parseMessage(line: string): ProtocolMessage {
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

    private escapeValue(value: string): string {
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
