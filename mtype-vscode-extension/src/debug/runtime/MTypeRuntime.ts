import { EventEmitter } from 'events';
import { InterpreterConnection } from '../communication/InterpreterConnection';

/**
 * Represents a breakpoint in the runtime
 */
export interface RuntimeBreakpoint {
    id: number;
    line: number;
    verified: boolean;
}

/**
 * Represents a stack frame
 */
export interface RuntimeStackFrame {
    index: number;
    name: string;
    file: string;
    line: number;
    column: number;
}

/**
 * Runtime state manager for mType debugger
 *
 * This class manages the mType interpreter process and communicates
 * with it via the debug protocol over stdin/stdout.
 */
export class MTypeRuntime extends EventEmitter {

    private connection: InterpreterConnection;
    private breakpoints = new Map<string, RuntimeBreakpoint[]>();
    private stackFrames: RuntimeStackFrame[] = [];
    private breakpointIdCounter = 1;
    private variables = new Map<string, any[]>(); // scope -> variables
    private variableReferences = new Map<number, any[]>(); // refId -> children
    private pendingVariableRequest: { scope: string; resolve: (vars: any[]) => void } | null = null;
    private pendingExpandRequest: { refId: number; resolve: (children: any[]) => void } | null = null;
    private lastStoppedLocation: { file: string; line: number } | null = null;

    constructor() {
        super();
        this.connection = new InterpreterConnection();

        // Setup event handlers from interpreter connection
        this.connection.on('stopped', (data) => {
            this.handleStoppedEvent(data);
        });

        this.connection.on('output', (text: string, category: string) => {
            this.emit('output', text, category);
        });

        this.connection.on('terminated', () => {
            this.emit('end');
        });

        this.connection.on('stacktrace', (frames: any[]) => {
            this.handleStackTraceResponse(frames);
        });

        this.connection.on('variables', (variables: any[]) => {
            this.handleVariablesResponse(variables);
        });

        this.connection.on('expandedVariable', (children: any[]) => {
            this.handleExpandedVariableResponse(children);
        });
    }

    /**
     * Start the debugging session
     */
    public async start(
        program: string,
        interpreterPath: string,
        stopOnEntry: boolean,
        cwd: string,
        additionalArgs?: string[]
    ): Promise<void> {

        // Start the interpreter in debug mode
        await this.connection.start(program, interpreterPath, cwd, additionalArgs);

        // If stop on entry, the interpreter will pause at first line
        if (stopOnEntry) {
            // The interpreter starts paused, so we just wait for the stopped event
        }
    }

    /**
     * Stop the debugging session
     */
    public stop(): void {
        this.connection.sendCommand('STOP');
        this.connection.stop();
    }

    /**
     * Set a breakpoint
     */
    public async setBreakpoint(
        path: string,
        line: number,
        condition?: string,
        logMessage?: string
    ): Promise<RuntimeBreakpoint> {
        const bp: RuntimeBreakpoint = {
            id: this.breakpointIdCounter++,
            line: line,
            verified: false // Will be verified by interpreter
        };

        // Store breakpoint
        if (!this.breakpoints.has(path)) {
            this.breakpoints.set(path, []);
        }
        this.breakpoints.get(path)!.push(bp);

        // Build command with optional condition and logMessage
        let command = `SETBREAKPOINT file=${path} line=${line}`;
        if (condition) {
            command += ` condition="${condition.replace(/"/g, '\\"')}"`;
        }
        if (logMessage) {
            command += ` logMessage="${logMessage.replace(/"/g, '\\"')}"`;
        }

        // Send to interpreter
        await this.connection.sendCommand(command);

        // Mark as verified (optimistic - interpreter will confirm)
        bp.verified = true;

        return bp;
    }

    /**
     * Clear all breakpoints for a file
     */
    public async clearBreakpoints(path: string): Promise<void> {
        this.breakpoints.delete(path);
        // Send CLEARFILE command to interpreter
        await this.connection.sendCommand(`CLEARFILE file=${path}`);
    }

