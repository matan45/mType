import * as vscode from 'vscode';

class MTypeFormatter implements vscode.DocumentFormattingEditProvider {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
        const edits: vscode.TextEdit[] = [];
        const text = document.getText();

        const formatted = this.formatMTypeCode(text);

        const fullRange = new vscode.Range(
            document.positionAt(0),
            document.positionAt(text.length)
        );

        edits.push(vscode.TextEdit.replace(fullRange, formatted));
        return edits;
    }

    private formatMTypeCode(code: string): string {
        const lines = code.split('\n');
        const formatted: string[] = [];
        let indentLevel = 0;
        const indentSize = 4;

        for (let i = 0; i < lines.length; i++) {
            let line = lines[i].trim();

            // Skip empty lines
            if (line === '') {
                formatted.push('');
                continue;
            }

            // Decrease indent before closing braces
            if (line.includes('}')) {
                indentLevel = Math.max(0, indentLevel - 1);
            }

            // Apply current indent
            const indent = ' '.repeat(indentLevel * indentSize);

            // Format the line content
            line = this.formatLineContent(line);

            formatted.push(indent + line);

            // Increase indent after opening braces
            if (line.includes('{')) {
                indentLevel++;
            }
        }

        return formatted.join('\n');
    }

    private formatLineContent(line: string): string {
        // Don't format comments
        if (line.startsWith('//')) {
            return line;
        }

        // Format operators with proper spacing
        line = line
            // Assignment operators
            .replace(/\s*=\s*/g, ' = ')
            .replace(/\s*\+=\s*/g, ' += ')
            .replace(/\s*-=\s*/g, ' -= ')
            .replace(/\s*\*=\s*/g, ' *= ')
            .replace(/\s*\/=\s*/g, ' /= ')

            // Arithmetic operators (be careful not to affect negative numbers)
            .replace(/([^\s])\+([^\s])/g, '$1 + $2')
            .replace(/([^\s])-([^\s])/g, '$1 - $2')
            .replace(/([^\s])\*([^\s])/g, '$1 * $2')
            .replace(/([^\s])\/([^\s])/g, '$1 / $2')
            .replace(/([^\s])%([^\s])/g, '$1 % $2')

            // Comparison operators
            .replace(/\s*==\s*/g, ' == ')
            .replace(/\s*!=\s*/g, ' != ')
            .replace(/\s*<=\s*/g, ' <= ')
            .replace(/\s*>=\s*/g, ' >= ')
            .replace(/([^\s<>])(<)([^\s=])/g, '$1 $2 $3')
            .replace(/([^\s<>])(>)([^\s=])/g, '$1 $2 $3')

            // Logical operators
            .replace(/\s*&&\s*/g, ' && ')
            .replace(/\s*\|\|\s*/g, ' || ')

            // Commas
            .replace(/\s*,\s*/g, ', ')

            // Semicolons (ensure no space before, space after except at end of line)
            .replace(/\s*;\s*/g, '; ')
            .replace(/;\s*$/g, ';') // Remove space after semicolon at end of line

            // Braces
            .replace(/\s*{\s*/g, ' {')
            .replace(/\s*}\s*/g, '}')

            // Parentheses for control structures
            .replace(/\b(if|while|for|foreach|switch)\s*\(/g, '$1 (')

            // Function calls and declarations - no space before parentheses
            .replace(/([a-zA-Z_][a-zA-Z0-9_]*)\s+\(/g, '$1(')

            // Colons in foreach loops
            .replace(/\s*:\s*/g, ' : ')

            // Clean up multiple spaces
            .replace(/\s+/g, ' ');

        return line;
    }
}

// Simple mType code analyzer
class MTypeCodeAnalyzer {
    public classes = new Map<string, any>();
    private variables = new Map<string, string>();

    analyzeDocument(text: string): void {
        this.parseClasses(text);
        this.parseVariables(text);
        console.log('Classes found:', Array.from(this.classes.keys()));
        console.log('Variables found:', Array.from(this.variables.entries()));
    }

    private parseClasses(text: string): void {
        this.classes.clear();

        // Improved class parsing to handle multiple classes better
        const lines = text.split('\n');
        let currentClass = '';
        let braceCount = 0;
        let classBody = '';
        let inClass = false;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const classMatch = line.match(/^\s*class\s+(\w+)\s*\{/);

            if (classMatch && !inClass) {
                // Start of a new class
                currentClass = classMatch[1];
                classBody = '';
                braceCount = 1;
                inClass = true;
                console.log(`Starting to parse class ${currentClass}`);
                continue;
            }

            if (inClass) {
                // Count braces to find end of class
                const openBraces = (line.match(/\{/g) || []).length;
                const closeBraces = (line.match(/\}/g) || []).length;
                braceCount += openBraces - closeBraces;

                if (braceCount > 0) {
                    classBody += line + '\n';
                } else {
                    // End of class found
                    console.log(`Finished parsing class ${currentClass}, body length: ${classBody.length}`);
                    const members = this.parseClassMembers(classBody);
                    this.classes.set(currentClass, { name: currentClass, members: members });

                    inClass = false;
                    currentClass = '';
                    classBody = '';
                }
            }
        }
    }

    private parseClassMembers(classBody: string): any[] {
        const members: any[] = [];
        console.log('Parsing class body:', classBody);

        // Parse mType function syntax: [static] function methodName(parameters): returnType {
        const functionRegex = /(?:(static)\s+)?function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)\s*\{/g;
        let functionMatch;

        while ((functionMatch = functionRegex.exec(classBody)) !== null) {
            const isStatic = functionMatch[1] === 'static';
            const methodName = functionMatch[2];
            const parametersStr = functionMatch[3];
            const returnType = functionMatch[4];

            console.log(`Found method: ${methodName}, static: ${isStatic}, return: ${returnType}, params: ${parametersStr}`);

            const parameters = parametersStr.split(',')
                .map(p => p.trim())
                .filter(p => p.length > 0);

            members.push({
                name: methodName,
                type: 'method',
                returnType: returnType,
                isStatic: isStatic,
                parameters: parameters
            });
        }

        // Parse fields: [static] [final] type fieldName; (at class level, not inside methods)
        const fieldRegex = /^(?:\s*)(?:(static)\s+)?(?:(final)\s+)?(\w+)\s+(\w+)\s*(?:=.*)?;/gm;
        let fieldMatch;

        while ((fieldMatch = fieldRegex.exec(classBody)) !== null) {
            const isStatic = fieldMatch[1] === 'static';
            const fieldType = fieldMatch[3];
            const fieldName = fieldMatch[4];

            // Skip if it's inside a method body or constructor (rough heuristic)
            const beforeMatch = classBody.substring(0, fieldMatch.index);
            const methodCount = (beforeMatch.match(/function\s+\w+|constructor\s*\(/g) || []).length;
            const closeBraceCount = (beforeMatch.match(/^\s*}/gm) || []).length;

            // If we're inside a method/constructor, skip this field
            if (methodCount > closeBraceCount) {
                continue;
            }

            console.log(`Found field: ${fieldName}, static: ${isStatic}, type: ${fieldType}`);

            if (fieldType === 'function' || fieldType === 'return') continue;

            // Check if this field name already exists to avoid duplicates
            const existingMember = members.find(m => m.name === fieldName);
            if (!existingMember) {
                members.push({
                    name: fieldName,
                    type: 'field',
                    returnType: fieldType,
                    isStatic: isStatic
                });
            }
        }

        return members;
    }

    private parseVariables(text: string): void {
        this.variables.clear();

        // Parse global variable declarations (outside classes)
        const globalVarRegex = /^(?!.*class\s)(\w+)\s+(\w+)\s*=\s*(?:new\s+(\w+)\s*\(|([^;]+))/gm;
        let match;

        while ((match = globalVarRegex.exec(text)) !== null) {
            const declaredType = match[1];
            const varName = match[2];
            const newType = match[3];

            const actualType = newType || declaredType;
            this.variables.set(varName, actualType);
            console.log(`Found global variable: ${varName} of type ${actualType}`);
        }

        // Parse variable declarations inside methods/constructors
        const localVarRegex = /(\w+)\s+(\w+)\s*=\s*(?:new\s+(\w+)\s*\(|([^;]+))/g;
        let localMatch;

        while ((localMatch = localVarRegex.exec(text)) !== null) {
            const declaredType = localMatch[1];
            const varName = localMatch[2];
            const newType = localMatch[3];

            const actualType = newType || declaredType;
            if (!this.variables.has(varName)) { // Don't override global variables
                this.variables.set(varName, actualType);
                console.log(`Found local variable: ${varName} of type ${actualType}`);
            }
        }

        // Parse function parameters (for method context)
        const functionParamRegex = /function\s+\w+\s*\(([^)]*)\)/g;
        let funcMatch;

        while ((funcMatch = functionParamRegex.exec(text)) !== null) {
            const params = funcMatch[1];
            if (params.trim()) {
                const paramList = params.split(',');
                for (const param of paramList) {
                    const paramTrimmed = param.trim();
                    const paramMatch = paramTrimmed.match(/(\w+)\s+(\w+)/);
                    if (paramMatch) {
                        const paramType = paramMatch[1];
                        const paramName = paramMatch[2];
                        this.variables.set(paramName, paramType);
                        console.log(`Found function parameter: ${paramName} of type ${paramType}`);
                    }
                }
            }
        }

        // Parse constructor parameters (for constructor context)
        const constructorParamRegex = /constructor\s*\(([^)]*)\)/g;
        let constructorMatch;

        while ((constructorMatch = constructorParamRegex.exec(text)) !== null) {
            const params = constructorMatch[1];
            if (params.trim()) {
                const paramList = params.split(',');
                for (const param of paramList) {
                    const paramTrimmed = param.trim();
                    const paramMatch = paramTrimmed.match(/(\w+)\s+(\w+)/);
                    if (paramMatch) {
                        const paramType = paramMatch[1];
                        const paramName = paramMatch[2];
                        this.variables.set(paramName, paramType);
                        console.log(`Found constructor parameter: ${paramName} of type ${paramType}`);
                    }
                }
            }
        }
    }

    getObjectMembers(objectName: string): any[] {
        // Check if it's a regular variable
        const directType = this.variables.get(objectName);
        if (directType && this.classes.has(directType)) {
            return this.classes.get(directType)!.members.filter((m: any) => !m.isStatic);
        }

        // Check if it's a static class reference
        if (this.classes.has(objectName)) {
            return this.classes.get(objectName)!.members.filter((m: any) => m.isStatic);
        }

        // Check if it's a field within any class (for member access within class methods/constructors)
        for (const [className, classInfo] of this.classes.entries()) {
            const field = classInfo.members.find((m: any) => m.name === objectName && m.type === 'field');
            if (field && this.classes.has(field.returnType)) {
                console.log(`Found field ${objectName} of type ${field.returnType} in class ${className}`);
                return this.classes.get(field.returnType)!.members.filter((m: any) => !m.isStatic);
            }
        }

        return [];
    }
}

// Simple completion provider
class MTypeCompletionProvider implements vscode.CompletionItemProvider {
    private analyzer = new MTypeCodeAnalyzer();

    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
    ): vscode.CompletionItem[] {
        const text = document.getText();
        this.analyzer.analyzeDocument(text);

        const lineText = document.lineAt(position).text.substring(0, position.character);
        console.log('Completion requested for line:', lineText);
        console.log('Position:', position.line, position.character);

        // Check for static member access (ClassName::)
        const staticMatch = lineText.match(/(\w+)::\s*$/);
        if (staticMatch) {
            const className = staticMatch[1];
            console.log(`Static completion requested for class: ${className}`);

            if (this.analyzer.classes.has(className)) {
                const classInfo = this.analyzer.classes.get(className)!;
                const staticMembers = classInfo.members.filter((m: any) => m.isStatic);
                console.log(`Found ${staticMembers.length} static members for ${className}:`, staticMembers.map((m: any) => m.name));

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

            console.log(`Class ${className} not found`);
            return [];
        }

        // Check for instance member access (object.)
        const dotMatch = lineText.match(/(\w+)\.\s*$/);
        if (dotMatch) {
            const objectName = dotMatch[1];
            const members = this.analyzer.getObjectMembers(objectName);
            console.log(`Found ${members.length} members for ${objectName}:`, members.map(m => m.name));

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

export function activate(context: vscode.ExtensionContext) {
    console.log('mType extension is now active!');
    vscode.window.showInformationMessage('mType extension activated!');

    // Register completion provider
    const completionProvider = new MTypeCompletionProvider();
    const completionDisposable = vscode.languages.registerCompletionItemProvider(
        'mtype',
        completionProvider,
        '.',   // Trigger on dot (for instance members)
        ':'    // Trigger on colon (for static members with ::)
    );

    // Register document formatter
    const formatter = new MTypeFormatter();
    const formatterDisposable = vscode.languages.registerDocumentFormattingEditProvider('mtype', formatter);

    // Register additional commands
    const disposables = [
        // Command to run mType file
        vscode.commands.registerCommand('mtype.run', async () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                vscode.window.showErrorMessage('No active editor');
                return;
            }

            if (editor.document.languageId !== 'mtype') {
                vscode.window.showErrorMessage('Current file is not an mType file');
                return;
            }

            const config = vscode.workspace.getConfiguration('mTypeLanguageServer');
            const interpreterPath = config.get<string>('interpreterPath');

            if (!interpreterPath) {
                const result = await vscode.window.showErrorMessage(
                    'mType interpreter path not configured. Would you like to set it now?',
                    'Set Path'
                );
                if (result === 'Set Path') {
                    vscode.commands.executeCommand('workbench.action.openSettings', 'mTypeLanguageServer.interpreterPath');
                }
                return;
            }

            const filePath = editor.document.uri.fsPath;
            const terminal = vscode.window.createTerminal('mType');
            terminal.sendText(`"${interpreterPath}" "${filePath}"`);
            terminal.show();
        }),

        // Command to run mType tests
        vscode.commands.registerCommand('mtype.runTests', async () => {
            const config = vscode.workspace.getConfiguration('mTypeLanguageServer');
            const interpreterPath = config.get<string>('interpreterPath');

            if (!interpreterPath) {
                vscode.window.showErrorMessage('mType interpreter path not configured');
                return;
            }

            const terminal = vscode.window.createTerminal('mType Tests');
            terminal.sendText(`"${interpreterPath}" --tests`);
            terminal.show();
        }),

        // Command to format mType document
        vscode.commands.registerCommand('mtype.formatDocument', async () => {
            console.log('mtype.formatDocument command executed');
            const editor = vscode.window.activeTextEditor;
            if (!editor) {
                vscode.window.showErrorMessage('No active editor');
                return;
            }

            if (editor.document.languageId !== 'mtype') {
                vscode.window.showErrorMessage('Current file is not an mType file');
                return;
            }

            try {
                // Trigger formatting
                await vscode.commands.executeCommand('editor.action.formatDocument');
                vscode.window.showInformationMessage('Document formatted successfully');
            } catch (error) {
                vscode.window.showErrorMessage('Failed to format document: ' + error);
            }
        })
    ];

    context.subscriptions.push(completionDisposable, formatterDisposable, ...disposables);
    console.log('mType extension fully activated with completion provider');
}

export function deactivate(): void {
    console.log('mType extension deactivated');
}