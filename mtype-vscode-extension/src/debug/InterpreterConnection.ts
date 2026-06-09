import { ChildProcess, spawn } from 'child_process';
import * as readline from 'readline';
import { ProtocolConnection, ProtocolMessage } from './ProtocolConnection';

export { ProtocolMessage } from './ProtocolConnection';

/**
 * Process-spawning transport for the mType debug protocol (the `launch` flow).
 * Spawns the mType interpreter in --debug mode and talks the line protocol over
 * the child's stdin/stdout. Program output (print) arrives on stderr.
 */
export class InterpreterConnection extends ProtocolConnection {
    private process: ChildProcess | null = null;
    private stdoutReader: readline.Interface | null = null;
    private stderrReader: readline.Interface | null = null;

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

    protected writeLine(line: string): void {
        if (this.process && this.process.stdin && !this.process.stdin.destroyed) {
            this.process.stdin.write(line + '\n');
        }
    }
}
