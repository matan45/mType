import * as vscode from 'vscode';
import { MTypeDebugSession } from './MTypeDebugSession';

export class MTypeDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(
        _session: vscode.DebugSession,
        _executable: vscode.DebugAdapterExecutable | undefined
    ): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        return new vscode.DebugAdapterInlineImplementation(new MTypeDebugSession());
    }
}

export class MTypeDebugConfigurationProvider implements vscode.DebugConfigurationProvider {
    resolveDebugConfiguration(
        folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        _token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        // If no launch.json or empty config, provide defaults
        if (!config.type && !config.request && !config.name) {
            const editor = vscode.window.activeTextEditor;
            if (editor && editor.document.languageId === 'mtype') {
                config.type = 'mtype';
                config.name = 'Debug mType File';
                config.request = 'launch';
                config.program = '${file}';
                config.stopOnEntry = true;
            }
        }

        // Attach connects to an already-running host: no interpreter/program needed.
        if (config.request === 'attach') {
            config.host = config.host || 'localhost';
            config.port = config.port || 5005;
            return config;
        }

        // Resolve interpreter path from settings if not specified
        if (!config.interpreterPath) {
            const settings = vscode.workspace.getConfiguration('mTypeLanguageServer');
            config.interpreterPath = settings.get<string>('interpreterPath') || '';
        }

        if (!config.interpreterPath) {
            vscode.window.showErrorMessage(
                'mType interpreter path not configured. Set mTypeLanguageServer.interpreterPath in settings.'
            );
            return undefined;
        }

        if (!config.program) {
            vscode.window.showErrorMessage('No program specified to debug.');
            return undefined;
        }

        return config;
    }
}
