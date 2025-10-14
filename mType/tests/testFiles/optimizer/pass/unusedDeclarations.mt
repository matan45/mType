// Test file for O2 optimization - Unused Declaration Elimination
// This file has unused functions, classes, and variables
// ===== USED CODE (should be kept) =====

function usedFunction(int x): int {
    return x * 2;
}

class UsedClass {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

// ===== UNUSED CODE (should be removed by O2) =====

function unusedHelper(): int {
    return 42;
}



function anotherUnusedFunction(int x,int y): int {
    return x + y;
}

class UnusedClass {
    string data;

    constructor(string d) {
        this.data = d;
    }

    function printData(): void {
        print(this.data);
    }
}

int unusedVariable = 100;
string unusedString = "This is unused";

// ===== ENTRY POINT CODE (uses only some declarations) =====

int result = usedFunction(10);
print("Result: " + result);



UsedClass obj = new UsedClass(25);
print("Object value: " + obj.getValue());

print("O2 Test Complete!");
