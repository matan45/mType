import * as vscode from 'vscode';

export class MTypeContextAnalyzer {

    static analyzeContext(document: vscode.TextDocument, position: vscode.Position): string[] {
        const contexts: string[] = [];
        const currentLine = document.lineAt(position).text;
        const linePrefix = currentLine.substring(0, position.character);
        const documentText = document.getText();

        // Always include global context
        contexts.push('global');

        // Analyze document-level context
        const documentContext = this.getDocumentContext(documentText, position);
        contexts.push(...documentContext);

        // Analyze line-level context
        const lineContext = this.getLineContext(linePrefix, currentLine);
        contexts.push(...lineContext);

        // Analyze expression context
        const expressionContext = this.getExpressionContext(linePrefix);
        contexts.push(...expressionContext);

        return [...new Set(contexts)]; // Remove duplicates
    }

    private static getDocumentContext(text: string, position: vscode.Position): string[] {
        const contexts: string[] = [];
        const lines = text.split('\n');
        const currentLineIndex = position.line;

        // Check if we're inside a class or function
        let insideClass = false;
        let insideFunction = false;
        let insideSwitch = false;
        let braceCount = 0;
        let classBraceLevel = -1;
        let functionBraceLevel = -1;
        let switchBraceLevel = -1;

        for (let i = 0; i <= currentLineIndex; i++) {
            const line = lines[i];

            // Count braces BEFORE checking for new declarations
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;

            // Check if we've exited contexts BEFORE processing new line
            if (insideSwitch && braceCount <= switchBraceLevel) {
                insideSwitch = false;
            }
            if (insideFunction && braceCount <= functionBraceLevel) {
                insideFunction = false;
            }
            if (insideClass && braceCount <= classBraceLevel) {
                insideClass = false;
            }

            // Update brace count
            braceCount += openBraces - closeBraces;

            // Check for class declaration
            if (line.match(/^\s*class\s+\w+/) && !insideClass) {
                insideClass = true;
                classBraceLevel = braceCount; // Current brace level after opening
            }

            // Check for function declaration (both global and class methods)
            if ((line.match(/^\s*function\s+\w+/) || line.match(/^\s*constructor\s*\(/)) && !insideFunction) {
                insideFunction = true;
                functionBraceLevel = braceCount; // Current brace level after opening
            }

            // Check for switch statement
            if (line.match(/^\s*switch\s*\(/) && !insideSwitch) {
                insideSwitch = true;
                switchBraceLevel = braceCount; // Current brace level after opening
            }
        }

        // Determine context based on current state
        if (insideClass) {
            contexts.push('class');
            if (insideFunction) {
                contexts.push('function', 'class-method', 'block');
            }
        } else if (insideFunction) {
            contexts.push('function', 'global-function', 'block');
        }

        if (insideSwitch) {
            contexts.push('switch');
        }

        return contexts;
    }

    private static getLineContext(linePrefix: string, fullLine: string): string[] {
        const contexts: string[] = [];

        // Check if we're at the beginning of a line (after whitespace)
        if (linePrefix.trim() === '') {
            contexts.push('line-start');
        }

        // Check for type context (after type keywords or in variable declarations)
        if (this.isTypeContext(linePrefix)) {
            contexts.push('type-context');
        }

        // Check for return type context
        if (linePrefix.match(/function\s+\w+\s*\([^)]*\)\s*:\s*$/)) {
            contexts.push('return-type');
        }

        // Check if we're after an if statement
        if (this.isPreviousLineIf(fullLine)) {
            contexts.push('after-if');
        }

        // Check for foreach context
        if (linePrefix.match(/foreach\s*\(\s*\w*\s*\w*\s*$/)) {
            contexts.push('foreach');
        }

        // Check for loop context
        if (this.isInLoop(linePrefix)) {
            contexts.push('loop');
        }

        // Check for block context (inside any braces)
        if (this.isInBlock(linePrefix)) {
            contexts.push('block');
        }

        return contexts;
    }

    private static getExpressionContext(linePrefix: string): string[] {
        const contexts: string[] = [];

        // Check if we're in an expression context
        if (this.isExpressionContext(linePrefix)) {
            contexts.push('expression');
        }

        // Check for assignment context
        if (linePrefix.match(/=\s*$/)) {
            contexts.push('assignment', 'expression');
        }

        // Check for parameter context
        if (linePrefix.match(/\(\s*[^)]*$/)) {
            contexts.push('parameter');
        }

        // Check for condition context
        if (linePrefix.match(/(if|while)\s*\(\s*[^)]*$/)) {
            contexts.push('condition', 'expression');
        }

        return contexts;
    }

    private static isTypeContext(linePrefix: string): boolean {
        // Variable declaration: "int ", "string ", etc.
        if (linePrefix.match(/^\s*(private\s+)?(static\s+)?(final\s+)?$/)) {
            return true;
        }

        // Function parameter: "function name(int "
        if (linePrefix.match(/\(\s*([^,)]*,\s*)*$/)) {
            return true;
        }

        // Generic type: "Array<", "Map<string, "
        if (linePrefix.match(/[<,]\s*$/)) {
            return true;
        }

        return false;
    }

    private static isExpressionContext(linePrefix: string): boolean {
        // After operators
        if (linePrefix.match(/[=+\-*/%<>!&|]\s*$/)) {
            return true;
        }

        // After opening parenthesis
        if (linePrefix.match(/\(\s*$/)) {
            return true;
        }

        // After comma in function call
        if (linePrefix.match(/,\s*$/)) {
            return true;
        }

        return false;
    }

    private static isPreviousLineIf(fullLine: string): boolean {
        // This is a simplified check - in practice, you'd want to look at previous lines
        return fullLine.trim().startsWith('if ');
    }

    private static isInLoop(linePrefix: string): boolean {
        return linePrefix.match(/(for|while|do|foreach)\s*\(/) !== null;
    }

    private static isInBlock(linePrefix: string): boolean {
        // Simple check for being inside a block (after opening brace)
        const openBraces = (linePrefix.match(/\{/g) || []).length;
        const closeBraces = (linePrefix.match(/\}/g) || []).length;
        return openBraces > closeBraces;
    }

    // Get appropriate completion trigger context
    static getCompletionTriggerContext(linePrefix: string): string | null {
        // Static member access
        if (linePrefix.match(/\w+::\s*$/)) {
            return 'static-member';
        }

        // Instance member access
        if (linePrefix.match(/\w+\.\s*$/)) {
            return 'instance-member';
        }

        // Generic type parameter
        if (linePrefix.match(/\w+<[^>]*$/)) {
            return 'generic-parameter';
        }

        return null;
    }
}