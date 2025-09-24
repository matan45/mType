# mType Enhanced Keyword Completion Reference

## 🔧 Constructor Completions

| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `constructor` | Basic constructor | `constructor(parameters) { }` |
| `constructor-simple` | No parameters | `constructor() { // Initialize object }` |
| `constructor-params` | With parameter initialization | `constructor(type param1, type param2) { this.field1 = param1; this.field2 = param2; }` |
| `constructor-super` | With super call | `constructor(parameters) { // super(parentParams); }` |

## 🔄 Loop Completions

### While Loops
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `while` | Basic while loop | `while (condition) { }` |
| `while-simple` | Simple while with placeholder | `while (condition) { // TODO: Add code here }` |

### Do-While Loops
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `do` | Basic do-while | `do { } while (condition);` |
| `do-while` | With descriptive placeholder | `do { // Execute at least once } while (condition);` |

### For Loops
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `for` | Basic C-style for loop | `for (int i = 0; i < length; i++) { }` |
| `for-i` | Integer iterator | `for (int i = 0; i < length; i++) { }` |
| `for-reverse` | Reverse iteration | `for (int i = length - 1; i >= 0; i--) { }` |

### Foreach Loops
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `foreach` | Basic foreach | `foreach (elementType element in collection) { }` |
| `foreach-array` | Specifically for arrays | `foreach (int item in array) { }` |
| `foreach-collection` | For generic collections | `foreach (T item in collection) { }` |

## 🔀 Switch Statement Completions

### Switch Statements
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `switch` | Basic switch | `switch (expression) { case value: break; default: break; }` |
| `switch-int` | For integer values | `switch (intValue) { case 1: // Handle case 1 break; case 2: // Handle case 2 break; default: // Handle default break; }` |
| `switch-string` | For string values | `switch (stringValue) { case "option1": // Handle option1 break; case "option2": // Handle option2 break; default: // Handle default break; }` |
| `switch-simple` | Minimal cases | `switch (expression) { case value1: break; default: break; }` |

### Switch Cases
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `case` | Basic case | `case value: break;` |
| `case-fallthrough` | Fall-through case | `case value: // Fall through to next case` |
| `case-multiple` | Multiple cases | `case value1: case value2: case value3: // Handle multiple cases break;` |
| `default` | Default case | `default: break;` |

## 🎯 Loop Control
| Trigger | Description | Generated Code |
|---------|-------------|----------------|
| `break` | Exit loop/switch | `break;` |
| `continue` | Skip to next iteration | `continue;` |

## 📝 Usage Tips

1. **Start typing any prefix** - Completions appear automatically
2. **Use Ctrl+Space** - Manually trigger completion menu
3. **Tab navigation** - Use Tab to move through snippet placeholders
4. **Context-aware** - Different completions appear based on where you're typing:
   - Inside classes: constructor variations
   - Inside functions: loop and control flow statements
   - Inside switch blocks: case variations

## 🚀 Quick Examples

### Constructor Usage
```mtype
class MyClass {
    // Type "const" and select from:
    constructor() { }                    // constructor-simple
    constructor(string name) { }         // constructor-params
}
```

### Loop Usage
```mtype
function example() {
    // Type "for" and select from:
    for (int i = 0; i < 10; i++) { }           // for-i
    for (int i = 9; i >= 0; i--) { }           // for-reverse

    // Type "while" and select from:
    while (condition) { }                       // while
    while (condition) { // TODO: Add code }     // while-simple

    // Type "foreach" and select from:
    foreach (int item in array) { }             // foreach-array
    foreach (T item in collection) { }          // foreach-collection
}
```

### Switch Usage
```mtype
function example() {
    // Type "switch" and select from:
    switch (value) {                           // switch-simple
        case 1: break;
        default: break;
    }

    switch (intValue) {                        // switch-int
        case 1: // Handle case 1 break;
        case 2: // Handle case 2 break;
        default: // Handle default break;
    }
}
```