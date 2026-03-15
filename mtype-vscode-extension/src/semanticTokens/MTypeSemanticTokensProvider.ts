import * as vscode from 'vscode';
import { MTypeScopeAnalyzer, Visibility } from '../analysis/MTypeScopeAnalyzer';

/**
 * Token types supported by the semantic tokens provider
 * These map to standard VS Code semantic token types
 */
export const tokenTypes = [
    'namespace',      // 0
    'class',          // 1
    'enum',           // 2
    'interface',      // 3
    'struct',         // 4
    'typeParameter',  // 5
    'type',           // 6
    'parameter',      // 7
    'variable',       // 8
    'property',       // 9
    'function',       // 10
    'method',         // 11
    'keyword',        // 12
    'modifier',       // 13
    'comment',        // 14
    'string',         // 15
    'number',         // 16
    'regexp',         // 17
    'operator',       // 18
    'decorator'       // 19 (for annotations like @Override)
];

/**
 * Token modifiers that can be applied to tokens
 */
export const tokenModifiers = [
    'declaration',    // 0
    'definition',     // 1
    'readonly',       // 2
    'static',         // 3
    'deprecated',     // 4
    'abstract',       // 5
    'async',          // 6
    'modification',   // 7
    'documentation',  // 8
    'defaultLibrary'  // 9
];

/**
 * Provides semantic token information for enhanced syntax highlighting
 * This allows VS Code to color code based on semantic meaning, not just syntax
 */
export class MTypeSemanticTokensProvider implements vscode.DocumentSemanticTokensProvider {
    async provideDocumentSemanticTokens(
        document: vscode.TextDocument,
        token: vscode.CancellationToken
    ): Promise<vscode.SemanticTokens> {
        const builder = new vscode.SemanticTokensBuilder();
        const text = document.getText();
        const lines = text.split('\n');

        // Analyze the document
        const scopeAnalyzer = new MTypeScopeAnalyzer(document);
        scopeAnalyzer.analyzeDocument();

        // Process each line
        for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
            const line = lines[lineIndex];

            // Tokenize annotations (@Override, @Throw, etc.)
            this.tokenizeAnnotations(line, lineIndex, builder);

            // Tokenize class declarations
            this.tokenizeClassDeclarations(line, lineIndex, builder);

            // Tokenize interface declarations
            this.tokenizeInterfaceDeclarations(line, lineIndex, builder);

            // Tokenize method declarations
            this.tokenizeMethodDeclarations(line, lineIndex, builder, scopeAnalyzer);

            // Tokenize variable declarations
            this.tokenizeVariableDeclarations(line, lineIndex, builder);

            // Tokenize keywords
            this.tokenizeKeywords(line, lineIndex, builder);

            // Tokenize types
            this.tokenizeTypes(line, lineIndex, builder, scopeAnalyzer);

            // Tokenize modifiers
            this.tokenizeModifiers(line, lineIndex, builder);

            // Tokenize function/method calls
            this.tokenizeFunctionCalls(line, lineIndex, builder, scopeAnalyzer);
        }

