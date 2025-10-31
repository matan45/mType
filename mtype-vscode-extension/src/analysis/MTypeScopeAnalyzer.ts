import * as vscode from 'vscode';

export enum ScopeType {
    Global = 'global',
    Class = 'class',
    Method = 'method',
    Constructor = 'constructor',
    Function = 'function',
    Block = 'block',
    Loop = 'loop',
    Switch = 'switch',
    Conditional = 'conditional'
}

export enum Visibility {
    Public = 'public',
    Private = 'private',
    Protected = 'protected',
    Static = 'static'
}

export interface VariableInfo {
    name: string;
    type: string;
    visibility?: Visibility;
    isStatic?: boolean;
    isFinal?: boolean;
    scope: ScopeInfo;
    declarationLocation: vscode.Position;
    isParameter?: boolean;
}

export interface MethodInfo {
    name: string;
    returnType: string;
    parameters: ParameterInfo[];
    visibility: Visibility;
    isStatic: boolean;
    declarationLocation: vscode.Position;
    parentClass?: string;
}

export interface ParameterInfo {
    name: string;
    type: string;
}

export interface ScopeInfo {
    id: string;
    type: ScopeType;
    name?: string; // Class name, method name, etc.
    startLine: number;
    endLine: number;
    parentScope?: ScopeInfo;
    variables: Map<string, VariableInfo>;
    methods: Map<string, MethodInfo>;
    isStatic?: boolean; // For static method scopes
    className?: string; // For method scopes within classes
}

export class MTypeScopeAnalyzer {
    private document: vscode.TextDocument;
    private scopes: Map<string, ScopeInfo> = new Map();
    private rootScope: ScopeInfo;
    private classes: Map<string, ClassInfo> = new Map();
    private interfaces: Map<string, InterfaceInfo> = new Map();

    constructor(document: vscode.TextDocument) {
        this.document = document;
        this.rootScope = this.createScope(ScopeType.Global, 'global', 0, document.lineCount - 1);
        this.scopes.set('global', this.rootScope);
    }

    analyzeDocument(): void {
        const text = this.document.getText();
        const lines = text.split('\n');

        this.parseInterfaces(lines);
        this.parseClasses(lines);
        this.parseGlobalFunctions(lines);
        this.parseGlobalVariables(lines);
    }

    private createScope(type: ScopeType, name: string, startLine: number, endLine: number, parent?: ScopeInfo): ScopeInfo {
        const id = `${type}_${name}_${startLine}`;
        const scope: ScopeInfo = {
            id,
            type,
            name,
            startLine,
            endLine,
            parentScope: parent,
            variables: new Map(),
            methods: new Map()
        };

        this.scopes.set(id, scope);
        return scope;
    }

    private parseInterfaces(lines: string[]): void {
        let currentInterface: InterfaceInfo | null = null;
        let braceCount = 0;
        let interfaceStartLine = -1;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;

            // Check for interface declaration
            const interfaceMatch = line.match(/^\s*interface\s+(\w+)(?:<[^>]+>)?\s*\{/);
            if (interfaceMatch && !currentInterface) {
                const interfaceName = interfaceMatch[1];
                interfaceStartLine = i;
                braceCount = 1;

                currentInterface = {
                    name: interfaceName,
                    scope: this.createScope(ScopeType.Class, interfaceName, i, -1, this.rootScope),
                    methods: new Map()
                };

                this.interfaces.set(interfaceName, currentInterface);
            } else if (currentInterface) {
                braceCount += openBraces - closeBraces;

                if (braceCount <= 0) {
                    // End of interface
                    currentInterface.scope.endLine = i;
                    this.parseInterfaceBody(currentInterface, lines, interfaceStartLine + 1, i - 1);
                    currentInterface = null;
                }
            }
        }
    }

