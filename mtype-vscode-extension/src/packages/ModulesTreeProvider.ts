import * as fs from 'fs';
import * as path from 'path';
import * as vscode from 'vscode';

export type ModuleNode =
    | { kind: 'package'; name: string; version: string; dir: string; sourceDir: string }
    | { kind: 'file'; uri: vscode.Uri }
    | { kind: 'info'; message: string };

interface PackageManifest {
    name?: string;
    version?: string;
    source?: string;
}

function readManifest(manifestPath: string): PackageManifest | undefined {
    try {
        const text = fs.readFileSync(manifestPath, 'utf8');
        return JSON.parse(text) as PackageManifest;
    } catch {
        return undefined;
    }
}

function walkMtFiles(dir: string, out: string[]): void {
    let entries: fs.Dirent[];
    try {
        entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch {
        return;
    }
    for (const entry of entries) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            walkMtFiles(full, out);
        } else if (entry.isFile() && entry.name.endsWith('.mt')) {
            out.push(full);
        }
    }
}

export class ModulesTreeProvider implements vscode.TreeDataProvider<ModuleNode> {
    private readonly _emitter = new vscode.EventEmitter<void>();
    readonly onDidChangeTreeData = this._emitter.event;

    refresh(): void {
        this._emitter.fire();
    }

    getTreeItem(node: ModuleNode): vscode.TreeItem {
        if (node.kind === 'package') {
            const item = new vscode.TreeItem(node.name, vscode.TreeItemCollapsibleState.Collapsed);
            item.description = node.version;
            item.iconPath = new vscode.ThemeIcon('package');
            item.contextValue = 'mtypePackage';
            item.tooltip = `${node.name}@${node.version}\n${node.dir}`;
            return item;
        }
        if (node.kind === 'file') {
            const item = new vscode.TreeItem(node.uri, vscode.TreeItemCollapsibleState.None);
            item.resourceUri = node.uri;
            item.command = {
                command: 'vscode.open',
                title: 'Open',
                arguments: [node.uri]
            };
            return item;
        }
        const item = new vscode.TreeItem(node.message, vscode.TreeItemCollapsibleState.None);
        item.iconPath = new vscode.ThemeIcon('info');
        return item;
    }

    getChildren(element?: ModuleNode): ModuleNode[] {
        if (!element) {
            return this.getRootChildren();
        }
        if (element.kind === 'package') {
            return this.getPackageChildren(element);
        }
        return [];
    }

    private getRootChildren(): ModuleNode[] {
        const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
        if (!root) {
            return [{ kind: 'info', message: 'Open a folder to manage packages' }];
        }
        const modulesDir = path.join(root, 'mt_modules');
        let entries: fs.Dirent[];
        try {
            entries = fs.readdirSync(modulesDir, { withFileTypes: true });
        } catch {
            return [{ kind: 'info', message: 'No packages installed' }];
        }
        const packages: ModuleNode[] = [];
        for (const entry of entries) {
            if (!entry.isDirectory() || !entry.name.startsWith('@')) {
                continue;
            }
            const dir = path.join(modulesDir, entry.name);
            const manifest = readManifest(path.join(dir, 'mtpkg.json'));
            const bareName = entry.name.slice(1);
            packages.push({
                kind: 'package',
                name: manifest?.name ?? bareName,
                version: manifest?.version ?? '(unknown)',
                dir,
                sourceDir: manifest?.source?.trim() || 'src'
            });
        }
        if (packages.length === 0) {
            return [{ kind: 'info', message: 'No packages installed' }];
        }
        packages.sort((a, b) => (a.kind === 'package' && b.kind === 'package'
            ? a.name.localeCompare(b.name)
            : 0));
        return packages;
    }

    private getPackageChildren(node: Extract<ModuleNode, { kind: 'package' }>): ModuleNode[] {
        const srcRoot = path.join(node.dir, node.sourceDir);
        const files: string[] = [];
        walkMtFiles(srcRoot, files);
        if (files.length === 0) {
            return [{ kind: 'info', message: '(no .mt files)' }];
        }
        files.sort();
        return files.map((file) => ({ kind: 'file', uri: vscode.Uri.file(file) }));
    }
}
