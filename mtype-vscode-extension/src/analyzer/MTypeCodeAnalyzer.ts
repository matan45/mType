export interface SymbolLocation {
    line: number;
    character: number;
}

export interface ClassInfo {
    name: string;
    members: any[];
    location?: SymbolLocation;
}

export interface VariableInfo {
    type: string;
    location?: SymbolLocation;
}

export class MTypeCodeAnalyzer {
    public classes = new Map<string, ClassInfo>();
    private variables = new Map<string, VariableInfo>();
    private symbolLocations = new Map<string, SymbolLocation>();
    private namespaces = new Map<string, SymbolLocation>();

    analyzeDocument(text: string): void {
        this.parseClasses(text);
        this.parseVariables(text);
    }

    private parseClasses(text: string): void {
        this.classes.clear();

        // Improved class parsing to handle multiple classes better
        const lines = text.split('\n');
        let currentClass = '';
        let braceCount = 0;
        let classBody = '';
        let inClass = false;
        let classStartLine = 0;

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const classMatch = line.match(/^\s*class\s+(\w+)\s*\{/);

            if (classMatch && !inClass) {
                // Start of a new class
                currentClass = classMatch[1];
                classBody = '';
                braceCount = 1;
                inClass = true;
                classStartLine = i;

                // Store class location
                const classKeywordIndex = line.indexOf('class');
                const classNameIndex = line.indexOf(currentClass);
                this.symbolLocations.set(currentClass, {
                    line: i,
                    character: classNameIndex
                });
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
                    const members = this.parseClassMembers(classBody, currentClass, classStartLine + 1);
                    const classLocation: SymbolLocation = {
                        line: classStartLine,
                        character: lines[classStartLine].indexOf(currentClass)
                    };

                    this.classes.set(currentClass, {
                        name: currentClass,
                        members: members,
                        location: classLocation
                    });

                    inClass = false;
                    currentClass = '';
                    classBody = '';
                }
            }
        }
    }

    private parseClassMembers(classBody: string, className: string, startLine: number): any[] {
        const members: any[] = [];

        // Parse mType function syntax: [static] function methodName(parameters): returnType {
        const functionRegex = /(?:(static)\s+)?function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)\s*\{/g;
        let functionMatch;
        const lines = classBody.split('\n');

        while ((functionMatch = functionRegex.exec(classBody)) !== null) {
            const isStatic = functionMatch[1] === 'static';
            const methodName = functionMatch[2];
            const parametersStr = functionMatch[3];
            const returnType = functionMatch[4];

            const parameters = parametersStr.split(',')
                .map(p => p.trim())
                .filter(p => p.length > 0);

            // Find line number within class body
            const matchText = functionMatch[0];
            let lineIndex = 0;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes(methodName) && lines[i].includes('function')) {
                    lineIndex = i;
                    break;
                }
            }

            // Store method location
            const methodKey = `${className}.${methodName}`;
            this.symbolLocations.set(methodKey, {
                line: startLine + lineIndex,
                character: lines[lineIndex].indexOf(methodName)
            });

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

            if (fieldType === 'function' || fieldType === 'return') continue;

            // Check if this field name already exists to avoid duplicates
            const existingMember = members.find(m => m.name === fieldName);
            if (!existingMember) {
                // Find line number for field
                let fieldLineIndex = 0;
                for (let i = 0; i < lines.length; i++) {
                    if (lines[i].includes(fieldName) && lines[i].includes(fieldType) && lines[i].includes(';')) {
                        fieldLineIndex = i;
                        break;
                    }
                }

                // Store field location
                const fieldKey = `${className}.${fieldName}`;
                this.symbolLocations.set(fieldKey, {
                    line: startLine + fieldLineIndex,
                    character: lines[fieldLineIndex].indexOf(fieldName)
                });

                members.push({
                    name: fieldName,
                    type: 'field',
                    returnType: fieldType,
                    isStatic: isStatic
                });
            }
        }

        // Parse constructors
        const constructorRegex = /constructor\s*\(([^)]*)\)\s*\{/g;
        let constructorMatch;

        while ((constructorMatch = constructorRegex.exec(classBody)) !== null) {
            const parametersStr = constructorMatch[1];
            const parameters = parametersStr.split(',')
                .map(p => p.trim())
                .filter(p => p.length > 0);

            // Find line number for constructor
            let constructorLineIndex = 0;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes('constructor')) {
                    constructorLineIndex = i;
                    break;
                }
            }

            // Store constructor location
            const constructorKey = `${className}.constructor`;
            this.symbolLocations.set(constructorKey, {
                line: startLine + constructorLineIndex,
                character: lines[constructorLineIndex].indexOf('constructor')
            });

            members.push({
                name: 'constructor',
                type: 'constructor',
                returnType: className,
                isStatic: false,
                parameters: parameters
            });
        }

        return members;
    }

    private parseVariables(text: string): void {
        this.variables.clear();
        const lines = text.split('\n');

        // Parse global variable declarations (outside classes)
        const globalVarRegex = /^(?!.*class\s)(\w+)\s+(\w+)\s*=\s*(?:new\s+(\w+)\s*\(|([^;]+))/gm;
        let match;

        while ((match = globalVarRegex.exec(text)) !== null) {
            const declaredType = match[1];
            const varName = match[2];
            const newType = match[3];

            const actualType = newType || declaredType;

            // Find line number
            const lineIndex = this.findLineIndex(lines, match.index);
            const charIndex = lines[lineIndex].indexOf(varName);

            this.variables.set(varName, {
                type: actualType,
                location: { line: lineIndex, character: charIndex }
            });

            // Store in symbol locations for easy lookup
            this.symbolLocations.set(varName, {
                line: lineIndex,
                character: charIndex
            });
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
                const lineIndex = this.findLineIndex(lines, localMatch.index);
                const charIndex = lines[lineIndex].indexOf(varName);

                this.variables.set(varName, {
                    type: actualType,
                    location: { line: lineIndex, character: charIndex }
                });

                this.symbolLocations.set(varName, {
                    line: lineIndex,
                    character: charIndex
                });
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

                        const lineIndex = this.findLineIndex(lines, funcMatch.index);
                        const charIndex = lines[lineIndex].indexOf(paramName);

                        this.variables.set(paramName, {
                            type: paramType,
                            location: { line: lineIndex, character: charIndex }
                        });
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

                        const lineIndex = this.findLineIndex(lines, constructorMatch.index);
                        const charIndex = lines[lineIndex].indexOf(paramName);

                        this.variables.set(paramName, {
                            type: paramType,
                            location: { line: lineIndex, character: charIndex }
                        });
                    }
                }
            }
        }
    }


    private findLineIndex(lines: string[], charIndex: number): number {
        let currentIndex = 0;
        for (let i = 0; i < lines.length; i++) {
            const lineLength = lines[i].length + 1; // +1 for newline
            if (currentIndex + lineLength > charIndex) {
                return i;
            }
            currentIndex += lineLength;
        }
        return lines.length - 1;
    }

    // Public methods for the DefinitionProvider and ReferenceProvider
    getSymbolLocation(symbol: string, type?: string): SymbolLocation | null {
        return this.symbolLocations.get(symbol) || null;
    }

    getVariableType(variableName: string): string | null {
        const variable = this.variables.get(variableName);
        return variable ? variable.type : null;
    }

    // Helper method to check if a member exists in a class
    hasMember(className: string, memberName: string, isStatic?: boolean): boolean {
        const classInfo = this.classes.get(className);
        if (!classInfo) return false;

        const member = classInfo.members.find(m => {
            return m.name === memberName && (isStatic === undefined || m.isStatic === isStatic);
        });

        return !!member;
    }

    // Get member info from a class
    getMemberInfo(className: string, memberName: string): any | null {
        const classInfo = this.classes.get(className);
        if (!classInfo) return null;

        return classInfo.members.find(m => m.name === memberName) || null;
    }

    // Get all variables of a specific type
    getVariablesOfType(typeName: string): string[] {
        const variablesOfType: string[] = [];
        for (const [varName, varInfo] of this.variables.entries()) {
            if (varInfo.type === typeName) {
                variablesOfType.push(varName);
            }
        }
        return variablesOfType;
    }

    getObjectMembers(objectName: string): any[] {
        // Check if it's a regular variable
        const directType = this.variables.get(objectName);
        if (directType && directType.type && this.classes.has(directType.type)) {
            return this.classes.get(directType.type)!.members.filter((m: any) => !m.isStatic);
        }

        // Check if it's a static class reference
        if (this.classes.has(objectName)) {
            return this.classes.get(objectName)!.members.filter((m: any) => m.isStatic);
        }

        // Check if it's a field within any class (for member access within class methods/constructors)
        for (const [className, classInfo] of this.classes.entries()) {
            const field = classInfo.members.find((m: any) => m.name === objectName && m.type === 'field');
            if (field && this.classes.has(field.returnType)) {
                return this.classes.get(field.returnType)!.members.filter((m: any) => !m.isStatic);
            }
        }

        return [];
    }
}