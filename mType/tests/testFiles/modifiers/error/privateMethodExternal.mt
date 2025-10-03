// Test: Private method access violation from external context
class Helper {
    private int calculate(int x) {
        return x * 2;
    }

    public int doubleValue(int x) {
        return calculate(x);
    }
}

Helper h = new Helper();
print(h.calculate(5));  // ERROR: Cannot access private method 'calculate'
