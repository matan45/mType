class Calculator {
    static function compute(): int {
        return 100;
    }
    
    static function getMessage(): string {
        return "Computed";
    }
}

int result = Calculator::compute();
string msg = Calculator::getMessage();

print("Static method returns test passed");