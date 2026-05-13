import * as fs from 'fs';
import * as path from 'path';
import * as vscode from 'vscode';
import { ModulesTreeProvider } from './ModulesTreeProvider';
import { resolveMtpmPath } from './mtpmLocator';
import { runMtpm } from './mtpmRunner';

function getWorkspaceRoot(): string | undefined {
    const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root) {
        void vscode.window.showErrorMessage('Open a folder before running mType package commands.');
        return undefined;
    }
    return root;
}

function listInstalledPackages(root: string): string[] {
    const modulesDir = path.join(root, 'mt_modules');
    let entries: fs.Dirent[];
    try {
        entries = fs.readdirSync(modulesDir, { withFileTypes: true });
    } catch {
        return [];
    }
    return entries
        .filter((e) => e.isDirectory() && e.name.startsWith('@'))
        .map((e) => e.name.slice(1))
        .sort();
}

async function runAndRefresh(
    exe: string,
    args: string[],
    root: string,
    channel: vscode.OutputChannel,
    treeProvider: ModulesTreeProvider
): Promise<void> {
    channel.show(true);
    try {
        await runMtpm(exe, args, root, channel);
    } catch {
        // runMtpm already logged + toasted; swallow so we still refresh
    }
    treeProvider.refresh();
}

export function registerPackageCommands(
    context: vscode.ExtensionContext,
    channel: vscode.OutputChannel,
    treeProvider: ModulesTreeProvider
): void {
    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.packages.install', async () => {
            const root = getWorkspaceRoot();
            if (!root) return;
            const exe = resolveMtpmPath(context, channel);
            if (!exe) return;
            await runAndRefresh(exe, ['install'], root, channel, treeProvider);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.packages.add', async () => {
            const root = getWorkspaceRoot();
            if (!root) return;
            const exe = resolveMtpmPath(context, channel);
            if (!exe) return;

            const pkg = await vscode.window.showInputBox({
                prompt: 'Package to add',
                placeHolder: 'name@version (e.g. mathlib@1.2.0)',
                validateInput: (value) => {
                    const trimmed = value.trim();
                    if (!trimmed) return 'Required';
                    if (!/^\S+@\S+$/.test(trimmed)) return 'Use name@version';
                    return null;
                }
            });
            if (!pkg) return;

            const source = await vscode.window.showInputBox({
                prompt: 'Optional source',
                placeHolder: 'github:user/repo (leave empty to use the registry)'
            });
            if (source === undefined) return;

            const args = ['add', pkg.trim()];
            const trimmedSource = source.trim();
            if (trimmedSource) {
                args.push('--source', trimmedSource);
            }
            await runAndRefresh(exe, args, root, channel, treeProvider);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.packages.remove', async () => {
            const root = getWorkspaceRoot();
            if (!root) return;
            const exe = resolveMtpmPath(context, channel);
            if (!exe) return;

            const installed = listInstalledPackages(root);
            if (installed.length === 0) {
                void vscode.window.showInformationMessage('No packages installed.');
                return;
            }
            const pick = await vscode.window.showQuickPick(installed, {
                placeHolder: 'Select package to remove'
            });
            if (!pick) return;

            await runAndRefresh(exe, ['remove', pick], root, channel, treeProvider);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('mtype.packages.list', async () => {
            const root = getWorkspaceRoot();
            if (!root) return;
            const exe = resolveMtpmPath(context, channel);
            if (!exe) return;
            await runAndRefresh(exe, ['list'], root, channel, treeProvider);
        })
    );
}
