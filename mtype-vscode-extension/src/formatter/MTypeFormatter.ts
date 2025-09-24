import * as vscode from 'vscode';

export class MTypeFormatter implements vscode.DocumentFormattingEditProvider {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
        const edits: vscode.TextEdit[] = [];
        const text = document.getText();

        // Get configuration settings
        const config = vscode.workspace.getConfiguration('mTypeFormatter');
        const insertSpaceAroundOperators = config.get<boolean>('insertSpaceAroundOperators', true);

        const formatted = this.formatMTypeCode(text, insertSpaceAroundOperators);

        const fullRange = new vscode.Range(
            document.positionAt(0),
            document.positionAt(text.length)
        );

        edits.push(vscode.TextEdit.replace(fullRange, formatted));
        return edits;
    }

    private formatMTypeCode(code: string, insertSpaceAroundOperators: boolean): string {
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
            line = this.formatLineContent(line, insertSpaceAroundOperators);

            formatted.push(indent + line);

            // Increase indent after opening braces
            if (line.includes('{')) {
                indentLevel++;
            }
        }

        return formatted.join('\n');
    }

    private formatLineContent(line: string, insertSpaceAroundOperators: boolean): string {
        // Don't format comments
        if (line.startsWith('//')) {
            return line;
        }

        // FIRST: Handle comparison operators completely separately
        line = this.formatComparisonOperators(line);

        // Format other operators with proper spacing
        line = line

            // Increment/decrement operators (no spaces)
            .replace(/\s*\+\+\s*/g, '++')
            .replace(/\s*--\s*/g, '--')

            // Compound assignment operators (with spaces)
            .replace(/\s*\+=\s*/g, ' += ')
            .replace(/\s*-=\s*/g, ' -= ')
            .replace(/\s*\*=\s*/g, ' *= ')
            .replace(/\s*\/=\s*/g, ' /= ')
            .replace(/\s*%=\s*/g, ' %= ')

            // Regular assignment operator (with spaces)
            .replace(/(?<![+\-*\/%])\s*=\s*(?![=])/g, ' = ')  // = but not +=, -=, *=, /=, %=, ==

            // Arithmetic operators - always add spaces
            .replace(/([^\s+])\+([^\s+=])/g, '$1 + $2')  // + but not ++ or +=
            .replace(/([^\s-])-([^\s-=])/g, '$1 - $2')   // - but not -- or -=
            .replace(/([^\s])\*([^\s=])/g, '$1 * $2')    // * but not *=
            .replace(/([^\s])\/([^\s=])/g, '$1 / $2')    // / but not /=
            .replace(/([^\s])%([^\s=])/g, '$1 % $2')     // % but not %=

            // Note: Comparison operators are handled separately

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

    private formatComparisonOperators(line: string): string {
        // Handle comparison operators with spaces - isolated from other formatting
        return line
            // Fix already-split operators first (merge them back together) - more comprehensive
            .replace(/=\s+=\s*/g, '==')      // Fix = = to == (one or more spaces between)
            .replace(/!\s+=\s*/g, '!=')      // Fix ! = to != (one or more spaces between)
            .replace(/<\s+=\s*/g, '<=')      // Fix < = to <= (one or more spaces between)
            .replace(/>\s+=\s*/g, '>=')      // Fix > = to >= (one or more spaces between)

            // Also handle any remaining spacing variations
            .replace(/=\s*=/g, '==')         // Fix = = to == (any spacing)
            .replace(/!\s*=/g, '!=')         // Fix ! = to != (any spacing)
            .replace(/<\s*=/g, '<=')         // Fix < = to <= (any spacing)
            .replace(/>\s*=/g, '>=')         // Fix > = to >= (any spacing)

            // Then handle spacing around all comparison operators (add spaces) - more flexible patterns
            .replace(/([a-zA-Z0-9_)])\s*<=\s*([a-zA-Z0-9_(])/g, '$1 <= $2')    // Add spaces around <=
            .replace(/([a-zA-Z0-9_)])\s*>=\s*([a-zA-Z0-9_(])/g, '$1 >= $2')    // Add spaces around >=
            .replace(/([a-zA-Z0-9_)])\s*==\s*([a-zA-Z0-9_(])/g, '$1 == $2')    // Add spaces around ==
            .replace(/([a-zA-Z0-9_)])\s*!=\s*([a-zA-Z0-9_(])/g, '$1 != $2')    // Add spaces around !=
            .replace(/([a-zA-Z0-9_)])\s*<\s*([a-zA-Z0-9_(])/g, '$1 < $2')      // Add spaces around <
            .replace(/([a-zA-Z0-9_)])\s*>\s*([a-zA-Z0-9_(])/g, '$1 > $2');     // Add spaces around >
    }
}