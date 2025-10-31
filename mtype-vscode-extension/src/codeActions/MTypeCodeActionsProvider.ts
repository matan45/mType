import * as vscode from 'vscode';
import { MTypeImportResolver } from '../imports/MTypeImportResolver';
import { MTypeScopeAnalyzer } from '../analysis/MTypeScopeAnalyzer';

/**
 * Provides code actions (quick fixes) for common mType issues
 */
export class MTypeCodeActionsProvider implements vscode.CodeActionProvider {
    private importResolver: MTypeImportResolver;

    constructor(importResolver: MTypeImportResolver) {
        this.importResolver = importResolver;
    }

    async provideCodeActions(
        document: vscode.TextDocument,
        range: vscode.Range | vscode.Selection,
        context: vscode.CodeActionContext,
        token: vscode.CancellationToken
    ): Promise<vscode.CodeAction[]> {
        const actions: vscode.CodeAction[] = [];

        // Get the word at the cursor position
        const wordRange = document.getWordRangeAtPosition(range.start);
        if (!wordRange) {
            // Even if no word, still check for interface implementation
            const interfaceActions = this.createInterfaceImplementationActions(document, range);
            actions.push(...interfaceActions);
            return actions;
        }

        const word = document.getText(wordRange);
        const line = document.lineAt(range.start.line);

        // Check if it's an undefined class/type
        if (this.isUndefinedIdentifier(word, line.text)) {
            // Offer import suggestions
            const importActions = await this.createImportActions(word, document);
            actions.push(...importActions);
        }

        // Check if we're in a class that implements an interface
        const interfaceImplementationActions = this.createInterfaceImplementationActions(document, range);
        actions.push(...interfaceImplementationActions);

        // Check for missing override annotation
        const overrideActions = this.createOverrideAnnotationActions(document, range);
        actions.push(...overrideActions);

        return actions;
    }

