import * as vscode from 'vscode';
import { MTypeFormatter } from './formatter/MTypeFormatter';
import { MTypeCompletionProvider } from './completion/MTypeCompletionProvider';

export function activate(context: vscode.ExtensionContext) {
    vscode.window.showInformationMessage('mType extension activated!');

    // Register completion provider
    const completionProvider = new MTypeCompletionProvider();
    const completionDisposable = vscode.languages.registerCompletionItemProvider(
        'mtype',
        completionProvider,
        '.',   // Trigger on dot (for instance members)
        ':'    // Trigger on colon (for static members with ::)
    );

    // Register document formatter
    const formatter = new MTypeFormatter();
    const formatterDisposable = vscode.languages.registerDocumentFormattingEditProvider('mtype', formatter);

    // Register additional commands
    const disposables = [
        // Command to run mType file
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
        }),

        // Command to run mType tests
        vscode.commands.registerCommand('mtype.runTests', async () => {
            const config = vscode.workspace.getConfiguration('mTypeLanguageServer');
            const interpreterPath = config.get<string>('interpreterPath');

            if (!interpreterPath) {
                vscode.window.showErrorMessage('mType interpreter path not configured');
                return;
            }

            const terminal = vscode.window.createTerminal('mType Tests');
            terminal.sendText(`"${interpreterPath}" --tests`);
            terminal.show();
        }),

        // Command to format mType document
        vscode.commands.registerCommand('mtype.formatDocument', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                vscode.window.showErrorMessage('No active editor');
                return;
            }

            if (editor.document.languageId !== 'mtype') {
                vscode.window.showErrorMessage('Current file is not an mType file');
                return;
            }

            try {
                // Trigger formatting
                await vscode.commands.executeCommand('editor.action.formatDocument');
                vscode.window.showInformationMessage('Document formatted successfully');
            } catch (error) {
                vscode.window.showErrorMessage('Failed to format document: ' + error);
            }
        })
    ];

    context.subscriptions.push(completionDisposable, formatterDisposable, ...disposables);
    console.log('mType extension fully activated with completion provider');
}

export function deactivate(): void {
}