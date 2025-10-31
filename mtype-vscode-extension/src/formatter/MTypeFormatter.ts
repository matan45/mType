import * as vscode from 'vscode';

interface FormatterOptions {
    insertSpaceAroundOperators: boolean;
    useTabs: boolean;
    tabSize: number;
    insertBlankLines: boolean;
    organizeImports: boolean;
}

export class MTypeFormatter implements vscode.DocumentFormattingEditProvider {
    provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
        const edits: vscode.TextEdit[] = [];
        const text = document.getText();

        // Get configuration settings
        const config = vscode.workspace.getConfiguration('mTypeFormatter');
        const insertSpaceAroundOperators = config.get<boolean>('insertSpaceAroundOperators', true);
        const useTabs = config.get<boolean>('useTabs', false);
        const tabSize = config.get<number>('tabSize', 4);
        const insertBlankLines = config.get<boolean>('insertBlankLines', true);
        const organizeImports = config.get<boolean>('organizeImports', true);

        const formatted = this.formatMTypeCode(text, {
            insertSpaceAroundOperators,
            useTabs,
            tabSize,
            insertBlankLines,
            organizeImports
        });

        const fullRange = new vscode.Range(
            document.positionAt(0),
            document.positionAt(text.length)
        );

        edits.push(vscode.TextEdit.replace(fullRange, formatted));
        return edits;
    }

    private formatMTypeCode(code: string, options: FormatterOptions): string {
        let lines = code.split('\n');

        // Step 1: Organize imports if enabled
        if (options.organizeImports) {
            lines = this.organizeImportLines(lines);
        }

        // Step 2: Format lines with proper indentation and spacing
        const formatted: string[] = [];
        let indentLevel = 0;
        let previousLineType: 'empty' | 'import' | 'class' | 'method' | 'other' = 'empty';

        for (let i = 0; i < lines.length; i++) {
            let line = lines[i].trim();

            // Preserve empty lines in some cases
            if (line === '') {
                // Don't add multiple consecutive empty lines
                if (previousLineType !== 'empty') {
                    formatted.push('');
                    previousLineType = 'empty';
                }
                continue;
            }

            // Determine line type
            const lineType = this.getLineType(line);

            // Add blank lines before classes and methods if enabled
            if (options.insertBlankLines) {
                // Don't add blank line if previous line is an annotation
                const isAfterAnnotation = formatted.length > 0 && formatted[formatted.length - 1].trim().startsWith('@');

                if (lineType === 'class' && previousLineType !== 'empty' && previousLineType !== 'import' && !isAfterAnnotation && formatted.length > 0) {
                    formatted.push(''); // Blank line before class
                } else if (lineType === 'method' && previousLineType === 'method' && formatted.length > 0) {
                    formatted.push(''); // Blank line between methods
                }
            }

            // Count braces in this line (excluding strings)
            const openBraceCount = this.countChar(line, '{');
            const closeBraceCount = this.countChar(line, '}');

            // Calculate net brace change
            const netBraceChange = openBraceCount - closeBraceCount;

            // Only decrease indent BEFORE the line if there are MORE closing braces than opening
            if (netBraceChange < 0) {
                indentLevel = Math.max(0, indentLevel + netBraceChange);
            }

            // Apply current indent
            const indentChar = options.useTabs ? '\t' : ' '.repeat(options.tabSize);
            const indent = indentChar.repeat(indentLevel);

            // Format the line content (protecting strings)
            line = this.formatLineContent(line, options.insertSpaceAroundOperators);

            formatted.push(indent + line);

            // Increase indent AFTER the line if there are MORE opening braces than closing
            if (netBraceChange > 0) {
                indentLevel += netBraceChange;
            }

            previousLineType = lineType;
        }

        return formatted.join('\n');
    }

    private countChar(line: string, char: string): number {
        let count = 0;
        let inString = false;
        let stringChar = '';

        for (let i = 0; i < line.length; i++) {
            const c = line[i];
            const prevChar = i > 0 ? line[i - 1] : '';

            if ((c === '"' || c === "'") && prevChar !== '\\') {
                if (inString && c === stringChar) {
                    inString = false;
                    stringChar = '';
                } else if (!inString) {
                    inString = true;
                    stringChar = c;
                }
            } else if (!inString && c === char) {
                count++;
            }
        }

        return count;
    }

    private getLineType(line: string): 'import' | 'class' | 'method' | 'other' | 'empty' {
        if (line === '') return 'empty';
        if (line.startsWith('import ')) return 'import';
        if (line.match(/^\s*(abstract\s+)?class\s+\w+/)) return 'class';
        if (line.match(/^\s*constructor\s*\(/)) return 'method'; // Constructor
        if (line.match(/^\s*(public|private|protected)?\s*(static\s+)?(async\s+)?function\s+\w+/)) return 'method';
        return 'other';
    }

    private isInsideString(line: string, position: number): boolean {
        let inString = false;
        let stringChar = '';

        for (let i = 0; i < position; i++) {
            const char = line[i];
            if ((char === '"' || char === "'") && (i === 0 || line[i - 1] !== '\\')) {
                if (inString && char === stringChar) {
                    inString = false;
                    stringChar = '';
                } else if (!inString) {
                    inString = true;
                    stringChar = char;
                }
            }
        }

        return inString;
    }

    private organizeImportLines(lines: string[]): string[] {
        const imports: string[] = [];
        const nonImports: string[] = [];
        let inImportSection = true;

        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('import ')) {
                imports.push(trimmed);
            } else {
                if (trimmed !== '' || !inImportSection) {
                    inImportSection = false;
                }
                nonImports.push(line);
            }
        }

        // Sort imports alphabetically
        imports.sort((a, b) => {
            // Extract paths for comparison
            const pathA = a.match(/from\s+["']([^"']+)["']/)?.[1] || '';
            const pathB = b.match(/from\s+["']([^"']+)["']/)?.[1] || '';

            // Sort by: relative depth, then alphabetically
            const depthA = (pathA.match(/\.\.\//g) || []).length;
            const depthB = (pathB.match(/\.\.\//g) || []).length;

            if (depthA !== depthB) {
                return depthA - depthB; // Shallower imports first
            }

            return pathA.localeCompare(pathB);
        });

        // Combine: sorted imports, then blank line, then rest
        if (imports.length > 0 && nonImports.some(l => l.trim() !== '')) {
            return [...imports, '', ...nonImports];
        }

        return [...imports, ...nonImports];
    }

    private formatLineContent(line: string, insertSpaceAroundOperators: boolean): string {
        // Don't format comments
        if (line.startsWith('//')) {
            return line;
        }

        // CRITICAL: Extract and protect strings from formatting
        const { code, strings } = this.extractStrings(line);

        // Format the code (without strings)
        let formatted = code;

        if (insertSpaceAroundOperators) {
            // FIRST: Handle comparison operators completely separately
            formatted = this.formatComparisonOperators(formatted);

            // Format other operators with proper spacing
            formatted = formatted

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

                // Colons in type annotations and foreach
                .replace(/\s*:\s*/g, ' : ')

                // Clean up multiple spaces
                .replace(/\s+/g, ' ');
        }

        // Restore strings to their original positions
        return this.restoreStrings(formatted, strings);
    }

    private extractStrings(line: string): { code: string; strings: string[] } {
        const strings: string[] = [];
        let code = '';
        let inString = false;
        let stringChar = '';
        let currentString = '';

        for (let i = 0; i < line.length; i++) {
            const char = line[i];
            const prevChar = i > 0 ? line[i - 1] : '';

            if ((char === '"' || char === "'") && prevChar !== '\\') {
                if (inString && char === stringChar) {
                    // End of string
                    currentString += char;
                    strings.push(currentString);
                    code += `__STRING_${strings.length - 1}__`;
                    currentString = '';
                    inString = false;
                    stringChar = '';
                } else if (!inString) {
                    // Start of string
                    inString = true;
                    stringChar = char;
                    currentString = char;
                }
            } else if (inString) {
                currentString += char;
            } else {
                code += char;
            }
        }

        // Handle unclosed strings
        if (inString) {
            strings.push(currentString);
            code += `__STRING_${strings.length - 1}__`;
        }

        return { code, strings };
    }

    private restoreStrings(code: string, strings: string[]): string {
        let result = code;
        for (let i = 0; i < strings.length; i++) {
            result = result.replace(`__STRING_${i}__`, strings[i]);
        }
        return result;
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