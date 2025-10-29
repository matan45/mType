import { EventEmitter } from 'events';
import { spawn, ChildProcess } from 'child_process';
import * as path from 'path';

/**
 * Manages the connection to the mType interpreter process
 *
 * This class spawns the mType interpreter with the --debug flag and
 * communicates with it via stdin/stdout using the debug protocol.
 */
export class InterpreterConnection extends EventEmitter {

    private process: ChildProcess | null = null;
    private outputBuffer: string = '';

    constructor() {
        super();
    }

    /**
     * Start the interpreter process in debug mode
     */
    public async start(
        program: string,
        interpreterPath: string,
        cwd: string,
        additionalArgs?: string[]
    ): Promise<void> {

        return new Promise((resolve, reject) => {
            // Spawn the interpreter with --debug flag and any additional args
            const args = ['--debug'];
            if (additionalArgs) {
                console.log(`[DEBUG EXTENSION] Additional args provided:`, additionalArgs);
                args.push(...additionalArgs);
            }
            args.push(program);

            console.log(`[DEBUG EXTENSION] Starting interpreter with args:`, args);

            this.process = spawn(interpreterPath, args, {
                cwd: cwd,
                stdio: ['pipe', 'pipe', 'pipe']
            });

            if (!this.process.stdin || !this.process.stdout || !this.process.stderr) {
                reject(new Error('Failed to setup process stdio'));
                return;
            }

            // Handle stdout (debug protocol messages)
            this.process.stdout.on('data', (data: Buffer) => {
                this.handleOutput(data.toString());
            });

            // Handle stderr (error messages)
            this.process.stderr.on('data', (data: Buffer) => {
                const text = data.toString();
                // Check if it's a debug protocol message or actual stderr
                if (text.startsWith('OUTPUT ') || text.startsWith('ERROR ')) {
                    this.handleOutput(text);
                } else {
                    this.emit('output', text, 'stderr');
                }
            });

            // Handle process exit
            this.process.on('exit', (code: number | null) => {
                this.emit('terminated');
                this.process = null;
            });

            // Handle process errors
            this.process.on('error', (err: Error) => {
                this.emit('output', `Process error: ${err.message}`, 'stderr');
                reject(err);
            });

            // Give the process a moment to start
            setTimeout(() => {
                resolve();
            }, 100);
        });
    }

    /**
     * Stop the interpreter process
     */
    public stop(): void {
        if (this.process) {
            this.process.kill();
            this.process = null;
        }
    }

    /**
     * Send a command to the interpreter
     */
    public sendCommand(command: string): void {
        if (this.process && this.process.stdin) {
            this.process.stdin.write(command + '\n');
        }
    }

    /**
     * Handle output from the interpreter
     */
    private handleOutput(data: string): void {
        // Add to buffer
        this.outputBuffer += data;

        // Process complete lines
        let newlineIndex: number;
        while ((newlineIndex = this.outputBuffer.indexOf('\n')) !== -1) {
            const line = this.outputBuffer.substring(0, newlineIndex).trim();
            this.outputBuffer = this.outputBuffer.substring(newlineIndex + 1);

            if (line.length > 0) {
                this.processProtocolMessage(line);
            }
        }
    }

    /**
     * Process a debug protocol message
     */
    private processProtocolMessage(line: string): void {
        // Parse the protocol message
        const message = this.parseProtocolLine(line);

        if (!message.command) {
            return;
        }

        // DEBUG: Log all protocol messages
        console.log(`[DEBUG] Received protocol message: ${message.command}`, message.params);

        // Handle different message types
        switch (message.command) {
            case 'STOPPED':
                console.log(`[DEBUG] STOPPED event - reason: ${message.params.reason}, file: ${message.params.file}, line: ${message.params.line}`);
                this.emit('stopped', message.params);
                break;

            case 'TERMINATED':
                this.emit('terminated');
                break;

            case 'OUTPUT':
                this.emit('output', message.params.text || '', message.params.category || 'console');
                break;

            case 'STACKTRACE':
                this.emit('stacktrace', this.parseStackTrace(message.params));
                break;

            case 'VARIABLES':
                this.emit('variables', this.parseVariables(message.params));
                break;

            case 'EXPANDEDVAR':
                this.emit('expandedVariable', this.parseVariables(message.params));
                break;

            case 'RESULT':
                this.emit('evaluateResult', {
                    value: message.params.value || '',
                    type: message.params.type || 'unknown',
                    refId: parseInt(message.params.ref || '0')
                });
                break;

            case 'OK':
                // Command acknowledged
                break;

            case 'ERROR':
                this.emit('error', message.params.message || 'Unknown error');
                this.emit('output', `Debug error: ${message.params.message}`, 'stderr');
                break;

            default:
                console.log(`Unknown debug message: ${message.command}`);
        }
    }

