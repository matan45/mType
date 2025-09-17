import {
    createConnection,
    TextDocuments,
    Diagnostic,
    DiagnosticSeverity,
    ProposedFeatures,
    InitializeParams,
    DidChangeConfigurationNotification,
    CompletionItem,
    CompletionItemKind,
    TextDocumentPositionParams,
    TextDocumentSyncKind,
    InitializeResult,
    DocumentFormattingParams,
    DocumentRangeFormattingParams,
    TextEdit,
    Range,
    Position
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';

// Create a connection for the server
const connection = createConnection(ProposedFeatures.all);

// Create a simple text document manager
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasDiagnosticRelatedInformationCapability = false;

connection.onInitialize((params: InitializeParams) => {
    const capabilities = params.capabilities;

    // Does the client support the `workspace/configuration` request?
    hasConfigurationCapability = !!(
        capabilities.workspace && !!capabilities.workspace.configuration
    );
    hasWorkspaceFolderCapability = !!(
        capabilities.workspace && !!capabilities.workspace.workspaceFolders
    );
    hasDiagnosticRelatedInformationCapability = !!(
        capabilities.textDocument &&
        capabilities.textDocument.publishDiagnostics &&
        capabilities.textDocument.publishDiagnostics.relatedInformation
    );

    const result: InitializeResult = {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Incremental,
            completionProvider: {
                resolveProvider: true,
                triggerCharacters: ['.', ':']
            },
            documentFormattingProvider: true,
            documentRangeFormattingProvider: true
        }
    };
    if (hasWorkspaceFolderCapability) {
        result.capabilities.workspace = {
            workspaceFolders: {
                supported: true
            }
        };
    }
    return result;
});

connection.onInitialized(() => {
    if (hasConfigurationCapability) {
        connection.client.register(DidChangeConfigurationNotification.type, undefined);
    }
    if (hasWorkspaceFolderCapability) {
        connection.workspace.onDidChangeWorkspaceFolders(_event => {
            connection.console.log('Workspace folder change event received.');
        });
    }
});

// mType language keywords and types
const mTypeKeywords = [
    'if', 'else', 'while', 'for', 'foreach', 'do', 'switch', 'case', 'default',
    'break', 'continue', 'return', 'class', 'namespace', 'import', 'using',
    'new', 'this', 'null', 'static', 'final', 'private', 'public', 'protected',
    'true', 'false'
];

const mTypePrimitiveTypes = [
    'int', 'float', 'string', 'bool', 'void', 'object'
];

const mTypeCollectionTypes = [
    'Array', 'Set', 'Map', 'Stack', 'Queue'
];

const mTypeBuiltinFunctions = [
    'print', 'println', 'toString', 'size', 'length', 'add', 'remove', 'contains',
    'clear', 'isEmpty', 'push', 'pop', 'peek', 'enqueue', 'dequeue'
];

