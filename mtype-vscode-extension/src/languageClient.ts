import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

export function activateLanguageServer(context: vscode.ExtensionContext): void {
    const config = vscode.workspace.getConfiguration('mType');

    // Get language server executable path from settings
    let serverExecutable = config.get<string>('languageServer.path', '');

    // If not configured, use default path relative to the extension
    if (!serverExecutable) {
        // Default: look in languageserver/bin/Debug-windows-x86_64/ relative to extension
        // Extension is in mtype-vscode-extension/, server is in languageserver/
        // Both are siblings in the mType/ directory
        const mtypeRootDir = path.dirname(context.extensionPath);
        serverExecutable = path.join(
            mtypeRootDir,
            'languageserver',
            'bin',
            'Debug-windows-x86_64',
            'mtype-language-server.exe'
        );
    }

    // If not an absolute path, try to find it relative to workspace or in PATH
    if (!path.isAbsolute(serverExecutable)) {
        // Check if it's in workspace
        const workspaceFolders = vscode.workspace.workspaceFolders;
        if (workspaceFolders && workspaceFolders.length > 0) {
            const workspacePath = workspaceFolders[0].uri.fsPath;
            const localServer = path.join(workspacePath, 'bin', serverExecutable);
            if (require('fs').existsSync(localServer)) {
                serverExecutable = localServer;
            }
        }
    }

    // Verify the server exists
    if (!require('fs').existsSync(serverExecutable)) {
        const errorMsg = `mType Language Server not found at: ${serverExecutable}\n\nPlease build the language server or configure the path in settings.`;
        vscode.window.showErrorMessage(errorMsg);
        return;
    }

    // Create output channel for LSP communication
    const outputChannel = vscode.window.createOutputChannel('mType Language Server');
    outputChannel.appendLine('[mType LSP] Initializing language server...');

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
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.mt')
        },
        outputChannel: outputChannel
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
        outputChannel.appendLine('[mType LSP] Language server started successfully!');
        outputChannel.appendLine('[mType LSP] Ready for requests.');
        vscode.window.showInformationMessage('mType Language Server started successfully!');
    }).catch((error) => {
        outputChannel.appendLine(`[mType LSP] ERROR: Failed to start: ${error.message}`);
        vscode.window.showErrorMessage(`Failed to start mType Language Server: ${error.message}`);
    });

    // Register command to restart server
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.restartLanguageServer', () => {
            if (client) {
                client.stop().then(() => {
                    client!.start();
                    vscode.window.showInformationMessage('mType Language Server restarted');
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
