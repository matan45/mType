import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { MTypeProjectConfig } from '../project/MTypeProjectConfig';

export interface ImportInfo {
    importPath: string;
    resolvedPath: string;
    isValid: boolean;
    exportedSymbols: ExportedSymbol[];
}

export interface ExportedSymbol {
    name: string;
    type: 'function' | 'class' | 'variable';
    signature?: string;
    returnType?: string;
    parameters?: ParameterInfo[];
}

export interface ParameterInfo {
    name: string;
    type: string;
}

export class MTypeImportResolver {
    private importCache: Map<string, ImportInfo> = new Map();
    private workspaceRoot: string;
    private searchPaths: string[] = [];
    private aliases: Map<string, string> = new Map();

    constructor(workspaceRoot: string, projectConfig?: MTypeProjectConfig | null) {
        this.workspaceRoot = workspaceRoot;
        if (projectConfig) {
            this.searchPaths = projectConfig.searchPaths;
            this.aliases = projectConfig.aliases;
        }
    }

    /**
     * Update project configuration and clear cache
     */
    setProjectConfig(config: MTypeProjectConfig | null): void {
        this.searchPaths = config?.searchPaths ?? [];
        this.aliases = config?.aliases ?? new Map();
        this.clearCache();
    }

    /**
     * Expand path aliases (e.g., @core/utils.mt -> /abs/path/src/core/utils.mt)
     */
    private expandAlias(importPath: string): string {
        if (!importPath.startsWith('@') || this.aliases.size === 0) {
            return importPath;
        }

        const slashIndex = importPath.indexOf('/');
        const aliasName = slashIndex !== -1 ? importPath.substring(0, slashIndex) : importPath;
        const aliasTarget = this.aliases.get(aliasName);

        if (!aliasTarget) {
            return importPath;
        }

        const remainder = slashIndex !== -1 ? importPath.substring(slashIndex) : '';
        return aliasTarget + remainder;
    }

    /**
     * Try resolving a path against configured search paths.
     * Matches ImportManager.cpp behavior: tries each searchPath directory.
     */
    private resolveViaSearchPaths(filePath: string): string | null {
        for (const searchPath of this.searchPaths) {
            const candidate = path.join(searchPath, filePath);
            if (fs.existsSync(candidate)) {
                return candidate;
            }
        }
        return null;
    }

