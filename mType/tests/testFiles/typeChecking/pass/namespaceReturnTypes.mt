namespace math {
    function calculate(): int {
        return 42;
    }
    
    function getPI(): float {
        return 3.14159;
    }
}

namespace text {
    function getMessage(): string {
        return "Hello from namespace";
    }
    
    function isValid(): bool {
        return true;
    }
}

// Valid assignments
int mathResult = math::calculate();
float piValue = math::getPI();
string textMessage = text::getMessage();
bool validFlag = text::isValid();

print("Namespace return types test passed");