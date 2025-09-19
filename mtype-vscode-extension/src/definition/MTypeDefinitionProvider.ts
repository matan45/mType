import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';
import { MTypeImportResolver } from '../imports/MTypeImportResolver';
import { MTypeImportedSymbolProvider } from '../imports/MTypeImportCompletionProvider';

export class MTypeDefinitionProvider implements vscode.DefinitionProvider {
    private analyzer = new MTypeCodeAnalyzer();
    private importResolver: MTypeImportResolver | null = null;
    private importedSymbolProvider: MTypeImportedSymbolProvider | null = null;

    setImportResolver(resolver: MTypeImportResolver): void {
        this.importResolver = resolver;
    }

    setImportedSymbolProvider(provider: MTypeImportedSymbolProvider): void {
        this.importedSymbolProvider = provider;
    }

    async provideDefinition(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken
    ): Promise<vscode.Definition | null> {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

        const wordRange = document.getWordRangeAtPosition(position);
        if (!wordRange) {
            return null;
        }

        const word = document.getText(wordRange);
        const line = document.lineAt(position.line).text;

        // Check different contexts for the symbol
        const definition = await this.findDefinition(word, line, position, document);

        return definition;
    }

    private async findDefinition(
        word: string,
        line: string,
        position: vscode.Position,
        document: vscode.TextDocument
    ): Promise<vscode.Definition | null> {

        // 1. Check for class definitions
        const classDefinition = this.findClassDefinition(word, document);
        if (classDefinition) {
            return classDefinition;
        }

        // 2. Check for method definitions within classes
        const methodDefinition = this.findMethodDefinition(word, line, document);
        if (methodDefinition) {
            return methodDefinition;
        }

        // 3. Check for field definitions within classes
        const fieldDefinition = this.findFieldDefinition(word, line, document);
        if (fieldDefinition) {
            return fieldDefinition;
        }

        // 4. Check for variable definitions
        const variableDefinition = this.findVariableDefinition(word, document, position);
        if (variableDefinition) {
            return variableDefinition;
        }

        // 5. Check for constructor definitions
        const constructorDefinition = this.findConstructorDefinition(word, line, document);
        if (constructorDefinition) {
            return constructorDefinition;
        }

        // 6. Check imported files for the symbol
        const importedDefinition = await this.findImportedDefinition(word, document);
        if (importedDefinition) {
            return importedDefinition;
        }

        return null;
    }

    /**
     * Find definition of a symbol in imported files
     */
    private async findImportedDefinition(symbolName: string, document: vscode.TextDocument): Promise<vscode.Location | null> {
        if (!this.importResolver || !this.importedSymbolProvider) {
            return null;
        }

        try {
            // First, ensure imports are analyzed for this document
            await this.importedSymbolProvider.analyzeDocumentImports(document);

            // Get all imports for this document
            const imports = this.importedSymbolProvider.getImportedSymbols(document);

            // Search through each imported file for the symbol
            for (const importInfo of imports) {
                for (const exportedSymbol of importInfo.exportedSymbols) {
                    if (exportedSymbol.name === symbolName) {
                        // Found the symbol, now find its definition in the imported file
                        const importedFileUri = vscode.Uri.file(importInfo.resolvedPath);

                        try {
                            const importedDocument = await vscode.workspace.openTextDocument(importedFileUri);
                            const location = this.findSymbolInDocument(symbolName, exportedSymbol.type, importedDocument);
                            if (location) {
                                return location;
                            }
                        } catch (error) {
                            console.error(`Error opening imported file ${importInfo.resolvedPath}:`, error);
                        }
                    }
                }
            }
        } catch (error) {
            console.error('Error finding imported definition:', error);
        }

        return null;
    }

    /**
     * Find a specific symbol definition within a document
     */
    private findSymbolInDocument(symbolName: string, symbolType: string, document: vscode.TextDocument): vscode.Location | null {
        const text = document.getText();
        const lines = text.split('\n');

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            switch (symbolType) {
                case 'function':
                    const functionMatch = line.match(new RegExp(`^\\s*function\\s+${symbolName}\\s*\\(`));
                    if (functionMatch) {
                        return new vscode.Location(document.uri, new vscode.Position(i, line.indexOf(symbolName)));
                    }
                    break;

                case 'class':
                    const classMatch = line.match(new RegExp(`^\\s*class\\s+${symbolName}\\s*\\{`));
                    if (classMatch) {
                        return new vscode.Location(document.uri, new vscode.Position(i, line.indexOf(symbolName)));
                    }
                    break;

                case 'variable':
                    const varMatch = line.match(new RegExp(`^\\s*(?:final\\s+)?\\w+\\s+${symbolName}\\s*=`));
                    if (varMatch) {
                        return new vscode.Location(document.uri, new vscode.Position(i, line.indexOf(symbolName)));
                    }
                    break;
            }
        }

