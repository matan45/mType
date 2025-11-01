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
    // Check if LSP is enabled in settings
    const config = vscode.workspace.getConfiguration('mType');
    const useLSP = config.get<boolean>('languageServer.enable', false);

    if (!useLSP) {
        vscode.window.showInformationMessage('mType LSP is disabled. Enable it in settings to use Language Server features.');
        return;
    }

    // Get language server executable path from settings
    let serverExecutable = config.get<string>('languageServer.path', 'mtype-language-server');

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

    // Server options
    const serverOptions: ServerOptions = {
        command: serverExecutable,
        args: ['--stdio'],
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
        outputChannelName: 'mType Language Server'
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
        vscode.window.showInformationMessage('mType Language Server started successfully!');
    }).catch((error) => {
        vscode.window.showErrorMessage(`Failed to start mType Language Server: ${error.message}`);
        console.error('Language server error:', error);
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