// Code completion provider
connection.onCompletion(
    (_textDocumentPosition: TextDocumentPositionParams): CompletionItem[] => {
        const completionItems: CompletionItem[] = [];

        // Add keywords
        mTypeKeywords.forEach(keyword => {
            completionItems.push({
                label: keyword,
                kind: CompletionItemKind.Keyword,
                data: keyword
            });
        });

        // Add primitive types
        mTypePrimitiveTypes.forEach(type => {
            completionItems.push({
                label: type,
                kind: CompletionItemKind.TypeParameter,
                data: type,
                detail: 'Primitive type'
            });
        });

        // Add collection types
        mTypeCollectionTypes.forEach(type => {
            completionItems.push({
                label: type,
                kind: CompletionItemKind.Class,
                data: type,
                detail: 'Collection type',
                insertText: `${type}<$1>`,
                insertTextFormat: 2 // Snippet format
            });
        });

        // Add built-in functions
        mTypeBuiltinFunctions.forEach(func => {
            completionItems.push({
                label: func,
                kind: CompletionItemKind.Function,
                data: func,
                detail: 'Built-in function'
            });
        });

        // Add class templates
        completionItems.push({
            label: 'class',
            kind: CompletionItemKind.Snippet,
            data: 'class_template',
            detail: 'Class definition template',
            insertText: [
                'class ${1:ClassName} {',
                '\t${2:// Constructor}',
                '\t${1:ClassName}() {',
                '\t\t${3:// Constructor body}',
                '\t}',
                '',
                '\t${4:// Methods}',
                '}'
            ].join('\n'),
            insertTextFormat: 2
        });

        // Add method templates
        completionItems.push({
            label: 'method',
            kind: CompletionItemKind.Snippet,
            data: 'method_template',
            detail: 'Method definition template',
            insertText: [
                '${1:returnType} ${2:methodName}(${3:parameters}) {',
                '\t${4:// Method body}',
                '}'
            ].join('\n'),
            insertTextFormat: 2
        });

        // Add namespace template
        completionItems.push({
            label: 'namespace',
            kind: CompletionItemKind.Snippet,
            data: 'namespace_template',
            detail: 'Namespace definition template',
            insertText: [
                'namespace ${1:NamespaceName} {',
                '\t${2:// Namespace content}',
                '}'
            ].join('\n'),
            insertTextFormat: 2
        });

        return completionItems;
    }
);

connection.onCompletionResolve(
    (item: CompletionItem): CompletionItem => {
        if (item.data === 'class_template') {
            item.documentation = 'Creates a new class with constructor';
        } else if (item.data === 'method_template') {
            item.documentation = 'Creates a new method';
        } else if (item.data === 'namespace_template') {
            item.documentation = 'Creates a new namespace';
        } else if (mTypeKeywords.includes(item.data)) {
            item.documentation = `mType keyword: ${item.data}`;
        } else if (mTypePrimitiveTypes.includes(item.data)) {
            item.documentation = `mType primitive type: ${item.data}`;
        } else if (mTypeCollectionTypes.includes(item.data)) {
            item.documentation = `mType collection type: ${item.data}`;
        } else if (mTypeBuiltinFunctions.includes(item.data)) {
            item.documentation = `mType built-in function: ${item.data}`;
        }
        return item;
    }
);

// Basic diagnostics (syntax validation could be added here)
async function validateTextDocument(textDocument: TextDocument): Promise<void> {
    const text = textDocument.getText();
    const diagnostics: Diagnostic[] = [];

    // Simple validation example: check for unclosed braces
    const openBraces = (text.match(/\{/g) || []).length;
    const closeBraces = (text.match(/\}/g) || []).length;

    if (openBraces !== closeBraces) {
        const diagnostic: Diagnostic = {
            severity: DiagnosticSeverity.Error,
            range: {
                start: textDocument.positionAt(0),
                end: textDocument.positionAt(text.length)
            },
            message: `Unmatched braces: ${openBraces} opening braces, ${closeBraces} closing braces`,
            source: 'mType'
        };

        if (hasDiagnosticRelatedInformationCapability) {
            diagnostic.relatedInformation = [
                {
                    location: {
                        uri: textDocument.uri,
                        range: Object.assign({}, diagnostic.range)
                    },
                    message: 'Check for missing or extra braces'
                }
            ];
        }
        diagnostics.push(diagnostic);
    }

    // Send the computed diagnostics to VS Code
    connection.sendDiagnostics({ uri: textDocument.uri, diagnostics });
}

documents.onDidChangeContent(change => {
    validateTextDocument(change.document);
});

// mType Formatter
class MTypeFormatter {
    private indentSize = 4;
    private useSpaces = true;

