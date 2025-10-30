import * as vscode from 'vscode';

export interface KeywordInfo {
    keyword: string;
    detail: string;
    documentation: string;
    insertText?: string;
    kind: vscode.CompletionItemKind;
    contexts?: string[]; // Where this keyword can be used
    priority?: number; // Higher numbers = higher priority
}

export class MTypeKeywords {
    // Core language keywords
    static readonly CORE_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'class',
            detail: 'class declaration',
            documentation: 'Defines a new class',
            insertText: 'class ${1:ClassName} {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['global', 'namespace'],
            priority: 9
        },
        {
            keyword: 'interface',
            detail: 'interface declaration',
            documentation: 'Defines a new interface',
            insertText: 'interface ${1:InterfaceName} {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['global', 'namespace'],
            priority: 9
        },
        {
            keyword: 'abstract',
            detail: 'abstract modifier',
            documentation: 'Marks a class as abstract or defines an abstract method',
            insertText: 'abstract ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['global', 'class'],
            priority: 9
        },
        {
            keyword: 'abstract-class',
            detail: 'abstract class',
            documentation: 'Abstract class with abstract and concrete methods',
            insertText: 'abstract class ${1:ClassName} {\n\tabstract function ${2:abstractMethod}(): ${3:void};\n\n\tpublic function ${4:concreteMethod}(): void {\n\t\t$0\n\t}\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['global', 'namespace'],
            priority: 8
        },
        {
            keyword: 'abstract-method',
            detail: 'abstract method',
            documentation: 'Abstract method declaration (no implementation)',
            insertText: 'abstract function ${1:methodName}(${2:parameters}): ${3:returnType};',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'extends',
            detail: 'class/interface extension',
            documentation: 'Extends a parent class or interface',
            insertText: 'extends ${1:ParentClass}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['after-class-name', 'after-interface-name'],
            priority: 9
        },
        {
            keyword: 'implements',
            detail: 'interface implementation',
            documentation: 'Implements one or more interfaces',
            insertText: 'implements ${1:InterfaceName}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['after-class-name'],
            priority: 9
        },
        {
            keyword: 'function',
            detail: 'function declaration',
            documentation: 'Defines a function or method',
            insertText: 'function ${1:functionName}(${2:parameters}): ${3:returnType} {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class', 'global', 'global-function'],
            priority: 9
        },
        {
            keyword: 'constructor',
            detail: 'constructor declaration',
            documentation: 'Defines a class constructor',
            insertText: 'constructor(${1:parameters}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class'],
            priority: 9
        },
        {
            keyword: 'constructor-simple',
            detail: 'constructor (no parameters)',
            documentation: 'Simple constructor with no parameters',
            insertText: 'constructor() {\n\t${1:// Initialize object}\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'constructor-params',
            detail: 'constructor (with parameters)',
            documentation: 'Constructor with parameter initialization',
            insertText: 'constructor(${1:type} ${2:param1}, ${3:type} ${4:param2}) {\n\t${5:// Initialize fields}\n\tthis.${6:field1} = ${2:param1};\n\tthis.${7:field2} = ${4:param2};\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'constructor-super',
            detail: 'constructor (with super call)',
            documentation: 'Constructor that calls parent constructor',
            insertText: 'constructor(${1:parameters}) {\n\t${2:// Call parent constructor if needed}\n\t${3:// super(${4:parentParams});}\n\t${5:// Initialize this class}\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['class'],
            priority: 7
        },
        {
            keyword: 'static',
            detail: 'static modifier',
            documentation: 'Makes a member static (class-level)',
            insertText: 'static ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'private',
            detail: 'private modifier',
            documentation: 'Makes a member private (accessible only within the class)',
            insertText: 'private ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'public',
            detail: 'public modifier',
            documentation: 'Makes a member public (accessible from anywhere)',
            insertText: 'public ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'protected',
            detail: 'protected modifier',
            documentation: 'Makes a member protected (accessible within the class and subclasses)',
            insertText: 'protected ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class'],
            priority: 8
        },
        {
            keyword: 'final',
            detail: 'final modifier',
            documentation: 'Makes a variable immutable',
            insertText: 'final ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class', 'function', 'global', 'global-function'],
            priority: 7
        },
        {
            keyword: 'new',
            detail: 'object instantiation',
            documentation: 'Creates a new instance of a class',
            insertText: 'new ${1:ClassName}(${2:arguments})',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['expression', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'this',
            detail: 'current instance reference',
            documentation: 'Reference to the current object instance',
            insertText: 'this',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class-method'],
            priority: 8
        },
        {
            keyword: 'return',
            detail: 'return statement',
            documentation: 'Returns a value from a function',
            insertText: 'return ${1:value};',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'global-function'],
            priority: 8
        },
        {
            keyword: 'native',
            detail: 'native modifier',
            documentation: 'Marks a method as implemented in native code',
            insertText: 'native ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class', 'function'],
            priority: 7
        },
        {
            keyword: 'isClassOf',
            detail: 'type check operator',
            documentation: 'Checks if an object is an instance of a class',
            insertText: 'isClassOf ${1:ClassName}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        },
        {
            keyword: 'await',
            detail: 'await expression',
            documentation: 'Waits for an async operation to complete',
            insertText: 'await ${1:expression}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['expression', 'function', 'global-function'],
            priority: 8
        },
        {
            keyword: 'async',
            detail: 'async modifier',
            documentation: 'Marks a function as asynchronous',
            insertText: 'async ',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['class', 'global'],
            priority: 8
        },
        {
            keyword: 'async-function',
            detail: 'async function',
            documentation: 'Asynchronous function that returns a Promise',
            insertText: 'function async ${1:functionName}(${2:parameters}): Promise<${3:void}> {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['class', 'global', 'global-function'],
            priority: 8
        },
        {
            keyword: 'Promise',
            detail: 'Promise<T> type',
            documentation: 'Promise type for async operations',
            insertText: 'Promise<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'from',
            detail: 'import from clause',
            documentation: 'Specifies the source module for imports',
            insertText: 'from "${1:module}"',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['global', 'after-import'],
            priority: 8
        }
    ];

    // Exception handling keywords
    static readonly EXCEPTION_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'try',
            detail: 'try-catch block',
            documentation: 'Exception handling block',
            insertText: 'try {\n\t$0\n} catch (${1:Exception} ${2:e}) {\n\t${3:// Handle exception}\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'try-catch-finally',
            detail: 'try-catch-finally block',
            documentation: 'Exception handling block with finally clause',
            insertText: 'try {\n\t$0\n} catch (${1:Exception} ${2:e}) {\n\t${3:// Handle exception}\n} finally {\n\t${4:// Cleanup}\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'catch',
            detail: 'catch block',
            documentation: 'Catches and handles exceptions',
            insertText: 'catch (${1:Exception} ${2:e}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['after-try'],
            priority: 9
        },
        {
            keyword: 'finally',
            detail: 'finally block',
            documentation: 'Code that always executes after try-catch',
            insertText: 'finally {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['after-catch'],
            priority: 9
        },
        {
            keyword: 'throw',
            detail: 'throw statement',
            documentation: 'Throws an exception',
            insertText: 'throw new ${1:Exception}("${2:message}");',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'throw-RuntimeException',
            detail: 'throw RuntimeException',
            documentation: 'Throws a runtime exception',
            insertText: 'throw new RuntimeException("${1:message}");',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'throw-NullPointerException',
            detail: 'throw NullPointerException',
            documentation: 'Throws a null pointer exception',
            insertText: 'throw new NullPointerException("${1:message}");',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'throw-IllegalArgumentException',
            detail: 'throw IllegalArgumentException',
            documentation: 'Throws an illegal argument exception',
            insertText: 'throw new IllegalArgumentException("${1:message}");',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'throw-IndexOutOfBoundsException',
            detail: 'throw IndexOutOfBoundsException',
            documentation: 'Throws an index out of bounds exception',
            insertText: 'throw new IndexOutOfBoundsException("${1:message}");',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        }
    ];

    // Control flow keywords
    static readonly CONTROL_FLOW_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'if',
            detail: 'if statement',
            documentation: 'Conditional execution',
            insertText: 'if (${1:condition}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'else',
            detail: 'else statement',
            documentation: 'Alternative execution path',
            insertText: 'else {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['after-if'],
            priority: 9
        },
        {
            keyword: 'while',
            detail: 'while loop',
            documentation: 'Repeats code while condition is true',
            insertText: 'while (${1:condition}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'do',
            detail: 'do-while loop',
            documentation: 'Executes code at least once, then repeats while condition is true',
            insertText: 'do {\n\t$0\n} while (${1:condition});',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'for',
            detail: 'for loop (C-style)',
            documentation: 'Repeats code with initialization, condition, and increment',
            insertText: 'for (${1:int i = 0}; ${2:i < length}; ${3:i++}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'for',
            detail: 'for loop (enhanced for)',
            documentation: 'Iterates over collection elements',
            insertText: 'for (${1:elementType} ${2:element} : ${3:collection}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'switch',
            detail: 'switch statement',
            documentation: 'Multi-way conditional execution',
            insertText: 'switch (${1:expression}) {\n\tcase ${2:value}:\n\t\t$0\n\t\tbreak;\n\tdefault:\n\t\tbreak;\n}',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'case',
            detail: 'switch case',
            documentation: 'Switch case label',
            insertText: 'case ${1:value}:\n\t$0\n\tbreak;',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['switch'],
            priority: 8
        },
        {
            keyword: 'default',
            detail: 'switch default case',
            documentation: 'Default case in switch statement',
            insertText: 'default:\n\t$0\n\tbreak;',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['switch'],
            priority: 8
        },
        {
            keyword: 'break',
            detail: 'break statement',
            documentation: 'Exits from loop or switch',
            insertText: 'break;',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['loop', 'switch', 'global-function'],
            priority: 7
        },
        {
            keyword: 'continue',
            detail: 'continue statement',
            documentation: 'Skips to next iteration of loop',
            insertText: 'continue;',
            kind: vscode.CompletionItemKind.Keyword,
            contexts: ['loop', 'global-function'],
            priority: 7
        },
        // Additional loop variations
        {
            keyword: 'while-simple',
            detail: 'while loop (simple)',
            documentation: 'Simple while loop template',
            insertText: 'while (${1:condition}) {\n\t${2:// TODO: Add code here}\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'do-while',
            detail: 'do-while loop (full)',
            documentation: 'Do-while loop with descriptive placeholder',
            insertText: 'do {\n\t${1:// Execute at least once}\n\t$0\n} while (${2:condition});',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'for-i',
            detail: 'for loop (integer iterator)',
            documentation: 'Standard for loop with integer iterator',
            insertText: 'for (int ${1:i} = 0; ${1:i} < ${2:length}; ${1:i}++) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'for-reverse',
            detail: 'for loop (reverse)',
            documentation: 'For loop iterating in reverse order',
            insertText: 'for (int ${1:i} = ${2:length} - 1; ${1:i} >= 0; ${1:i}--) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'for-collection',
            detail: 'for loop (Collection)',
            documentation: 'Enhanced for loop for collections with generic type',
            insertText: 'for (${1:T} ${2:item} : ${3:collection}) {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        // Additional switch variations
        {
            keyword: 'switch-int',
            detail: 'switch statement (int)',
            documentation: 'Switch statement for integer values',
            insertText: 'switch (${1:intValue}) {\n\tcase ${2:1}:\n\t\t${3:// Handle case 1}\n\t\tbreak;\n\tcase ${4:2}:\n\t\t${5:// Handle case 2}\n\t\tbreak;\n\tdefault:\n\t\t${6:// Handle default case}\n\t\tbreak;\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'switch-string',
            detail: 'switch statement (string)',
            documentation: 'Switch statement for string values',
            insertText: 'switch (${1:stringValue}) {\n\tcase "${2:option1}":\n\t\t${3:// Handle option1}\n\t\tbreak;\n\tcase "${4:option2}":\n\t\t${5:// Handle option2}\n\t\tbreak;\n\tdefault:\n\t\t${6:// Handle default case}\n\t\tbreak;\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'switch-simple',
            detail: 'switch statement (simple)',
            documentation: 'Simple switch statement with minimal cases',
            insertText: 'switch (${1:expression}) {\n\tcase ${2:value1}:\n\t\t$0\n\t\tbreak;\n\tdefault:\n\t\tbreak;\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'case-fallthrough',
            detail: 'switch case (fall-through)',
            documentation: 'Switch case that falls through to next case',
            insertText: 'case ${1:value}:\n\t${2:// Fall through to next case}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['switch'],
            priority: 7
        },
        {
            keyword: 'case-multiple',
            detail: 'multiple switch cases',
            documentation: 'Multiple switch cases with same handler',
            insertText: 'case ${1:value1}:\ncase ${2:value2}:\ncase ${3:value3}:\n\t${4:// Handle multiple cases}\n\tbreak;',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['switch'],
            priority: 7
        }
    ];

    // Data types
    static readonly TYPE_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'int',
            detail: 'integer type',
            documentation: 'Whole number data type',
            insertText: 'int',
            kind: vscode.CompletionItemKind.TypeParameter,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'float',
            detail: 'floating-point type',
            documentation: 'Decimal number data type',
            insertText: 'float',
            kind: vscode.CompletionItemKind.TypeParameter,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'string',
            detail: 'string type',
            documentation: 'Text data type',
            insertText: 'string',
            kind: vscode.CompletionItemKind.TypeParameter,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'bool',
            detail: 'boolean type',
            documentation: 'True/false data type',
            insertText: 'bool',
            kind: vscode.CompletionItemKind.TypeParameter,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 9
        },
        {
            keyword: 'void',
            detail: 'void type',
            documentation: 'No return value',
            insertText: 'void',
            kind: vscode.CompletionItemKind.TypeParameter,
            contexts: ['return-type', 'global-function'],
            priority: 8
        }
    ];

    // Standard library exception types
    static readonly EXCEPTION_TYPES: KeywordInfo[] = [
        {
            keyword: 'Exception',
            detail: 'Exception class',
            documentation: 'Base exception class for all exceptions',
            insertText: 'Exception',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global', 'after-catch'],
            priority: 8
        },
        {
            keyword: 'RuntimeException',
            detail: 'RuntimeException class',
            documentation: 'Runtime exception for errors during execution',
            insertText: 'RuntimeException',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global', 'after-catch'],
            priority: 8
        },
        {
            keyword: 'NullPointerException',
            detail: 'NullPointerException class',
            documentation: 'Exception thrown when accessing null reference',
            insertText: 'NullPointerException',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global', 'after-catch'],
            priority: 7
        },
        {
            keyword: 'IndexOutOfBoundsException',
            detail: 'IndexOutOfBoundsException class',
            documentation: 'Exception thrown when array/collection index is out of bounds',
            insertText: 'IndexOutOfBoundsException',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global', 'after-catch'],
            priority: 7
        },
        {
            keyword: 'IllegalArgumentException',
            detail: 'IllegalArgumentException class',
            documentation: 'Exception thrown when method receives illegal or inappropriate argument',
            insertText: 'IllegalArgumentException',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global', 'after-catch'],
            priority: 7
        }
    ];

    // Standard library wrapper types
    static readonly WRAPPER_TYPES: KeywordInfo[] = [
        {
            keyword: 'Int',
            detail: 'Int wrapper class',
            documentation: 'Integer wrapper class with utility methods',
            insertText: 'Int',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'Float',
            detail: 'Float wrapper class',
            documentation: 'Float wrapper class with utility methods',
            insertText: 'Float',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'String',
            detail: 'String wrapper class',
            documentation: 'String wrapper class with utility methods',
            insertText: 'String',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'Bool',
            detail: 'Bool wrapper class',
            documentation: 'Boolean wrapper class with utility methods',
            insertText: 'Bool',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        }
    ];

    // Standard library collection types
    static readonly STDLIB_COLLECTION_TYPES: KeywordInfo[] = [
        {
            keyword: 'List',
            detail: 'List<T> class',
            documentation: 'Dynamic list collection (similar to ArrayList)\nMethods: add, remove, get, set, size, isEmpty, clear, contains',
            insertText: 'List<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'LinkedList',
            detail: 'LinkedList<T> class',
            documentation: 'Doubly-linked list collection\nMethods: add, remove, get, size, isEmpty, clear, contains',
            insertText: 'LinkedList<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'HashMap',
            detail: 'HashMap<K,V> class',
            documentation: 'Hash-based key-value map\nMethods: put, get, remove, containsKey, keySet, values, size, isEmpty, clear',
            insertText: 'HashMap<${1:K}, ${2:V}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'HashSet',
            detail: 'HashSet<T> class',
            documentation: 'Hash-based set of unique elements\nMethods: add, remove, contains, size, isEmpty, clear',
            insertText: 'HashSet<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'Stack',
            detail: 'Stack<T> class',
            documentation: 'Last-in-first-out (LIFO) stack\nMethods: push, pop, peek, isEmpty, size, clear',
            insertText: 'Stack<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'Queue',
            detail: 'Queue<T> class',
            documentation: 'First-in-first-out (FIFO) queue\nMethods: enqueue, dequeue, peek, isEmpty, size, clear',
            insertText: 'Queue<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        }
    ];

    // Collection types
    static readonly COLLECTION_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'Array',
            detail: 'Array<T> collection',
            documentation: 'Dynamic array collection with indexed access',
            insertText: 'Array<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'Set',
            detail: 'Set<T> collection',
            documentation: 'Collection of unique elements',
            insertText: 'Set<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'Map',
            detail: 'Map<K,V> collection',
            documentation: 'Key-value pair collection',
            insertText: 'Map<${1:K}, ${2:V}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'Stack',
            detail: 'Stack<T> collection',
            documentation: 'Last-in-first-out (LIFO) collection',
            insertText: 'Stack<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'Queue',
            detail: 'Queue<T> collection',
            documentation: 'First-in-first-out (FIFO) collection',
            insertText: 'Queue<${1:T}>',
            kind: vscode.CompletionItemKind.Class,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        }
    ];

    // Literals and values
    static readonly LITERAL_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'true',
            detail: 'boolean true',
            documentation: 'Boolean true value',
            insertText: 'true',
            kind: vscode.CompletionItemKind.Value,
            contexts: ['expression', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'false',
            detail: 'boolean false',
            documentation: 'Boolean false value',
            insertText: 'false',
            kind: vscode.CompletionItemKind.Value,
            contexts: ['expression', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'null',
            detail: 'null value',
            documentation: 'Null reference value',
            insertText: 'null',
            kind: vscode.CompletionItemKind.Value,
            contexts: ['expression', 'global-function', 'global'],
            priority: 7
        }
    ];

    // Operators (for completion in expressions)
    static readonly OPERATOR_KEYWORDS: KeywordInfo[] = [
        // Note: 'in' operator removed as mType now uses ':' syntax for enhanced for loops
        // If you need other operators, add them here
    ];

    // Built-in functions
    static readonly BUILTIN_FUNCTIONS: KeywordInfo[] = [
        {
            keyword: 'print',
            detail: 'print(value)',
            documentation: 'Prints a value to the console',
            insertText: 'print(${1:value});',
            kind: vscode.CompletionItemKind.Function,
            contexts: ['function', 'block', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'hashCode',
            detail: 'hashCode(object)',
            documentation: 'Returns the hash code of an object',
            insertText: 'hashCode(${1:object})',
            kind: vscode.CompletionItemKind.Function,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        },
        {
            keyword: 'strLength',
            detail: 'strLength(string)',
            documentation: 'Returns the length of a string',
            insertText: 'strLength(${1:string})',
            kind: vscode.CompletionItemKind.Function,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        },
        {
            keyword: 'parsePrimitive',
            detail: 'parsePrimitive(value)',
            documentation: 'Converts a value to its string representation',
            insertText: 'parsePrimitive(${1:value})',
            kind: vscode.CompletionItemKind.Function,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        }
    ];

    // Lambda expressions and functional programming
    static readonly LAMBDA_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'lambda',
            detail: 'lambda expression',
            documentation: 'Lambda expression (parameter -> expression)',
            insertText: '${1:param} -> ${2:expression}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['expression', 'function', 'global-function'],
            priority: 8
        },
        {
            keyword: 'lambda-block',
            detail: 'lambda block',
            documentation: 'Lambda expression with block body',
            insertText: '${1:param} -> {\n\t${2:// code}\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['expression', 'function', 'global-function'],
            priority: 8
        },
        {
            keyword: 'lambda-typed',
            detail: 'lambda (typed)',
            documentation: 'Lambda expression with type annotation',
            insertText: '(${1:type} ${2:param}) -> ${3:expression}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        },
        {
            keyword: 'Function',
            detail: 'Function type',
            documentation: 'Functional interface type for lambda expressions',
            insertText: 'Function',
            kind: vscode.CompletionItemKind.Interface,
            contexts: ['type-context', 'global-function', 'global'],
            priority: 7
        }
    ];

    // Type casting and type checking
    static readonly TYPE_CASTING_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'cast',
            detail: 'type cast',
            documentation: 'Cast expression to specified type',
            insertText: '(${1:Type})${2:expression}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        },
        {
            keyword: 'isClassOf-check',
            detail: 'type check with cast',
            documentation: 'Type checking with safe casting pattern',
            insertText: 'if (${1:object}.isClassOf(${2:ClassName})) {\n\t${2:ClassName} ${3:casted} = (${2:ClassName})${1:object};\n\t$0\n}',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'block', 'global-function'],
            priority: 7
        }
    ];

    // Array initialization and operations
    static readonly ARRAY_KEYWORDS: KeywordInfo[] = [
        {
            keyword: 'array-declaration',
            detail: 'array declaration',
            documentation: 'Declare array with specified size',
            insertText: '${1:type}[] ${2:name} = new ${1:type}[${3:size}];',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'class', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'array-literal',
            detail: 'array literal',
            documentation: 'Initialize array with literal values',
            insertText: '${1:type}[] ${2:name} = [${3:elements}];',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'class', 'global-function', 'global'],
            priority: 8
        },
        {
            keyword: 'array-2d',
            detail: '2D array declaration',
            documentation: 'Declare two-dimensional array',
            insertText: '${1:type}[][] ${2:name} = new ${1:type}[${3:rows}][${4:cols}];',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['function', 'class', 'global-function', 'global'],
            priority: 7
        },
        {
            keyword: 'array-length',
            detail: 'array length property',
            documentation: 'Get array length using .length property',
            insertText: '${1:array}.length',
            kind: vscode.CompletionItemKind.Snippet,
            contexts: ['expression', 'function', 'global-function'],
            priority: 7
        }
    ];

    // Annotations
    static readonly ANNOTATION_KEYWORDS: KeywordInfo[] = [
        {
            keyword: '@Override',
            detail: 'Override annotation',
            documentation: 'Marks a method as overriding a parent class or interface method.\n\nValidation ensures:\n- The method exists in the parent class or implemented interface\n- The method signature matches exactly (name, parameters, return type)',
            insertText: '@Override\n${1:returnType} ${2:methodName}(${3:parameters}): ${4:returnType} {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Property,
            contexts: ['class', 'before-method'],
            priority: 9
        },
        {
            keyword: '@Throw',
            detail: 'Throw annotation',
            documentation: 'Documents that a method can throw specific exception types',
            insertText: '@Throw(${1:ExceptionType})\npublic function ${2:methodName}(${3:parameters}): ${4:void} {\n\t$0\n}',
            kind: vscode.CompletionItemKind.Property,
            contexts: ['class', 'before-method'],
            priority: 8
        },
        {
            keyword: '@Script',
            detail: 'Script class annotation',
            documentation: 'Marks a class as a script that can be instantiated and updated from C++.\n\nRequirements:\n- Class must not be abstract\n- Must have a default constructor (no parameters)\n- Must have an update(dt: float): void method\n\nUsage: For game objects, behaviors, and entities managed by the engine',
            insertText: '@Script\nclass ${1:ScriptName} {\n\tconstructor() {\n\t\t${2:// Initialize}\n\t}\n\n\tupdate(dt: float): void {\n\t\t${3:// Update logic}\n\t}\n\n\t$0\n}',
            kind: vscode.CompletionItemKind.Property,
            contexts: ['global', 'before-class'],
            priority: 9
        }
    ];

    // Get all keywords
    static getAllKeywords(): KeywordInfo[] {
        return [
            ...this.CORE_KEYWORDS,
            ...this.EXCEPTION_KEYWORDS,
            ...this.CONTROL_FLOW_KEYWORDS,
            ...this.TYPE_KEYWORDS,
            ...this.EXCEPTION_TYPES,
            ...this.WRAPPER_TYPES,
            ...this.STDLIB_COLLECTION_TYPES,
            ...this.COLLECTION_KEYWORDS,
            ...this.LITERAL_KEYWORDS,
            ...this.OPERATOR_KEYWORDS,
            ...this.BUILTIN_FUNCTIONS,
            ...this.LAMBDA_KEYWORDS,
            ...this.TYPE_CASTING_KEYWORDS,
            ...this.ARRAY_KEYWORDS,
            ...this.ANNOTATION_KEYWORDS
        ];
    }

    // Get keywords by context
    static getKeywordsByContext(context: string): KeywordInfo[] {
        return this.getAllKeywords().filter(keyword => {
            if (!keyword.contexts) return true; // No context restriction

            // Include if matches the context
            if (keyword.contexts.includes(context)) return true;

            // Include global keywords for any context
            if (keyword.contexts.includes('global')) return true;

            // Special case: include function keywords for global-function context
            if (context === 'global-function' && keyword.contexts.includes('function')) return true;

            // Special case: include block keywords for function contexts
            if ((context === 'function' || context === 'global-function') && keyword.contexts.includes('block')) return true;

            return false;
        });
    }

    // Get type keywords only
    static getTypeKeywords(): KeywordInfo[] {
        return [
            ...this.TYPE_KEYWORDS,
            ...this.EXCEPTION_TYPES,
            ...this.WRAPPER_TYPES,
            ...this.STDLIB_COLLECTION_TYPES,
            ...this.COLLECTION_KEYWORDS
        ];
    }

    // Convert to completion items
    static toCompletionItems(keywords: KeywordInfo[]): vscode.CompletionItem[] {
        return keywords.map(keyword => {
            const item = new vscode.CompletionItem(keyword.keyword, keyword.kind);
            item.detail = keyword.detail;
            item.documentation = new vscode.MarkdownString(keyword.documentation);

            if (keyword.insertText) {
                item.insertText = new vscode.SnippetString(keyword.insertText);
            }

            // Set sort text based on priority
            const priority = keyword.priority || 5;
            item.sortText = `${10 - priority}_${keyword.keyword}`;

            return item;
        });
    }
}