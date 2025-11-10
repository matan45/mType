import * as vscode from 'vscode';
import { MTypeFormatter } from './formatter/MTypeFormatter';
import { MTypeCompletionProvider } from './completion/MTypeCompletionProvider';
import { MTypeDefinitionProvider } from './definition/MTypeDefinitionProvider';
import { MTypeReferenceProvider } from './references/MTypeReferenceProvider';
import { MTypeImportResolver } from './imports/MTypeImportResolver';
import { MTypeImportCompletionProvider, MTypeImportedSymbolProvider } from './imports/MTypeImportCompletionProvider';
import { MTypeImportDiagnostics } from './imports/MTypeImportDiagnostics';
import { MTypeDebugConfigurationProvider, MTypeDebugAdapterDescriptorFactory } from './debug/MTypeDebugAdapter';
import { MTypeSignatureHelpProvider } from './signature/MTypeSignatureHelpProvider';
import { MTypeCodeActionsProvider } from './codeActions/MTypeCodeActionsProvider';
import { MTypeCodeLensProvider } from './codeLens/MTypeCodeLensProvider';
import { MTypeSemanticTokensProvider, legend } from './semanticTokens/MTypeSemanticTokensProvider';
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
    );

    // Command to format document
    context.subscriptions.push(
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
                // Trigger format document
                await vscode.commands.executeCommand('editor.action.formatDocument');
            } catch (error) {
                vscode.window.showErrorMessage('Failed to format document: ' + error);
            }
        })
    );

    // Command to show references (converts JSON from LSP to VS Code types)
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.showReferences', async (uriString: string, position: any, locations: any[]) => {
            try {
                console.log('[mtype.showReferences] Called with:', { uriString, position, locations });

                // Convert URI string to vscode.Uri
                const uri = vscode.Uri.parse(uriString);
                console.log('[mtype.showReferences] Parsed URI:', uri.toString());

                // Convert position object to vscode.Position
                const pos = new vscode.Position(position.line, position.character);
                console.log('[mtype.showReferences] Created Position:', pos);

                // Convert location objects to vscode.Location[]
                const locs: vscode.Location[] = locations.map((loc: any) => {
                    const locUri = vscode.Uri.parse(loc.uri);
                    const range = new vscode.Range(
                        new vscode.Position(loc.range.start.line, loc.range.start.character),
                        new vscode.Position(loc.range.end.line, loc.range.end.character)
                    );
                    return new vscode.Location(locUri, range);
                });
                console.log('[mtype.showReferences] Created', locs.length, 'locations');

                // Call VS Code's built-in showReferences command with proper types
                await vscode.commands.executeCommand('editor.action.showReferences', uri, pos, locs);
                console.log('[mtype.showReferences] Successfully showed references');
            } catch (error) {
                console.error('[mtype.showReferences] Error:', error);
                vscode.window.showErrorMessage('Failed to show references: ' + error);
            }
        })
    );
}

export function activate(context: vscode.ExtensionContext) {
    // Register commands that should always be available regardless of LSP mode
    registerCommonCommands(context);

    // Check if LSP is enabled
    const config = vscode.workspace.getConfiguration('mType');
    const useLSP = config.get<boolean>('languageServer.enable', false);

    if (useLSP) {
        // Use Language Server Protocol
        vscode.window.showInformationMessage('mType extension activated with LSP mode!');
        activateLanguageServer(context);
        return; // Skip built-in providers when using LSP
    }

    // Use built-in providers (original mode)
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

    // Register signature help provider (parameter hints)
    const signatureHelpProvider = new MTypeSignatureHelpProvider();
    signatureHelpProvider.setImportedSymbolProvider(importedSymbolProvider);
    const signatureHelpDisposable = vscode.languages.registerSignatureHelpProvider(
        'mtype',
        signatureHelpProvider,
        '(',   // Trigger on opening parenthesis
        ','    // Trigger on comma (next parameter)
    );

    // Register code actions provider (quick fixes)
    const codeActionsProvider = new MTypeCodeActionsProvider(importResolver);
    const codeActionsDisposable = vscode.languages.registerCodeActionsProvider(
        'mtype',
        codeActionsProvider,
        {
            providedCodeActionKinds: [
                vscode.CodeActionKind.QuickFix,
                vscode.CodeActionKind.SourceOrganizeImports
            ]
        }
    );

    // Register code lens provider (reference counts)
    const codeLensProvider = new MTypeCodeLensProvider(referenceProvider);
    const codeLensDisposable = vscode.languages.registerCodeLensProvider(
        'mtype',
        codeLensProvider
    );

    // Register semantic tokens provider (advanced syntax highlighting)
    const semanticTokensProvider = new MTypeSemanticTokensProvider();
    const semanticTokensDisposable = vscode.languages.registerDocumentSemanticTokensProvider(
        'mtype',
        semanticTokensProvider,
        legend
    );

    // Set up document change listeners for import analysis
    const documentChangeDisposable = vscode.workspace.onDidChangeTextDocument(async (event) => {
        if (event.document.languageId === 'mtype') {
            // Clear import cache for this file first
            importResolver.clearCacheForFile(event.document.uri.fsPath);
            // Re-analyze imports when document changes
            await importedSymbolProvider.analyzeDocumentImports(event.document);
            // Update import diagnostics
            await importDiagnostics.updateDiagnostics(event.document);
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
            // Don't clear diagnostics - they should persist until the file is fixed or deleted
        }
    });
	
	// Register document formatter
    const formatter = new MTypeFormatter();
    const formatterDisposable = vscode.languages.registerDocumentFormattingEditProvider('mtype', formatter);

    // TODO: Debug adapter temporarily disabled to focus on LSP testing
    // Register debug configuration provider
    // const debugConfigProvider = new MTypeDebugConfigurationProvider();
    // const debugConfigDisposable = vscode.debug.registerDebugConfigurationProvider('mtype', debugConfigProvider);

    // Register debug adapter descriptor factory
    // const debugAdapterFactory = new MTypeDebugAdapterDescriptorFactory();
    // const debugAdapterDisposable = vscode.debug.registerDebugAdapterDescriptorFactory('mtype', debugAdapterFactory);

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

    // Initial scan of all .mt files in workspace for diagnostics
    vscode.workspace.findFiles('**/*.mt', '**/node_modules/**').then(async (files) => {
        for (const fileUri of files) {
            try {
                const document = await vscode.workspace.openTextDocument(fileUri);
                await importDiagnostics.updateDiagnostics(document);
            } catch (error) {
                // Skip files that can't be opened
            }
        }
    });

    context.subscriptions.push(
        completionDisposable,
        importCompletionDisposable,
        formatterDisposable,
        definitionDisposable,
        referenceDisposable,
        signatureHelpDisposable,
        codeActionsDisposable,
        codeLensDisposable,
        semanticTokensDisposable,
        // debugConfigDisposable,  // Temporarily disabled
        // debugAdapterDisposable,  // Temporarily disabled
        documentChangeDisposable,
        documentOpenDisposable,
        documentCloseDisposable,
        ...disposables
    );
}

export function deactivate(): Thenable<void> | undefined {
    // Deactivate language server if it's running
    return deactivateLanguageServer();
}