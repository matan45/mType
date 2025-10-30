import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { MTypeImportResolver, ImportInfo } from './MTypeImportResolver';

export class MTypeImportCompletionProvider implements vscode.CompletionItemProvider {
    private importResolver: MTypeImportResolver;

    constructor(importResolver: MTypeImportResolver) {
        this.importResolver = importResolver;
    }

    async provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken
    ): Promise<vscode.CompletionItem[]> {
        const lineText = document.lineAt(position).text;
        const linePrefix = lineText.substring(0, position.character);

        // Check if we're in an import statement
        const importContext = this.getImportContext(linePrefix);

        if (!importContext) {
            return [];
        }

        switch (importContext.type) {
            case 'import-keyword':
                return this.getImportKeywordCompletion();
            case 'from-keyword':
                return this.getFromKeywordCompletion();
            case 'import-statement':
                return this.getImportStatementCompletions(document, position);
            case 'import-path':
                return this.getImportPathCompletions(document, position, importContext.partialPath || '');
            default:
                return [];
        }
    }

    /**
     * Determine the context of import completion
     */
    private getImportContext(linePrefix: string): ImportContext | null {
        // Check for beginning of import statement: "import "
        if (linePrefix.match(/^\s*import\s+$/)) {
            return { type: 'import-statement' };
        }

        // Check for "from" keyword after "import *": "import * " or "import * f" etc.
        if (linePrefix.match(/^\s*import\s+\*\s+$/) || linePrefix.match(/^\s*import\s+\*\s+f(r(o(m)?)?)?$/)) {
            return { type: 'from-keyword' };
        }

        // Check for path after "from": "import * from "
        if (linePrefix.match(/^\s*import\s+\*\s+from\s+$/)) {
            return { type: 'import-statement' };
        }

        // Check for import path completion: import * from "partial
        const importPathMatch = linePrefix.match(/^\s*import\s+\*\s+from\s+["']([^"']*)$/);
        if (importPathMatch) {
            return {
                type: 'import-path',
                partialPath: importPathMatch[1]
            };
        }

        // Check if we're starting to type "import"
        if (linePrefix.match(/^\s*i(m(p(o(r(t)?)?)?)?)?$/)) {
            return { type: 'import-keyword' };
        }

        return null;
    }

    /**
     * Provide import keyword completion
     */
    private getImportKeywordCompletion(): vscode.CompletionItem[] {
        const item = new vscode.CompletionItem('import', vscode.CompletionItemKind.Keyword);
        item.detail = 'Import statement';
        item.documentation = new vscode.MarkdownString('Import symbols from another file');
        item.insertText = new vscode.SnippetString('import * from "${1:path}";');
        item.sortText = '0import';
        return [item];
    }

    /**
     * Provide from keyword completion
     */
    private getFromKeywordCompletion(): vscode.CompletionItem[] {
        const item = new vscode.CompletionItem('from', vscode.CompletionItemKind.Keyword);
        item.detail = 'from keyword';
        item.documentation = new vscode.MarkdownString('Specify the file path to import from');
        item.insertText = new vscode.SnippetString('from "${1:path}";');
        item.sortText = '0from';
        return [item];
    }

    /**
     * Provide completions for import statements
     */
    private async getImportStatementCompletions(
        document: vscode.TextDocument,
        position: vscode.Position
    ): Promise<vscode.CompletionItem[]> {
        const completions: vscode.CompletionItem[] = [];

        // Get available import files
        const availableImports = await this.importResolver.getAvailableImports(document.uri.fsPath);

        for (const importPath of availableImports) {
            const item = new vscode.CompletionItem(
                `"${importPath}"`,
                vscode.CompletionItemKind.File
            );

            item.detail = `Import ${importPath}`;
            item.documentation = new vscode.MarkdownString(
                `Import symbols from \`${importPath}\``
            );

            // Create snippet that includes the semicolon
            item.insertText = new vscode.SnippetString(`"${importPath}";`);
            item.sortText = '0' + importPath;

            // Try to get preview of exported symbols
            try {
                const importInfo = await this.importResolver.getImportInfo(importPath, document.uri.fsPath);
                if (importInfo && importInfo.isValid && importInfo.exportedSymbols.length > 0) {
                    const symbolNames = importInfo.exportedSymbols.map(s => s.name).slice(0, 5);
                    const preview = symbolNames.join(', ');
                    const more = importInfo.exportedSymbols.length > 5 ? '...' : '';

                    item.documentation = new vscode.MarkdownString(
                        `Import symbols from \`${importPath}\`\n\n**Exports:** ${preview}${more}`
                    );
                }
            } catch (error) {
                // Ignore errors in preview generation
            }

            completions.push(item);
        }

        return completions;
    }

    /**
     * Provide completions for import paths (when partially typed)
     */
    private async getImportPathCompletions(
        document: vscode.TextDocument,
        position: vscode.Position,
        partialPath: string
    ): Promise<vscode.CompletionItem[]> {
        const completions: vscode.CompletionItem[] = [];

        console.log('Getting import path completions for partial path:', partialPath);

        // Use smart directory-based completion
        const smartCompletions = await this.getSmartPathCompletions(document.uri.fsPath, partialPath);
        if (smartCompletions.length > 0) {
            return smartCompletions;
        }

        // Fallback to old behavior if smart completion doesn't work
        const availableImports = await this.importResolver.getAvailableImports(document.uri.fsPath);
        console.log('Available imports:', availableImports);

        const filteredImports = availableImports.filter(importPath => {
            const lowercaseImportPath = importPath.toLowerCase();
            const lowercasePartialPath = partialPath.toLowerCase();

            // Match by filename or full path
            const fileName = importPath.split('/').pop() || '';
            const fileNameLower = fileName.toLowerCase();

            return lowercaseImportPath.includes(lowercasePartialPath) ||
                   fileNameLower.includes(lowercasePartialPath) ||
                   lowercaseImportPath.startsWith(lowercasePartialPath);
        });

        console.log('Filtered imports:', filteredImports);

        for (const importPath of filteredImports) {
            const item = new vscode.CompletionItem(
                importPath,
                vscode.CompletionItemKind.File
            );

            item.detail = `Import ${importPath}`;
            item.insertText = importPath;

            // Sort by relevance: exact matches first, then by path type
            let sortPriority = '5'; // Default
            if (importPath.toLowerCase().startsWith(partialPath.toLowerCase())) {
                sortPriority = '1'; // Exact prefix match
            } else if (importPath.startsWith('./')) {
                sortPriority = '2'; // Current directory
            } else if (importPath.startsWith('../')) {
                sortPriority = '3'; // Parent directory
            } else if (!importPath.includes('/')) {
                sortPriority = '4'; // Workspace root files
            }

            item.sortText = sortPriority + importPath;

            // Add symbol preview
            try {
                const importInfo = await this.importResolver.getImportInfo(importPath, document.uri.fsPath);
                if (importInfo && importInfo.isValid && importInfo.exportedSymbols.length > 0) {
                    const symbolNames = importInfo.exportedSymbols.map(s => s.name).slice(0, 3);
                    const preview = symbolNames.join(', ');
                    const more = importInfo.exportedSymbols.length > 3 ? '...' : '';

                    item.detail = `${importPath} (${preview}${more})`;
                    item.documentation = new vscode.MarkdownString(
                        `**Import Path:** \`${importPath}\`\n\n**Available Symbols:** ${preview}${more}`
                    );
                }
            } catch (error) {
                console.log('Error getting import info for path completion:', error);
            }

            completions.push(item);
        }

        console.log('Final path completions:', completions.length);
        return completions;
    }

    /**
     * Smart directory-based path completion (like C++ includes)
     */
    private async getSmartPathCompletions(
        currentFilePath: string,
        partialPath: string
    ): Promise<vscode.CompletionItem[]> {
        const completions: vscode.CompletionItem[] = [];

        try {
            // Determine the directory to search based on partial path
            const currentFileDir = path.dirname(currentFilePath);

            // Parse the partial path to get directory and file prefix
            const lastSlashIndex = partialPath.lastIndexOf('/');
            let searchDir: string;
            let filePrefix: string;

            if (lastSlashIndex === -1) {
                // No slash yet, not a path-based search
                return [];
            }

            const dirPart = partialPath.substring(0, lastSlashIndex + 1); // includes trailing slash
            filePrefix = partialPath.substring(lastSlashIndex + 1);

            // Resolve the directory to search
            if (dirPart.startsWith('./')) {
                searchDir = path.join(currentFileDir, dirPart.substring(2));
            } else if (dirPart.startsWith('../')) {
                searchDir = path.resolve(currentFileDir, dirPart);
            } else if (dirPart === '/') {
                // Absolute path (rare in imports)
                searchDir = '/';
            } else {
                // Relative path without ./ prefix
                searchDir = path.join(currentFileDir, dirPart);
            }

            console.log('[Smart Path] Searching directory:', searchDir, 'with prefix:', filePrefix);

            // Check if directory exists
            if (!fs.existsSync(searchDir)) {
                console.log('[Smart Path] Directory does not exist:', searchDir);
                return [];
            }

            // Read directory contents
            const entries = fs.readdirSync(searchDir, { withFileTypes: true });

            for (const entry of entries) {
                // Filter by prefix
                if (filePrefix && !entry.name.toLowerCase().startsWith(filePrefix.toLowerCase())) {
                    continue;
                }

                if (entry.isDirectory()) {
                    // Add directory with trailing slash
                    const item = new vscode.CompletionItem(
                        entry.name + '/',
                        vscode.CompletionItemKind.Folder
                    );
                    item.detail = 'Directory';
                    item.insertText = entry.name + '/';
                    item.command = {
                        command: 'editor.action.triggerSuggest',
                        title: 'Re-trigger completions'
                    };
                    item.sortText = '0' + entry.name; // Directories first
                    completions.push(item);

                } else if (entry.isFile() && entry.name.endsWith('.mt')) {
                    // Add .mt file without extension
                    const fileNameWithoutExt = entry.name.replace(/\.mt$/, '');
                    const item = new vscode.CompletionItem(
                        fileNameWithoutExt,
                        vscode.CompletionItemKind.File
                    );
                    item.detail = entry.name;
                    item.insertText = fileNameWithoutExt;
                    item.sortText = '1' + entry.name; // Files after directories

                    // Try to get exported symbols preview
                    const fullPath = path.join(searchDir, entry.name);
                    const relativePath = partialPath.substring(0, lastSlashIndex + 1) + fileNameWithoutExt;
                    try {
                        const importInfo = await this.importResolver.getImportInfo(relativePath, currentFilePath);
                        if (importInfo && importInfo.exportedSymbols.length > 0) {
                            const symbolNames = importInfo.exportedSymbols.map(s => s.name).slice(0, 3);
                            const preview = symbolNames.join(', ');
                            const more = importInfo.exportedSymbols.length > 3 ? '...' : '';
                            item.documentation = new vscode.MarkdownString(
                                `**Exports:** ${preview}${more}`
                            );
                        }
                    } catch (error) {
                        // Ignore preview errors
                    }

                    completions.push(item);
                }
            }

            console.log('[Smart Path] Found', completions.length, 'completions');

        } catch (error) {
            console.error('[Smart Path] Error:', error);
        }

        return completions;
    }
}

interface ImportContext {
    type: 'import-statement' | 'import-path' | 'import-keyword' | 'from-keyword';
    partialPath?: string;
}

/**
 * Provider for imported symbol completions
 */
export class MTypeImportedSymbolProvider {
    private importResolver: MTypeImportResolver;
    private documentImports: Map<string, ImportInfo[]> = new Map();

    constructor(importResolver: MTypeImportResolver) {
        this.importResolver = importResolver;
    }

    /**
     * Analyze document imports and cache them
     */
    async analyzeDocumentImports(document: vscode.TextDocument): Promise<void> {
        const imports: ImportInfo[] = [];
        const processedPaths = new Set<string>(); // Track processed import paths to avoid duplicates
        const text = document.getText();
        const lines = text.split('\n');

        for (const line of lines) {
            const importMatch = line.match(/^\s*import\s+["']([^"']+)["']\s*;?/);
            if (importMatch) {
                const importPath = importMatch[1];

                // Skip if we've already processed this import path
                if (processedPaths.has(importPath)) {
                    console.log(`Skipping duplicate import: ${importPath}`);
                    continue;
                }

                const importInfo = await this.importResolver.getImportInfo(importPath, document.uri.fsPath);
                if (importInfo && importInfo.isValid) {
                    imports.push(importInfo);
                    processedPaths.add(importPath);
                }
            }
        }

        this.documentImports.set(document.uri.toString(), imports);
    }

    /**
     * Get all imported symbols for a document
     */
    getImportedSymbols(document: vscode.TextDocument): ImportInfo[] {
        return this.documentImports.get(document.uri.toString()) || [];
    }

    /**
     * Get completion items for imported symbols
     */
    getImportedSymbolCompletions(document: vscode.TextDocument): vscode.CompletionItem[] {
        const completions: vscode.CompletionItem[] = [];
        const imports = this.getImportedSymbols(document);
        const addedSymbols = new Set<string>(); // Track added symbols to avoid duplicates

        console.log('Getting imported symbol completions for:', document.uri.toString());
        console.log('Found imports:', imports.length);

        for (const importInfo of imports) {
            console.log('Processing import:', importInfo.importPath, 'with symbols:', importInfo.exportedSymbols.length);
            for (const symbol of importInfo.exportedSymbols) {
                // Skip if we've already added this symbol
                if (addedSymbols.has(symbol.name)) {
                    continue;
                }
                let item: vscode.CompletionItem;

                switch (symbol.type) {
                    case 'function':
                        item = new vscode.CompletionItem(symbol.name, vscode.CompletionItemKind.Function);
                        item.detail = symbol.signature;
                        item.documentation = new vscode.MarkdownString(
                            `**Imported Function** from \`${importInfo.importPath}\`\n\n\`${symbol.signature}\``
                        );

                        // Create snippet for function calls
                        if (symbol.parameters && symbol.parameters.length > 0) {
                            const snippetParams = symbol.parameters.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ');
                            item.insertText = new vscode.SnippetString(`${symbol.name}(${snippetParams})`);
                        } else {
                            item.insertText = new vscode.SnippetString(`${symbol.name}()`);
                        }
                        break;

                    case 'class':
                        item = new vscode.CompletionItem(symbol.name, vscode.CompletionItemKind.Class);
                        item.detail = symbol.signature;
                        item.documentation = new vscode.MarkdownString(
                            `**Imported Class** from \`${importInfo.importPath}\`\n\n\`${symbol.signature}\``
                        );
                        break;

                    case 'variable':
                        item = new vscode.CompletionItem(symbol.name, vscode.CompletionItemKind.Variable);
                        item.detail = symbol.signature;
                        item.documentation = new vscode.MarkdownString(
                            `**Imported Variable** from \`${importInfo.importPath}\`\n\n\`${symbol.signature}\``
                        );
                        break;

                    default:
                        continue;
                }

                item.sortText = '1' + symbol.name; // Lower priority than local symbols
                completions.push(item);
                addedSymbols.add(symbol.name); // Mark this symbol as added
            }
        }

        return completions;
    }

    /**
     * Clear cache for a document
     */
    clearDocumentCache(document: vscode.TextDocument): void {
        this.documentImports.delete(document.uri.toString());
    }
}