    /**
     * Check if an identifier is undefined (likely needs an import)
     */
    private isUndefinedIdentifier(word: string, lineText: string): boolean {
        // Check if it looks like a class name (starts with uppercase)
        if (!/^[A-Z]/.test(word)) {
            return false;
        }

        // Check if it's in a context where a type/class is expected
        const typeContextPatterns = [
            /:\s*\w+$/,           // After colon (type annotation)
            /new\s+\w+$/,         // After new keyword
            /extends\s+\w+$/,     // After extends
            /implements\s+\w+$/,  // After implements
            /^\s*\w+\s+\w+/,      // Variable declaration
            /<\w+>/,              // Generic parameter
            /\(\w+\s+\w+/         // Function parameter
        ];

        return typeContextPatterns.some(pattern => pattern.test(lineText));
    }

    /**
     * Create import actions for undefined identifiers
     */
    private async createImportActions(className: string, document: vscode.TextDocument): Promise<vscode.CodeAction[]> {
        const actions: vscode.CodeAction[] = [];

        // Find all files that define this class
        const availableImports = await this.importResolver.findSymbolInWorkspace(className);

        for (const importInfo of availableImports) {
            const action = new vscode.CodeAction(
                `Import ${className} from "${importInfo.relativePath}"`,
                vscode.CodeActionKind.QuickFix
            );

            action.diagnostics = [];
            action.isPreferred = availableImports.indexOf(importInfo) === 0; // First one is preferred

            // Create the edit to add the import
            const edit = new vscode.WorkspaceEdit();
            const importStatement = this.createImportStatement(className, importInfo.relativePath);
            const insertPosition = this.findImportInsertPosition(document);

            edit.insert(document.uri, insertPosition, importStatement);
            action.edit = edit;

            actions.push(action);
        }

        return actions;
    }

    /**
     * Create import statement
     */
    private createImportStatement(className: string, filePath: string): string {
        // Remove .mt extension if present
        const cleanPath = filePath.replace(/\.mt$/, '');
        return `import { ${className} } from "${cleanPath}";\n`;
    }

    /**
     * Find the position to insert a new import statement
     */
    private findImportInsertPosition(document: vscode.TextDocument): vscode.Position {
        let lastImportLine = -1;

        // Find the last import statement
        for (let i = 0; i < document.lineCount; i++) {
            const line = document.lineAt(i).text.trim();
            if (line.startsWith('import ')) {
                lastImportLine = i;
            } else if (line && !line.startsWith('//') && lastImportLine !== -1) {
                // Found a non-import, non-comment line after imports
                break;
            }
        }

        if (lastImportLine !== -1) {
            // Insert after the last import
            return new vscode.Position(lastImportLine + 1, 0);
        } else {
            // Insert at the beginning of the file
            return new vscode.Position(0, 0);
        }
    }

    /**
     * Create actions to implement missing interface methods
     */
    private createInterfaceImplementationActions(
        document: vscode.TextDocument,
        range: vscode.Range
    ): vscode.CodeAction[] {
        const actions: vscode.CodeAction[] = [];

        // Parse the document to find if we're in a class that implements an interface
        const scopeAnalyzer = new MTypeScopeAnalyzer(document);
        scopeAnalyzer.analyzeDocument();

        // Get the current class name
        const currentClassName = scopeAnalyzer.getCurrentClassName(range.start);

        if (!currentClassName) {
            return actions;
        }

        const classInfo = scopeAnalyzer.getClassInfo(currentClassName);

        if (!classInfo || !classInfo.implementedInterfaces || classInfo.implementedInterfaces.length === 0) {
            return actions;
        }

        // For each implemented interface, check if all methods are implemented
        for (const interfaceName of classInfo.implementedInterfaces) {
            const missingMethods = this.findMissingInterfaceMethods(
                document,
                currentClassName,
                interfaceName,
                scopeAnalyzer
            );

            if (missingMethods.length > 0) {
                const action = new vscode.CodeAction(
                    `Implement all methods from ${interfaceName}`,
                    vscode.CodeActionKind.QuickFix
                );

                action.diagnostics = [];
                action.isPreferred = true;

                // Create the edit to add the methods
                const edit = new vscode.WorkspaceEdit();
                const insertPosition = this.findMethodInsertPosition(document, currentClassName);
                const methodsCode = this.generateInterfaceMethodStubs(missingMethods);

                edit.insert(document.uri, insertPosition, methodsCode);
                action.edit = edit;

                actions.push(action);
            }
        }

        return actions;
    }

    /**
     * Find missing interface methods
     */
    private findMissingInterfaceMethods(
        document: vscode.TextDocument,
        className: string,
        interfaceName: string,
        scopeAnalyzer: MTypeScopeAnalyzer
    ): Array<{ name: string; parameters: Array<{ name: string; type: string }>; returnType: string }> {
        const missingMethods: Array<{ name: string; parameters: Array<{ name: string; type: string }>; returnType: string }> = [];

        // Get interface methods
        const interfaceInfo = scopeAnalyzer.getInterfaceInfo(interfaceName);
        if (!interfaceInfo) {
            return missingMethods;
        }

        // Get class methods
        const classInfo = scopeAnalyzer.getClassInfo(className);
        if (!classInfo) {
            return missingMethods;
        }

        const classMethodNames = Array.from(classInfo.methods.keys());

        // Find methods in interface that are not in class
        for (const [methodName, method] of interfaceInfo.methods) {
            if (!classMethodNames.includes(methodName)) {
                missingMethods.push({
                    name: method.name,
                    parameters: method.parameters,
                    returnType: method.returnType
                });
            }
        }

        return missingMethods;
    }

    /**
     * Find position to insert new methods in a class
     */
    private findMethodInsertPosition(document: vscode.TextDocument, className: string): vscode.Position {
        const text = document.getText();
        const lines = text.split('\n');

        // Find the class and its closing brace
        let inClass = false;
        let braceCount = 0;
        let lastMethodLine = -1;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Match class declaration with optional extends/implements clauses
            // e.g., "class MyClass {", "class MyClass extends Parent {", "class MyClass implements Interface {"
            if (line.match(new RegExp(`class\\s+${className}\\b[^{]*\\{`))) {
                inClass = true;
                braceCount = 1;
                continue;
            }

            if (inClass) {
                const openBraces = (line.match(/\{/g) || []).length;
                const closeBraces = (line.match(/\}/g) || []).length;
                braceCount += openBraces - closeBraces;

                // Track methods
                if (line.match(/\s*(?:public|private|protected)?\s*function\s+\w+/)) {
                    lastMethodLine = i;
                }

                if (braceCount <= 0) {
                    // Found the closing brace of the class
                    if (lastMethodLine !== -1) {
                        // Insert after last method
                        return new vscode.Position(lastMethodLine + 1, 0);
                    } else {
                        // Insert before closing brace
                        return new vscode.Position(i, 0);
                    }
                }
            }
        }

        // Fallback
        return new vscode.Position(document.lineCount, 0);
    }

