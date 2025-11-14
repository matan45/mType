// Test file for selective import system
// Defines public and private symbols

public class Calculator {
    public function add(int a, int b) : int {
        return a + b;
    }

    public function multiply(int a, int b) : int {
        return a * b;
    }
}

public function divide(float a, float b) : float {
    if (b == 0.0) {
        return 0.0;
    }
    return a / b;
}

public function subtract(int a, int b) : int {
    return a - b;
}

private function internalHelper(int x) : int {
    return x * 2;
}

private class InternalCache {
    int value;

    function getValue() : int {
        return this.value;
    }
}

public interface Comparable {
    function compare(object other) : int;
}