    /**
     * Parse a protocol line into command and parameters
     *
     * Format: COMMAND key1=value1 key2=value2 ...
     */
    private parseProtocolLine(line: string): { command: string, params: any } {
        const parts = line.split(' ');
        const command = parts[0];
        const params: any = {};

        for (let i = 1; i < parts.length; i++) {
            const part = parts[i];
            const eqIndex = part.indexOf('=');

            if (eqIndex > 0) {
                const key = part.substring(0, eqIndex);
                let value = part.substring(eqIndex + 1);

                // Handle quoted values
                if (value.startsWith('"')) {
                    // Collect until closing quote
                    value = value.substring(1);
                    while (i < parts.length - 1 && !value.endsWith('"')) {
                        i++;
                        value += ' ' + parts[i];
                    }
                    if (value.endsWith('"')) {
                        value = value.substring(0, value.length - 1);
                    }
                }

                // Unescape special characters
                value = value.replace(/\\n/g, '\n').replace(/\\"/g, '"').replace(/\\\\/g, '\\');

                params[key] = value;
            }
        }

        return { command, params };
    }

    /**
     * Parse stack trace from protocol format
     *
     * Format: frame0="functionName@filename:line" frame1="..."
     * Returns: [{name: "functionName", file: "filename", line: 123}, ...]
     */
    private parseStackTrace(params: any): any[] {
        const frames: any[] = [];

        // Find all frame0, frame1, frame2, etc.
        const keys = Object.keys(params).filter(k => k.startsWith('frame'));

        for (const key of keys) {
            const frameStr = params[key];
            // Parse: functionName@filename:line
            const atIndex = frameStr.indexOf('@');
            if (atIndex < 0) continue;

            const functionName = frameStr.substring(0, atIndex);
            const rest = frameStr.substring(atIndex + 1);

            // Split filename:line
            const colonIndex = rest.lastIndexOf(':');
            if (colonIndex < 0) continue;

            const file = rest.substring(0, colonIndex);
            const line = parseInt(rest.substring(colonIndex + 1));

            frames.push({
                name: functionName,
                file: file,
                line: line
            });
        }

        return frames;
    }

    /**
     * Parse variables from protocol format
     *
     * Format: var0="x=42:int:0" var1="obj={MyClass}:MyClass:123"
     * Returns: [{name: "x", value: "42", type: "int", refId: 0}, ...]
     */
    private parseVariables(params: any): any[] {
        const variables: any[] = [];

        // Find all var0, var1, child0, child1, etc.
        const keys = Object.keys(params).filter(k => k.startsWith('var') || k.startsWith('child'));

        for (const key of keys) {
            const varStr = params[key];
            // Parse: name=value:type:refId
            const eqIndex = varStr.indexOf('=');
            if (eqIndex < 0) continue;

            const name = varStr.substring(0, eqIndex);
            const rest = varStr.substring(eqIndex + 1);

            // Split by : to get value, type, refId
            const parts = rest.split(':');
            if (parts.length < 3) continue;

            // The value might contain colons, so rejoin all but the last 2 parts
            const refId = parseInt(parts[parts.length - 1]);
            const type = parts[parts.length - 2];
            const value = parts.slice(0, parts.length - 2).join(':');

            variables.push({
                name,
                value,
                type,
                refId
            });
        }

        return variables;
    }
}
