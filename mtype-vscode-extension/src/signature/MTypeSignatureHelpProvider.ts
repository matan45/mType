import * as vscode from 'vscode';
import { MTypeScopeAnalyzer, MethodInfo, Visibility } from '../analysis/MTypeScopeAnalyzer';
import { MTypeImportedSymbolProvider } from '../imports/MTypeImportCompletionProvider';

/**
 * Provides signature help (parameter hints) for function and method calls
 */
export class MTypeSignatureHelpProvider implements vscode.SignatureHelpProvider {
    private importedSymbolProvider: MTypeImportedSymbolProvider | null = null;

    setImportedSymbolProvider(provider: MTypeImportedSymbolProvider): void {
        this.importedSymbolProvider = provider;
    }

    async provideSignatureHelp(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken,
        context: vscode.SignatureHelpContext
    ): Promise<vscode.SignatureHelp | undefined> {
        const lineText = document.lineAt(position).text;
        const textBeforeCursor = lineText.substring(0, position.character);

        // Find the function/method being called
        const functionCallMatch = this.findFunctionCall(textBeforeCursor);
        if (!functionCallMatch) {
            return undefined;
        }

        const { functionName, isMethodCall, objectName } = functionCallMatch;

        // Analyze document for scope-aware completion
        const scopeAnalyzer = new MTypeScopeAnalyzer(document);
        scopeAnalyzer.analyzeDocument();

        let methodInfos: MethodInfo[] = [];

        if (isMethodCall && objectName) {
            // Method call - find methods on the object
            methodInfos = this.findMethodSignatures(scopeAnalyzer, objectName, functionName, document, position);
        } else {
            // Function call or constructor
            methodInfos = this.findFunctionSignatures(scopeAnalyzer, functionName, document, position);
        }

        // Also check built-in functions
        const builtInSignature = this.getBuiltInFunctionSignature(functionName);
        if (builtInSignature) {
            methodInfos.push(builtInSignature);
        }

        if (methodInfos.length === 0) {
            return undefined;
        }

        // Determine which parameter the cursor is on
        const activeParameter = this.getActiveParameter(textBeforeCursor);

        const signatureHelp = new vscode.SignatureHelp();
        signatureHelp.signatures = methodInfos.map(method => this.createSignatureInformation(method));
        signatureHelp.activeSignature = 0;
        signatureHelp.activeParameter = activeParameter;

        return signatureHelp;
    }

    /**
     * Find the function/method call at the cursor position
     */
    private findFunctionCall(textBeforeCursor: string): {
        functionName: string;
        isMethodCall: boolean;
        objectName?: string;
    } | null {
        // Match method calls: object.method( or ClassName::staticMethod(
        const methodCallMatch = textBeforeCursor.match(/(\w+)\.(\w+)\s*\(([^)]*)$/);
        if (methodCallMatch) {
            return {
                functionName: methodCallMatch[2],
                isMethodCall: true,
                objectName: methodCallMatch[1]
            };
        }

        const staticMethodMatch = textBeforeCursor.match(/(\w+)::(\w+)\s*\(([^)]*)$/);
        if (staticMethodMatch) {
            return {
                functionName: staticMethodMatch[2],
                isMethodCall: true,
                objectName: staticMethodMatch[1]
            };
        }

        // Match regular function calls: functionName(
        const functionCallMatch = textBeforeCursor.match(/(\w+)\s*\(([^)]*)$/);
        if (functionCallMatch) {
            return {
                functionName: functionCallMatch[1],
                isMethodCall: false
            };
        }

        return null;
    }

    /**
     * Find method signatures for a given object and method name
     */
    private findMethodSignatures(
        scopeAnalyzer: MTypeScopeAnalyzer,
        objectName: string,
        methodName: string,
        document: vscode.TextDocument,
        position: vscode.Position
    ): MethodInfo[] {
        const methods: MethodInfo[] = [];

        // Check if objectName is "this"
        if (objectName === 'this') {
            // Get current scope to find which class we're in
            const currentScope = scopeAnalyzer.getScopeAtPosition(position);
            if (currentScope && currentScope.className) {
                const classInfo = scopeAnalyzer.getClassInfo(currentScope.className);
                if (classInfo) {
                    const method = classInfo.methods.get(methodName);
                    if (method) {
                        methods.push(method);
                    }
                }
            }
        } else {
            // Find the variable's type
            const variables = scopeAnalyzer.getVisibleVariables(position);
            const variable = variables.find(v => v.name === objectName);

            if (variable) {
                const className = this.extractClassName(variable.type);
                if (className) {
                    const accessibleMembers = scopeAnalyzer.getAccessibleMembers(className, position, false);
                    // Filter for methods only
                    const method = accessibleMembers.find(m => 'returnType' in m && m.name === methodName) as MethodInfo | undefined;
                    if (method) {
                        methods.push(method);
                    }
                }
            }
        }

        return methods;
    }

    /**
     * Find function signatures (global functions or constructors)
     */
    private findFunctionSignatures(
        scopeAnalyzer: MTypeScopeAnalyzer,
        functionName: string,
        document: vscode.TextDocument,
        position: vscode.Position
    ): MethodInfo[] {
        const methods: MethodInfo[] = [];

        // Check if it's a constructor call
        const classInfo = scopeAnalyzer.getClassInfo(functionName);
        if (classInfo) {
            // Find constructor
            const constructor = classInfo.methods.get('constructor');
            if (constructor) {
                methods.push(constructor);
            }
        }

        // Check for global functions (would need to be implemented in scope analyzer)
        // For now, we'll rely on built-in functions

        return methods;
    }

