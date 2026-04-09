import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

export interface MTypeProjectConfig {
    projectRoot: string;
    searchPaths: string[];
    aliases: Map<string, string>;
}

/**
 * Parse a .mtproj file and extract import configuration.
 * Uses regex-based parsing since the format is simple and well-defined.
 */
function parseMtproj(filePath: string): MTypeProjectConfig {
    const projectRoot = path.dirname(filePath);
    const content = fs.readFileSync(filePath, 'utf8');

    const searchPaths: string[] = [];
    const aliases = new Map<string, string>();

    // Extract <SearchPath> entries
    const searchPathRegex = /<SearchPath>(.*?)<\/SearchPath>/g;
    let match: RegExpExecArray | null;
    while ((match = searchPathRegex.exec(content)) !== null) {
        const relativePath = match[1].trim();
        if (relativePath) {
            searchPaths.push(path.resolve(projectRoot, relativePath));
        }
    }

    // Extract <Alias Name="@name" Path="path" /> entries
    const aliasRegex = /<Alias\s+Name="([^"]+)"\s+Path="([^"]+)"\s*\/>/g;
    while ((match = aliasRegex.exec(content)) !== null) {
        const aliasName = match[1].trim();
        const aliasPath = match[2].trim();
        if (aliasName && aliasPath) {
            aliases.set(aliasName, path.resolve(projectRoot, aliasPath));
        }
    }

    return { projectRoot, searchPaths, aliases };
}

/**
 * Find and parse the .mtproj file in the workspace.
 * Returns null if no .mtproj file is found.
 */
export async function loadProjectConfig(workspaceRoot: string): Promise<MTypeProjectConfig | null> {
    try {
        const files = await vscode.workspace.findFiles('**/*.mtproj', '**/node_modules/**', 5);

        if (files.length === 0) {
            return null;
        }

        // Prefer the .mtproj closest to workspace root (shortest path)
        const sorted = files.sort((a, b) => a.fsPath.length - b.fsPath.length);
        const chosen = sorted[0];

        if (files.length > 1) {
            console.log(`Multiple .mtproj files found, using: ${chosen.fsPath}`);
        }

        return parseMtproj(chosen.fsPath);
    } catch (error) {
        console.error('Error loading .mtproj config:', error);
        return null;
    }
}