        return null;
    }

    private findClassDefinition(className: string, document: vscode.TextDocument): vscode.Location | null {
        const symbolLocation = this.analyzer.getSymbolLocation(className, 'class');
        if (symbolLocation) {
            return new vscode.Location(
                document.uri,
                new vscode.Position(symbolLocation.line, symbolLocation.character)
            );
        }
        return null;
    }

    private findMethodDefinition(methodName: string, line: string, document: vscode.TextDocument): vscode.Location | null {
        // Check for static method call (ClassName::methodName)
        const staticMethodMatch = line.match(/(\w+)::\s*(\w+)/);
        if (staticMethodMatch && staticMethodMatch[2] === methodName) {
            const className = staticMethodMatch[1];
            const symbolLocation = this.analyzer.getSymbolLocation(`${className}.${methodName}`, 'method');
            if (symbolLocation) {
                return new vscode.Location(
                    document.uri,
                    new vscode.Position(symbolLocation.line, symbolLocation.character)
                );
            }
        }

        // Check for instance method call (object.methodName)
        const instanceMethodMatch = line.match(/(\w+)\.\s*(\w+)/);
        if (instanceMethodMatch && instanceMethodMatch[2] === methodName) {
            const objectName = instanceMethodMatch[1];
            const objectType = this.analyzer.getVariableType(objectName);
            if (objectType) {
                const symbolLocation = this.analyzer.getSymbolLocation(`${objectType}.${methodName}`, 'method');
                if (symbolLocation) {
                    return new vscode.Location(
                        document.uri,
                        new vscode.Position(symbolLocation.line, symbolLocation.character)
                    );
                }
            }
        }

        return null;
    }

    private findFieldDefinition(fieldName: string, line: string, document: vscode.TextDocument): vscode.Location | null {
        // Check for static field access (ClassName::fieldName)
        const staticFieldMatch = line.match(/(\w+)::\s*(\w+)/);
        if (staticFieldMatch && staticFieldMatch[2] === fieldName) {
            const className = staticFieldMatch[1];
            const symbolLocation = this.analyzer.getSymbolLocation(`${className}.${fieldName}`, 'field');
            if (symbolLocation) {
                return new vscode.Location(
                    document.uri,
                    new vscode.Position(symbolLocation.line, symbolLocation.character)
                );
            }
        }

        // Check for instance field access (object.fieldName)
        const instanceFieldMatch = line.match(/(\w+)\.\s*(\w+)/);
        if (instanceFieldMatch && instanceFieldMatch[2] === fieldName) {
            const objectName = instanceFieldMatch[1];
            const objectType = this.analyzer.getVariableType(objectName);
            if (objectType) {
                const symbolLocation = this.analyzer.getSymbolLocation(`${objectType}.${fieldName}`, 'field');
                if (symbolLocation) {
                    return new vscode.Location(
                        document.uri,
                        new vscode.Position(symbolLocation.line, symbolLocation.character)
                    );
                }
            }
        }

        return null;
    }

    private findVariableDefinition(variableName: string, document: vscode.TextDocument, currentPosition: vscode.Position): vscode.Location | null {
        const symbolLocation = this.analyzer.getSymbolLocation(variableName, 'variable');
        if (symbolLocation && symbolLocation.line < currentPosition.line) {
            return new vscode.Location(
                document.uri,
                new vscode.Position(symbolLocation.line, symbolLocation.character)
            );
        }
        return null;
    }

    private findConstructorDefinition(className: string, line: string, document: vscode.TextDocument): vscode.Location | null {
        // Check for 'new ClassName(' pattern
        const newMatch = line.match(/new\s+(\w+)\s*\(/);
        if (newMatch && newMatch[1] === className) {
            const symbolLocation = this.analyzer.getSymbolLocation(`${className}.constructor`, 'constructor');
            if (symbolLocation) {
                return new vscode.Location(
                    document.uri,
                    new vscode.Position(symbolLocation.line, symbolLocation.character)
                );
            }
        }
        return null;
    }

}