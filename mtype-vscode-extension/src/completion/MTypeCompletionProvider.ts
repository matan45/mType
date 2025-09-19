import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';
import { MTypeKeywords } from './MTypeKeywords';
import { MTypeContextAnalyzer } from './MTypeContextAnalyzer';
import { MTypeScopeAnalyzer, VariableInfo, MethodInfo, Visibility } from '../analysis/MTypeScopeAnalyzer';
import { MTypeImportedSymbolProvider } from '../imports/MTypeImportCompletionProvider';

export class MTypeCompletionProvider implements vscode.CompletionItemProvider {
    private analyzer = new MTypeCodeAnalyzer();
    private scopeAnalyzer: MTypeScopeAnalyzer | null = null;
    private importedSymbolProvider: MTypeImportedSymbolProvider | null = null;

    setImportedSymbolProvider(provider: MTypeImportedSymbolProvider): void {
        this.importedSymbolProvider = provider;
    }

    async provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
    ): Promise<vscode.CompletionItem[]> {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

        // Initialize scope analyzer for this document
        this.scopeAnalyzer = new MTypeScopeAnalyzer(document);
        this.scopeAnalyzer.analyzeDocument();

        const lineText = document.lineAt(position).text.substring(0, position.character);
        const completionItems: vscode.CompletionItem[] = [];

        // Check for specific completion triggers first
        const triggerContext = MTypeContextAnalyzer.getCompletionTriggerContext(lineText);


        // Handle static member access (ClassName::)
        if (triggerContext === 'static-member') {
            const staticMatch = lineText.match(/(\w+)::\s*$/);
            if (staticMatch) {
                const className = staticMatch[1];
                console.log('Static member access for class:', className);
                // Ensure imports are analyzed for static access
                if (this.importedSymbolProvider) {
                    await this.importedSymbolProvider.analyzeDocumentImports(document);
                }
                const staticCompletions = await this.getStaticMemberCompletions(className, document);
                console.log('Found static completions:', staticCompletions.length);
                // Return ONLY static member completions, no other suggestions
                return staticCompletions;
            }
        }
        // Handle instance member access (object.)
        else if (triggerContext === 'instance-member') {
            const dotMatch = lineText.match(/(\w+)\.\s*$/);
            if (dotMatch) {
                const objectName = dotMatch[1];
                console.log('Instance member access for object:', objectName);
                // Ensure imports are analyzed for member access
                if (this.importedSymbolProvider) {
                    await this.importedSymbolProvider.analyzeDocumentImports(document);
                }
                const instanceCompletions = await this.getScopeAwareInstanceCompletions(objectName, document, position);
                console.log('Found instance completions:', instanceCompletions.length);
                // Return ONLY instance member completions, no other suggestions
                return instanceCompletions;
            }
        }
        // Handle generic type parameters
        else if (triggerContext === 'generic-parameter') {
            const typeCompletions = this.getTypeCompletions();
            completionItems.push(...typeCompletions);
        }
        // Handle general keyword completion
        else {
            const contexts = MTypeContextAnalyzer.analyzeContext(document, position);

            // Always ensure we have at least global context for fallback
            if (contexts.length === 0) {
                contexts.push('global');
            }

            const keywordCompletions = this.getKeywordCompletions(contexts, lineText);
            completionItems.push(...keywordCompletions);

            // Add scope-aware variable completions
            const variableCompletions = this.getScopeAwareVariableCompletions(document, position);
            completionItems.push(...variableCompletions);

            // Add imported symbol completions
            if (this.importedSymbolProvider) {
                // Ensure imports are analyzed before getting completions
                await this.importedSymbolProvider.analyzeDocumentImports(document);
                const importedCompletions = this.importedSymbolProvider.getImportedSymbolCompletions(document);
                console.log('Adding imported completions:', importedCompletions.length);
                completionItems.push(...importedCompletions);
            }

            // Add class and variable completions for non-trigger contexts
            const symbolCompletions = this.getSymbolCompletions(contexts, lineText);
            completionItems.push(...symbolCompletions);
        }

        return completionItems;
    }

    private async getStaticMemberCompletions(className: string, document: vscode.TextDocument): Promise<vscode.CompletionItem[]> {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return this.getFallbackStaticCompletions(className);
        }

        const classInfo = this.scopeAnalyzer.getClassInfo(className);
        if (!classInfo) {
            // Class not found locally, check if it's an imported class
            if (this.importedSymbolProvider) {
                console.log('Class not found locally, checking imports for:', className);
                const importedStaticMembers = await this.getImportedStaticMembers(className, document);
                if (importedStaticMembers.length > 0) {
                    console.log('Found imported static members:', importedStaticMembers.length);
                    return importedStaticMembers;
                }
            }
            return completionItems;
        }

        // Add static methods
        for (const method of classInfo.staticMethods.values()) {
            const item = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
            const paramStr = method.parameters.map(p => `${p.type} ${p.name}`).join(', ');
            item.detail = `static ${method.returnType} ${method.name}(${paramStr})`;
            item.documentation = new vscode.MarkdownString(
                `**Static Method**\n\nReturns: \`${method.returnType}\`\n\nParameters: ${paramStr || 'none'}`
            );

            // Create snippet for parameters
            if (method.parameters.length > 0) {
                const snippetParams = method.parameters.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ');
                item.insertText = new vscode.SnippetString(`${method.name}(${snippetParams})`);
            } else {
                item.insertText = new vscode.SnippetString(`${method.name}()`);
            }

            item.sortText = '0' + method.name;
            completionItems.push(item);
        }

        // Add static fields
        for (const field of classInfo.staticFields.values()) {
            const item = new vscode.CompletionItem(field.name, vscode.CompletionItemKind.Field);
            const modifiers = [
                field.visibility === Visibility.Private ? 'private' : '',
                'static',
                field.isFinal ? 'final' : ''
            ].filter(Boolean).join(' ');

            item.detail = `${modifiers} ${field.type} ${field.name}`;
            item.documentation = new vscode.MarkdownString(
                `**Static Field**\n\nType: \`${field.type}\`\n\nModifiers: ${modifiers}`
            );
            item.sortText = '1' + field.name;
            completionItems.push(item);
        }

        return completionItems;
    }

    private getFallbackStaticCompletions(className: string): vscode.CompletionItem[] {
        // Fallback to old analyzer if scope analyzer not available
        const completionItems: vscode.CompletionItem[] = [];

        if (this.analyzer.classes.has(className)) {
            const classInfo = this.analyzer.classes.get(className)!;
            const staticMembers = classInfo.members.filter((m: any) => m.isStatic);

            staticMembers.forEach((member: any) => {
                if (member.type === 'method') {
                    const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Method);
                    const paramStr = member.parameters && member.parameters.length > 0
                        ? member.parameters.join(', ')
                        : '';
                    item.detail = `static ${member.returnType} ${member.name}(${paramStr})`;
                    item.documentation = `Static method returning ${member.returnType}`;
                    item.insertText = new vscode.SnippetString(
                        member.parameters && member.parameters.length > 0
                            ? `${member.name}($1)`
                            : `${member.name}()`
                    );
                    item.sortText = '0' + member.name;
                    completionItems.push(item);
                } else if (member.type === 'field') {
                    const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Field);
                    item.detail = `static ${member.returnType} ${member.name}`;
                    item.documentation = `Static field of type ${member.returnType}`;
                    item.sortText = '1' + member.name;
                    completionItems.push(item);
                }
            });
        }

        return completionItems;
    }


    private getKeywordCompletions(contexts: string[], lineText: string): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        // Get keywords based on current contexts
        for (const context of contexts) {
            const contextKeywords = MTypeKeywords.getKeywordsByContext(context);
            const keywordItems = MTypeKeywords.toCompletionItems(contextKeywords);
            completionItems.push(...keywordItems);
        }

        // Add type keywords if we're in a type context or global function
        if (contexts.includes('type-context') || contexts.includes('return-type') || contexts.includes('global-function')) {
            const typeKeywords = MTypeKeywords.getTypeKeywords();
            const typeItems = MTypeKeywords.toCompletionItems(typeKeywords);
            completionItems.push(...typeItems);
        }

        // Remove duplicates by keyword name
        const uniqueItems = new Map<string, vscode.CompletionItem>();
        completionItems.forEach(item => {
            if (!uniqueItems.has(item.label as string)) {
                uniqueItems.set(item.label as string, item);
            }
        });

        return Array.from(uniqueItems.values());
    }

    private getTypeCompletions(): vscode.CompletionItem[] {
        const typeKeywords = MTypeKeywords.getTypeKeywords();
        return MTypeKeywords.toCompletionItems(typeKeywords);
    }

    private getSymbolCompletions(contexts: string[], lineText: string): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        // Add class names in appropriate contexts
        if (contexts.includes('type-context') || contexts.includes('expression') || contexts.includes('global-function')) {
            for (const [className, classInfo] of this.analyzer.classes.entries()) {
                const item = new vscode.CompletionItem(className, vscode.CompletionItemKind.Class);
                item.detail = `class ${className}`;
                item.documentation = `User-defined class`;
                item.sortText = '2' + className; // Lower priority than keywords
                completionItems.push(item);
            }
        }

        // Add variable names in expression contexts
        if (contexts.includes('expression')) {
            const variableType = this.analyzer.getVariableType.bind(this.analyzer);
            // This is a simplified approach - in practice you'd want to track all variables
        }

        return completionItems;
    }

    private getScopeAwareVariableCompletions(document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return completionItems;
        }

        // Get all variables visible at current position
        const visibleVariables = this.scopeAnalyzer.getVisibleVariables(position);

        for (const variable of visibleVariables) {
            const item = new vscode.CompletionItem(variable.name, vscode.CompletionItemKind.Variable);

            // Build detailed information
            const modifiers = [
                variable.visibility === Visibility.Private ? 'private' : '',
                variable.isStatic ? 'static' : '',
                variable.isFinal ? 'final' : ''
            ].filter(Boolean);

            const modifierStr = modifiers.length > 0 ? modifiers.join(' ') + ' ' : '';
            item.detail = `${modifierStr}${variable.type} ${variable.name}`;

            // Determine scope description
            let scopeDesc = '';
            switch (variable.scope.type) {
                case 'global':
                    scopeDesc = 'Global variable';
                    break;
                case 'method':
                case 'function':
                    scopeDesc = variable.isParameter ? 'Parameter' : 'Local variable';
                    break;
                case 'class':
                    scopeDesc = variable.isStatic ? 'Static field' : 'Instance field';
                    break;
                default:
                    scopeDesc = 'Variable';
            }

            item.documentation = new vscode.MarkdownString(
                `**${scopeDesc}**\n\nType: \`${variable.type}\`\n\nScope: ${variable.scope.name || variable.scope.type}\n\nModifiers: ${modifierStr || 'none'}`
            );

            // Set priority based on scope proximity
            let priority = 5;
            if (variable.isParameter) priority = 9; // Parameters highest priority
            else if (variable.scope.type === 'method' || variable.scope.type === 'function') priority = 8; // Local vars
            else if (variable.scope.type === 'class') priority = 7; // Class members
            else if (variable.scope.type === 'global') priority = 6; // Global vars

            item.sortText = `${10 - priority}_${variable.name}`;
            completionItems.push(item);
        }

        return completionItems;
    }

    private async getScopeAwareInstanceCompletions(objectName: string, document: vscode.TextDocument, position: vscode.Position): Promise<vscode.CompletionItem[]> {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return this.getFallbackInstanceCompletions(objectName);
        }

        // First, find the variable to get its type
        const visibleVariables = this.scopeAnalyzer.getVisibleVariables(position);
        let objectVariable = visibleVariables.find(v => v.name === objectName);

        console.log('Looking for object variable:', objectName);
        console.log('Visible variables:', visibleVariables.map(v => `${v.name}:${v.type}`));
        console.log('Found object variable:', objectVariable ? `${objectVariable.name}:${objectVariable.type}` : 'not found');

        // If not found in local scope, check if it's an imported class instance
        if (!objectVariable && this.importedSymbolProvider) {
            const importedSymbols = this.importedSymbolProvider.getImportedSymbols(document);
            for (const importInfo of importedSymbols) {
                for (const symbol of importInfo.exportedSymbols) {
                    if (symbol.type === 'class' && symbol.name === objectName) {
                        // This is a direct class reference (like Rectangle.method)
                        const importedMembers = await this.getImportedClassMembers(symbol.name, importInfo.resolvedPath);
                        return importedMembers;
                    }
                }
            }

        }

        if (!objectVariable) {
            return completionItems;
        }

        // Get accessible members for the object's type
        let accessibleMembers = this.scopeAnalyzer.getAccessibleMembers(objectVariable.type, position, false);

        // If no members found locally, check if it's an imported class type
        if (accessibleMembers.length === 0 && this.importedSymbolProvider) {
            console.log('No local members found, checking if type is imported:', objectVariable.type);
            const importedMembers = await this.getImportedClassMembers(objectVariable.type, null);
            console.log('Found imported members:', importedMembers.length);
            if (importedMembers.length > 0) {
                return importedMembers;
            }
        }

        for (const member of accessibleMembers) {
            if ('parameters' in member) {
                // It's a method
                const method = member as MethodInfo;

                // Skip constructor from member access completions
                if (method.name === 'constructor') {
                    continue;
                }

                const item = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
                const paramStr = method.parameters.map(p => `${p.type} ${p.name}`).join(', ');

                const modifiers = [
                    method.visibility === Visibility.Private ? 'private' : '',
                    method.isStatic ? 'static' : ''
                ].filter(Boolean).join(' ');

                item.detail = `${modifiers} ${method.returnType} ${method.name}(${paramStr})`;
                item.documentation = new vscode.MarkdownString(
                    `**Instance Method**\n\nReturns: \`${method.returnType}\`\n\nParameters: ${paramStr || 'none'}\n\nVisibility: ${method.visibility}`
                );

                // Create snippet for parameters
                if (method.parameters.length > 0) {
                    const snippetParams = method.parameters.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ');
                    item.insertText = new vscode.SnippetString(`${method.name}(${snippetParams})`);
                } else {
                    item.insertText = new vscode.SnippetString(`${method.name}()`);
                }

                item.sortText = '0' + method.name;
                completionItems.push(item);
            } else {
                // It's a field
                const field = member as VariableInfo;
                const item = new vscode.CompletionItem(field.name, vscode.CompletionItemKind.Field);

                const modifiers = [
                    field.visibility === Visibility.Private ? 'private' : '',
                    field.isStatic ? 'static' : '',
                    field.isFinal ? 'final' : ''
                ].filter(Boolean).join(' ');

                item.detail = `${modifiers} ${field.type} ${field.name}`;
                item.documentation = new vscode.MarkdownString(
                    `**Instance Field**\n\nType: \`${field.type}\`\n\nModifiers: ${modifiers}\n\nVisibility: ${field.visibility}`
                );
                item.sortText = '1' + field.name;
                completionItems.push(item);
            }
        }

        return completionItems;
    }

    /**
     * Get members of an imported class
     */
    private async getImportedClassMembers(className: string, filePath: string | null): Promise<vscode.CompletionItem[]> {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.importedSymbolProvider) {
            return completionItems;
        }

        try {
            // If we have a specific file path, analyze that file
            if (filePath) {
                const fileUri = vscode.Uri.file(filePath);
                const document = await vscode.workspace.openTextDocument(fileUri);
                const classMembers = this.parseClassMembersFromDocument(className, document);
                return classMembers;
            }

            // Otherwise, search through all imported files
            const imports = this.importedSymbolProvider.getImportedSymbols(vscode.window.activeTextEditor?.document!);
            for (const importInfo of imports) {
                for (const symbol of importInfo.exportedSymbols) {
                    if (symbol.type === 'class' && symbol.name === className) {
                        const fileUri = vscode.Uri.file(importInfo.resolvedPath);
                        const document = await vscode.workspace.openTextDocument(fileUri);
                        return this.parseClassMembersFromDocument(className, document);
                    }
                }
            }
        } catch (error) {
            console.error('Error getting imported class members:', error);
        }

        return completionItems;
    }

    /**
     * Parse class members from a document
     */
    private parseClassMembersFromDocument(className: string, document: vscode.TextDocument): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        let inClass = false;
        let braceCount = 0;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Check if we're entering the target class
            const classMatch = line.match(new RegExp(`^\\s*class\\s+${className}\\s*\\{`));
            if (classMatch) {
                inClass = true;
                braceCount = 1;
                continue;
            }

            if (inClass) {
                const openBraces = (line.match(/\{/g) || []).length;
                const closeBraces = (line.match(/\}/g) || []).length;
                braceCount += openBraces - closeBraces;

                if (braceCount <= 0) {
                    // End of class
                    break;
                }

                // Parse methods - exclude static methods for instance completion
                const methodMatch = line.match(/^\s*(private\s+)?(static\s+)?function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)/);
                if (methodMatch) {
                    const isPrivate = !!methodMatch[1];
                    const isStatic = !!methodMatch[2];
                    const methodName = methodMatch[3];
                    const parameters = methodMatch[4];
                    const returnType = methodMatch[5];

                    // Only include non-static methods for instance completion
                    if (!isStatic && methodName !== 'constructor') {
                        const item = new vscode.CompletionItem(methodName, vscode.CompletionItemKind.Method);
                        item.detail = `${returnType} ${methodName}(${parameters})`;
                        item.documentation = `Imported method from ${className}`;

                        // Create snippet
                        if (parameters.trim()) {
                            const paramCount = parameters.split(',').length;
                            const snippetParams = Array.from({ length: paramCount }, (_, i) => `\${${i + 1}:arg${i + 1}}`).join(', ');
                            item.insertText = new vscode.SnippetString(`${methodName}(${snippetParams})`);
                        } else {
                            item.insertText = new vscode.SnippetString(`${methodName}()`);
                        }

                        item.sortText = '0' + methodName;
                        completionItems.push(item);
                    }
                }

                // Parse fields - exclude static fields for instance completion
                const fieldMatch = line.match(/^\s*(private\s+)?(static\s+)?(final\s+)?(\w+)\s+(\w+)\s*[=;]?/);
                if (fieldMatch) {
                    const isPrivate = !!fieldMatch[1];
                    const isStatic = !!fieldMatch[2];
                    const isFinal = !!fieldMatch[3];
                    const fieldType = fieldMatch[4];
                    const fieldName = fieldMatch[5];

                    // Skip non-field lines and static fields for instance completion
                    const trimmedLine = line.trim();
                    const isReturnStatement = trimmedLine.startsWith('return ');
                    const isControlFlow = trimmedLine.startsWith('if ') || trimmedLine.startsWith('while ') || trimmedLine.startsWith('for ');
                    const isKeywordType = ['return', 'if', 'while', 'for', 'else', 'switch', 'case', 'break', 'continue'].includes(fieldType);

                    if (!line.includes('(') &&
                        !line.includes('function') &&
                        !isStatic &&
                        !isReturnStatement &&
                        !isControlFlow &&
                        !isKeywordType) {

                        const item = new vscode.CompletionItem(fieldName, vscode.CompletionItemKind.Field);
                        item.detail = `${fieldType} ${fieldName}`;
                        item.documentation = `Imported field from ${className}`;
                        item.sortText = '1' + fieldName;
                        completionItems.push(item);
                    }
                }
            }
        }

        return completionItems;
    }

    /**
     * Get static members of an imported class
     */
    private async getImportedStaticMembers(className: string, document: vscode.TextDocument): Promise<vscode.CompletionItem[]> {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.importedSymbolProvider) {
            return completionItems;
        }

        try {
            const imports = this.importedSymbolProvider.getImportedSymbols(document);
            for (const importInfo of imports) {
                for (const symbol of importInfo.exportedSymbols) {
                    if (symbol.type === 'class' && symbol.name === className) {
                        const fileUri = vscode.Uri.file(importInfo.resolvedPath);
                        const importedDocument = await vscode.workspace.openTextDocument(fileUri);
                        return this.parseStaticMembersFromDocument(className, importedDocument);
                    }
                }
            }
        } catch (error) {
            console.error('Error getting imported static members:', error);
        }

        return completionItems;
    }

    /**
     * Parse static members from a document
     */
    private parseStaticMembersFromDocument(className: string, document: vscode.TextDocument): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];
        const text = document.getText();
        const lines = text.split('\n');

        let inClass = false;
        let braceCount = 0;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Check if we're entering the target class
            const classMatch = line.match(new RegExp(`^\\s*class\\s+${className}\\s*\\{`));
            if (classMatch) {
                inClass = true;
                braceCount = 1;
                continue;
            }

            if (inClass) {
                const openBraces = (line.match(/\{/g) || []).length;
                const closeBraces = (line.match(/\}/g) || []).length;
                braceCount += openBraces - closeBraces;

                if (braceCount <= 0) {
                    // End of class
                    break;
                }

                // Parse static methods
                const staticMethodMatch = line.match(/^\s*static\s+function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)/);
                if (staticMethodMatch) {
                    const methodName = staticMethodMatch[1];
                    const parameters = staticMethodMatch[2];
                    const returnType = staticMethodMatch[3];

                    const item = new vscode.CompletionItem(methodName, vscode.CompletionItemKind.Method);
                    item.detail = `static ${returnType} ${methodName}(${parameters})`;
                    item.documentation = `Static method from imported class ${className}`;

                    // Create snippet
                    if (parameters.trim()) {
                        const paramCount = parameters.split(',').length;
                        const snippetParams = Array.from({ length: paramCount }, (_, i) => `\${${i + 1}:arg${i + 1}}`).join(', ');
                        item.insertText = new vscode.SnippetString(`${methodName}(${snippetParams})`);
                    } else {
                        item.insertText = new vscode.SnippetString(`${methodName}()`);
                    }

                    item.sortText = '0' + methodName;
                    completionItems.push(item);
                }

                // Parse static fields
                const staticFieldMatch = line.match(/^\s*static\s+(?:final\s+)?(\w+)\s+(\w+)\s*;?/);
                if (staticFieldMatch) {
                    const fieldType = staticFieldMatch[1];
                    const fieldName = staticFieldMatch[2];

                    // Skip if it looks like a method
                    if (!line.includes('(') && !line.includes('function')) {
                        const item = new vscode.CompletionItem(fieldName, vscode.CompletionItemKind.Field);
                        item.detail = `static ${fieldType} ${fieldName}`;
                        item.documentation = `Static field from imported class ${className}`;
                        item.sortText = '1' + fieldName;
                        completionItems.push(item);
                    }
                }
            }
        }

        return completionItems;
    }


    private getFallbackInstanceCompletions(objectName: string): vscode.CompletionItem[] {
        // Fallback to old analyzer
        const completionItems: vscode.CompletionItem[] = [];
        const members = this.analyzer.getObjectMembers(objectName);

        members.forEach(member => {
            if (member.type === 'method') {
                const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Method);
                const paramStr = member.parameters && member.parameters.length > 0
                    ? member.parameters.join(', ')
                    : '';
                item.detail = `${member.returnType} ${member.name}(${paramStr})`;
                item.documentation = `Instance method returning ${member.returnType}`;
                item.insertText = new vscode.SnippetString(
                    member.parameters && member.parameters.length > 0
                        ? `${member.name}($1)`
                        : `${member.name}()`
                );
                item.sortText = '0' + member.name;
                completionItems.push(item);
            } else if (member.type === 'field') {
                const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Field);
                item.detail = `${member.returnType} ${member.name}`;
                item.documentation = `Instance field of type ${member.returnType}`;
                item.sortText = '1' + member.name;
                completionItems.push(item);
            }
        });

        return completionItems;
    }
}