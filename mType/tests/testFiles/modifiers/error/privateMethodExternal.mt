// Test: Private method access violation from external context
class Helper {
    private function calculate(int x): int {
        return x * 2;
    }

    public function doubleValue(int x): int {
        return calculate(x);
    }
}

Helper h = new Helper();
print(h.calculate(5));  // ERROR: Cannot access private method 'calculate'
