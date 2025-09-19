import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';
import { MTypeKeywords } from './MTypeKeywords';
import { MTypeContextAnalyzer } from './MTypeContextAnalyzer';
import { MTypeScopeAnalyzer, VariableInfo, MethodInfo, Visibility } from '../analysis/MTypeScopeAnalyzer';

export class MTypeCompletionProvider implements vscode.CompletionItemProvider {
    private analyzer = new MTypeCodeAnalyzer();
    private scopeAnalyzer: MTypeScopeAnalyzer | null = null;

    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
    ): vscode.CompletionItem[] {
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
                const staticCompletions = this.getStaticMemberCompletions(className);
                // Return ONLY static member completions, no other suggestions
                return staticCompletions;
            }
        }
        // Handle instance member access (object.)
        else if (triggerContext === 'instance-member') {
            const dotMatch = lineText.match(/(\w+)\.\s*$/);
            if (dotMatch) {
                const objectName = dotMatch[1];
                const instanceCompletions = this.getScopeAwareInstanceCompletions(objectName, document, position);
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

            // Add class and variable completions for non-trigger contexts
            const symbolCompletions = this.getSymbolCompletions(contexts, lineText);
            completionItems.push(...symbolCompletions);
        }

        return completionItems;
    }

    private getStaticMemberCompletions(className: string): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return this.getFallbackStaticCompletions(className);
        }

        const classInfo = this.scopeAnalyzer.getClassInfo(className);
        if (!classInfo) {
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

    private getScopeAwareInstanceCompletions(objectName: string, document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
        const completionItems: vscode.CompletionItem[] = [];

        if (!this.scopeAnalyzer) {
            return this.getFallbackInstanceCompletions(objectName);
        }

        // First, find the variable to get its type
        const visibleVariables = this.scopeAnalyzer.getVisibleVariables(position);
        const objectVariable = visibleVariables.find(v => v.name === objectName);

        if (!objectVariable) {
            return completionItems;
        }

        // Get accessible members for the object's type
        const accessibleMembers = this.scopeAnalyzer.getAccessibleMembers(objectVariable.type, position, false);

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