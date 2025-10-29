import {
    Logger, logger,
    LoggingDebugSession,
    InitializedEvent, TerminatedEvent, StoppedEvent, BreakpointEvent, OutputEvent,
    Thread, StackFrame, Scope, Source, Handles, Breakpoint
} from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import * as path from 'path';
import { MTypeRuntime, RuntimeBreakpoint } from './runtime/MTypeRuntime';

/**
 * Interface for launch request arguments
 */
interface LaunchRequestArguments extends DebugProtocol.LaunchRequestArguments {
    program: string;
    stopOnEntry?: boolean;
    trace?: boolean;
    interpreterPath?: string;
    args?: string[];
    cwd?: string;
}

/**
 * Main debug adapter session for mType
 *
 * This class implements the Debug Adapter Protocol (DAP) and acts as a bridge
 * between VSCode's debug UI and the mType interpreter's debug server.
 */
export class MTypeDebugSession extends LoggingDebugSession {

    // Thread ID constant (mType is single-threaded)
    private static THREAD_ID = 1;

    // Fixed scope IDs that won't collide with interpreter refIds (which start at 1000)
    private static LOCAL_SCOPE_ID = 1;
    private static GLOBAL_SCOPE_ID = 2;

    private _runtime: MTypeRuntime;
    private _configurationDone = false;

    /**
     * Creates a new debug adapter session
     */
    public constructor() {
        super("mtype-debug");

        // Setup line number conversion (VSCode uses 1-based, we use 1-based too)
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);

        // Create runtime interface
        this._runtime = new MTypeRuntime();

        // Setup runtime event handlers
        this._runtime.on('stopOnEntry', () => {
            this.sendEvent(new StoppedEvent('entry', MTypeDebugSession.THREAD_ID));
        });

        this._runtime.on('stopOnStep', () => {
            this.sendEvent(new StoppedEvent('step', MTypeDebugSession.THREAD_ID));
        });

        this._runtime.on('stopOnBreakpoint', () => {
            this.sendEvent(new StoppedEvent('breakpoint', MTypeDebugSession.THREAD_ID));
        });

        this._runtime.on('stopOnException', (message: string) => {
            this.sendEvent(new StoppedEvent('exception', MTypeDebugSession.THREAD_ID, message));
        });

        this._runtime.on('breakpointValidated', (bp: RuntimeBreakpoint) => {
            this.sendEvent(new BreakpointEvent('changed', {
                verified: bp.verified,
                id: bp.id,
                line: bp.line
            } as DebugProtocol.Breakpoint));
        });

        this._runtime.on('output', (text: string, category: string) => {
            const e: DebugProtocol.OutputEvent = new OutputEvent(`${text}\n`);
            e.body.category = category as 'console' | 'stdout' | 'stderr';
            this.sendEvent(e);
        });

