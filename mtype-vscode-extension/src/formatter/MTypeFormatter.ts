import * as vscode from 'vscode';

export class MTypeFormatter implements vscode.DocumentFormattingEditProvider {
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
            .replace(/\s*\:\:\s*/g, '::')

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

			
            // Colons in foreach loops (but not double colons)
            .replace(/(?<!\:)\s*:\s*(?!\:)/g, ' : ')

            // Clean up multiple spaces
            .replace(/\s+/g, ' ');

        return line;
    }
}