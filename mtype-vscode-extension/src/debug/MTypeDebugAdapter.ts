import * as vscode from 'vscode';
import { WorkspaceFolder, DebugConfiguration, ProviderResult, CancellationToken } from 'vscode';
import { MTypeDebugSession } from './MTypeDebugSession';
import * as Net from 'net';

/**
 * Debug configuration provider for mType
 *
 * This class is responsible for resolving debug configurations
 * and providing initial configurations for launch.json
 */
export class MTypeDebugConfigurationProvider implements vscode.DebugConfigurationProvider {

    /**
     * Resolve a debug configuration before launching
     */
    resolveDebugConfiguration(
        folder: WorkspaceFolder | undefined,
        config: DebugConfiguration,
        token?: CancellationToken
    ): ProviderResult<DebugConfiguration> {

        // If launch.json is missing or empty
        if (!config.type && !config.request && !config.name) {
            const editor = vscode.window.activeTextEditor;
            if (editor && editor.document.languageId === 'mtype') {
                config.type = 'mtype';
                config.name = 'Launch';
                config.request = 'launch';
                config.program = '${file}';
                config.stopOnEntry = true;
            }
        }

        // Ensure we have a program to debug
        if (!config.program) {
            return vscode.window.showInformationMessage('Cannot find a program to debug').then(_ => {
                return undefined;
            });
        }

        // Get interpreter path from configuration
        if (!config.interpreterPath) {
            const interpreterPath = vscode.workspace.getConfiguration('mTypeLanguageServer').get<string>('interpreterPath');
            if (interpreterPath) {
                config.interpreterPath = interpreterPath;
            } else {
                config.interpreterPath = 'mType.exe'; // Default
            }
        }

        return config;
    }
}

/**
 * Debug adapter descriptor factory for mType
 *
 * This creates the actual debug adapter session
 */
export class MTypeDebugAdapterDescriptorFactory implements vscode.DebugAdapterDescriptorFactory {

    createDebugAdapterDescriptor(
        session: vscode.DebugSession,
        executable: vscode.DebugAdapterExecutable | undefined
    ): ProviderResult<vscode.DebugAdapterDescriptor> {

        // Run the debug adapter inline (in the extension host process)
        return new vscode.DebugAdapterInlineImplementation(new MTypeDebugSession());
    }
}
