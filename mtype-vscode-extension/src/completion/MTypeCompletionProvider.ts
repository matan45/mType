import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';
import { MTypeKeywords } from './MTypeKeywords';
import { MTypeContextAnalyzer } from './MTypeContextAnalyzer';

export class MTypeCompletionProvider implements vscode.CompletionItemProvider {
    private analyzer = new MTypeCodeAnalyzer();

    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
    ): vscode.CompletionItem[] {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

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
                completionItems.push(...staticCompletions);
            }
        }
        // Handle instance member access (object.)
        else if (triggerContext === 'instance-member') {
            const dotMatch = lineText.match(/(\w+)\.\s*$/);
            if (dotMatch) {
                const objectName = dotMatch[1];
                const instanceCompletions = this.getInstanceMemberCompletions(objectName);
                completionItems.push(...instanceCompletions);
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

            // Add class and variable completions for non-trigger contexts
            const symbolCompletions = this.getSymbolCompletions(contexts, lineText);
            completionItems.push(...symbolCompletions);
        }

        return completionItems;
    }

    private getStaticMemberCompletions(className: string): vscode.CompletionItem[] {
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

    private getInstanceMemberCompletions(objectName: string): vscode.CompletionItem[] {
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
}