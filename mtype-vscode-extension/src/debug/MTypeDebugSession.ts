import {
    LoggingDebugSession,
    InitializedEvent,
    StoppedEvent,
    TerminatedEvent,
    OutputEvent,
    Thread,
    StackFrame,
    Scope,
    Source,
    Breakpoint
} from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import { InterpreterConnection, ProtocolMessage } from './InterpreterConnection';
import * as path from 'path';

const THREAD_ID = 1;

// Fixed scope reference IDs (must not collide with variable refIds which start at 1000+)
const SCOPE_LOCAL = 1;
const SCOPE_GLOBAL = 2;
const SCOPE_STATIC = 3;

interface LaunchRequestArguments extends DebugProtocol.LaunchRequestArguments {
    program: string;
    stopOnEntry?: boolean;
    trace?: boolean;
    interpreterPath?: string;
    args?: string[];
    cwd?: string;
}

export class MTypeDebugSession extends LoggingDebugSession {
    private connection: InterpreterConnection;
    private stopOnEntry: boolean = true;
    private lastStoppedFile: string = '';
    private lastStoppedLine: number = 0;

    // Cache variables between requests
    private variableCache: Map<number, DebugProtocol.Variable[]> = new Map();

    constructor() {
        super('mtype-debug.txt');

        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);

        this.connection = new InterpreterConnection();

        this.connection.on('stopped', (msg: ProtocolMessage) => {
            const reason = msg.parameters.get('reason') || 'breakpoint';
            this.lastStoppedFile = msg.parameters.get('file') || '';
            this.lastStoppedLine = parseInt(msg.parameters.get('line') || '0', 10);

            // Clear variable cache on each stop
            this.variableCache.clear();

            if (reason === 'exception') {
                const message = msg.parameters.get('message') || 'Exception occurred';
                this.sendEvent(new StoppedEvent('exception', THREAD_ID, message));
            } else {
                this.sendEvent(new StoppedEvent(reason, THREAD_ID));
            }
        });

        this.connection.on('terminated', () => {
            this.sendEvent(new TerminatedEvent());
        });

        this.connection.on('output', (text: string, category: string) => {
            this.sendEvent(new OutputEvent(text + '\n', category));
        });