    formatDocument(document: TextDocument): TextEdit[] {
        const text = document.getText();
        const lines = text.split('\n');
        const edits: TextEdit[] = [];
        let indentLevel = 0;
        let inMultiLineComment = false;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const trimmedLine = line.trim();

            // Skip empty lines
            if (trimmedLine === '') {
                continue;
            }

            // Handle multi-line comments
            if (trimmedLine.startsWith('/*')) {
                inMultiLineComment = true;
            }
            if (trimmedLine.endsWith('*/')) {
                inMultiLineComment = false;
                continue;
            }
            if (inMultiLineComment) {
                continue;
            }

            // Skip single-line comments
            if (trimmedLine.startsWith('//')) {
                continue;
            }

            // Decrease indent for closing braces
            if (trimmedLine === '}' || trimmedLine.startsWith('} ') || trimmedLine.endsWith('}')) {
                indentLevel = Math.max(0, indentLevel - 1);
            }

            // Calculate expected indentation
            const expectedIndent = this.getIndentString(indentLevel);
            const currentIndent = line.substring(0, line.length - line.trimStart().length);

            // Create edit if indentation is wrong
            if (currentIndent !== expectedIndent) {
                const range: Range = {
                    start: Position.create(i, 0),
                    end: Position.create(i, currentIndent.length)
                };
                edits.push(TextEdit.replace(range, expectedIndent));
            }

            // Handle spacing around operators
            const formattedLine = this.formatOperators(trimmedLine);
            if (formattedLine !== trimmedLine) {
                const range: Range = {
                    start: Position.create(i, currentIndent.length),
                    end: Position.create(i, line.length)
                };
                edits.push(TextEdit.replace(range, formattedLine));
            }

            // Increase indent for opening braces
            if (trimmedLine.endsWith('{') ||
                trimmedLine.includes('namespace ') ||
                trimmedLine.includes('class ') ||
                trimmedLine.includes('if (') ||
                trimmedLine.includes('else') ||
                trimmedLine.includes('while (') ||
                trimmedLine.includes('for (') ||
                trimmedLine.includes('foreach (') ||
                trimmedLine.includes('do') ||
                trimmedLine.includes('switch (')) {
                indentLevel++;
            }
        }

        return edits;
    }

    formatRange(document: TextDocument, range: Range): TextEdit[] {
        // For range formatting, we'll format the entire document for simplicity
        // In a production formatter, you'd want to format only the specified range
        return this.formatDocument(document);
    }

    private getIndentString(level: number): string {
        if (this.useSpaces) {
            return ' '.repeat(level * this.indentSize);
        } else {
            return '\t'.repeat(level);
        }
    }

    private formatOperators(line: string): string {
        let formatted = line;

        // Add spaces around assignment operators
        formatted = formatted.replace(/([a-zA-Z0-9_])(=)([^=])/g, '$1 $2 $3');
        formatted = formatted.replace(/([a-zA-Z0-9_])(\+=|\-=|\*=|\/=|%=)([a-zA-Z0-9_])/g, '$1 $2 $3');

        // Add spaces around comparison operators
        formatted = formatted.replace(/([a-zA-Z0-9_])(==|!=|<=|>=|<|>)([a-zA-Z0-9_])/g, '$1 $2 $3');

        // Add spaces around arithmetic operators
        formatted = formatted.replace(/([a-zA-Z0-9_])(\+|\-|\*|\/)([a-zA-Z0-9_])/g, '$1 $2 $3');

        // Add spaces around logical operators
        formatted = formatted.replace(/([a-zA-Z0-9_])(&&|\|\|)([a-zA-Z0-9_])/g, '$1 $2 $3');

        // Format function calls - space after comma
        formatted = formatted.replace(/,([^ ])/g, ', $1');

        // Remove extra spaces
        formatted = formatted.replace(/\s+/g, ' ');

        return formatted;
    }
}

const formatter = new MTypeFormatter();

// Document formatting
connection.onDocumentFormatting((params: DocumentFormattingParams): TextEdit[] => {
    const document = documents.get(params.textDocument.uri);
    if (!document) {
        return [];
    }
    return formatter.formatDocument(document);
});

// Range formatting
connection.onDocumentRangeFormatting((params: DocumentRangeFormattingParams): TextEdit[] => {
    const document = documents.get(params.textDocument.uri);
    if (!document) {
        return [];
    }
    return formatter.formatRange(document, params.range);
});

// Make the text document manager listen on the connection
documents.listen(connection);

// Listen on the connection
connection.listen();