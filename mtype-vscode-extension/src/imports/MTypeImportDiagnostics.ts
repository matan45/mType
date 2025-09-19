import * as vscode from 'vscode';
import { MTypeImportResolver } from './MTypeImportResolver';

export class MTypeImportDiagnostics {
    private diagnostics = vscode.languages.createDiagnosticCollection('mtype-imports');
    private importResolver: MTypeImportResolver;

    constructor(importResolver: MTypeImportResolver) {
        this.importResolver = importResolver;
    }

    /**
     * Update diagnostics for a document
     */
    async updateDiagnostics(document: vscode.TextDocument): Promise<void> {
        if (document.languageId !== 'mtype') {
            return;
        }

        try {
            const diagnostics = await this.importResolver.validateImports(document);
            this.diagnostics.set(document.uri, diagnostics);
        } catch (error) {
            console.error('Error updating import diagnostics:', error);
            this.diagnostics.set(document.uri, []);
        }
    }

    /**
     * Clear diagnostics for a document
     */
    clearDiagnostics(document: vscode.TextDocument): void {
        this.diagnostics.delete(document.uri);
    }

    /**
     * Dispose of the diagnostic collection
     */
    dispose(): void {
        this.diagnostics.dispose();
    }
}