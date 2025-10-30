import * as vscode from 'vscode';
import { MTypeScopeAnalyzer } from '../analysis/MTypeScopeAnalyzer';
import { MTypeReferenceProvider } from '../references/MTypeReferenceProvider';

/**
 * Provides code lenses (inline information) for mType code
 * Shows reference counts above functions, classes, and methods
 */
export class MTypeCodeLensProvider implements vscode.CodeLensProvider {
    private referenceProvider: MTypeReferenceProvider;
    private _onDidChangeCodeLenses: vscode.EventEmitter<void> = new vscode.EventEmitter<void>();
    public readonly onDidChangeCodeLenses: vscode.Event<void> = this._onDidChangeCodeLenses.event;

    constructor(referenceProvider: MTypeReferenceProvider) {
        this.referenceProvider = referenceProvider;
    }

    /**
     * Trigger a refresh of code lenses
     */
    refresh(): void {
        this._onDidChangeCodeLenses.fire();
    }

    async provideCodeLenses(
        document: vscode.TextDocument,
        token: vscode.CancellationToken
    ): Promise<vscode.CodeLens[]> {
        const codeLenses: vscode.CodeLens[] = [];

        // Analyze document to find classes, functions, and methods
        const scopeAnalyzer = new MTypeScopeAnalyzer(document);
        scopeAnalyzer.analyzeDocument();

        // Add code lenses for classes
        const classes = scopeAnalyzer.getAllClasses();
        for (const [className, classInfo] of classes) {
            // Find the position of the class name on the line
            const line = document.lineAt(classInfo.scope.startLine).text;
            const classNameIndex = line.indexOf(className);

            if (classNameIndex !== -1) {
                const range = new vscode.Range(
                    classInfo.scope.startLine,
                    classNameIndex,
                    classInfo.scope.startLine,
                    classNameIndex + className.length
                );

                // Add reference count lens for class - store document URI in data
                const classLens = new vscode.CodeLens(range);
                (classLens as any).documentUri = document.uri;
                codeLenses.push(classLens);
            }

            // Add code lenses for methods in the class
            for (const [methodName, methodInfo] of classInfo.methods) {
                const methodRange = new vscode.Range(
                    methodInfo.declarationLocation,
                    methodInfo.declarationLocation
                );

                const methodLens = new vscode.CodeLens(methodRange);
                (methodLens as any).documentUri = document.uri;
                codeLenses.push(methodLens);
            }

            // Add code lenses for static methods
            for (const [methodName, methodInfo] of classInfo.staticMethods) {
                const methodRange = new vscode.Range(
                    methodInfo.declarationLocation,
                    methodInfo.declarationLocation
                );

                const methodLens = new vscode.CodeLens(methodRange);
                (methodLens as any).documentUri = document.uri;
                codeLenses.push(methodLens);
            }
        }

        // Add code lenses for global functions (if any)
        // This would require global function tracking in scope analyzer
        // For now, we focus on classes and methods

        return codeLenses;
    }

    async resolveCodeLens(
        codeLens: vscode.CodeLens,
        token: vscode.CancellationToken
    ): Promise<vscode.CodeLens> {
        try {
            // Get the document URI that was stored when the code lens was created
            const uri = (codeLens as any).documentUri as vscode.Uri;
            if (!uri) {
                codeLens.command = {
                    title: 'References unavailable',
                    command: ''
                };
                return codeLens;
            }

            const document = await vscode.workspace.openTextDocument(uri);
            const position = codeLens.range.start;

            // Find all references to the symbol at this position
            const references = await this.referenceProvider.provideReferences(
                document,
                position,
                { includeDeclaration: false },
                token
            );

            const refCount = references ? references.length : 0;

            // Update the code lens with the reference count
            codeLens.command = {
                title: refCount === 1 ? '1 reference' : `${refCount} references`,
                command: 'editor.action.showReferences',
                arguments: [uri, position, references || []]
            };

            // Add a visual indicator if no references
            if (refCount === 0) {
                codeLens.command.title = '0 references';
            }
        } catch (error) {
            console.error('Error resolving code lens:', error);
            codeLens.command = {
                title: 'Error loading references',
                command: ''
            };
        }

        return codeLens;
    }
}
