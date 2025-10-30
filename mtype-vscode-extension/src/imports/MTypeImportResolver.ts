import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

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

    constructor(workspaceRoot: string) {
        this.workspaceRoot = workspaceRoot;
    }

    /**
     * Resolve an import path to an absolute file path
     */
    resolveImportPath(importPath: string, currentFilePath: string): string | null {
        // Remove quotes from import path
        const cleanPath = importPath.replace(/['"]/g, '');
        const currentDir = path.dirname(currentFilePath);

        console.log('Resolving import path:', cleanPath, 'from:', currentFilePath);

        // For relative imports starting with ./ or ../
        if (cleanPath.startsWith('./') || cleanPath.startsWith('../')) {
            const resolvedPath = path.resolve(currentDir, cleanPath);
            console.log('Relative path resolved to:', resolvedPath);

            if (fs.existsSync(resolvedPath)) {
                return resolvedPath;
            }
            return null;
        }

        // For absolute imports starting with /
        if (cleanPath.startsWith('/')) {
            const resolvedPath = path.join(this.workspaceRoot, cleanPath.substring(1));
            console.log('Absolute path resolved to:', resolvedPath);

            if (fs.existsSync(resolvedPath)) {
                return resolvedPath;
            }
            return null;
        }

        // For paths with subdirectories (contains /)
        if (cleanPath.includes('/')) {
            // Try relative to workspace root
            const workspaceRelativePath = path.join(this.workspaceRoot, cleanPath);
            console.log('Workspace relative path:', workspaceRelativePath);

            if (fs.existsSync(workspaceRelativePath)) {
                return workspaceRelativePath;
            }

            // Try relative to current directory
            const currentRelativePath = path.join(currentDir, cleanPath);
            console.log('Current directory relative path:', currentRelativePath);

            if (fs.existsSync(currentRelativePath)) {
                return currentRelativePath;
            }
        }

        // For simple filenames without path separators
        // 1. Look in same directory first
        const sameDirPath = path.join(currentDir, cleanPath);
        console.log('Same directory path:', sameDirPath);

        if (fs.existsSync(sameDirPath)) {
            return sameDirPath;
        }

        // 2. Look in workspace root
        const workspaceRootPath = path.join(this.workspaceRoot, cleanPath);
        console.log('Workspace root path:', workspaceRootPath);

        if (fs.existsSync(workspaceRootPath)) {
            return workspaceRootPath;
        }

        // 3. Search recursively in workspace for the file
        const foundPath = this.findFileRecursively(this.workspaceRoot, cleanPath);
        if (foundPath) {
            console.log('Found file recursively:', foundPath);
            return foundPath;
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

            // 3. Recursively get files in all subdirectories from workspace root
            const allFiles = this.getAllMTypeFiles(this.workspaceRoot, currentFilePath);
            console.log('All workspace files found:', allFiles.length);
            imports.push(...allFiles);

            // 4. Get files relative to current directory (subdirectories of current dir)
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
            const importMatch = line.match(/^\s*import\s+["']([^"']+)["']\s*;?/);

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