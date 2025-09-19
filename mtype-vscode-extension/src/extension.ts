import * as vscode from 'vscode';
import { MTypeFormatter } from './formatter/MTypeFormatter';
import { MTypeCompletionProvider } from './completion/MTypeCompletionProvider';
import { MTypeDefinitionProvider } from './definition/MTypeDefinitionProvider';
import { MTypeReferenceProvider } from './references/MTypeReferenceProvider';
import { MTypeImportResolver } from './imports/MTypeImportResolver';
import { MTypeImportCompletionProvider, MTypeImportedSymbolProvider } from './imports/MTypeImportCompletionProvider';
import { MTypeImportDiagnostics } from './imports/MTypeImportDiagnostics';

export function activate(context: vscode.ExtensionContext) {
    vscode.window.showInformationMessage('mType extension activated!');

    // Get workspace root for import resolution
    const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '';

    // Initialize import system
    const importResolver = new MTypeImportResolver(workspaceRoot);
    const importedSymbolProvider = new MTypeImportedSymbolProvider(importResolver);
    const importDiagnostics = new MTypeImportDiagnostics(importResolver);

    // Register import completion provider (for import statements)
    const importCompletionProvider = new MTypeImportCompletionProvider(importResolver);
    const importCompletionDisposable = vscode.languages.registerCompletionItemProvider(
        'mtype',
        importCompletionProvider,
        '"',   // Trigger on quote (for import paths)
        "'",   // Trigger on single quote
        ' '    // Trigger on space (for import keyword)
    );

    // Register main completion provider
    const completionProvider = new MTypeCompletionProvider();
    completionProvider.setImportedSymbolProvider(importedSymbolProvider);
    const completionDisposable = vscode.languages.registerCompletionItemProvider(
        'mtype',
        completionProvider,
        '.',   // Trigger on dot (for instance members)
        ':',   // Trigger on colon (for static members with ::)
        ' ',   // Trigger on space (for keywords)
        '<',   // Trigger on < (for generic types)
        '(',   // Trigger on ( (for parameters)
        ','    // Trigger on , (for parameter lists)
    );

    // Register document formatter
    const formatter = new MTypeFormatter();
    const formatterDisposable = vscode.languages.registerDocumentFormattingEditProvider('mtype', formatter);

    // Register definition provider
    const definitionProvider = new MTypeDefinitionProvider();
    definitionProvider.setImportResolver(importResolver);
    definitionProvider.setImportedSymbolProvider(importedSymbolProvider);
    const definitionDisposable = vscode.languages.registerDefinitionProvider('mtype', definitionProvider);

    // Register reference provider
    const referenceProvider = new MTypeReferenceProvider();
    referenceProvider.setImportResolver(importResolver);
    referenceProvider.setImportedSymbolProvider(importedSymbolProvider);
    const referenceDisposable = vscode.languages.registerReferenceProvider('mtype', referenceProvider);

    // Set up document change listeners for import analysis
    const documentChangeDisposable = vscode.workspace.onDidChangeTextDocument(async (event) => {
        if (event.document.languageId === 'mtype') {
            // Re-analyze imports when document changes
            await importedSymbolProvider.analyzeDocumentImports(event.document);
            // Update import diagnostics
            await importDiagnostics.updateDiagnostics(event.document);
            // Clear import cache for this file
            importResolver.clearCacheForFile(event.document.uri.fsPath);
        }
    });

    const documentOpenDisposable = vscode.workspace.onDidOpenTextDocument(async (document) => {
        if (document.languageId === 'mtype') {
            // Analyze imports when document opens
            await importedSymbolProvider.analyzeDocumentImports(document);
            // Update import diagnostics
            await importDiagnostics.updateDiagnostics(document);
        }
    });

    const documentCloseDisposable = vscode.workspace.onDidCloseTextDocument((document) => {
        if (document.languageId === 'mtype') {
            // Clear cache when document closes
            importedSymbolProvider.clearDocumentCache(document);
            importDiagnostics.clearDiagnostics(document);
        }
    });

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
        }),

        // Command to find all references
        vscode.commands.registerCommand('mtype.findReferences', async () => {
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
                // Trigger find references
                await vscode.commands.executeCommand('editor.action.referenceSearch.trigger');
            } catch (error) {
                vscode.window.showErrorMessage('Failed to find references: ' + error);
            }
        })
    ];

    context.subscriptions.push(
        completionDisposable,
        importCompletionDisposable,
        formatterDisposable,
        definitionDisposable,
        referenceDisposable,
        documentChangeDisposable,
        documentOpenDisposable,
        documentCloseDisposable,
        ...disposables
    );
}

export function deactivate(): void {
}