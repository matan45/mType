import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';
import { MTypeImportResolver } from '../imports/MTypeImportResolver';
import { MTypeImportedSymbolProvider } from '../imports/MTypeImportCompletionProvider';

export class MTypeReferenceProvider implements vscode.ReferenceProvider {
    private analyzer = new MTypeCodeAnalyzer();
    private importResolver: MTypeImportResolver | null = null;
    private importedSymbolProvider: MTypeImportedSymbolProvider | null = null;

    setImportResolver(resolver: MTypeImportResolver): void {
        this.importResolver = resolver;
    }

    setImportedSymbolProvider(provider: MTypeImportedSymbolProvider): void {
        this.importedSymbolProvider = provider;
    }

    async provideReferences(
        document: vscode.TextDocument,
        position: vscode.Position,
        context: vscode.ReferenceContext,
        token: vscode.CancellationToken
    ): Promise<vscode.Location[]> {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

        const wordRange = document.getWordRangeAtPosition(position);
        if (!wordRange) {
            return [];
        }

        const word = document.getText(wordRange);
        const line = document.lineAt(position.line).text;

        // Find all references to the symbol
        const references = await this.findAllReferences(word, line, position, document, context.includeDeclaration);

        return references || [];
    }

    private async findAllReferences(
        word: string,
        line: string,
        position: vscode.Position,
        document: vscode.TextDocument,
        includeDeclaration: boolean
    ): Promise<vscode.Location[]> {
        const references: vscode.Location[] = [];

        // First, find references in the current document
        const localReferences = this.findLocalReferences(word, line, position, document, includeDeclaration);
        references.push(...localReferences);

        // Then, find references in other files that import this symbol or are imported by this file
        const crossFileReferences = await this.findCrossFileReferences(word, document, includeDeclaration);
        references.push(...crossFileReferences);

        return references;
    }

    /**
     * Find references within the current document
     */
    private findLocalReferences(
        word: string,
        line: string,
        position: vscode.Position,
        document: vscode.TextDocument,
        includeDeclaration: boolean
    ): vscode.Location[] {
        const references: vscode.Location[] = [];

        // Determine what type of symbol we're looking for
        const symbolType = this.getSymbolType(word, line, position);

        switch (symbolType.type) {
            case 'class':
                references.push(...this.findClassReferences(word, document, includeDeclaration));
                break;
            case 'method':
                references.push(...this.findMethodReferences(word, symbolType.className!, document, includeDeclaration));
                break;
            case 'field':
                references.push(...this.findFieldReferences(word, symbolType.className!, document, includeDeclaration));
                break;
            case 'variable':
                references.push(...this.findVariableReferences(word, document, includeDeclaration));
                break;
            case 'constructor':
                references.push(...this.findConstructorReferences(word, document, includeDeclaration));
                break;
        }

        return references;
    }

    /**
     * Find references across imported files
     */
    private async findCrossFileReferences(
        symbolName: string,
        document: vscode.TextDocument,
        includeDeclaration: boolean
    ): Promise<vscode.Location[]> {
        const references: vscode.Location[] = [];

        if (!this.importResolver || !this.importedSymbolProvider) {
            return references;
        }

        try {
            // Ensure imports are analyzed
            await this.importedSymbolProvider.analyzeDocumentImports(document);

            // Get all workspace .mt files
            const allMTypeFiles = await vscode.workspace.findFiles('**/*.mt');

            // Search each file for references to this symbol
            for (const fileUri of allMTypeFiles) {
                try {
                    // Skip the current document to avoid double-counting
                    if (fileUri.toString() === document.uri.toString()) {
                        continue;
                    }

                    const fileDocument = await vscode.workspace.openTextDocument(fileUri);
                    const fileReferences = this.findSymbolReferencesInDocument(symbolName, fileDocument);
                    references.push(...fileReferences);
                } catch (error) {
                    console.error(`Error searching file ${fileUri.fsPath}:`, error);
                }
            }
        } catch (error) {
            console.error('Error finding cross-file references:', error);
        }

        return references;
    }