    /**
     * Clear all breakpoints
     */
    public async clearAllBreakpoints(): Promise<void> {
        this.breakpoints.clear();
        await this.connection.sendCommand('CLEARALL');
    }

    /**
     * Set exception breakpoints
     */
    public async setExceptionBreakpoints(filters: string[]): Promise<void> {
        // Build command with filters
        let command = 'SETEXCEPTIONBREAKPOINTS';
        filters.forEach((filter, index) => {
            command += ` filter${index}=${filter}`;
        });
        await this.connection.sendCommand(command);
    }

    /**
     * Continue execution
     */
    public continue(): void {
        this.connection.sendCommand('CONTINUE');
    }

    /**
     * Step into
     */
    public stepInto(): void {
        this.connection.sendCommand('STEPINTO');
    }

    /**
     * Step over
     */
    public stepOver(): void {
        this.connection.sendCommand('STEPOVER');
    }

    /**
     * Step out
     */
    public stepOut(): void {
        this.connection.sendCommand('STEPOUT');
    }

    /**
     * Get stack frames
     */
    public getStack(startFrame: number, maxLevels: number): { frames: RuntimeStackFrame[], count: number } {
        const frames = this.stackFrames.slice(startFrame, startFrame + maxLevels);
        return {
            frames: frames,
            count: this.stackFrames.length
        };
    }

    /**
     * Get variables for a scope
     */
    public async getVariables(scope: string): Promise<any[]> {
        return new Promise((resolve) => {
            // Store pending request so we know which scope this response is for
            this.pendingVariableRequest = { scope, resolve };

            // Request variables from interpreter
            this.connection.sendCommand(`GETVARIABLES scope=${scope}`);

            // Timeout after 1 second (return cached or empty)
            setTimeout(() => {
                if (this.pendingVariableRequest && this.pendingVariableRequest.scope === scope) {
                    this.pendingVariableRequest = null;
                    resolve(this.variables.get(scope) || []);
                }
            }, 1000);
        });
    }

    /**
     * Get children of an expandable variable
     */
    public async getVariableChildren(refId: number): Promise<any[]> {
        return new Promise((resolve) => {
            // Store pending request so we know which refId this response is for
            this.pendingExpandRequest = { refId, resolve };

            // Request expansion from interpreter
            this.connection.sendCommand(`EXPANDVARIABLE ref=${refId}`);

            // Timeout after 1 second (return cached or empty)
            setTimeout(() => {
                if (this.pendingExpandRequest && this.pendingExpandRequest.refId === refId) {
                    this.pendingExpandRequest = null;
                    resolve(this.variableReferences.get(refId) || []);
                }
            }, 1000);
        });
    }

    /**
     * Evaluate an expression
     */
    public async evaluate(expression: string, frameId: number): Promise<{value: string, type: string, refId: number}> {
        return new Promise((resolve, reject) => {
            // Set up one-time listener for the result
            const resultHandler = (result: any) => {
                this.connection.removeListener('evaluateResult', resultHandler);
                this.connection.removeListener('error', errorHandler);
                resolve(result);
            };

            const errorHandler = (error: string) => {
                this.connection.removeListener('evaluateResult', resultHandler);
                this.connection.removeListener('error', errorHandler);
                reject(error);
            };

            this.connection.once('evaluateResult', resultHandler);
            this.connection.once('error', errorHandler);

            // Send evaluate command
            this.connection.sendCommand(`EVALUATE expr="${expression}" frame=${frameId}`);

            // Timeout after 5 seconds
            setTimeout(() => {
                this.connection.removeListener('evaluateResult', resultHandler);
                this.connection.removeListener('error', errorHandler);
                reject('Evaluation timeout');
            }, 5000);
        });
    }