    /**
     * Get built-in function signatures
     */
    private getBuiltInFunctionSignature(functionName: string): MethodInfo | null {
        const builtInFunctions: { [key: string]: MethodInfo } = {
            'print': {
                name: 'print',
                parameters: [{ name: 'value', type: 'any' }],
                returnType: 'void',
                visibility: Visibility.Public,
                isStatic: false,
                declarationLocation: new vscode.Position(0, 0)
            },
            'hashCode': {
                name: 'hashCode',
                parameters: [{ name: 'object', type: 'any' }],
                returnType: 'int',
                visibility: Visibility.Public,
                isStatic: false,
                declarationLocation: new vscode.Position(0, 0)
            },
            'strLength': {
                name: 'strLength',
                parameters: [{ name: 'string', type: 'string' }],
                returnType: 'int',
                visibility: Visibility.Public,
                isStatic: false,
                declarationLocation: new vscode.Position(0, 0)
            },
            'parsePrimitive': {
                name: 'parsePrimitive',
                parameters: [{ name: 'value', type: 'any' }],
                returnType: 'string',
                visibility: Visibility.Public,
                isStatic: false,
                declarationLocation: new vscode.Position(0, 0)
            }
        };

        return builtInFunctions[functionName] || null;
    }

    /**
     * Extract class name from a type (handles generics)
     */
    private extractClassName(type: string): string | null {
        // Remove generic parameters
        const genericMatch = type.match(/^([A-Z]\w*)</);
        if (genericMatch) {
            return genericMatch[1];
        }
        // Check if it's a simple class name
        if (/^[A-Z]\w*$/.test(type)) {
            return type;
        }
        return null;
    }

    /**
     * Determine which parameter the cursor is currently on
     */
    private getActiveParameter(textBeforeCursor: string): number {
        // Find the opening parenthesis
        const lastOpenParen = textBeforeCursor.lastIndexOf('(');
        if (lastOpenParen === -1) {
            return 0;
        }

        // Count commas after the opening parenthesis
        const textAfterParen = textBeforeCursor.substring(lastOpenParen + 1);

        // Don't count commas inside nested parentheses, brackets, or strings
        let parameterIndex = 0;
        let depth = 0;
        let inString = false;
        let stringChar = '';

        let inInterpolation = false;
        let interpBraceDepth = 0;

        for (let i = 0; i < textAfterParen.length; i++) {
            const char = textAfterParen[i];
            const prevChar = i > 0 ? textAfterParen[i - 1] : '';

            // Handle interpolated string start: $"
            if (!inString && char === '$' && i + 1 < textAfterParen.length && textAfterParen[i + 1] === '"') {
                inString = true;
                inInterpolation = true;
                interpBraceDepth = 0;
                stringChar = '"';
                i++; // skip the "
                continue;
            }

            // Handle string literals
            if ((char === '"' || char === "'") && prevChar !== '\\') {
                if (!inString) {
                    inString = true;
                    stringChar = char;
                } else if (char === stringChar && (!inInterpolation || interpBraceDepth === 0)) {
                    inString = false;
                    inInterpolation = false;
                }
                continue;
            }

            if (inString) {
                // Track brace depth inside interpolated strings
                if (inInterpolation) {
                    if (char === '{' && prevChar !== '\\') {
                        interpBraceDepth++;
                    } else if (char === '}' && prevChar !== '\\') {
                        interpBraceDepth--;
                    }
                }
                continue;
            }

            // Handle nested parentheses and brackets
            if (char === '(' || char === '[' || char === '<') {
                depth++;
            } else if (char === ')' || char === ']' || char === '>') {
                depth--;
            } else if (char === ',' && depth === 0) {
                parameterIndex++;
            }
        }

        return parameterIndex;
    }

    /**
     * Create signature information from method info
     */
    private createSignatureInformation(method: MethodInfo): vscode.SignatureInformation {
        const paramStrings = method.parameters.map(p => `${p.type} ${p.name}`);
        const signature = `${method.name}(${paramStrings.join(', ')}): ${method.returnType}`;

        const sigInfo = new vscode.SignatureInformation(signature);
        sigInfo.documentation = new vscode.MarkdownString(this.getMethodDocumentation(method));

        // Add parameter information
        sigInfo.parameters = method.parameters.map(param => {
            const paramLabel = `${param.type} ${param.name}`;
            const paramInfo = new vscode.ParameterInformation(paramLabel);
            paramInfo.documentation = `Parameter: ${param.name} (${param.type})`;
            return paramInfo;
        });

        return sigInfo;
    }

    /**
     * Generate documentation for a method
     */
    private getMethodDocumentation(method: MethodInfo): string {
        let doc = `**${method.name}**\n\n`;

        if (method.isStatic) {
            doc += '_Static method_\n\n';
        }

        if (method.parameters.length > 0) {
            doc += '**Parameters:**\n';
            method.parameters.forEach(param => {
                doc += `- \`${param.name}\`: ${param.type}\n`;
            });
            doc += '\n';
        }

        doc += `**Returns:** ${method.returnType}`;

        return doc;
    }
}