        this.connection.on('exit', () => {
            this.sendEvent(new TerminatedEvent());
        });
    }

    protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {
        response.body = response.body || {};
        response.body.supportsConfigurationDoneRequest = true;
        response.body.supportsConditionalBreakpoints = true;
        response.body.supportsLogPoints = true;
        response.body.supportsExceptionInfoRequest = false;
        response.body.supportsExceptionFilterOptions = true;
        response.body.exceptionBreakpointFilters = [
            {
                filter: 'all',
                label: 'All Exceptions',
                description: 'Break on all exceptions',
                default: false
            },
            {
                filter: 'uncaught',
                label: 'Uncaught Exceptions',
                description: 'Break on uncaught exceptions only',
                default: true
            }
        ];
        response.body.supportsEvaluateForHovers = true;
        response.body.supportsTerminateRequest = true;

        this.sendResponse(response);
    }

    protected async launchRequest(response: DebugProtocol.LaunchResponse, args: LaunchRequestArguments): Promise<void> {
        this.stopOnEntry = args.stopOnEntry !== false;

        const interpreterPath = args.interpreterPath || '';
        if (!interpreterPath) {
            this.sendErrorResponse(response, 1, 'Interpreter path not configured. Set mTypeLanguageServer.interpreterPath in settings.');
            return;
        }

        const program = args.program;
        if (!program) {
            this.sendErrorResponse(response, 2, 'No program specified');
            return;
        }

        try {
            await this.connection.start(
                interpreterPath,
                program,
                args.args || [],
                args.cwd
            );

            // Interpreter is now paused at entry
            this.sendResponse(response);
            this.sendEvent(new InitializedEvent());

        } catch (err: any) {
            this.sendErrorResponse(response, 3, `Failed to launch: ${err.message}`);
        }
    }

    protected async setBreakPointsRequest(response: DebugProtocol.SetBreakpointsResponse, args: DebugProtocol.SetBreakpointsArguments): Promise<void> {
        const filePath = args.source.path || '';
        const breakpoints: Breakpoint[] = [];

        try {
            // Clear existing breakpoints for this file
            await this.connection.sendCommandExpectOK('CLEARFILE', { file: filePath });

            // Set new breakpoints
            if (args.breakpoints) {
                for (const bp of args.breakpoints) {
                    const params: Record<string, string | number> = {
                        file: filePath,
                        line: bp.line
                    };
                    if (bp.condition) {
                        params['condition'] = bp.condition;
                    }
                    if (bp.logMessage) {
                        params['logMessage'] = bp.logMessage;
                    }

                    await this.connection.sendCommandExpectOK('SETBREAKPOINT', params);
                    breakpoints.push(new Breakpoint(true, bp.line));
                }
            }
        } catch (err: any) {
            // Mark all breakpoints as unverified on error
            if (args.breakpoints) {
                for (const bp of args.breakpoints) {
                    breakpoints.push(new Breakpoint(false, bp.line));
                }
            }
        }

        response.body = { breakpoints };
        this.sendResponse(response);
    }

    protected async setExceptionBreakPointsRequest(response: DebugProtocol.SetExceptionBreakpointsResponse, args: DebugProtocol.SetExceptionBreakpointsArguments): Promise<void> {
        const params: Record<string, string | number> = {};
        if (args.filters) {
            args.filters.forEach((filter, i) => {
                params[`filter${i}`] = filter;
            });
        }

        try {
            await this.connection.sendCommandExpectOK('SETEXCEPTIONBREAKPOINTS', params);
        } catch {
            // Non-fatal
        }

        this.sendResponse(response);
    }

    protected async configurationDoneRequest(response: DebugProtocol.ConfigurationDoneResponse, args: DebugProtocol.ConfigurationDoneArguments): Promise<void> {
        this.sendResponse(response);

        if (this.stopOnEntry) {
            // Already stopped at entry — send a stopped event so VS Code shows it
            this.sendEvent(new StoppedEvent('entry', THREAD_ID));
        } else {
            // Resume past entry
            try {
                await this.connection.sendCommandExpectOK('CONTINUE');
            } catch {
                // If continue fails, we're still stopped at entry
            }
        }
    }

    protected threadsRequest(response: DebugProtocol.ThreadsResponse): void {
        response.body = {
            threads: [new Thread(THREAD_ID, 'Main Thread')]
        };
        this.sendResponse(response);
    }

    protected async stackTraceRequest(response: DebugProtocol.StackTraceResponse, args: DebugProtocol.StackTraceArguments): Promise<void> {
        try {
            const msg = await this.connection.sendCommand('GETSTACKTRACE');

            if (msg.command === 'STACKTRACE') {
                const frames: StackFrame[] = [];
                let frameIdx = 0;

                while (true) {
                    const frameStr = msg.parameters.get(`frame${frameIdx}`);
                    if (!frameStr) { break; }

                    // Parse "funcName@file.mt:line"
                    const atIdx = frameStr.lastIndexOf('@');
                    let funcName = '<unknown>';
                    let file = '';
                    let line = 0;

                    if (atIdx !== -1) {
                        funcName = frameStr.substring(0, atIdx);
                        const locStr = frameStr.substring(atIdx + 1);
                        const colonIdx = locStr.lastIndexOf(':');
                        if (colonIdx !== -1) {
                            file = locStr.substring(0, colonIdx);
                            line = parseInt(locStr.substring(colonIdx + 1), 10) || 0;
                        } else {
                            file = locStr;
                        }
                    }

                    // Override frame 0 with the actual stopped location
                    if (frameIdx === 0 && this.lastStoppedFile) {
                        file = this.lastStoppedFile;
                        line = this.lastStoppedLine;
                    }

                    const source = file ? new Source(path.basename(file), file) : undefined;
                    frames.push(new StackFrame(frameIdx, funcName, source, line));
                    frameIdx++;
                }

                response.body = {
                    stackFrames: frames,
                    totalFrames: frames.length
                };
            } else {
                response.body = { stackFrames: [], totalFrames: 0 };
            }
        } catch {
            response.body = { stackFrames: [], totalFrames: 0 };
        }

        this.sendResponse(response);
    }

    protected scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments): void {
        response.body = {
            scopes: [
                new Scope('Local', SCOPE_LOCAL, false),
                new Scope('Global', SCOPE_GLOBAL, true),
                new Scope('Static', SCOPE_STATIC, true)
            ]
        };
        this.sendResponse(response);
    }

    protected async variablesRequest(response: DebugProtocol.VariablesResponse, args: DebugProtocol.VariablesArguments): Promise<void> {
        const ref = args.variablesReference;
        const variables: DebugProtocol.Variable[] = [];

        try {
            let msg: ProtocolMessage;

            if (ref === SCOPE_LOCAL || ref === SCOPE_GLOBAL || ref === SCOPE_STATIC) {
                // Scope request
                const scopeName = ref === SCOPE_LOCAL ? 'local' : ref === SCOPE_GLOBAL ? 'global' : 'static';
                msg = await this.connection.sendCommand('GETVARIABLES', { scope: scopeName });

                if (msg.command === 'VARIABLES') {
                    this.parseVariables(msg, variables, 'var');
                }
            } else {
                // Variable expansion request
                msg = await this.connection.sendCommand('EXPANDVARIABLE', { ref });

                if (msg.command === 'EXPANDEDVAR') {
                    this.parseVariables(msg, variables, 'child');
                }
            }
        } catch {
            // Return empty on timeout/error
        }

        response.body = { variables };
        this.sendResponse(response);
    }

    protected async evaluateRequest(response: DebugProtocol.EvaluateResponse, args: DebugProtocol.EvaluateArguments): Promise<void> {
        try {
            const params: Record<string, string | number> = {
                expr: args.expression
            };
            if (args.frameId !== undefined) {
                params['frame'] = args.frameId;
            }

            const msg = await this.connection.sendCommand('EVALUATE', params, 5000);

            if (msg.command === 'RESULT') {
                const value = msg.parameters.get('value') || '';
                const type = msg.parameters.get('type') || '';
                const refStr = msg.parameters.get('ref');
                const refId = refStr ? parseInt(refStr, 10) : 0;

                response.body = {
                    result: value,
                    type: type,
                    variablesReference: refId
                };
            } else if (msg.command === 'ERROR') {
                this.sendErrorResponse(response, 4, msg.parameters.get('message') || 'Evaluation failed');
                return;
            } else {
                response.body = {
                    result: '',
                    variablesReference: 0
                };
            }
        } catch {
            this.sendErrorResponse(response, 4, 'Evaluation timeout');
            return;
        }

        this.sendResponse(response);
    }

    protected async continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments): Promise<void> {
        this.variableCache.clear();
        try {
            await this.connection.sendCommandExpectOK('CONTINUE');
        } catch {
            // Non-fatal
        }
        response.body = { allThreadsContinued: true };
        this.sendResponse(response);
    }

    protected async nextRequest(response: DebugProtocol.NextResponse, args: DebugProtocol.NextArguments): Promise<void> {
        this.variableCache.clear();
        try {
            await this.connection.sendCommandExpectOK('STEPOVER');
        } catch {
            // Non-fatal
        }
        this.sendResponse(response);
    }

    protected async stepInRequest(response: DebugProtocol.StepInResponse, args: DebugProtocol.StepInArguments): Promise<void> {
        this.variableCache.clear();
        try {
            await this.connection.sendCommandExpectOK('STEPINTO');
        } catch {
            // Non-fatal
        }
        this.sendResponse(response);
    }

    protected async stepOutRequest(response: DebugProtocol.StepOutResponse, args: DebugProtocol.StepOutArguments): Promise<void> {
        this.variableCache.clear();
        try {
            await this.connection.sendCommandExpectOK('STEPOUT');
        } catch {
            // Non-fatal
        }
        this.sendResponse(response);
    }

    protected async terminateRequest(response: DebugProtocol.TerminateResponse, args: DebugProtocol.TerminateArguments): Promise<void> {
        this.connection.stop();
        this.sendResponse(response);
    }

    protected async disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments): Promise<void> {
        this.connection.kill();
        this.sendResponse(response);
    }

    /**
     * Parse variable entries from a protocol message.
     * Format: var0="name=value:type:refId" or child0="name=value:type:refId"
     */
    private parseVariables(msg: ProtocolMessage, variables: DebugProtocol.Variable[], prefix: string): void {
        let idx = 0;
        while (true) {
            const varStr = msg.parameters.get(`${prefix}${idx}`);
            if (!varStr) { break; }

            // Parse "name=value:type:refId"
            // Split from the right: last ':' is refId, second-to-last is type
            const lastColon = varStr.lastIndexOf(':');
            if (lastColon === -1) {
                idx++;
                continue;
            }
            const refIdStr = varStr.substring(lastColon + 1);
            const rest1 = varStr.substring(0, lastColon);

            const secondLastColon = rest1.lastIndexOf(':');
            if (secondLastColon === -1) {
                idx++;
                continue;
            }
            const type = rest1.substring(secondLastColon + 1);
            const nameValue = rest1.substring(0, secondLastColon);

            // Split name=value on first '='
            const eqIdx = nameValue.indexOf('=');
            if (eqIdx === -1) {
                idx++;
                continue;
            }
            const name = nameValue.substring(0, eqIdx);
            const value = nameValue.substring(eqIdx + 1);
            const refId = parseInt(refIdStr, 10) || 0;

            variables.push({
                name,
                value,
                type,
                variablesReference: refId,
                evaluateName: name
            });

            idx++;
        }
    }
}