    /**
     * Handle stopped event from interpreter
     */
    private handleStoppedEvent(data: any): void {
        const reason = data.reason || 'unknown';
        const file = data.file || '';
        const line = parseInt(data.line) || 0;

        // Update stack frames
        this.updateStackFrames(file, line);

        // Emit appropriate event
        switch (reason) {
            case 'entry':
                this.emit('stopOnEntry');
                break;
            case 'step':
                this.emit('stopOnStep');
                break;
            case 'breakpoint':
                this.emit('stopOnBreakpoint');
                break;
            case 'exception':
                this.emit('stopOnException', data.message || 'Exception occurred');
                break;
        }
    }

    /**
     * Update stack frames from current location
     */
    private updateStackFrames(file: string, line: number): void {
        console.log(`[DEBUG] updateStackFrames called - file: ${file}, line: ${line}`);

        // Store the current stopped location
        this.lastStoppedLocation = { file, line };

        // Request stack trace from interpreter
        this.connection.sendCommand('GETSTACKTRACE');

        // The actual stack frames will be set when STACKTRACE response arrives
        // For now, create a temporary frame so VSCode has something to show immediately
        this.stackFrames = [
            {
                index: 0,
                name: 'main',
                file: file,
                line: line,
                column: 1
            }
        ];

        console.log(`[DEBUG] Created temporary stack frame:`, this.stackFrames[0]);

        // Request variables when we stop
        this.requestVariables();
    }

    /**
     * Handle stack trace response from interpreter
     */
    private handleStackTraceResponse(frames: any[]): void {
        console.log(`[DEBUG] handleStackTraceResponse - received ${frames.length} frames:`, frames);

        // Convert to RuntimeStackFrame format
        this.stackFrames = frames.map((frame, index) => ({
            index: index,
            name: frame.name || 'unknown',
            file: frame.file || '',
            line: frame.line || 0,
            column: 1
        }));

        console.log(`[DEBUG] Converted stack frames:`, this.stackFrames);

        // IMPORTANT: Use the STOPPED event's line number for frame 0 (current location)
        // The STACKTRACE shows call sites, not the current execution line
        if (this.lastStoppedLocation && this.stackFrames.length > 0) {
            console.log(`[DEBUG] Overriding frame 0 with lastStoppedLocation:`, this.lastStoppedLocation);
            this.stackFrames[0].file = this.lastStoppedLocation.file;
            this.stackFrames[0].line = this.lastStoppedLocation.line;
        }

        console.log(`[DEBUG] Final stack frames:`, this.stackFrames);
    }

    /**
     * Request variables from the interpreter
     */
    private requestVariables(): void {
        this.connection.sendCommand('GETVARIABLES scope=local');
        this.connection.sendCommand('GETVARIABLES scope=global');
    }

    /**
     * Handle variables response from interpreter
     */
    private handleVariablesResponse(variables: any[]): void {
        // Check if this response is for a pending request
        if (this.pendingVariableRequest) {
            const { scope, resolve } = this.pendingVariableRequest;
            this.pendingVariableRequest = null;

            // Store in cache with correct scope
            this.variables.set(scope, variables);

            // Store references for expandable variables
            for (const variable of variables) {
                if (variable.refId > 0) {
                    // Initialize empty children array for this reference
                    this.variableReferences.set(variable.refId, []);
                }
            }

            // Resolve the promise with the variables
            resolve(variables);
        }
    }

    /**
     * Handle expanded variable response from interpreter
     */
    private handleExpandedVariableResponse(children: any[]): void {
        // Check if this response is for a pending expand request
        if (this.pendingExpandRequest) {
            const { refId, resolve } = this.pendingExpandRequest;
            this.pendingExpandRequest = null;

            // Store children associated with their parent refId
            this.variableReferences.set(refId, children);

            // Store references for any expandable children
            for (const child of children) {
                if (child.refId > 0) {
                    this.variableReferences.set(child.refId, []);
                }
            }

            // Resolve the promise with the children
            resolve(children);
        }
    }
}