        this._runtime.on('end', () => {
            this.sendEvent(new TerminatedEvent());
        });
    }

    /**
     * Initialize request - first request from client
     */
    protected initializeRequest(
        response: DebugProtocol.InitializeResponse,
        args: DebugProtocol.InitializeRequestArguments
    ): void {

        // Build and send the capabilities of this debug adapter
        response.body = response.body || {};

        // Supported features
        response.body.supportsConfigurationDoneRequest = true;
        response.body.supportsEvaluateForHovers = true;
        response.body.supportsStepBack = false;
        response.body.supportsSetVariable = false;
        response.body.supportsRestartFrame = false;
        response.body.supportsGotoTargetsRequest = false;
        response.body.supportsStepInTargetsRequest = false;
        response.body.supportsCompletionsRequest = false;
        response.body.supportsCancelRequest = false;
        response.body.supportsBreakpointLocationsRequest = false;
        response.body.supportsConditionalBreakpoints = true;
        response.body.supportsHitConditionalBreakpoints = false;  // Not implemented yet
        response.body.supportsFunctionBreakpoints = false;
        response.body.supportsExceptionInfoRequest = false;
        response.body.supportsExceptionOptions = false;
        response.body.exceptionBreakpointFilters = [
            { filter: 'all', label: 'All Exceptions', default: false },
            { filter: 'uncaught', label: 'Uncaught Exceptions', default: true }
        ];
        response.body.supportsValueFormattingOptions = false;
        response.body.supportsExceptionFilterOptions = false;
        response.body.supportsClipboardContext = false;
        response.body.supportsLogPoints = true;

        this.sendResponse(response);

        // Signal that initialization is complete
        this.sendEvent(new InitializedEvent());
    }

    /**
     * Configuration done request - all configuration requests have been sent
     */
    protected configurationDoneRequest(
        response: DebugProtocol.ConfigurationDoneResponse,
        args: DebugProtocol.ConfigurationDoneArguments
    ): void {
        super.configurationDoneRequest(response, args);
        this._configurationDone = true;
    }

    /**
     * Launch request - start debugging
     */
    protected async launchRequest(
        response: DebugProtocol.LaunchResponse,
        args: LaunchRequestArguments
    ): Promise<void> {

        // Setup logging if trace is enabled
        logger.setup(args.trace ? Logger.LogLevel.Verbose : Logger.LogLevel.Error, false);
        // Wait for configuration to complete
        await this._runtime.start(
            args.program,
            args.interpreterPath || 'mType.exe',
            args.stopOnEntry || false,
            args.cwd || path.dirname(args.program),
            args.args  // Pass additional args (like --bytecode) to the interpreter
        );

        this.sendResponse(response);
    }

    /**
     * Set breakpoints request
     */
    protected async setBreakPointsRequest(
        response: DebugProtocol.SetBreakpointsResponse,
        args: DebugProtocol.SetBreakpointsArguments
    ): Promise<void> {

        const path = args.source.path as string;
        const sourceBreakpoints = args.breakpoints || [];

        // Clear all breakpoints for this file
        await this._runtime.clearBreakpoints(path);

        // Set new breakpoints with conditions and log messages
        const actualBreakpoints = await Promise.all(
            sourceBreakpoints.map(async sourceBp => {
                const bp = await this._runtime.setBreakpoint(
                    path,
                    sourceBp.line,
                    sourceBp.condition,
                    sourceBp.logMessage
                );
                return {
                    verified: bp.verified,
                    line: bp.line,
                    id: bp.id
                } as DebugProtocol.Breakpoint;
            })
        );

        // Send response with actual breakpoints
        response.body = {
            breakpoints: actualBreakpoints
        };
        this.sendResponse(response);
    }

    /**
     * Set exception breakpoints request
     */
    protected async setExceptionBreakpointsRequest(
        response: DebugProtocol.SetExceptionBreakpointsResponse,
        args: DebugProtocol.SetExceptionBreakpointsArguments
    ): Promise<void> {

        const filters = args.filters || [];
        await this._runtime.setExceptionBreakpoints(filters);

        this.sendResponse(response);
    }

    /**
     * Threads request - return available threads
     */
    protected threadsRequest(response: DebugProtocol.ThreadsResponse): void {
        // mType is single-threaded
        response.body = {
            threads: [
                new Thread(MTypeDebugSession.THREAD_ID, "Main Thread")
            ]
        };
        this.sendResponse(response);
    }

    /**
     * Stack trace request - return call stack
     */
    protected stackTraceRequest(
        response: DebugProtocol.StackTraceResponse,
        args: DebugProtocol.StackTraceArguments
    ): void {

        const startFrame = typeof args.startFrame === 'number' ? args.startFrame : 0;
        const maxLevels = typeof args.levels === 'number' ? args.levels : 1000;

        const stack = this._runtime.getStack(startFrame, maxLevels);

        response.body = {
            stackFrames: stack.frames.map(f => {
                const sf = new StackFrame(
                    f.index,
                    f.name,
                    new Source(path.basename(f.file), f.file),
                    f.line,
                    f.column
                );
                return sf;
            }),
            totalFrames: stack.count
        };
        this.sendResponse(response);
    }

    /**
     * Scopes request - return variable scopes for a stack frame
     */
    protected scopesRequest(
        response: DebugProtocol.ScopesResponse,
        args: DebugProtocol.ScopesArguments
    ): void {
        // Use fixed scope IDs (1, 2) that won't collide with interpreter refIds (starting at 1000)
        const scopes: Scope[] = [
            new Scope("Local", MTypeDebugSession.LOCAL_SCOPE_ID, false),
            new Scope("Global", MTypeDebugSession.GLOBAL_SCOPE_ID, true)
        ];

        response.body = {
            scopes: scopes
        };
        this.sendResponse(response);
    }

    /**
     * Variables request - return variables for a scope
     */
    protected async variablesRequest(
        response: DebugProtocol.VariablesResponse,
        args: DebugProtocol.VariablesArguments
    ): Promise<void> {
        console.log(`[DEBUG TS] variablesRequest: variablesReference=${args.variablesReference}`);

        let runtimeVars: any[];

        // Check if this is a scope request (using our fixed scope IDs)
        if (args.variablesReference === MTypeDebugSession.LOCAL_SCOPE_ID) {
            console.log('[DEBUG TS] Requesting LOCAL variables');
            // Local scope
            runtimeVars = await this._runtime.getVariables("local");
            console.log(`[DEBUG TS] Got ${runtimeVars.length} local variables`);
        } else if (args.variablesReference === MTypeDebugSession.GLOBAL_SCOPE_ID) {
            console.log('[DEBUG TS] Requesting GLOBAL variables');
            // Global scope
            runtimeVars = await this._runtime.getVariables("global");
            console.log(`[DEBUG TS] Got ${runtimeVars.length} global variables:`, runtimeVars.map(v => v.name));
        } else {
            console.log(`[DEBUG TS] Requesting variable children for refId=${args.variablesReference}`);
            // This is an expandable variable request (refId from interpreter, >= 1000)
            runtimeVars = await this._runtime.getVariableChildren(args.variablesReference);
            console.log(`[DEBUG TS] Got ${runtimeVars.length} children`);
        }

        // Convert to DAP format
        const variables: DebugProtocol.Variable[] = runtimeVars.map(v => ({
            name: v.name,
            value: v.value,
            type: v.type,
            variablesReference: v.refId
        }));

        console.log(`[DEBUG TS] Sending ${variables.length} variables to VS Code`);
        response.body = {
            variables: variables
        };
        this.sendResponse(response);
    }

    /**
     * Continue request - resume execution
     */
    protected continueRequest(
        response: DebugProtocol.ContinueResponse,
        args: DebugProtocol.ContinueArguments
    ): void {

        this._runtime.continue();
        this.sendResponse(response);
    }

    /**
     * Next request - step over
     */
    protected nextRequest(
        response: DebugProtocol.NextResponse,
        args: DebugProtocol.NextArguments
    ): void {

        this._runtime.stepOver();
        this.sendResponse(response);
    }

    /**
     * Step in request - step into function
     */
    protected stepInRequest(
        response: DebugProtocol.StepInResponse,
        args: DebugProtocol.StepInArguments
    ): void {

        this._runtime.stepInto();
        this.sendResponse(response);
    }

    /**
     * Step out request - step out of function
     */
    protected stepOutRequest(
        response: DebugProtocol.StepOutResponse,
        args: DebugProtocol.StepOutArguments
    ): void {

        this._runtime.stepOut();
        this.sendResponse(response);
    }

    /**
     * Evaluate request - evaluate an expression
     */
    protected async evaluateRequest(
        response: DebugProtocol.EvaluateResponse,
        args: DebugProtocol.EvaluateArguments
    ): Promise<void> {

        try {
            const result = await this._runtime.evaluate(args.expression, args.frameId || 0);

            response.body = {
                result: result.value,
                type: result.type,
                variablesReference: result.refId || 0
            };
            this.sendResponse(response);
        } catch (error) {
            response.body = {
                result: `Error: ${error}`,
                variablesReference: 0
            };
            this.sendResponse(response);
        }
    }

    /**
     * Disconnect request - end debug session
     */
    protected disconnectRequest(
        response: DebugProtocol.DisconnectResponse,
        args: DebugProtocol.DisconnectArguments
    ): void {

        this._runtime.stop();
        this.sendResponse(response);
    }
}
