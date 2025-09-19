import * as vscode from 'vscode';
import { MTypeCodeAnalyzer } from '../analyzer/MTypeCodeAnalyzer';

export class MTypeCompletionProvider implements vscode.CompletionItemProvider {
    private analyzer = new MTypeCodeAnalyzer();

    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
    ): vscode.CompletionItem[] {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

        const lineText = document.lineAt(position).text.substring(0, position.character);

        // Check for static member access (ClassName::)
        const staticMatch = lineText.match(/(\w+)::\s*$/);
        if (staticMatch) {
            const className = staticMatch[1];

            if (this.analyzer.classes.has(className)) {
                const classInfo = this.analyzer.classes.get(className)!;
                const staticMembers = classInfo.members.filter((m: any) => m.isStatic);

                const completionItems: vscode.CompletionItem[] = [];

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
                        // Set higher priority for methods
                        item.sortText = '0' + member.name;
                        completionItems.push(item);
                    } else if (member.type === 'field') {
                        const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Field);
                        item.detail = `static ${member.returnType} ${member.name}`;
                        item.documentation = `Static field of type ${member.returnType}`;
                        // Set lower priority for fields
                        item.sortText = '1' + member.name;
                        completionItems.push(item);
                    }
                });

                return completionItems;
            }

            return [];
        }

        // Check for instance member access (object.)
        const dotMatch = lineText.match(/(\w+)\.\s*$/);
        if (dotMatch) {
            const objectName = dotMatch[1];
            const members = this.analyzer.getObjectMembers(objectName);

            const completionItems: vscode.CompletionItem[] = [];

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
                    // Set higher priority for methods
                    item.sortText = '0' + member.name;
                    completionItems.push(item);
                } else if (member.type === 'field') {
                    const item = new vscode.CompletionItem(member.name, vscode.CompletionItemKind.Field);
                    item.detail = `${member.returnType} ${member.name}`;
                    item.documentation = `Instance field of type ${member.returnType}`;
                    // Set lower priority for fields
                    item.sortText = '1' + member.name;
                    completionItems.push(item);
                }
            });

            return completionItems;
        }

        // Return general completions if no specific context
        return [];
    }
}