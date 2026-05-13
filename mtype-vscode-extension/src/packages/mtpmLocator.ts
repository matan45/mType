import * as path from 'path';
import * as fs from 'fs';
import * as vscode from 'vscode';

function mtpmExecutableName(): string {
    return process.platform === 'win32' ? 'mtpm.exe' : 'mtpm';
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

function showSettingsErrorToast(message: string): void {
    void vscode.window.showErrorMessage(message, 'Open Settings').then((selection) => {
        if (selection === 'Open Settings') {
            void vscode.commands.executeCommand(
                'workbench.action.openSettings',
                'mType.packageManager.path'
            );
        }
    });
}

export function resolveMtpmPath(
    context: vscode.ExtensionContext,
    channel: vscode.OutputChannel
): string | undefined {
    const config = vscode.workspace.getConfiguration('mType');
    const configuredPath = config.get<string>('packageManager.path', '').trim();
    const executable = mtpmExecutableName();

    if (configuredPath) {
        let resolved: string;
        if (path.isAbsolute(configuredPath)) {
            resolved = configuredPath;
        } else {
            const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
            if (!workspaceRoot) {
                const message = `Configured mType.packageManager.path is relative (${configuredPath}) but no workspace folder is open. Use an absolute path or open a folder.`;
                channel.appendLine(`[mType PM] ERROR: ${message}`);
                showSettingsErrorToast(message);
                return undefined;
            }
            resolved = path.resolve(workspaceRoot, configuredPath);
        }

        if (isExistingFile(resolved)) {
            channel.appendLine(`[mType PM] Using configured mtpm: ${resolved}`);
            return resolved;
        }

        const message = `Configured mtpm path does not exist: ${resolved}. Update mType.packageManager.path.`;
        channel.appendLine(`[mType PM] ERROR: ${message}`);
        showSettingsErrorToast(message);
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
            channel.appendLine(`[mType PM] Using bundled mtpm: ${candidate}`);
            return candidate;
        }
    }

    const pathMatch = findOnPath(executable);
    if (pathMatch) {
        channel.appendLine(`[mType PM] Using mtpm from PATH: ${pathMatch}`);
        return pathMatch;
    }

    const message = `mtpm binary not found. Set mType.packageManager.path, install ${executable} on PATH, or install a release bundle that includes the binary.`;
    channel.appendLine(`[mType PM] ERROR: ${message}`);
    channel.appendLine('[mType PM] Checked bundled paths:');
    for (const candidate of bundledCandidates) {
        channel.appendLine(`[mType PM]   ${candidate}`);
    }
    showSettingsErrorToast(message);
    return undefined;
}
