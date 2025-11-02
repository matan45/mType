// Test: Class cannot extend interface
// Should fail at parse time with clear error message

interface Comparable {
    function compareTo(Comparable other): int;
}

class MyClass extends Comparable {
    function compareTo(Comparable other): int {
        return 0;
    }
}
