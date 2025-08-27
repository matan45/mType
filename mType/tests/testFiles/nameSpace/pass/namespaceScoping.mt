namespace outer {
int globalVar = 100;
string message = "outer";

function outerFunction(): int {
    return globalVar;
}

function getMessage(): string {
    return message;
}

namespace inner {
    int globalVar = 200;  // Same name, different scope
    string message = "inner";
    
    function innerFunction(): int {
        return globalVar;  // Should refer to inner::globalVar
    }
    
    function getInnerMessage(): string {
        return message;
    }
    
    namespace deeper {
        int globalVar = 300;  // Another same name
        string message = "deeper";
        
        function deepFunction(): int {
            return globalVar;  // Should refer to deeper::globalVar
        }
        
        function getDeepMessage(): string {
            return message;
        }
    }
}

namespace sibling {
    int siblingVar = 400;
    string message = "sibling";
    
    function siblingFunction(): int {
        return siblingVar;
    }
    
    function getSiblingMessage(): string {
        return message;
    }
}
}

namespace completely_separate {
int globalVar = 500;  // Same name, completely separate
string message = "separate";

function separateFunction(): int {
    return globalVar;
}

function getSeparateMessage(): string {
    return message;
}
}

// Test variable shadowing through function calls
int outer = outer::outerFunction();
print(outer);

int inner = outer::inner::innerFunction();
print(inner);

int deeper = outer::inner::deeper::deepFunction();
print(deeper);

int sibling = outer::sibling::siblingFunction();
print(sibling);

int separate = completely_separate::separateFunction();
print(separate);
string outerMsg = outer::getMessage();
print(outerMsg);
string innerMsg = outer::inner::getInnerMessage();
print(innerMsg);
string deeperMsg = outer::inner::deeper::getDeepMessage();
print(deeperMsg);
string siblingMsg = outer::sibling::getSiblingMessage();
print(siblingMsg);