    /**
     * Find all references to a symbol within a specific document
     */
    private findSymbolReferencesInDocument(symbolName: string, document: vscode.TextDocument): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Find all occurrences of the symbol in this line
            let startIndex = 0;
            while (true) {
                const index = line.indexOf(symbolName, startIndex);
                if (index === -1) break;

                // Check if this is a whole word match (not part of another identifier)
                const prevChar = index > 0 ? line[index - 1] : ' ';
                const nextChar = index + symbolName.length < line.length ? line[index + symbolName.length] : ' ';

                const isWholeWord = !/[a-zA-Z0-9_]/.test(prevChar) && !/[a-zA-Z0-9_]/.test(nextChar);

                if (isWholeWord) {
                    const position = new vscode.Position(i, index);
                    references.push(new vscode.Location(document.uri, position));
                }

                startIndex = index + 1;
            }
        }

        return references;
    }

    private getSymbolType(word: string, line: string, position: vscode.Position): { type: string; className?: string } {
        // Check for static method/field access (ClassName::member)
        const staticMatch = line.match(/(\w+)::\s*(\w+)/);
        if (staticMatch && staticMatch[2] === word) {
            const className = staticMatch[1];
            if (this.analyzer.classes.has(className)) {
                const classInfo = this.analyzer.classes.get(className)!;
                const member = classInfo.members.find(m => m.name === word && m.isStatic);
                if (member) {
                    return { type: member.type, className };
                }
            }
        }

        // Check for instance method/field access (object.member)
        const instanceMatch = line.match(/(\w+)\.\s*(\w+)/);
        if (instanceMatch && instanceMatch[2] === word) {
            const objectName = instanceMatch[1];
            const objectType = this.analyzer.getVariableType(objectName);
            if (objectType && this.analyzer.classes.has(objectType)) {
                const classInfo = this.analyzer.classes.get(objectType)!;
                const member = classInfo.members.find(m => m.name === word && !m.isStatic);
                if (member) {
                    return { type: member.type, className: objectType };
                }
            }
        }

        // Check for constructor (new ClassName)
        const newMatch = line.match(/new\s+(\w+)\s*\(/);
        if (newMatch && newMatch[1] === word) {
            return { type: 'constructor', className: word };
        }

        // Check if it's a class name
        if (this.analyzer.classes.has(word)) {
            return { type: 'class' };
        }

        // Check if it's a variable
        if (this.analyzer.getVariableType(word)) {
            return { type: 'variable' };
        }

        // Default to variable if nothing else matches
        return { type: 'variable' };
    }

    private findClassReferences(className: string, document: vscode.TextDocument, includeDeclaration: boolean): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        // Include declaration if requested
        if (includeDeclaration) {
            const classLocation = this.analyzer.getSymbolLocation(className, 'class');
            if (classLocation) {
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(classLocation.line, classLocation.character)
                ));
            }
        }

        // Find all references to the class
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Skip the declaration line if includeDeclaration is false
            const isDeclarationLine = line.match(new RegExp(`^\\s*(?:interface|class)\\s+${className}\\b`));
            if (isDeclarationLine && !includeDeclaration) {
                continue;
            }

            // Variable declarations with class type
            const varDeclRegex = new RegExp(`\\b${className}\\s+\\w+`, 'g');
            let match;
            while ((match = varDeclRegex.exec(line)) !== null) {
                const charIndex = match.index;
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }

            // Constructor calls (new ClassName)
            const newRegex = new RegExp(`\\bnew\\s+${className}\\s*\\(`, 'g');
            while ((match = newRegex.exec(line)) !== null) {
                const charIndex = line.indexOf(className, match.index);
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }

            // Static member access (ClassName::member)
            const staticRegex = new RegExp(`\\b${className}::`, 'g');
            while ((match = staticRegex.exec(line)) !== null) {
                const charIndex = match.index;
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }

            // Implements clauses (class X implements ClassName)
            const implementsRegex = new RegExp(`\\bimplements\\s+([^{]+)`, 'g');
            while ((match = implementsRegex.exec(line)) !== null) {
                const implementsList = match[1];
                const interfaces = implementsList.split(',').map(i => i.trim());
                for (const interfaceName of interfaces) {
                    if (interfaceName === className) {
                        const charIndex = line.indexOf(className, match.index);
                        references.push(new vscode.Location(
                            document.uri,
                            new vscode.Position(i, charIndex)
                        ));
                    }
                }
            }

            // Extends clauses (class X extends ClassName or interface X extends ClassName)
            const extendsRegex = new RegExp(`\\bextends\\s+${className}\\b`, 'g');
            while ((match = extendsRegex.exec(line)) !== null) {
                const charIndex = line.indexOf(className, match.index);
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }
        }

        return references;
    }

    private findMethodReferences(methodName: string, className: string, document: vscode.TextDocument, includeDeclaration: boolean): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        // Include declaration if requested
        if (includeDeclaration) {
            const methodLocation = this.analyzer.getSymbolLocation(`${className}.${methodName}`, 'method');
            if (methodLocation) {
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(methodLocation.line, methodLocation.character)
                ));
            }
        }

        // Find all method calls
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Static method calls (ClassName::methodName)
            const staticCallRegex = new RegExp(`\\b${className}::\\s*${methodName}\\s*\\(`, 'g');
            let match;
            while ((match = staticCallRegex.exec(line)) !== null) {
                const charIndex = line.indexOf(methodName, match.index);
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }

            // Instance method calls (object.methodName)
            const instanceCallRegex = new RegExp(`\\w+\\.\\s*${methodName}\\s*\\(`, 'g');
            while ((match = instanceCallRegex.exec(line)) !== null) {
                // Verify the object is of the correct type
                const objectMatch = line.substring(0, match.index).match(/(\w+)\s*$/);
                if (objectMatch) {
                    const objectName = objectMatch[1];
                    const objectType = this.analyzer.getVariableType(objectName);
                    if (objectType === className) {
                        const charIndex = line.indexOf(methodName, match.index);
                        references.push(new vscode.Location(
                            document.uri,
                            new vscode.Position(i, charIndex)
                        ));
                    }
                }
            }
        }

        return references;
    }

    private findFieldReferences(fieldName: string, className: string, document: vscode.TextDocument, includeDeclaration: boolean): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        // Include declaration if requested
        if (includeDeclaration) {
            const fieldLocation = this.analyzer.getSymbolLocation(`${className}.${fieldName}`, 'field');
            if (fieldLocation) {
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(fieldLocation.line, fieldLocation.character)
                ));
            }
        }

        // Find all field references
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Static field access (ClassName::fieldName)
            const staticAccessRegex = new RegExp(`\\b${className}::\\s*${fieldName}\\b`, 'g');
            let match;
            while ((match = staticAccessRegex.exec(line)) !== null) {
                const charIndex = line.indexOf(fieldName, match.index);
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }

            // Instance field access (object.fieldName or this.fieldName)
            const instanceAccessRegex = new RegExp(`(\\w+|this)\\.\\s*${fieldName}\\b`, 'g');
            while ((match = instanceAccessRegex.exec(line)) !== null) {
                const objectName = match[1];

                // Handle 'this' references within the same class
                if (objectName === 'this') {
                    const charIndex = line.indexOf(fieldName, match.index);
                    references.push(new vscode.Location(
                        document.uri,
                        new vscode.Position(i, charIndex)
                    ));
                } else {
                    // Verify the object is of the correct type
                    const objectType = this.analyzer.getVariableType(objectName);
                    if (objectType === className) {
                        const charIndex = line.indexOf(fieldName, match.index);
                        references.push(new vscode.Location(
                            document.uri,
                            new vscode.Position(i, charIndex)
                        ));
                    }
                }
            }
        }

        return references;
    }

    private findVariableReferences(variableName: string, document: vscode.TextDocument, includeDeclaration: boolean): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        // Include declaration if requested
        if (includeDeclaration) {
            const variableLocation = this.analyzer.getSymbolLocation(variableName, 'variable');
            if (variableLocation) {
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(variableLocation.line, variableLocation.character)
                ));
            }
        }

        // Find all variable usages
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Find all occurrences of the variable name
            const variableRegex = new RegExp(`\\b${variableName}\\b`, 'g');
            let match;
            while ((match = variableRegex.exec(line)) !== null) {
                // Skip if it's part of a declaration we already included
                if (includeDeclaration) {
                    const variableLocation = this.analyzer.getSymbolLocation(variableName, 'variable');
                    if (variableLocation && i === variableLocation.line && match.index === variableLocation.character) {
                        continue;
                    }
                }

                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, match.index)
                ));
            }
        }

        return references;
    }

    private findConstructorReferences(className: string, document: vscode.TextDocument, includeDeclaration: boolean): vscode.Location[] {
        const references: vscode.Location[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        // Include declaration if requested
        if (includeDeclaration) {
            const constructorLocation = this.analyzer.getSymbolLocation(`${className}.constructor`, 'constructor');
            if (constructorLocation) {
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(constructorLocation.line, constructorLocation.character)
                ));
            }
        }

        // Find all constructor calls
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Constructor calls (new ClassName())
            const newRegex = new RegExp(`\\bnew\\s+${className}\\s*\\(`, 'g');
            let match;
            while ((match = newRegex.exec(line)) !== null) {
                const charIndex = line.indexOf(className, match.index);
                references.push(new vscode.Location(
                    document.uri,
                    new vscode.Position(i, charIndex)
                ));
            }
        }

        return references;
    }
}