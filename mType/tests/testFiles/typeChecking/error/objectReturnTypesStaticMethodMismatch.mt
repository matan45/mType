class Calculator {
    static function compute(): int {
        return 100;
    }
}

bool wrongCompute = Calculator::compute();  // Should fail