    private parseInterfaceBody(interfaceInfo: InterfaceInfo, lines: string[], startLine: number, endLine: number): void {
        for (let i = startLine; i <= endLine; i++) {
            const line = lines[i];

            // Parse method declarations in interface (no body, just signatures)
            const methodMatch = line.match(/^\s*function\s+(\w+)\s*\(([^)]*)\)\s*:\s*([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s*;/);
            if (methodMatch) {
                const methodName = methodMatch[1];
                const parametersStr = methodMatch[2];
                const returnType = methodMatch[3];

                const parameters = this.parseParameters(parametersStr);

                const methodInfo: MethodInfo = {
                    name: methodName,
                    returnType,
                    parameters,
                    visibility: Visibility.Public, // Interface methods are always public
                    isStatic: false,
                    declarationLocation: new vscode.Position(i, 0)
                };

                interfaceInfo.methods.set(methodName, methodInfo);
            }
        }
    }

    private parseClasses(lines: string[]): void {
        let currentClass: ClassInfo | null = null;
        let braceCount = 0;
        let classStartLine = -1;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;

            // Check for class declaration (support generic classes like HashMap<K,V>)
            // Also capture extends and implements clauses
            const classMatch = line.match(/^\s*(?:abstract\s+)?class\s+(\w+)(?:<[^>]+>)?(?:\s+extends\s+(\w+))?(?:\s+implements\s+([^{]+))?\s*\{/);
            if (classMatch && !currentClass) {
                const className = classMatch[1];
                const parentClass = classMatch[2] || undefined;
                const implementsClause = classMatch[3] || '';

                // Parse implemented interfaces (comma-separated list)
                const implementedInterfaces: string[] = [];
                if (implementsClause) {
                    const interfaces = implementsClause.split(',').map(i => i.trim()).filter(i => i.length > 0);
                    implementedInterfaces.push(...interfaces);
                }

                classStartLine = i;
                braceCount = 1;

                currentClass = {
                    name: className,
                    scope: this.createScope(ScopeType.Class, className, i, -1, this.rootScope),
                    methods: new Map(),
                    fields: new Map(),
                    staticMethods: new Map(),
                    staticFields: new Map(),
                    implementedInterfaces,
                    parentClass
                };

                currentClass.scope.className = className;
                this.classes.set(className, currentClass);
            } else if (currentClass) {
                braceCount += openBraces - closeBraces;

                if (braceCount <= 0) {
                    // End of class
                    currentClass.scope.endLine = i;
                    this.parseClassBody(currentClass, lines, classStartLine + 1, i - 1);
                    currentClass = null;
                }
            }
        }
    }

    private parseClassBody(classInfo: ClassInfo, lines: string[], startLine: number, endLine: number): void {
        let currentMethod: MethodInfo | null = null;
        let methodBraceCount = 0;
        let methodStartLine = -1;

        for (let i = startLine; i <= endLine; i++) {
            const line = lines[i];
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;

            // Parse field declarations (only when not inside a method)
            if (!currentMethod) {
                this.parseClassFields(line, i, classInfo);
            }

            // Parse method declarations
            // Updated regex to handle generic return types and all visibility modifiers (public, private, protected)
            const methodMatch = line.match(/^\s*(?:(public|private|protected)\s+)?(static\s+)?function\s+(\w+)\s*\(([^)]*)\)\s*:\s*([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s*\{/);
            const constructorMatch = line.match(/^\s*constructor\s*\(([^)]*)\)\s*\{/);

            if ((methodMatch || constructorMatch) && !currentMethod) {
                let methodName: string;
                let returnType: string;
                let parametersStr: string;
                let visibilityModifier: string | undefined = undefined;
                let isStatic = false;

                if (methodMatch) {
                    visibilityModifier = methodMatch[1]; // public, private, protected, or undefined
                    isStatic = !!methodMatch[2];
                    methodName = methodMatch[3];
                    parametersStr = methodMatch[4];
                    returnType = methodMatch[5];
                } else {
                    methodName = 'constructor';
                    parametersStr = constructorMatch![1];
                    returnType = classInfo.name;
                }

                const parameters = this.parseParameters(parametersStr);
                // Determine visibility based on modifier
                let visibility: Visibility;
                if (visibilityModifier === 'private') {
                    visibility = Visibility.Private;
                } else if (visibilityModifier === 'protected') {
                    visibility = Visibility.Protected;
                } else {
                    // public or no modifier = public
                    visibility = Visibility.Public;
                }

                currentMethod = {
                    name: methodName,
                    returnType,
                    parameters,
                    visibility,
                    isStatic,
                    declarationLocation: new vscode.Position(i, 0),
                    parentClass: classInfo.name
                };

                methodStartLine = i;
                methodBraceCount = 1;

                // Create method scope
                const methodScope = this.createScope(
                    methodName === 'constructor' ? ScopeType.Constructor : ScopeType.Method,
                    methodName,
                    i,
                    -1,
                    classInfo.scope
                );
                methodScope.isStatic = isStatic;
                methodScope.className = classInfo.name;

                // Add parameters to method scope
                parameters.forEach(param => {
                    const varInfo: VariableInfo = {
                        name: param.name,
                        type: param.type,
                        scope: methodScope,
                        declarationLocation: new vscode.Position(i, 0),
                        isParameter: true
                    };
                    methodScope.variables.set(param.name, varInfo);
                });

                // Store method info
                if (isStatic) {
                    classInfo.staticMethods.set(methodName, currentMethod);
                } else {
                    classInfo.methods.set(methodName, currentMethod);
                }

            } else if (currentMethod) {
                methodBraceCount += openBraces - closeBraces;

                // Parse local variables within method
                this.parseLocalVariables(line, i, this.getMethodScope(classInfo.name, currentMethod.name));

                if (methodBraceCount <= 0) {
                    // End of method
                    const methodScope = this.getMethodScope(classInfo.name, currentMethod.name);
                    if (methodScope) {
                        methodScope.endLine = i;
                    }
                    currentMethod = null;
                }
            }
        }
    }

    private parseClassFields(line: string, lineIndex: number, classInfo: ClassInfo): void {
        // Parse field declarations: [public|private|protected] [static] [final] type fieldName [= value];
        // More strict pattern: must be at start of line, end with semicolon, and have proper field name
        // Updated regex to handle generic types and all visibility modifiers
        const fieldMatch = line.match(/^\s*(?:(public|private|protected)\s+)?(static\s+)?(final\s+)?([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:=.*)?;\s*$/);
        if (fieldMatch) {
            const visibilityModifier = fieldMatch[1]; // public, private, protected, or undefined
            const isStatic = !!fieldMatch[2];
            const isFinal = !!fieldMatch[3];
            const fieldType = fieldMatch[4];
            const fieldName = fieldMatch[5];

            // Determine visibility based on modifier
            let visibility: Visibility;
            if (visibilityModifier === 'private') {
                visibility = Visibility.Private;
            } else if (visibilityModifier === 'protected') {
                visibility = Visibility.Protected;
            } else {
                // public or no modifier = public
                visibility = Visibility.Public;
            }

            const fieldInfo: VariableInfo = {
                name: fieldName,
                type: fieldType,
                visibility,
                isStatic,
                isFinal,
                scope: classInfo.scope,
                declarationLocation: new vscode.Position(lineIndex, 0)
            };

            if (isStatic) {
                classInfo.staticFields.set(fieldName, fieldInfo);
            } else {
                classInfo.fields.set(fieldName, fieldInfo);
            }

            classInfo.scope.variables.set(fieldName, fieldInfo);
        }
    }

    private parseLocalVariables(line: string, lineIndex: number, scope: ScopeInfo | null): void {
        if (!scope) return;

        // Skip lines that are not variable declarations
        const trimmedLine = line.trim();
        if (trimmedLine.startsWith('return ') ||
            trimmedLine.startsWith('if ') ||
            trimmedLine.startsWith('while ') ||
            trimmedLine.startsWith('for ') ||
            trimmedLine.startsWith('//') ||
            trimmedLine.includes('(') && !trimmedLine.includes('new ')) {
            return;
        }

        // Parse local variable declarations: [final] type varName = value;
        // Support both simple assignments and 'new' constructor calls
        // Updated regex to handle generic types like Array<int>, Map<string, int>, etc.
        const varMatch = line.match(/^\s*(final\s+)?([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s+(\w+)\s*=\s*(?:new\s+[A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?\(.*\)|.*);?\s*$/);
        if (varMatch) {
            const isFinal = !!varMatch[1];
            const varType = varMatch[2];
            const varName = varMatch[3];

            // Additional validation: ensure the type is not a keyword
            const keywords = ['return', 'if', 'while', 'for', 'else', 'switch', 'case', 'break', 'continue'];
            if (keywords.includes(varType)) {
                return;
            }

            const varInfo: VariableInfo = {
                name: varName,
                type: varType,
                isFinal,
                scope,
                declarationLocation: new vscode.Position(lineIndex, 0)
            };

            scope.variables.set(varName, varInfo);
        }
    }

    private parseParameters(parametersStr: string): ParameterInfo[] {
        if (!parametersStr.trim()) return [];

        return parametersStr.split(',').map(param => {
            const trimmed = param.trim();
            // Updated regex to handle generic types like Array<int>, Map<string, int>, etc.
            const match = trimmed.match(/([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s+(\w+)/);
            if (match) {
                return {
                    type: match[1],
                    name: match[2]
                };
            }
            return { type: 'unknown', name: trimmed };
        });
    }

    private parseGlobalFunctions(lines: string[]): void {
        // Parse global functions (outside of classes)
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Skip if we're inside a class
            if (this.isInsideClass(i)) continue;

            // Updated regex to handle generic return types like Array<int>, Map<string, int>, etc.
            const functionMatch = line.match(/^\s*function\s+(\w+)\s*\(([^)]*)\)\s*:\s*([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s*\{/);
            if (functionMatch) {
                const functionName = functionMatch[1];
                const parametersStr = functionMatch[2];
                const returnType = functionMatch[3];
                const parameters = this.parseParameters(parametersStr);

                const functionScope = this.createScope(ScopeType.Function, functionName, i, -1, this.rootScope);

                // Add parameters to function scope
                parameters.forEach(param => {
                    const varInfo: VariableInfo = {
                        name: param.name,
                        type: param.type,
                        scope: functionScope,
                        declarationLocation: new vscode.Position(i, 0),
                        isParameter: true
                    };
                    functionScope.variables.set(param.name, varInfo);
                });

                // Find function end and parse body
                this.parseFunctionBody(lines, i, functionScope);
            }
        }
    }

    private parseGlobalVariables(lines: string[]): void {
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];

            // Skip if we're inside a class or function
            if (this.isInsideClass(i) || this.isInsideFunction(i)) {
                continue;
            }

            // Skip lines that are not variable declarations
            const trimmedLine = line.trim();
            if (trimmedLine.startsWith('return ') ||
                trimmedLine.startsWith('if ') ||
                trimmedLine.startsWith('while ') ||
                trimmedLine.startsWith('for ') ||
                trimmedLine.startsWith('//') ||
                trimmedLine.includes('(') && !trimmedLine.includes('new ')) {
                continue;
            }

            // Parse global variable declarations - more flexible pattern
            // Match: [final] Type varName = anything;
            let varMatch = line.match(/^\s*(final\s+)?(\w+)\s+(\w+)\s*=.*$/);
            if (!varMatch) {
                // Try without semicolon requirement
                varMatch = line.match(/^\s*(final\s+)?(\w+)\s+(\w+)\s*=/);
            }

            if (varMatch) {
                const isFinal = !!varMatch[1];
                const varType = varMatch[2];
                const varName = varMatch[3];

                // Additional validation: ensure the type is not a keyword
                const keywords = ['return', 'if', 'while', 'for', 'else', 'switch', 'case', 'break', 'continue'];
                if (keywords.includes(varType)) {
                    continue;
                }

                const varInfo: VariableInfo = {
                    name: varName,
                    type: varType,
                    isFinal,
                    scope: this.rootScope,
                    declarationLocation: new vscode.Position(i, 0)
                };

                this.rootScope.variables.set(varName, varInfo);
            }
        }
    }

    private parseFunctionBody(lines: string[], startLine: number, functionScope: ScopeInfo): void {
        let braceCount = 1;
        let i = startLine + 1;

        while (i < lines.length && braceCount > 0) {
            const line = lines[i];
            const openBraces = (line.match(/\{/g) || []).length;
            const closeBraces = (line.match(/\}/g) || []).length;
            braceCount += openBraces - closeBraces;

            if (braceCount > 0) {
                this.parseLocalVariables(line, i, functionScope);
            }

            i++;
        }

        functionScope.endLine = i - 1;
    }

    // Helper methods
    private isInsideClass(lineIndex: number): boolean {
        return Array.from(this.scopes.values()).some(scope =>
            scope.type === ScopeType.Class &&
            lineIndex >= scope.startLine &&
            lineIndex <= scope.endLine
        );
    }

    private isInsideFunction(lineIndex: number): boolean {
        return Array.from(this.scopes.values()).some(scope =>
            (scope.type === ScopeType.Function || scope.type === ScopeType.Method) &&
            lineIndex >= scope.startLine &&
            lineIndex <= scope.endLine
        );
    }

    private getMethodScope(className: string, methodName: string): ScopeInfo | null {
        const scopeId = Array.from(this.scopes.keys()).find(key =>
            key.includes(methodName) && key.includes('method')
        );
        return scopeId ? this.scopes.get(scopeId) || null : null;
    }

    // Public API methods
    getScopeAtPosition(position: vscode.Position): ScopeInfo | null {
        const lineIndex = position.line;

        // Find the most specific scope containing this position
        let currentScope: ScopeInfo | null = null;

        for (const scope of this.scopes.values()) {
            if (lineIndex >= scope.startLine && lineIndex <= scope.endLine) {
                // Select the most specific scope: prefer non-global scopes, and among scopes with the same start line, prefer the one with smaller range
                if (!currentScope ||
                    (scope.type !== ScopeType.Global && currentScope.type === ScopeType.Global) ||
                    (scope.startLine > currentScope.startLine) ||
                    (scope.startLine === currentScope.startLine && (scope.endLine - scope.startLine) < (currentScope.endLine - currentScope.startLine))) {
                    currentScope = scope;
                }
            }
        }

        return currentScope || this.rootScope;
    }

    getVisibleVariables(position: vscode.Position): VariableInfo[] {
        const currentScope = this.getScopeAtPosition(position);
        const visibleVars: VariableInfo[] = [];

        // Traverse up the scope chain
        let scope: ScopeInfo | null = currentScope;
        while (scope) {
            // Add variables from current scope
            for (const variable of scope.variables.values()) {
                // Check if variable is declared before current position
                if (variable.declarationLocation.line <= position.line) {
                    visibleVars.push(variable);
                }
            }

            scope = scope.parentScope || null;
        }

        return visibleVars;
    }

    getAccessibleMembers(className: string, position: vscode.Position, isStatic: boolean = false): (VariableInfo | MethodInfo)[] {
        const classInfo = this.classes.get(className);
        if (!classInfo) {
            return [];
        }

        const currentScope = this.getScopeAtPosition(position);
        const isInsideClass = this.isPositionInClass(position, className);
        const members: (VariableInfo | MethodInfo)[] = [];

        if (isStatic) {
            // Static members - accessible from anywhere
            members.push(...classInfo.staticFields.values());
            members.push(...classInfo.staticMethods.values());
        } else {
            // Instance members
            if (isInsideClass) {
                // Inside class: access all members (public and private)
                members.push(...classInfo.fields.values());
                members.push(...classInfo.methods.values());
            } else {
                // Outside class: only public members
                members.push(...Array.from(classInfo.fields.values()).filter(f => f.visibility !== Visibility.Private));
                members.push(...Array.from(classInfo.methods.values()).filter(m => m.visibility !== Visibility.Private));
            }
        }

        return members;
    }

    private isPositionInClass(position: vscode.Position, className: string): boolean {
        const classInfo = this.classes.get(className);
        if (!classInfo) return false;

        const lineIndex = position.line;
        return lineIndex >= classInfo.scope.startLine && lineIndex <= classInfo.scope.endLine;
    }

    getClassInfo(className: string): ClassInfo | undefined {
        return this.classes.get(className);
    }

    getInterfaceInfo(interfaceName: string): InterfaceInfo | undefined {
        return this.interfaces.get(interfaceName);
    }

    getAllClasses(): Map<string, ClassInfo> {
        return this.classes;
    }

    getAllInterfaces(): Map<string, InterfaceInfo> {
        return this.interfaces;
    }

    getAllScopes(): Map<string, ScopeInfo> {
        return this.scopes;
    }

    /**
     * Get the class name at the current position (if inside a class)
     */
    getCurrentClassName(position: vscode.Position): string | null {
        const scope = this.getScopeAtPosition(position);
        if (!scope) {
            return null;
        }

        // Check if we're directly in a class scope
        if (scope.type === ScopeType.Class && scope.name) {
            return scope.name;
        }

        // Check if we're in a method scope that has a className
        if (scope.className) {
            return scope.className;
        }

        // Traverse up to find a class scope
        let currentScope: ScopeInfo | null | undefined = scope;
        while (currentScope) {
            if (currentScope.type === ScopeType.Class && currentScope.name) {
                return currentScope.name;
            }
            if (currentScope.className) {
                return currentScope.className;
            }
            currentScope = currentScope.parentScope;
        }

        return null;
    }
}

interface ClassInfo {
    name: string;
    scope: ScopeInfo;
    methods: Map<string, MethodInfo>;
    fields: Map<string, VariableInfo>;
    staticMethods: Map<string, MethodInfo>;
    staticFields: Map<string, VariableInfo>;
    implementedInterfaces: string[];
    parentClass?: string;
}

interface InterfaceInfo {
    name: string;
    scope: ScopeInfo;
    methods: Map<string, MethodInfo>;
}