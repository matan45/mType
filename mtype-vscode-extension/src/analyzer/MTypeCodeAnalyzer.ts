export class MTypeCodeAnalyzer {
    public classes = new Map<string, any>();
    private variables = new Map<string, string>();

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

        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const classMatch = line.match(/^\s*class\s+(\w+)\s*\{/);

            if (classMatch && !inClass) {
                // Start of a new class
                currentClass = classMatch[1];
                classBody = '';
                braceCount = 1;
                inClass = true;
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

        // Parse mType function syntax: [static] function methodName(parameters): returnType {
        const functionRegex = /(?:(static)\s+)?function\s+(\w+)\s*\(([^)]*)\)\s*:\s*(\w+)\s*\{/g;
        let functionMatch;

        while ((functionMatch = functionRegex.exec(classBody)) !== null) {
            const isStatic = functionMatch[1] === 'static';
            const methodName = functionMatch[2];
            const parametersStr = functionMatch[3];
            const returnType = functionMatch[4];

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
                return this.classes.get(field.returnType)!.members.filter((m: any) => !m.isStatic);
            }
        }

        return [];
    }
}