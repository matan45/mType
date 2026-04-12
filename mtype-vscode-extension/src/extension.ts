import * as vscode from 'vscode';
import { activateLanguageServer, deactivateLanguageServer } from './languageClient';

function registerCommonCommands(context: vscode.ExtensionContext): void {
    // Command to run mType file
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.run', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                vscode.window.showErrorMessage('No active editor');
                return;
            }

            if (editor.document.languageId !== 'mtype') {
                vscode.window.showErrorMessage('Current file is not an mType file');
                return;
            }

            const config = vscode.workspace.getConfiguration('mTypeLanguageServer');
            const interpreterPath = config.get<string>('interpreterPath');

            if (!interpreterPath) {
                const result = await vscode.window.showErrorMessage(
                    'mType interpreter path not configured. Would you like to set it now?',
                    'Set Path'
                );
                if (result === 'Set Path') {
                    vscode.commands.executeCommand('workbench.action.openSettings', 'mTypeLanguageServer.interpreterPath');
                }
                return;
            }

            const filePath = editor.document.uri.fsPath;
            const terminal = vscode.window.createTerminal('mType');
            terminal.sendText(`"${interpreterPath}" "${filePath}"`);
            terminal.show();
        })
    );

    // Command to find all references
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.findReferences', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor || editor.document.languageId !== 'mtype') {
                return;
            }
            await vscode.commands.executeCommand('editor.action.referenceSearch.trigger');
        })
    );

    // Command to format document
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.formatDocument', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor || editor.document.languageId !== 'mtype') {
                return;
            }
            await vscode.commands.executeCommand('editor.action.formatDocument');
        })
    );

    // Command to show references (converts JSON from LSP to VS Code types)
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.showReferences', async (uriString: string, position: any, locations: any[]) => {
            try {
                const uri = vscode.Uri.parse(uriString);
                const pos = new vscode.Position(position.line, position.character);
                const locs: vscode.Location[] = locations.map((loc: any) => {
                    const locUri = vscode.Uri.parse(loc.uri);
                    const range = new vscode.Range(
                        new vscode.Position(loc.range.start.line, loc.range.start.character),
                        new vscode.Position(loc.range.end.line, loc.range.end.character)
                    );
                    return new vscode.Location(locUri, range);
                });
                await vscode.commands.executeCommand('editor.action.showReferences', uri, pos, locs);
            } catch (error) {
                vscode.window.showErrorMessage('Failed to show references: ' + error);
            }
        })
    );
}

export async function activate(context: vscode.ExtensionContext) {
    registerCommonCommands(context);
    activateLanguageServer(context);
}

export function deactivate(): Thenable<void> | undefined {
    return deactivateLanguageServer();
}