    /**
     * Generate method stubs for interface methods
     */
    private generateInterfaceMethodStubs(
        methods: Array<{ name: string; parameters: Array<{ name: string; type: string }>; returnType: string }>
    ): string {
        let code = '\n';

        for (const method of methods) {
            const params = method.parameters.map(p => `${p.type} ${p.name}`).join(', ');
            code += `\t@Override\n`;
            code += `\tpublic function ${method.name}(${params}): ${method.returnType} {\n`;
            code += `\t\t// TODO: Implement ${method.name}\n`;

            if (method.returnType !== 'void') {
                code += `\t\treturn null; // Replace with actual implementation\n`;
            }

            code += `\t}\n\n`;
        }

        return code;
    }

    /**
     * Create actions to add @Override annotation
     */
    private createOverrideAnnotationActions(
        document: vscode.TextDocument,
        range: vscode.Range
    ): vscode.CodeAction[] {
        const actions: vscode.CodeAction[] = [];

        const line = document.lineAt(range.start.line);
        const lineText = line.text;

        // Check if this line is a method declaration in a class
        const methodMatch = lineText.match(/^\s*(?:public|private|protected)?\s*function\s+(\w+)/);
        if (!methodMatch) {
            return actions;
        }

        // Check if @Override is already present
        if (range.start.line > 0) {
            const previousLine = document.lineAt(range.start.line - 1).text.trim();
            if (previousLine === '@Override') {
                return actions; // Already has @Override
            }
        }

        // Create action to add @Override
        const action = new vscode.CodeAction(
            'Add @Override annotation',
            vscode.CodeActionKind.QuickFix
        );

        const edit = new vscode.WorkspaceEdit();
        const indent = lineText.match(/^\s*/)?.[0] || '';
        edit.insert(document.uri, new vscode.Position(range.start.line, 0), `${indent}@Override\n`);
        action.edit = edit;

        actions.push(action);

        return actions;
    }

    /**
     * Organize imports (sort and remove duplicates)
     */
    static createOrganizeImportsAction(document: vscode.TextDocument): vscode.CodeAction {
        const action = new vscode.CodeAction(
            'Organize Imports',
            vscode.CodeActionKind.SourceOrganizeImports
        );

        const edit = new vscode.WorkspaceEdit();
        const organizedImports = MTypeCodeActionsProvider.organizeImports(document);

        if (organizedImports) {
            edit.replace(document.uri, organizedImports.range, organizedImports.newText);
            action.edit = edit;
        }

        return action;
    }

    /**
     * Organize imports in the document
     */
    private static organizeImports(document: vscode.TextDocument): { range: vscode.Range; newText: string } | null {
        const imports: { line: number; text: string }[] = [];
        let firstImportLine = -1;
        let lastImportLine = -1;

        // Find all import statements
        for (let i = 0; i < document.lineCount; i++) {
            const line = document.lineAt(i);
            const trimmedText = line.text.trim();

            if (trimmedText.startsWith('import ')) {
                if (firstImportLine === -1) {
                    firstImportLine = i;
                }
                lastImportLine = i;
                imports.push({ line: i, text: trimmedText });
            }
        }

        if (imports.length === 0) {
            return null;
        }

        // Sort imports alphabetically
        const sortedImports = [...new Set(imports.map(imp => imp.text))].sort();

        // Create new import block
        const newImportsText = sortedImports.join('\n') + '\n';

        const range = new vscode.Range(
            new vscode.Position(firstImportLine, 0),
            new vscode.Position(lastImportLine + 1, 0)
        );

        return { range, newText: newImportsText };
    }
}