        return builder.build();
    }

    /**
     * Tokenize annotations like @Override, @Throw
     */
    private tokenizeAnnotations(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        const annotationRegex = /@(\w+)/g;
        let match;

        while ((match = annotationRegex.exec(line)) !== null) {
            const startChar = match.index;
            const length = match[0].length;

            builder.push(
                lineIndex,
                startChar,
                length,
                this.encodeTokenType('decorator'),
                this.encodeTokenModifiers([])
            );
        }
    }

    /**
     * Tokenize class declarations
     */
    private tokenizeClassDeclarations(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        const classRegex = /\b(abstract\s+)?class\s+(\w+)/g;
        let match;

        while ((match = classRegex.exec(line)) !== null) {
            const hasAbstract = !!match[1];
            const className = match[2];
            const classIndex = match.index + (hasAbstract ? match[1]!.length : 0) + 6; // 6 = "class ".length

            // Highlight "class" keyword
            builder.push(
                lineIndex,
                match.index + (hasAbstract ? match[1]!.length : 0),
                5, // "class".length
                this.encodeTokenType('keyword'),
                this.encodeTokenModifiers([])
            );

            // Highlight class name
            builder.push(
                lineIndex,
                classIndex,
                className.length,
                this.encodeTokenType('class'),
                this.encodeTokenModifiers(['declaration'])
            );
        }
    }

    /**
     * Tokenize interface declarations
     */
    private tokenizeInterfaceDeclarations(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        const interfaceRegex = /\binterface\s+(\w+)/g;
        let match;

        while ((match = interfaceRegex.exec(line)) !== null) {
            const interfaceName = match[1];
            const interfaceIndex = match.index + 10; // 10 = "interface ".length

            // Highlight "interface" keyword
            builder.push(
                lineIndex,
                match.index,
                9, // "interface".length
                this.encodeTokenType('keyword'),
                this.encodeTokenModifiers([])
            );

            // Highlight interface name
            builder.push(
                lineIndex,
                interfaceIndex,
                interfaceName.length,
                this.encodeTokenType('interface'),
                this.encodeTokenModifiers(['declaration'])
            );
        }
    }

    /**
     * Tokenize method declarations
     */
    private tokenizeMethodDeclarations(
        line: string,
        lineIndex: number,
        builder: vscode.SemanticTokensBuilder,
        scopeAnalyzer: MTypeScopeAnalyzer
    ): void {
        const methodRegex = /\b(static\s+)?(async\s+)?function\s+(\w+)\s*\(/g;
        let match;

        while ((match = methodRegex.exec(line)) !== null) {
            const isStatic = !!match[1];
            const isAsync = !!match[2];
            const methodName = match[3];

            let offset = match.index;

            // Highlight "static" if present
            if (isStatic) {
                builder.push(
                    lineIndex,
                    offset,
                    6, // "static".length
                    this.encodeTokenType('modifier'),
                    this.encodeTokenModifiers(['static'])
                );
                offset += 7; // 6 + space
            }

            // Highlight "async" if present
            if (isAsync) {
                builder.push(
                    lineIndex,
                    offset,
                    5, // "async".length
                    this.encodeTokenType('modifier'),
                    this.encodeTokenModifiers(['async'])
                );
                offset += 6; // 5 + space
            }

            // Highlight "function" keyword
            builder.push(
                lineIndex,
                offset,
                8, // "function".length
                this.encodeTokenType('keyword'),
                this.encodeTokenModifiers([])
            );

            // Highlight method name
            const methodNameStart = line.indexOf(methodName, offset + 8);
            const modifiers = [];
            if (isStatic) modifiers.push('static');
            if (isAsync) modifiers.push('async');
            modifiers.push('declaration');

            builder.push(
                lineIndex,
                methodNameStart,
                methodName.length,
                this.encodeTokenType('method'),
                this.encodeTokenModifiers(modifiers)
            );
        }
    }

    /**
     * Tokenize variable declarations
     */
    private tokenizeVariableDeclarations(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        // Match: [visibility] [static] [final] type variableName
        const varRegex = /\b(public|private|protected)?\s*(static)?\s*(final)?\s*(\w+(?:<[^>]+>)?)\s+(\w+)\s*[;=]/g;
        let match;

        while ((match = varRegex.exec(line)) !== null) {
            const visibility = match[1];
            const isStatic = !!match[2];
            const isFinal = !!match[3];
            const typeName = match[4];
            const varName = match[5];

            // Find variable name position
            const varNameIndex = line.indexOf(varName, match.index);

            const modifiers = [];
            if (isStatic) modifiers.push('static');
            if (isFinal) modifiers.push('readonly');
            modifiers.push('declaration');

            builder.push(
                lineIndex,
                varNameIndex,
                varName.length,
                this.encodeTokenType('variable'),
                this.encodeTokenModifiers(modifiers)
            );
        }
    }

    /**
     * Tokenize keywords
     */
    private tokenizeKeywords(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        const keywords = [
            'if', 'else', 'while', 'do', 'for', 'switch', 'case', 'default', 'break', 'continue', 'match',
            'return', 'new', 'this', 'super', 'try', 'catch', 'finally', 'throw',
            'import', 'from', 'extends', 'implements', 'abstract', 'final',
            'public', 'private', 'protected', 'static', 'async', 'await'
        ];

        for (const keyword of keywords) {
            const regex = new RegExp(`\\b${keyword}\\b`, 'g');
            let match;

            while ((match = regex.exec(line)) !== null) {
                builder.push(
                    lineIndex,
                    match.index,
                    keyword.length,
                    this.encodeTokenType('keyword'),
                    this.encodeTokenModifiers([])
                );
            }
        }
    }

    /**
     * Tokenize type names (class names used as types)
     */
    private tokenizeTypes(
        line: string,
        lineIndex: number,
        builder: vscode.SemanticTokensBuilder,
        scopeAnalyzer: MTypeScopeAnalyzer
    ): void {
        // Get all known classes
        const classes = scopeAnalyzer.getAllClasses();
        const classNames = Array.from(classes.keys());

        // Also include standard types
        const types = [...classNames, 'List', 'LinkedList', 'HashMap', 'HashSet', 'Stack', 'Queue', 'Array', 'Set', 'Map', 'Promise'];

        for (const typeName of types) {
            const regex = new RegExp(`\\b${typeName}\\b`, 'g');
            let match;

            while ((match = regex.exec(line)) !== null) {
                // Make sure it's not part of a declaration (that's already handled)
                const beforeMatch = line.substring(Math.max(0, match.index - 20), match.index);
                if (!beforeMatch.match(/\b(class|interface)\s*$/)) {
                    builder.push(
                        lineIndex,
                        match.index,
                        typeName.length,
                        this.encodeTokenType('class'),
                        this.encodeTokenModifiers([])
                    );
                }
            }
        }
    }

    /**
     * Tokenize modifiers (visibility, static, final, etc.)
     */
    private tokenizeModifiers(line: string, lineIndex: number, builder: vscode.SemanticTokensBuilder): void {
        const modifiers = ['public', 'private', 'protected', 'static', 'final', 'abstract', 'async'];

        for (const modifier of modifiers) {
            const regex = new RegExp(`\\b${modifier}\\b`, 'g');
            let match;

            while ((match = regex.exec(line)) !== null) {
                const tokenModifiers = [];
                if (modifier === 'static') tokenModifiers.push('static');
                if (modifier === 'final') tokenModifiers.push('readonly');
                if (modifier === 'abstract') tokenModifiers.push('abstract');
                if (modifier === 'async') tokenModifiers.push('async');

                builder.push(
                    lineIndex,
                    match.index,
                    modifier.length,
                    this.encodeTokenType('modifier'),
                    this.encodeTokenModifiers(tokenModifiers)
                );
            }
        }
    }

    /**
     * Tokenize function and method calls
     */
    private tokenizeFunctionCalls(
        line: string,
        lineIndex: number,
        builder: vscode.SemanticTokensBuilder,
        scopeAnalyzer: MTypeScopeAnalyzer
    ): void {
        // Match function calls: word followed by (
        const callRegex = /\b(\w+)\s*\(/g;
        let match;

        while ((match = callRegex.exec(line)) !== null) {
            const functionName = match[1];

            // Skip if it's a keyword or declaration
            const keywords = ['if', 'while', 'for', 'switch', 'catch', 'function'];
            if (keywords.includes(functionName)) {
                continue;
            }

            builder.push(
                lineIndex,
                match.index,
                functionName.length,
                this.encodeTokenType('function'),
                this.encodeTokenModifiers([])
            );
        }
    }

    private encodeTokenType(tokenType: string): number {
        return tokenTypes.indexOf(tokenType);
    }

    private encodeTokenModifiers(modifiers: string[]): number {
        let result = 0;
        for (const modifier of modifiers) {
            const index = tokenModifiers.indexOf(modifier);
            if (index !== -1) {
                result |= (1 << index);
            }
        }
        return result;
    }
}

/**
 * Legend describing the token types and modifiers
 */
export const legend = new vscode.SemanticTokensLegend(tokenTypes, tokenModifiers);