    /**
     * Resolve an import path to an absolute file path.
     * Resolution order matches the compiler's ImportManager::resolvePath:
     * 1. Alias expansion (@prefix)
     * 2. Relative imports (./ or ../)
     * 3. Absolute imports (/)
     * 4. Current file directory
     * 5. Workspace root (base directory)
     * 6. Search paths from .mtproj
     * 7. Recursive fallback search
     */
    resolveImportPath(importPath: string, currentFilePath: string): string | null {
        // Remove quotes from import path
        let cleanPath = importPath.replace(/['"]/g, '');
        const currentDir = path.dirname(currentFilePath);

        console.log('Resolving import path:', cleanPath, 'from:', currentFilePath);

        // 1. Alias expansion
        cleanPath = this.expandAlias(cleanPath);

        // 2. For relative imports starting with ./ or ../
        if (cleanPath.startsWith('./') || cleanPath.startsWith('../')) {
            const resolvedPath = path.resolve(currentDir, cleanPath);
            if (fs.existsSync(resolvedPath)) {
                return resolvedPath;
            }
            return null;
        }

        // 3. For absolute imports starting with /
        if (cleanPath.startsWith('/')) {
            const resolvedPath = path.join(this.workspaceRoot, cleanPath.substring(1));
            if (fs.existsSync(resolvedPath)) {
                return resolvedPath;
            }
            return null;
        }

        // 4. Try relative to current file's directory
        const currentRelativePath = path.join(currentDir, cleanPath);
        if (fs.existsSync(currentRelativePath)) {
            return currentRelativePath;
        }

        // 5. Try relative to workspace root (base directory)
        const workspaceRelativePath = path.join(this.workspaceRoot, cleanPath);
        if (fs.existsSync(workspaceRelativePath)) {
            return workspaceRelativePath;
        }

        // 6. Try each configured search path from .mtproj
        const searchPathResult = this.resolveViaSearchPaths(cleanPath);
        if (searchPathResult) {
            return searchPathResult;
        }

        // 7. Recursive fallback search (only for simple filenames)
        if (!cleanPath.includes('/')) {
            const foundPath = this.findFileRecursively(this.workspaceRoot, cleanPath);
            if (foundPath) {
                return foundPath;
            }
        }

        console.log('Could not resolve import path:', cleanPath);
        return null;
    }

    /**
     * Recursively search for a file in a directory tree
     */
    private findFileRecursively(searchDir: string, fileName: string): string | null {
        try {
            const entries = fs.readdirSync(searchDir, { withFileTypes: true });

            // First check if file exists in current directory
            for (const entry of entries) {
                if (entry.isFile() && entry.name === fileName) {
                    return path.join(searchDir, entry.name);
                }
            }

            // Then search subdirectories
            for (const entry of entries) {
                if (entry.isDirectory()) {
                    // Skip common non-source directories
                    if (['node_modules', '.git', 'bin', 'obj', '.vs', '.vscode'].includes(entry.name)) {
                        continue;
                    }

                    const subDirPath = path.join(searchDir, entry.name);
                    const result = this.findFileRecursively(subDirPath, fileName);
                    if (result) {
                        return result;
                    }
                }
            }
        } catch (error) {
            // Skip directories we can't read
        }

        return null;
    }

    /**
     * Get import information for a given import statement
     */
    async getImportInfo(importPath: string, currentFilePath: string): Promise<ImportInfo | null> {
        const cacheKey = `${currentFilePath}:${importPath}`;

        if (this.importCache.has(cacheKey)) {
            return this.importCache.get(cacheKey)!;
        }

        const resolvedPath = this.resolveImportPath(importPath, currentFilePath);

        if (!resolvedPath || !fs.existsSync(resolvedPath)) {
            const importInfo: ImportInfo = {
                importPath,
                resolvedPath: resolvedPath || '',
                isValid: false,
                exportedSymbols: []
            };
            this.importCache.set(cacheKey, importInfo);
            return importInfo;
        }

        try {
            const fileContent = fs.readFileSync(resolvedPath, 'utf8');
            const exportedSymbols = this.parseExportedSymbols(fileContent);

            const importInfo: ImportInfo = {
                importPath,
                resolvedPath,
                isValid: true,
                exportedSymbols
            };

            this.importCache.set(cacheKey, importInfo);
            return importInfo;
        } catch (error) {
            const importInfo: ImportInfo = {
                importPath,
                resolvedPath,
                isValid: false,
                exportedSymbols: []
            };
            this.importCache.set(cacheKey, importInfo);
            return importInfo;
        }
    }

    /**
     * Parse exported symbols from a mType file
     */
    private parseExportedSymbols(fileContent: string): ExportedSymbol[] {
        const symbols: ExportedSymbol[] = [];
        const lines = fileContent.split('\n');

        for (const line of lines) {
            // Parse function declarations
            const functionMatch = line.match(/^\s*function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)/);
            if (functionMatch) {
                const functionName = functionMatch[1];
                const parametersStr = functionMatch[2];
                const returnType = functionMatch[3];
                const parameters = this.parseParameters(parametersStr);

                symbols.push({
                    name: functionName,
                    type: 'function',
                    signature: `${functionName}(${parametersStr}): ${returnType}`,
                    returnType,
                    parameters
                });
            }

            // Parse class declarations
            const classMatch = line.match(/^\s*class\s+(\w+)/);
            if (classMatch) {
                const className = classMatch[1];
                symbols.push({
                    name: className,
                    type: 'class',
                    signature: `class ${className}`
                });
            }

            // Parse global variable declarations
            const varMatch = line.match(/^\s*(?:(final)\s+)?(\w+)\s+(\w+)\s*=/);
            if (varMatch && !line.trim().startsWith('//')) {
                const isFinal = !!varMatch[1];
                const varType = varMatch[2];
                const varName = varMatch[3];

                symbols.push({
                    name: varName,
                    type: 'variable',
                    signature: `${isFinal ? 'final ' : ''}${varType} ${varName}`
                });
            }
        }

        return symbols;
    }

    /**
     * Parse parameter string into ParameterInfo array
     */
    private parseParameters(parametersStr: string): ParameterInfo[] {
        if (!parametersStr.trim()) return [];

        return parametersStr.split(',').map(param => {
            const trimmed = param.trim();
            const match = trimmed.match(/(\w+)\s+(\w+)/);
            if (match) {
                return {
                    type: match[1],
                    name: match[2]
                };
            }
            return { type: 'unknown', name: trimmed };
        });
    }

    /**
     * Get all available .mt files in the workspace for import suggestions
     */
    async getAvailableImports(currentFilePath: string): Promise<string[]> {
        const imports: string[] = [];
        const currentDir = path.dirname(currentFilePath);

        console.log('MTypeImportResolver - Scanning for imports from:', currentFilePath);
        console.log('Current directory:', currentDir);
        console.log('Workspace root:', this.workspaceRoot);

        try {
            // 1. Get files in current directory (with ./ prefix)
            const currentDirFiles = fs.readdirSync(currentDir)
                .filter(file => file.endsWith('.mt') && file !== path.basename(currentFilePath))
                .map(file => `./${file}`);

            console.log('Files in current directory:', currentDirFiles);
            imports.push(...currentDirFiles);

            // 2. Get files in parent directory (with ../ prefix) - if we're not at workspace root
            const parentDir = path.dirname(currentDir);
            if (parentDir !== currentDir && parentDir.startsWith(this.workspaceRoot)) {
                try {
                    const parentDirFiles = fs.readdirSync(parentDir)
                        .filter(file => file.endsWith('.mt'))
                        .map(file => `../${file}`);

                    console.log('Files in parent directory:', parentDirFiles);
                    imports.push(...parentDirFiles);
                } catch (error) {
                    console.log('Cannot read parent directory:', error);
                }
            }

            // 3. Get files from configured search paths
            for (const searchPath of this.searchPaths) {
                if (fs.existsSync(searchPath)) {
                    const searchPathFiles = this.getAllMTypeFiles(searchPath, currentFilePath);
                    imports.push(...searchPathFiles);
                }
            }

            // 4. Get alias-prefixed imports
            for (const [aliasName, aliasPath] of this.aliases) {
                if (fs.existsSync(aliasPath)) {
                    const aliasFiles = this.getAllMTypeFiles(aliasPath, currentFilePath, aliasName + '/');
                    imports.push(...aliasFiles);
                }
            }

            // 5. Recursively get files in all subdirectories from workspace root
            const allFiles = this.getAllMTypeFiles(this.workspaceRoot, currentFilePath);
            console.log('All workspace files found:', allFiles.length);
            imports.push(...allFiles);

            // 6. Get files relative to current directory (subdirectories of current dir)
            const currentDirSubFiles = this.getAllMTypeFiles(currentDir, currentFilePath, './');
            console.log('Files in current directory subdirectories:', currentDirSubFiles);
            imports.push(...currentDirSubFiles);

        } catch (error) {
            console.error('Error scanning for import files:', error);
        }

        // Remove duplicates and sort
        const uniqueImports = [...new Set(imports)].sort();
        console.log('Final unique imports:', uniqueImports);
        return uniqueImports;
    }

    /**
     * Recursively get all .mt files in a directory and its subdirectories
     */
    private getAllMTypeFiles(searchDir: string, currentFilePath: string, prefix: string = ''): string[] {
        const files: string[] = [];
        const currentFileName = path.basename(currentFilePath);

        try {
            const entries = fs.readdirSync(searchDir, { withFileTypes: true });

            for (const entry of entries) {
                const fullPath = path.join(searchDir, entry.name);

                if (entry.isDirectory()) {
                    // Skip common non-source directories
                    if (['node_modules', '.git', 'bin', 'obj', '.vs', '.vscode'].includes(entry.name)) {
                        continue;
                    }

                    // Recursively scan subdirectory
                    const subDirPrefix = prefix ? `${prefix}${entry.name}/` : `${entry.name}/`;
                    const subFiles = this.getAllMTypeFiles(fullPath, currentFilePath, subDirPrefix);
                    files.push(...subFiles);
                } else if (entry.isFile() && entry.name.endsWith('.mt')) {
                    // Skip the current file
                    if (entry.name === currentFileName && fullPath === currentFilePath) {
                        continue;
                    }

                    const relativePath = prefix + entry.name;
                    files.push(relativePath);
                }
            }
        } catch (error) {
            console.log(`Cannot read directory ${searchDir}:`, error);
        }

        return files;
    }

    /**
     * Clear import cache (useful when files change)
     */
    clearCache(): void {
        this.importCache.clear();
    }

    /**
     * Clear cache for specific file
     */
    clearCacheForFile(filePath: string): void {
        const keysToDelete = Array.from(this.importCache.keys())
            .filter(key => key.startsWith(filePath + ':'));

        keysToDelete.forEach(key => this.importCache.delete(key));
    }

    /**
     * Validate all imports in a document
     */
    async validateImports(document: vscode.TextDocument): Promise<vscode.Diagnostic[]> {
        const diagnostics: vscode.Diagnostic[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const importMatch = line.match(/^\s*import\s+(?:.*\s+from\s+)?["']([^"']+)["']\s*;?/);

            if (importMatch) {
                const importPath = importMatch[1];
                const importInfo = await this.getImportInfo(importPath, document.uri.fsPath);

                if (!importInfo || !importInfo.isValid) {
                    const startPos = new vscode.Position(i, line.indexOf(importPath));
                    const endPos = new vscode.Position(i, line.indexOf(importPath) + importPath.length);
                    const range = new vscode.Range(startPos, endPos);

                    const diagnostic = new vscode.Diagnostic(
                        range,
                        `Cannot resolve import: '${importPath}'`,
                        vscode.DiagnosticSeverity.Error
                    );
                    diagnostic.source = 'mtype-imports';
                    diagnostics.push(diagnostic);
                }
            }
        }

        return diagnostics;
    }

    /**
     * Find all files in workspace that export a given symbol (class, function, etc.)
     */
    async findSymbolInWorkspace(symbolName: string): Promise<Array<{ relativePath: string; absolutePath: string; symbol: ExportedSymbol }>> {
        const results: Array<{ relativePath: string; absolutePath: string; symbol: ExportedSymbol }> = [];

        // Find all .mt files in workspace
        const mtFiles = await vscode.workspace.findFiles('**/*.mt', '**/node_modules/**');

        for (const fileUri of mtFiles) {
            try {
                const document = await vscode.workspace.openTextDocument(fileUri);
                const exportedSymbols = this.parseExportedSymbols(document.getText());

                // Check if this file exports the symbol we're looking for
                const matchingSymbol = exportedSymbols.find(s => s.name === symbolName);
                if (matchingSymbol) {
                    // Calculate relative path from workspace root
                    const absolutePath = fileUri.fsPath;
                    const relativePath = path.relative(this.workspaceRoot, absolutePath).replace(/\\/g, '/');

                    results.push({
                        relativePath: './' + relativePath,
                        absolutePath,
                        symbol: matchingSymbol
                    });
                }
            } catch (error) {
                console.log(`Error reading file ${fileUri.fsPath}:`, error);
            }
        }

        return results;
    }
}