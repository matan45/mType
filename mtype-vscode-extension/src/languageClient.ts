import * as path from 'path';
import * as fs from 'fs';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;
let outputChannel: vscode.OutputChannel | undefined;

function languageServerExecutableName(): string {
    return process.platform === 'win32' ? 'mtype-language-server.exe' : 'mtype-language-server';
}

function isExistingFile(filePath: string): boolean {
    try {
        return fs.statSync(filePath).isFile();
    } catch {
        return false;
    }
}

function pathEntries(): string[] {
    const pathValue = process.env.PATH || '';
    return pathValue.split(path.delimiter).filter((entry) => entry.length > 0);
}

function executableCandidates(directory: string, executable: string): string[] {
    const candidate = path.join(directory, executable);
    if (process.platform !== 'win32' || path.extname(executable)) {
        return [candidate];
    }

    const extensions = (process.env.PATHEXT || '.EXE;.CMD;.BAT')
        .split(';')
        .filter((ext) => ext.length > 0);
    return extensions.map((ext) => candidate + ext.toLowerCase());
}

function findOnPath(executable: string): string | undefined {
    for (const entry of pathEntries()) {
        for (const candidate of executableCandidates(entry, executable)) {
            if (isExistingFile(candidate)) {
                return candidate;
            }
        }
    }
    return undefined;
}

function formatError(error: unknown): string {
    return error instanceof Error ? error.message : String(error);
}

function resolveLanguageServerPath(
    context: vscode.ExtensionContext,
    channel: vscode.OutputChannel
): string | undefined {
    const config = vscode.workspace.getConfiguration('mType');
    const configuredPath = config.get<string>('languageServer.path', '').trim();
    const executable = languageServerExecutableName();

    if (configuredPath) {
        const resolved = path.isAbsolute(configuredPath)
            ? configuredPath
            : path.resolve(configuredPath);
        if (isExistingFile(resolved)) {
            channel.appendLine(`[mType LSP] Using configured language server: ${resolved}`);
            return resolved;
        }

        const message = `Configured mType Language Server path does not exist: ${resolved}. Update mType.languageServer.path or install mtype-language-server 1.0.0.`;
        channel.appendLine(`[mType LSP] ERROR: ${message}`);
        vscode.window.showErrorMessage(message);
        return undefined;
    }

    const bundledCandidates = [
        context.asAbsolutePath(path.join('bin', `${process.platform}-${process.arch}`, executable)),
        context.asAbsolutePath(path.join('bin', executable)),
        context.asAbsolutePath(path.join('server', executable)),
        context.asAbsolutePath(executable)
    ];

    for (const candidate of bundledCandidates) {
        if (isExistingFile(candidate)) {
            channel.appendLine(`[mType LSP] Using bundled language server: ${candidate}`);
            return candidate;
        }
    }

    const pathMatch = findOnPath(executable);
    if (pathMatch) {
        channel.appendLine(`[mType LSP] Using language server from PATH: ${pathMatch}`);
        return pathMatch;
    }

    const message = `mType Language Server binary not found. Set mType.languageServer.path, install ${executable} on PATH, or install a release bundle that includes the binary.`;
    channel.appendLine(`[mType LSP] ERROR: ${message}`);
    channel.appendLine('[mType LSP] Checked bundled paths:');
    for (const candidate of bundledCandidates) {
        channel.appendLine(`[mType LSP]   ${candidate}`);
    }
    vscode.window.showErrorMessage(message);
    return undefined;
}

export function activateLanguageServer(context: vscode.ExtensionContext): void {
    if (client) {
        return;
    }

    if (!outputChannel) {
        outputChannel = vscode.window.createOutputChannel('mType Language Server');
    }
    outputChannel.appendLine('[mType LSP] Initializing language server...');

    const serverExecutable = resolveLanguageServerPath(context, outputChannel);
    if (!serverExecutable) {
        return;
    }

    // Server options
    const serverOptions: ServerOptions = {
        command: serverExecutable,
        args: [],
        transport: TransportKind.stdio
    };

    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'mtype' }
        ],
        synchronize: {
            // MYT-309 — forward mtpkg.json events so the LSP can re-merge
            // mt_modules aliases when `mtpm` installs/removes a package,
            // without forcing a server restart.
            fileEvents: [
                vscode.workspace.createFileSystemWatcher('**/*.mt'),
                vscode.workspace.createFileSystemWatcher('**/mt_modules/**/mtpkg.json')
            ]
        },
        outputChannel
    };

    // Create the language client
    client = new LanguageClient(
        'mtypeLanguageServer',
        'mType Language Server',
        serverOptions,
        clientOptions
    );

    // Start the client and server
    client.start().then(() => {
        outputChannel?.appendLine('[mType LSP] Language server started successfully.');
        outputChannel?.appendLine('[mType LSP] Ready for requests.');
    }).catch((error) => {
        const message = formatError(error);
        outputChannel?.appendLine(`[mType LSP] ERROR: Failed to start: ${message}`);
        vscode.window.showErrorMessage(`Failed to start mType Language Server: ${message}`);
    });

    // Register command to restart server
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.restartLanguageServer', () => {
            if (client) {
                client.stop().then(() => {
                    client!.start();
                    outputChannel?.appendLine('[mType LSP] Language server restarted.');
                });
            }
        })
    );
}

export function deactivateLanguageServer(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
