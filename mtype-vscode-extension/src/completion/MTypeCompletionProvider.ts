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
        position: vscode.Position,
        token: vscode.CancellationToken,
        context: vscode.CompletionContext
    ): Promise<vscode.CompletionList | vscode.CompletionItem[]> {
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
                // Ensure imports are analyzed for static access
                if (this.importedSymbolProvider) {
                    await this.importedSymbolProvider.analyzeDocumentImports(document);
                }
                const staticCompletions = await this.getStaticMemberCompletions(className, document);
                // Return as CompletionList with incomplete=false to suppress other suggestions
                return new vscode.CompletionList(staticCompletions, false);
            }
        }
        // Handle instance member access (object. or this.)
        else if (triggerContext === 'instance-member') {
            // Check for "this." specifically
            const thisMatch = lineText.match(/this\.\s*$/);
            if (thisMatch) {
                // Get current class context for "this."
                const thisCompletions = await this.getThisCompletions(document, position);
                // Return as CompletionList with incomplete=false to suppress other suggestions
                return new vscode.CompletionList(thisCompletions, false);
            }

            // Regular object member access
            const dotMatch = lineText.match(/(\w+)\.\s*$/);
            if (dotMatch) {
                const objectName = dotMatch[1];

                // First, check if this is a collection by looking for the variable declaration in the current document
                const collectionType = this.findVariableTypeInDocument(document, objectName);
                if (collectionType && this.isCollectionType(collectionType)) {
                    const collectionMethods = this.getCollectionMethodCompletions(collectionType);
                    // Return as CompletionList with incomplete=false to suppress other suggestions
                    return new vscode.CompletionList(collectionMethods, false);
                }

                // Ensure imports are analyzed for member access
                if (this.importedSymbolProvider) {
                    await this.importedSymbolProvider.analyzeDocumentImports(document);
                }
                const instanceCompletions = await this.getScopeAwareInstanceCompletions(objectName, document, position);
                // Return as CompletionList with incomplete=false to suppress other suggestions
                return new vscode.CompletionList(instanceCompletions, false);
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
                const importedStaticMembers = await this.getImportedStaticMembers(className, document);
                if (importedStaticMembers.length > 0) {
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

        // Check if it's a collection type first
        if (this.isCollectionType(objectVariable.type)) {
            return this.getCollectionMethodCompletions(objectVariable.type);
        }

        // Get accessible members for the object's type
        let accessibleMembers = this.scopeAnalyzer.getAccessibleMembers(objectVariable.type, position, false);

        // If no members found locally, check if it's an imported class type
        if (accessibleMembers.length === 0 && this.importedSymbolProvider) {
            const importedMembers = await this.getImportedClassMembers(objectVariable.type, null);
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

    /**
     * Check if a type is a collection type
     */
    private isCollectionType(typeName: string): boolean {
        return typeName.startsWith('Array<') ||
               typeName.startsWith('Map<') ||
               typeName.startsWith('Set<') ||
               typeName.startsWith('Stack<') ||
               typeName.startsWith('Queue<');
    }

    /**
     * Get completion items for collection methods based on the actual C++ interpreter implementation
     */
    private getCollectionMethodCompletions(typeName: string): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        // Add common collection methods (available to all collections per ObjectEvaluator.cpp)
        completionItems.push(
            this.createMethodCompletion('size', 'int size()', 'Returns the number of elements in the collection', []),
            this.createMethodCompletion('empty', 'bool empty()', 'Returns true if the collection is empty', []),
            this.createMethodCompletion('clear', 'void clear()', 'Removes all elements from the collection', [])
        );

        // Add specific methods based on collection type (matching ObjectEvaluator.cpp implementation)
        if (typeName.startsWith('Array<')) {
            const elementType = this.extractGenericType(typeName);
            completionItems.push(
                this.createMethodCompletion('get', `${elementType} get(int index)`, 'Gets the element at the specified index', ['int index']),
                this.createMethodCompletion('set', 'void set(int index, value)', 'Sets the element at the specified index', ['int index', `${elementType} value`]),
                this.createMethodCompletion('add', `void add(${elementType} item)`, 'Adds an element to the end of the array', [`${elementType} item`]),
                this.createMethodCompletion('push', `void push(${elementType} item)`, 'Adds an element to the end of the array (alias for add)', [`${elementType} item`]),
                this.createMethodCompletion('removeAt', 'void removeAt(int index)', 'Removes the element at the specified index', ['int index'])
            );
        } else if (typeName.startsWith('Map<')) {
            const [keyType, valueType] = this.extractMapTypes(typeName);
            completionItems.push(
                this.createMethodCompletion('get', `${valueType} get(key)`, 'Gets the value associated with the specified key', [`${keyType} key`]),
                this.createMethodCompletion('put', 'void put(key, value)', 'Associates the specified value with the specified key', [`${keyType} key`, `${valueType} value`]),
                this.createMethodCompletion('containsKey', 'bool containsKey(key)', 'Returns true if the map contains the specified key', [`${keyType} key`]),
                this.createMethodCompletion('keySet', 'keySet()', 'Returns a set of all keys in the map', []),
                this.createMethodCompletion('remove', 'void remove(key)', 'Removes the key-value pair for the specified key', [`${keyType} key`])
            );
        } else if (typeName.startsWith('Set<')) {
            const elementType = this.extractGenericType(typeName);
            completionItems.push(
                this.createMethodCompletion('add', `bool add(${elementType} item)`, 'Adds the specified element to the set, returns true if added', [`${elementType} item`]),
                this.createMethodCompletion('contains', `bool contains(${elementType} item)`, 'Returns true if the set contains the specified element', [`${elementType} item`]),
                this.createMethodCompletion('remove', `bool remove(${elementType} item)`, 'Removes the specified element from the set, returns true if removed', [`${elementType} item`])
            );
        } else if (typeName.startsWith('Stack<')) {
            const elementType = this.extractGenericType(typeName);
            completionItems.push(
                this.createMethodCompletion('push', `void push(${elementType} item)`, 'Pushes an element onto the top of the stack', [`${elementType} item`]),
                this.createMethodCompletion('pop', `${elementType} pop()`, 'Removes and returns the top element of the stack', []),
                this.createMethodCompletion('top', `${elementType} top()`, 'Returns the top element without removing it', [])
            );
        } else if (typeName.startsWith('Queue<')) {
            const elementType = this.extractGenericType(typeName);
            completionItems.push(
                this.createMethodCompletion('enqueue', `void enqueue(${elementType} item)`, 'Adds an element to the rear of the queue', [`${elementType} item`]),
                this.createMethodCompletion('dequeue', `${elementType} dequeue()`, 'Removes and returns the front element of the queue', []),
                this.createMethodCompletion('front', `${elementType} front()`, 'Returns the front element without removing it', [])
            );
        }

        return completionItems;
    }

    /**
     * Create a method completion item
     */
    private createMethodCompletion(name: string, signature: string, documentation: string, parameters: string[]): vscode.CompletionItem {
        const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Method);
        item.detail = signature;
        item.documentation = new vscode.MarkdownString(`**Collection Method**\n\n${documentation}`);

        // Create snippet for parameters
        if (parameters.length > 0) {
            const snippetParams = parameters.map((param, index) => `\${${index + 1}:${param.split(' ')[1] || param}}`).join(', ');
            item.insertText = new vscode.SnippetString(`${name}(${snippetParams})`);
        } else {
            item.insertText = new vscode.SnippetString(`${name}()`);
        }

        item.sortText = '0' + name; // High priority for collection methods
        return item;
    }

    /**
     * Extract the generic type from a single-parameter generic type (Array<T>, Set<T>, etc.)
     */
    private extractGenericType(typeName: string): string {
        const start = typeName.indexOf('<') + 1;
        const end = typeName.lastIndexOf('>');
        if (start > 0 && end > start) {
            return typeName.substring(start, end);
        }
        return 'T'; // fallback
    }

    /**
     * Extract key and value types from Map<K,V>
     */
    private extractMapTypes(typeName: string): [string, string] {
        const start = typeName.indexOf('<') + 1;
        const end = typeName.lastIndexOf('>');
        if (start > 0 && end > start) {
            const typeParams = typeName.substring(start, end);
            const commaIndex = typeParams.indexOf(',');
            if (commaIndex > 0) {
                const keyType = typeParams.substring(0, commaIndex).trim();
                const valueType = typeParams.substring(commaIndex + 1).trim();
                return [keyType, valueType];
            }
        }
        return ['K', 'V']; // fallback
    }

    /**
     * Simple direct search for variable type in document text
     * This bypasses the complex scope analysis for collection detection
     */
    private findVariableTypeInDocument(document: vscode.TextDocument, variableName: string): string | null {
        const text = document.getText();
        const lines = text.split('\n');

        // Look for variable declarations with the pattern: Type variableName = new Type(...)
        for (const line of lines) {
            // Match generic types like Array<int> variableName = new Array<int>();
            const genericMatch = line.match(new RegExp(`([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\\s+${variableName}\\s*=`));
            if (genericMatch) {
                return genericMatch[1];
            }

            // Also check for simple types: int variableName = ...
            const simpleMatch = line.match(new RegExp(`([A-Za-z_][A-Za-z0-9_]*)\\s+${variableName}\\s*=`));
            if (simpleMatch) {
                return simpleMatch[1];
            }
        }

        return null;
    }

    /**
     * Get completions for "this." - shows instance members of the current class
     */
    private async getThisCompletions(document: vscode.TextDocument, position: vscode.Position): Promise<vscode.CompletionItem[]> {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return completionItems;
        }

        // Find the enclosing class at the current position
        const currentClassName = this.findEnclosingClass(document, position);
        if (!currentClassName) {
            return completionItems;
        }

        // Get class info
        const classInfo = this.scopeAnalyzer.getClassInfo(currentClassName);
        if (!classInfo) {
            return completionItems;
        }

        // Add instance methods (non-static methods)
        for (const method of classInfo.methods.values()) {
            if (method.isStatic) continue; // Skip static methods for "this."

            const item = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
            const paramStr = method.parameters.map(p => `${p.type} ${p.name}`).join(', ');

            const modifiers = [
                method.visibility === Visibility.Private ? 'private' : ''
            ].filter(Boolean).join(' ');

            item.detail = `${modifiers} ${method.returnType} ${method.name}(${paramStr})`;
            item.documentation = new vscode.MarkdownString(
                `**Instance Method**\n\nReturns: \`${method.returnType}\`\n\nParameters: ${paramStr || 'none'}`
            );

            // Create snippet for parameters
            if (method.parameters.length > 0) {
                const snippetParams = method.parameters.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ');
                item.insertText = new vscode.SnippetString(`${method.name}(${snippetParams})`);
            } else {
                item.insertText = new vscode.SnippetString(`${method.name}()`);
            }

            // Use very high priority sort text to appear at top
            item.sortText = '!0_' + method.name;
            item.filterText = method.name;
            completionItems.push(item);
        }

        // Add instance fields (non-static fields)
        for (const field of classInfo.fields.values()) {
            if (field.isStatic) continue; // Skip static fields for "this."

            const item = new vscode.CompletionItem(field.name, vscode.CompletionItemKind.Field);

            const modifiers = [
                field.visibility === Visibility.Private ? 'private' : '',
                field.isFinal ? 'final' : ''
            ].filter(Boolean).join(' ');

            item.detail = `${modifiers} ${field.type} ${field.name}`;
            item.documentation = new vscode.MarkdownString(
                `**Instance Field**\n\nType: \`${field.type}\`\n\nModifiers: ${modifiers || 'none'}`
            );
            // Use very high priority sort text to appear at top
            item.sortText = '!1_' + field.name;
            item.filterText = field.name;
            completionItems.push(item);
        }

        return completionItems;
    }

    /**
     * Find the enclosing class name at the given position
     */
    private findEnclosingClass(document: vscode.TextDocument, position: vscode.Position): string | null {
        const text = document.getText();
        const lines = text.split('\n');

        let currentClass: string | null = null;
        let braceCount = 0;
        let classBraceLevel = -1;

        for (let i = 0; i <= position.line; i++) {
            const line = lines[i];

            // Count braces BEFORE checking exit condition
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;

            // Check for class declaration (support multiple patterns including generics)
            const classMatch = line.match(/^\s*class\s+(\w+)(?:<[^>]+>)?/) ||
                              line.match(/class\s+(\w+)(?:<[^>]+>)?\s+implements/) ||
                              line.match(/class\s+(\w+)(?:<[^>]+>)?\s+extends/);
            if (classMatch) {
                currentClass = classMatch[1];
                classBraceLevel = braceCount; // Level BEFORE the opening brace
            }

            // Update brace count AFTER checking for class
            braceCount += openBraces - closeBraces;

            // Check if we've exited the class (brace count drops to or below class level)
            if (currentClass && braceCount <= classBraceLevel && i > 0) {
                currentClass = null;
                classBraceLevel = -1;
            }
        }

        return currentClass;
    }
}